PreservedAnalyses JumpThreadingPass::run(Function &F,
                                         FunctionAnalysisManager &AM) {
  auto &TTI = AM.getResult<TargetIRAnalysis>(F);
  // Jump Threading has no sense for the targets with divergent CF
  if (TTI.hasBranchDivergence())
    return PreservedAnalyses::all();
  auto &TLI = AM.getResult<TargetLibraryAnalysis>(F);
  auto &LVI = AM.getResult<LazyValueAnalysis>(F);
  auto &AA = AM.getResult<AAManager>(F);
  auto &DT = AM.getResult<DominatorTreeAnalysis>(F);

  bool Changed =
      runImpl(F, &AM, &TLI, &TTI, &LVI, &AA,
              std::make_unique<DomTreeUpdater>(
                  &DT, nullptr, DomTreeUpdater::UpdateStrategy::Lazy),
              std::nullopt, std::nullopt);

  return getPreservedAnalysis();
}

bool JumpThreadingPass::runImpl(Function &F_, FunctionAnalysisManager *FAM_,
                                TargetLibraryInfo *TLI_,
                                TargetTransformInfo *TTI_, LazyValueInfo *LVI_,
                                AliasAnalysis *AA_,
                                std::unique_ptr<DomTreeUpdater> DTU_,
                                std::optional<BlockFrequencyInfo *> BFI_,
                                std::optional<BranchProbabilityInfo *> BPI_) {
  LLVM_DEBUG(dbgs() << "Jump threading on function '" << F_.getName() << "'\n");
  F = &F_;
  FAM = FAM_;
  TLI = TLI_;
  TTI = TTI_;
  LVI = LVI_;
  AA = AA_;
  DTU = std::move(DTU_);
  BFI = BFI_;
  BPI = BPI_;
  auto *GuardDecl = F->getParent()->getFunction(
      Intrinsic::getName(Intrinsic::experimental_guard));
  HasGuards = GuardDecl && !GuardDecl->use_empty();

  bool EverChanged = false;
  bool Changed;
  do {
    Changed = false;
    for (auto &BB : *F) {
      if (Unreachable.count(&BB))
        continue;
      while (processBlock(&BB)) // Thread all of the branches we can over BB.
        Changed = ChangedSinceLastAnalysisUpdate = true;      // Jump threading may have introduced redundant debug values into BB
      // which should be removed.
      if (Changed)
        RemoveRedundantDbgInstrs(&BB);

      // Stop processing BB if it's the entry or is now deleted. The following
      // routines attempt to eliminate BB and locating a suitable replacement
      // for the entry is non-trivial.
      if (&BB == &F->getEntryBlock() || DTU->isBBPendingDeletion(&BB))
        continue;

      if (pred_empty(&BB)) {
        // When processBlock makes BB unreachable it doesn't bother to fix up
        // the instructions in it. We must remove BB to prevent invalid IR.
        LLVM_DEBUG(dbgs() << "  JT: Deleting dead block '" << BB.getName()
                          << "' with terminator: " << *BB.getTerminator()
                          << '\n');
        LoopHeaders.erase(&BB);
        LVI->eraseBlock(&BB);
        DeleteDeadBlock(&BB, DTU.get());
        Changed = ChangedSinceLastAnalysisUpdate = true;
        continue;
      }

      // processBlock doesn't thread BBs with unconditional TIs. However, if BB
      // is "almost empty", we attempt to merge BB with its sole successor.
      auto *BI = dyn_cast<BranchInst>(BB.getTerminator());
      if (BI && BI->isUnconditional()) {
        BasicBlock *Succ = BI->getSuccessor(0);
        if (
            // The terminator must be the only non-phi instruction in BB.
            BB.getFirstNonPHIOrDbg(true)->isTerminator() &&
            // Don't alter Loop headers and latches to ensure another pass can
            // detect and transform nested loops later.
            !LoopHeaders.count(&BB) && !LoopHeaders.count(Succ) &&
            TryToSimplifyUncondBranchFromEmptyBlock(&BB, DTU.get())) {
          RemoveRedundantDbgInstrs(Succ);
          // BB is valid for cleanup here because we passed in DTU. F remains
          // BB's parent until a DTU->getDomTree() event.
          LVI->eraseBlock(&BB);
          Changed = ChangedSinceLastAnalysisUpdate = true;
        }
      }
    }
    EverChanged |= Changed;
  } while (Changed);

  LoopHeaders.clear();
  return EverChanged;
}

