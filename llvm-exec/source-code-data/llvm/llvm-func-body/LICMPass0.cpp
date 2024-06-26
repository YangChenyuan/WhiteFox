bool llvm::sinkRegion(DomTreeNode *N, AAResults *AA, LoopInfo *LI,
                      DominatorTree *DT, TargetLibraryInfo *TLI,
                      TargetTransformInfo *TTI, Loop *CurLoop,
                      MemorySSAUpdater &MSSAU, ICFLoopSafetyInfo *SafetyInfo,
                      SinkAndHoistLICMFlags &Flags,
                      OptimizationRemarkEmitter *ORE, Loop *OutermostLoop) {

  // Verify inputs.
  assert(N != nullptr && AA != nullptr && LI != nullptr && DT != nullptr &&
         CurLoop != nullptr && SafetyInfo != nullptr &&
         "Unexpected input to sinkRegion.");

  bool Changed = false;
  for (DomTreeNode *DTN : reverse(Worklist)) {
    BasicBlock *BB = DTN->getBlock();
    // Only need to process the contents of this block if it is not part of a
    // subloop (which would already have been processed).
    if (inSubLoop(BB, CurLoop, LI))
      continue;    for (BasicBlock::iterator II = BB->end(); II != BB->begin();) {
      Instruction &I = *--II;

      // The instruction is not used in the loop if it is dead.  In this case,
      // we just delete it instead of sinking it.
      if (isInstructionTriviallyDead(&I, TLI)) {
        LLVM_DEBUG(dbgs() << "LICM deleting dead inst: " << I << '\n');
        salvageKnowledge(&I);
        salvageDebugInfo(I);
        ++II;
        eraseInstruction(I, *SafetyInfo, MSSAU);
        Changed = true;
        continue;
      }

      // Check to see if we can sink this instruction to the exit blocks
      // of the loop.  We can do this if the all users of the instruction are
      // outside of the loop.  In this case, it doesn't even matter if the
      // operands of the instruction are loop invariant.
      //
      bool FoldableInLoop = false;
      bool LoopNestMode = OutermostLoop != nullptr;
      if (!I.mayHaveSideEffects() &&
          isNotUsedOrFoldableInLoop(I, LoopNestMode ? OutermostLoop : CurLoop,
                                    SafetyInfo, TTI, FoldableInLoop,
                                    LoopNestMode) &&
          canSinkOrHoistInst(I, AA, DT, CurLoop, MSSAU, true, Flags, ORE)) {
        if (sink(I, LI, DT, CurLoop, SafetyInfo, MSSAU, ORE)) {
          if (!FoldableInLoop) {
            ++II;
            salvageDebugInfo(I);
            eraseInstruction(I, *SafetyInfo, MSSAU);
          }
          Changed = true;
        }
      }
    }
  }
  if (VerifyMemorySSA)
    MSSAU.getMemorySSA()->verifyMemorySSA();
  return Changed;
}

static bool sink(Instruction &I, LoopInfo *LI, DominatorTree *DT,
                 const Loop *CurLoop, ICFLoopSafetyInfo *SafetyInfo,
                 MemorySSAUpdater &MSSAU, OptimizationRemarkEmitter *ORE) {
  bool Changed = false;
  LLVM_DEBUG(dbgs() << "LICM sinking instruction: " << I << "\n");

  // Iterate over users to be ready for actual sinking. Replace users via
  // unreachable blocks with undef and make all user PHIs trivially replaceable.
  SmallPtrSet<Instruction *, 8> VisitedUsers;
  for (Value::user_iterator UI = I.user_begin(), UE = I.user_end(); UI != UE;) {
    auto *User = cast<Instruction>(*UI);
    Use &U = UI.getUse();
    ++UI;    if (VisitedUsers.count(User) || CurLoop->contains(User))
      continue;

    if (!DT->isReachableFromEntry(User->getParent())) {
      U = PoisonValue::get(I.getType());
      LLVM_DEBUG(dbgs() << "U = PoisonValue::get(I.getType());\n");
      Changed = true;
      continue;
    }

    // The user must be a PHI node.
    PHINode *PN = cast<PHINode>(User);

    // Surprisingly, instructions can be used outside of loops without any
    // exits.  This can only happen in PHI nodes if the incoming block is
    // unreachable.
    BasicBlock *BB = PN->getIncomingBlock(U);
    if (!DT->isReachableFromEntry(BB)) {
      U = PoisonValue::get(I.getType());
      Changed = true;
      continue;
    }

    VisitedUsers.insert(PN);
    if (isTriviallyReplaceablePHI(*PN, I))
      continue;

    if (!canSplitPredecessors(PN, SafetyInfo))
      return Changed;

    // Split predecessors of the PHI so that we can make users trivially
    // replaceable.
    splitPredecessorsOfLoopExit(PN, DT, LI, CurLoop, SafetyInfo, &MSSAU);

    // Should rebuild the iterators, as they may be invalidated by
    // splitPredecessorsOfLoopExit().
    UI = I.user_begin();
    UE = I.user_end();
  }

  if (VisitedUsers.empty())
    return Changed;

  // If this instruction is only used outside of the loop, then all users are
  // PHI nodes in exit blocks due to LCSSA form. Just RAUW them with clones of
  // the instruction.
  // First check if I is worth sinking for all uses. Sink only when it is worth
  // across all uses.
  SmallSetVector<User*, 8> Users(I.user_begin(), I.user_end());
  for (auto *UI : Users) {
    auto *User = cast<Instruction>(UI);    if (CurLoop->contains(User))
      continue;

    PHINode *PN = cast<PHINode>(User);
    assert(ExitBlockSet.count(PN->getParent()) &&
           "The LCSSA PHI is not in an exit block!");

    // The PHI must be trivially replaceable.
    Instruction *New = sinkThroughTriviallyReplaceablePHI(
        PN, &I, LI, SunkCopies, SafetyInfo, CurLoop, MSSAU);
    PN->replaceAllUsesWith(New);
    LLVM_DEBUG(dbgs() << "PN->replaceAllUsesWith(New);\n");
    eraseInstruction(*PN, *SafetyInfo, MSSAU);
    Changed = true;
  }
  return Changed;
}

