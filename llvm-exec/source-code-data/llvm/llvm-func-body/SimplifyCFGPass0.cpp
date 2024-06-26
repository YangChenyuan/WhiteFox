bool SimplifyCFGOpt::simplifyOnce(BasicBlock *BB) {
  bool Changed = false;

  // Remove basic blocks that have no predecessors (except the entry block)...
  // or that just have themself as a predecessor.  These are unreachable.
  if ((pred_empty(BB) && BB != &BB->getParent()->getEntryBlock()) ||
      BB->getSinglePredecessor() == BB) {
    LLVM_DEBUG(dbgs() << "Removing BB: \n" << *BB);
    DeleteDeadBlock(BB, DTU);
    return true;
  }

  // Merge basic blocks into their predecessor if there is only one distinct
  // pred, and if there is only one distinct successor of the predecessor, and
  // if there are no PHI nodes.
  if (MergeBlockIntoPredecessor(BB, DTU))
    return true;

  if (SinkCommon && Options.SinkCommonInsts)
    if (SinkCommonCodeFromPredecessors(BB, DTU) ||
        MergeCompatibleInvokes(BB, DTU)) {
      // SinkCommonCodeFromPredecessors() does not automatically CSE PHI's,
      // so we may now how duplicate PHI's.
      // Let's rerun EliminateDuplicatePHINodes() first,
      // before FoldTwoEntryPHINode() potentially converts them into select's,
      // after which we'd need a whole EarlyCSE pass run to cleanup them.
      return true;
    }

  if (Options.FoldTwoEntryPHINode) {
    // If there is a trivial two-entry PHI node in this basic block, and we can
    // eliminate it, do so now.
    if (auto *PN = dyn_cast<PHINode>(BB->begin()))
      if (PN->getNumIncomingValues() == 2)
        if (FoldTwoEntryPHINode(PN, TTI, DTU, DL))
          return true;
  }

  return Changed;
}

