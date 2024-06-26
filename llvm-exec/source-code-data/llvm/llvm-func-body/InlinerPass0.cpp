PreservedAnalyses InlinerPass::run(LazyCallGraph::SCC &InitialC,
                                   CGSCCAnalysisManager &AM, LazyCallGraph &CG,
                                   CGSCCUpdateResult &UR) {
  const auto &MAMProxy =
      AM.getResult<ModuleAnalysisManagerCGSCCProxy>(InitialC, CG);
  bool Changed = false;

  // Loop forward over all of the calls. Note that we cannot cache the size as
  // inlining can introduce new calls that need to be processed.
  for (int I = 0; I < (int)Calls.size(); ++I) {
    // We expect the calls to typically be batched with sequences of calls that
    // have the same caller, so we first set up some shared infrastructure for
    // this caller. We also do any pruning we can at this layer on the caller
    // alone.
    Function &F = *Calls[I].first->getCaller();
    LazyCallGraph::Node &N = *CG.lookup(F);
    if (CG.lookupSCC(N) != C)
      continue;    LLVM_DEBUG(dbgs() << "Inlining calls in: " << F.getName() << "\n"
                      << "    Function size: " << F.getInstructionCount()
                      << "\n");

    auto GetAssumptionCache = [&](Function &F) -> AssumptionCache & {
      return FAM.getResult<AssumptionAnalysis>(F);
    };

    // Now process as many calls as we have within this caller in the sequence.
    // We bail out as soon as the caller has to change so we can update the
    // call graph and prepare the context of that new caller.
    bool DidInline = false;
    for (; I < (int)Calls.size() && Calls[I].first->getCaller() == &F; ++I) {
      auto &P = Calls[I];
      CallBase *CB = P.first;
      const int InlineHistoryID = P.second;
      Function &Callee = *CB->getCalledFunction();

      if (InlineHistoryID != -1 &&
          inlineHistoryIncludes(&Callee, InlineHistoryID, InlineHistory)) {
        LLVM_DEBUG(dbgs() << "Skipping inlining due to history: " << F.getName()
                          << " -> " << Callee.getName() << "\n");
        setInlineRemark(*CB, "recursive");
        continue;
      }

      // Check if this inlining may repeat breaking an SCC apart that has
      // already been split once before. In that case, inlining here may
      // trigger infinite inlining, much like is prevented within the inliner
      // itself by the InlineHistory above, but spread across CGSCC iterations
      // and thus hidden from the full inline history.
      LazyCallGraph::SCC *CalleeSCC = CG.lookupSCC(*CG.lookup(Callee));
      if (CalleeSCC == C && UR.InlinedInternalEdges.count({&N, C})) {
        LLVM_DEBUG(dbgs() << "Skipping inlining internal SCC edge from a node "
                             "previously split out of this SCC by inlining: "
                          << F.getName() << " -> " << Callee.getName() << "\n");
        setInlineRemark(*CB, "recursive SCC split");
        continue;
      }

      std::unique_ptr<InlineAdvice> Advice =
          Advisor.getAdvice(*CB, OnlyMandatory);

      // Check whether we want to inline this callsite.
      if (!Advice)
        continue;

      if (!Advice->isInliningRecommended()) {
        Advice->recordUnattemptedInlining();
        continue;
      }

      int CBCostMult =
          getStringFnAttrAsInt(
              *CB, InlineConstants::FunctionInlineCostMultiplierAttributeName)
              .value_or(1);

      // Setup the data structure used to plumb customization into the
      // `InlineFunction` routine.
      InlineFunctionInfo IFI(
          GetAssumptionCache, PSI,
          &FAM.getResult<BlockFrequencyAnalysis>(*(CB->getCaller())),
          &FAM.getResult<BlockFrequencyAnalysis>(Callee));

      InlineResult IR =
          InlineFunction(*CB, IFI, /*MergeAttributes=*/true,
                         &FAM.getResult<AAManager>(*CB->getCaller()));
      if (!IR.isSuccess()) {
        Advice->recordUnsuccessfulInlining(IR);
        continue;
      }

      DidInline = true;
      InlinedCallees.insert(&Callee);
      ++NumInlined;

      LLVM_DEBUG(dbgs() << "    Size after inlining: "
                        << F.getInstructionCount() << "\n");

      // Add any new callsites to defined functions to the worklist.
      if (!IFI.InlinedCallSites.empty()) {
        int NewHistoryID = InlineHistory.size();
        InlineHistory.push_back({&Callee, InlineHistoryID});

        for (CallBase *ICB : reverse(IFI.InlinedCallSites)) {
          Function *NewCallee = ICB->getCalledFunction();
          assert(!(NewCallee && NewCallee->isIntrinsic()) &&
                 "Intrinsic calls should not be tracked.");
          if (!NewCallee) {
            // Try to promote an indirect (virtual) call without waiting for
            // the post-inline cleanup and the next DevirtSCCRepeatedPass
            // iteration because the next iteration may not happen and we may
            // miss inlining it.
            if (tryPromoteCall(*ICB))
              NewCallee = ICB->getCalledFunction();
          }
          if (NewCallee) {
            if (!NewCallee->isDeclaration()) {
              Calls.push_back({ICB, NewHistoryID});
              // Continually inlining through an SCC can result in huge compile
              // times and bloated code since we arbitrarily stop at some point
              // when the inliner decides it's not profitable to inline anymore.
              // We attempt to mitigate this by making these calls exponentially
              // more expensive.
              // This doesn't apply to calls in the same SCC since if we do
              // inline through the SCC the function will end up being
              // self-recursive which the inliner bails out on, and inlining
              // within an SCC is necessary for performance.
              if (CalleeSCC != C &&
                  CalleeSCC == CG.lookupSCC(CG.get(*NewCallee))) {
                Attribute NewCBCostMult = Attribute::get(
                    M.getContext(),
                    InlineConstants::FunctionInlineCostMultiplierAttributeName,
                    itostr(CBCostMult * IntraSCCCostMultiplier));
                ICB->addFnAttr(NewCBCostMult);
              }
            }
          }
        }
      }

      // For local functions or discardable functions without comdats, check
      // whether this makes the callee trivially dead. In that case, we can drop
      // the body of the function eagerly which may reduce the number of callers
      // of other functions to one, changing inline cost thresholds. Non-local
      // discardable functions with comdats are checked later on.
      bool CalleeWasDeleted = false;
      if (Callee.isDiscardableIfUnused() && Callee.hasZeroLiveUses() &&
          !CG.isLibFunction(Callee)) {
        if (Callee.hasLocalLinkage() || !Callee.hasComdat()) {
          Calls.erase(
              std::remove_if(Calls.begin() + I + 1, Calls.end(),
                             [&](const std::pair<CallBase *, int> &Call) {
                               return Call.first->getCaller() == &Callee;
                             }),
              Calls.end());

          // Clear the body and queue the function itself for deletion when we
          // finish inlining and call graph updates.
          // Note that after this point, it is an error to do anything other
          // than use the callee's address or delete it.
          Callee.dropAllReferences();
          assert(!is_contained(DeadFunctions, &Callee) &&
                 "Cannot put cause a function to become dead twice!");
          DeadFunctions.push_back(&Callee);
          CalleeWasDeleted = true;
        } else {
          DeadFunctionsInComdats.push_back(&Callee);
        }
      }
      if (CalleeWasDeleted)
        Advice->recordInliningWithCalleeDeleted();
      else
        Advice->recordInlining();
    }

    // Back the call index up by one to put us in a good position to go around
    // the outer loop.
    --I;

    if (!DidInline)
      continue;
    Changed = true;

    // At this point, since we have made changes we have at least removed
    // a call instruction. However, in the process we do some incremental
    // simplification of the surrounding code. This simplification can
    // essentially do all of the same things as a function pass and we can
    // re-use the exact same logic for updating the call graph to reflect the
    // change.

    // Inside the update, we also update the FunctionAnalysisManager in the
    // proxy for this particular SCC. We do this as the SCC may have changed and
    // as we're going to mutate this particular function we want to make sure
    // the proxy is in place to forward any invalidation events.
    LazyCallGraph::SCC *OldC = C;
    C = &updateCGAndAnalysisManagerForCGSCCPass(CG, *C, N, AM, UR, FAM);
    LLVM_DEBUG(dbgs() << "Updated inlining SCC: " << *C << "\n");

    // If this causes an SCC to split apart into multiple smaller SCCs, there
    // is a subtle risk we need to prepare for. Other transformations may
    // expose an "infinite inlining" opportunity later, and because of the SCC
    // mutation, we will revisit this function and potentially re-inline. If we
    // do, and that re-inlining also has the potentially to mutate the SCC
    // structure, the infinite inlining problem can manifest through infinite
    // SCC splits and merges. To avoid this, we capture the originating caller
    // node and the SCC containing the call edge. This is a slight over
    // approximation of the possible inlining decisions that must be avoided,
    // but is relatively efficient to store. We use C != OldC to know when
    // a new SCC is generated and the original SCC may be generated via merge
    // in later iterations.
    //
    // It is also possible that even if no new SCC is generated
    // (i.e., C == OldC), the original SCC could be split and then merged
    // into the same one as itself. and the original SCC will be added into
    // UR.CWorklist again, we want to catch such cases too.
    //
    // FIXME: This seems like a very heavyweight way of retaining the inline
    // history, we should look for a more efficient way of tracking it.
    if ((C != OldC || UR.CWorklist.count(OldC)) &&
        llvm::any_of(InlinedCallees, [&](Function *Callee) {
          return CG.lookupSCC(*CG.lookup(*Callee)) == OldC;
        })) {
      LLVM_DEBUG(dbgs() << "Inlined an internal call edge and split an SCC, "
                           "retaining this to avoid infinite inlining.\n");
      UR.InlinedInternalEdges.insert({&N, OldC});
    }
    InlinedCallees.clear();

    // Invalidate analyses for this function now so that we don't have to
    // invalidate analyses for all functions in this SCC later.
    FAM.invalidate(F, PreservedAnalyses::none());
  }

  PreservedAnalyses PA;
  // Even if we change the IR, we update the core CGSCC data structures and so
  // can preserve the proxy to the function analysis manager.
  PA.preserve<FunctionAnalysisManagerCGSCCProxy>();
  // We have already invalidated all analyses on modified functions.
  PA.preserveSet<AllAnalysesOn<Function>>();
  return PA;
}

