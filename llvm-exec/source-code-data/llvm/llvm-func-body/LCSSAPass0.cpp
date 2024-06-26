bool llvm::formLCSSA(Loop &L, const DominatorTree &DT, const LoopInfo *LI,
                     ScalarEvolution *SE) {
  bool Changed = false;

  SmallVector<BasicBlock *, 8> ExitBlocks;
  L.getExitBlocks(ExitBlocks);
  if (ExitBlocks.empty())
    return false;

  IRBuilder<> Builder(L.getHeader()->getContext());
  Changed = formLCSSAForInstructions(Worklist, DT, *LI, SE, Builder);

  return Changed;
}

bool llvm::formLCSSAForInstructions(SmallVectorImpl<Instruction *> &Worklist,
                                    const DominatorTree &DT, const LoopInfo &LI,
                                    ScalarEvolution *SE, IRBuilderBase &Builder,
                                    SmallVectorImpl<PHINode *> *PHIsToRemove) {
  SmallVector<Use *, 16> UsesToRewrite;
  SmallSetVector<PHINode *, 16> LocalPHIsToRemove;
  PredIteratorCache PredCache;
  bool Changed = false;

  while (!Worklist.empty()) {
    UsesToRewrite.clear();    Instruction *I = Worklist.pop_back_val();
    assert(!I->getType()->isTokenTy() && "Tokens shouldn't be in the worklist");
    BasicBlock *InstBB = I->getParent();
    Loop *L = LI.getLoopFor(InstBB);
    assert(L && "Instruction belongs to a BB that's not part of a loop");
    if (!LoopExitBlocks.count(L))
      L->getExitBlocks(LoopExitBlocks[L]);
    assert(LoopExitBlocks.count(L));
    const SmallVectorImpl<BasicBlock *> &ExitBlocks = LoopExitBlocks[L];

    if (ExitBlocks.empty())
      continue;

    for (Use &U : make_early_inc_range(I->uses())) {
      Instruction *User = cast<Instruction>(U.getUser());
      BasicBlock *UserBB = User->getParent();

      // Skip uses in unreachable blocks.
      if (!DT.isReachableFromEntry(UserBB)) {
        U.set(PoisonValue::get(I->getType()));
        continue;
      }

      // For practical purposes, we consider that the use in a PHI
      // occurs in the respective predecessor block. For more info,
      // see the `phi` doc in LangRef and the LCSSA doc.
      if (auto *PN = dyn_cast<PHINode>(User))
        UserBB = PN->getIncomingBlock(U);

      if (InstBB != UserBB && !L->contains(UserBB))
        UsesToRewrite.push_back(&U);
    }

    // If there are no uses outside the loop, exit with no change.
    if (UsesToRewrite.empty())
      continue;

    ++NumLCSSA; // We are applying the transformation

    // Invoke instructions are special in that their result value is not
    // available along their unwind edge. The code below tests to see whether
    // DomBB dominates the value, so adjust DomBB to the normal destination
    // block, which is effectively where the value is first usable.
    BasicBlock *DomBB = InstBB;
    if (auto *Inv = dyn_cast<InvokeInst>(I))
      DomBB = Inv->getNormalDest();

    const DomTreeNode *DomNode = DT.getNode(DomBB);

    SmallVector<PHINode *, 16> AddedPHIs;
    SmallVector<PHINode *, 8> PostProcessPHIs;

    SmallVector<PHINode *, 4> InsertedPHIs;
    SSAUpdater SSAUpdate(&InsertedPHIs);
    SSAUpdate.Initialize(I->getType(), I->getName());

    // Force re-computation of I, as some users now need to use the new PHI
    // node.
    if (SE)
      SE->forgetValue(I);

    // Insert the LCSSA phi's into all of the exit blocks dominated by the
    // value, and add them to the Phi's map.
    for (BasicBlock *ExitBB : ExitBlocks) {
      if (!DT.dominates(DomNode, DT.getNode(ExitBB)))
        continue;

      // If we already inserted something for this BB, don't reprocess it.
      if (SSAUpdate.HasValueForBlock(ExitBB))
        continue;
      Builder.SetInsertPoint(&ExitBB->front());
      LLVM_DEBUG(dbgs() << "Builder.SetInsertPoint(&ExitBB->front());\n");
      PHINode *PN = Builder.CreatePHI(I->getType(), PredCache.size(ExitBB),
                                      I->getName() + ".lcssa");
      // Get the debug location from the original instruction.
      PN->setDebugLoc(I->getDebugLoc());

      // Add inputs from inside the loop for this PHI. This is valid
      // because `I` dominates `ExitBB` (checked above).  This implies
      // that every incoming block/edge is dominated by `I` as well,
      // i.e. we can add uses of `I` to those incoming edges/append to the incoming
      // blocks without violating the SSA dominance property.
      for (BasicBlock *Pred : PredCache.get(ExitBB)) {
        PN->addIncoming(I, Pred);

        // If the exit block has a predecessor not within the loop, arrange for
        // the incoming value use corresponding to that predecessor to be
        // rewritten in terms of a different LCSSA PHI.
        if (!L->contains(Pred))
          UsesToRewrite.push_back(
              &PN->getOperandUse(PN->getOperandNumForIncomingValue(
                  PN->getNumIncomingValues() - 1)));
      }

      AddedPHIs.push_back(PN);

      // Remember that this phi makes the value alive in this block.
      SSAUpdate.AddAvailableValue(ExitBB, PN);

      // LoopSimplify might fail to simplify some loops (e.g. when indirect
      // branches are involved). In such situations, it might happen that an
      // exit for Loop L1 is the header of a disjoint Loop L2. Thus, when we
      // create PHIs in such an exit block, we are also inserting PHIs into L2's
      // header. This could break LCSSA form for L2 because these inserted PHIs
      // can also have uses outside of L2. Remember all PHIs in such situation
      // as to revisit than later on. FIXME: Remove this if indirectbr support
      // into LoopSimplify gets improved.
      if (auto *OtherLoop = LI.getLoopFor(ExitBB))
        if (!L->contains(OtherLoop))
          PostProcessPHIs.push_back(PN);
    }

    // Rewrite all uses outside the loop in terms of the new PHIs we just
    // inserted.
    for (Use *UseToRewrite : UsesToRewrite) {
      Instruction *User = cast<Instruction>(UseToRewrite->getUser());
      BasicBlock *UserBB = User->getParent();

      // For practical purposes, we consider that the use in a PHI
      // occurs in the respective predecessor block. For more info,
      // see the `phi` doc in LangRef and the LCSSA doc.
      if (auto *PN = dyn_cast<PHINode>(User))
        UserBB = PN->getIncomingBlock(*UseToRewrite);

      // If this use is in an exit block, rewrite to use the newly inserted PHI.
      // This is required for correctness because SSAUpdate doesn't handle uses
      // in the same block.  It assumes the PHI we inserted is at the end of the
      // block.
      if (isa<PHINode>(UserBB->begin()) && isExitBlock(UserBB, ExitBlocks)) {
        UseToRewrite->set(&UserBB->front());
        continue;
      }

      // If we added a single PHI, it must dominate all uses and we can directly
      // rename it.
      if (AddedPHIs.size() == 1) {
        UseToRewrite->set(AddedPHIs[0]);
        continue;
      }

      // Otherwise, do full PHI insertion.
      SSAUpdate.RewriteUse(*UseToRewrite);
    }

    SmallVector<DbgValueInst *, 4> DbgValues;
    llvm::findDbgValues(DbgValues, I);

    // Update pre-existing debug value uses that reside outside the loop.
    for (auto *DVI : DbgValues) {
      BasicBlock *UserBB = DVI->getParent();
      if (InstBB == UserBB || L->contains(UserBB))
        continue;
      // We currently only handle debug values residing in blocks that were
      // traversed while rewriting the uses. If we inserted just a single PHI,
      // we will handle all relevant debug values.
      Value *V = AddedPHIs.size() == 1 ? AddedPHIs[0]
                                       : SSAUpdate.FindValueForBlock(UserBB);
      if (V)
        DVI->replaceVariableLocationOp(I, V);
    }

    // SSAUpdater might have inserted phi-nodes inside other loops. We'll need
    // to post-process them to keep LCSSA form.
    for (PHINode *InsertedPN : InsertedPHIs) {
      if (auto *OtherLoop = LI.getLoopFor(InsertedPN->getParent()))
        if (!L->contains(OtherLoop))
          PostProcessPHIs.push_back(InsertedPN);
    }

    // Post process PHI instructions that were inserted into another disjoint
    // loop and update their exits properly.
    for (auto *PostProcessPN : PostProcessPHIs)
      if (!PostProcessPN->use_empty())
        Worklist.push_back(PostProcessPN);

    // Keep track of PHI nodes that we want to remove because they did not have
    // any uses rewritten.
    for (PHINode *PN : AddedPHIs)
      if (PN->use_empty())
        LocalPHIsToRemove.insert(PN);

    Changed = true;
  }

  // Remove PHI nodes that did not have any uses rewritten or add them to
  // PHIsToRemove, so the caller can remove them after some additional cleanup.
  // We need to redo the use_empty() check here, because even if the PHI node
  // wasn't used when added to LocalPHIsToRemove, later added PHI nodes can be
  // using it.  This cleanup is not guaranteed to handle trees/cycles of PHI
  // nodes that only are used by each other. Such situations has only been
  // noticed when the input IR contains unreachable code, and leaving some extra
  // redundant PHI nodes in such situations is considered a minor problem.
  if (PHIsToRemove) {
    PHIsToRemove->append(LocalPHIsToRemove.begin(), LocalPHIsToRemove.end());
  } else {
    for (PHINode *PN : LocalPHIsToRemove)
      if (PN->use_empty())
        PN->eraseFromParent();
  }
  return Changed;
}

