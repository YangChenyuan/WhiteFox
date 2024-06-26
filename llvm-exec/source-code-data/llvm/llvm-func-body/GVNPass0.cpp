bool GVNPass::runImpl(Function &F, AssumptionCache &RunAC, DominatorTree &RunDT,
                      const TargetLibraryInfo &RunTLI, AAResults &RunAA,
                      MemoryDependenceResults *RunMD, LoopInfo *LI,
                      OptimizationRemarkEmitter *RunORE, MemorySSA *MSSA) {
  AC = &RunAC;
  DT = &RunDT;
  VN.setDomTree(DT);
  TLI = &RunTLI;
  VN.setAliasAnalysis(&RunAA);
  MD = RunMD;
  ImplicitControlFlowTracking ImplicitCFT;
  ICF = &ImplicitCFT;
  this->LI = LI;
  VN.setMemDep(MD);
  ORE = RunORE;
  InvalidBlockRPONumbers = true;
  MemorySSAUpdater Updater(MSSA);
  MSSAU = MSSA ? &Updater : nullptr;

  DomTreeUpdater DTU(DT, DomTreeUpdater::UpdateStrategy::Eager);
  // Merge unconditional branches, allowing PRE to catch more
  // optimization opportunities.
  for (BasicBlock &BB : llvm::make_early_inc_range(F)) {
    bool removedBlock = MergeBlockIntoPredecessor(&BB, &DTU, LI, MSSAU, MD);
    if (removedBlock)
      ++NumGVNBlocks;

  return Changed;
}

bool llvm::MergeBlockIntoPredecessor(BasicBlock *BB, DomTreeUpdater *DTU,
                                     LoopInfo *LI, MemorySSAUpdater *MSSAU,
                                     MemoryDependenceResults *MemDep,
                                     bool PredecessorWithTwoSuccessors,
                                     DominatorTree *DT) {

  if (BB->hasAddressTaken())
    return false;

  // Can't merge if there are multiple predecessors, or no predecessors.
  BasicBlock *PredBB = BB->getUniquePredecessor();
  if (!PredBB) return false;

  // Don't break self-loops.
  if (PredBB == BB) return false;

  // Don't break unwinding instructions or terminators with other side-effects.
  Instruction *PTI = PredBB->getTerminator();
  if (PTI->isExceptionalTerminator() || PTI->mayHaveSideEffects())
    return false;

  // Can't merge if there are multiple distinct successors.
  if (!PredecessorWithTwoSuccessors && PredBB->getUniqueSuccessor() != BB)
    return false;

  // Currently only allow PredBB to have two predecessors, one being BB.
  // Update BI to branch to BB's only successor instead of BB.
  BranchInst *PredBB_BI;
  BasicBlock *NewSucc = nullptr;
  unsigned FallThruPath;
  if (PredecessorWithTwoSuccessors) {
    if (!(PredBB_BI = dyn_cast<BranchInst>(PTI)))
      return false;
    BranchInst *BB_JmpI = dyn_cast<BranchInst>(BB->getTerminator());
    if (!BB_JmpI || !BB_JmpI->isUnconditional())
      return false;
    NewSucc = BB_JmpI->getSuccessor(0);
    FallThruPath = PredBB_BI->getSuccessor(0) == BB ? 0 : 1;
  }

  // Can't merge if there is PHI loop.
  for (PHINode &PN : BB->phis())
    if (llvm::is_contained(PN.incoming_values(), &PN))
      return false;

  // Finally, erase the old block and update dominator info.
  DeleteDeadBlock(BB, DTU);

  return true;
}

