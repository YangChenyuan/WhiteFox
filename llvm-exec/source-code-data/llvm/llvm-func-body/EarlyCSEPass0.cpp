bool EarlyCSE::run() {
  // Note, deque is being used here because there is significant performance
  // gains over vector when the container becomes very large due to the
  // specific access patterns. For more information see the mailing list
  // discussion on this:
  // http://lists.llvm.org/pipermail/llvm-commits/Week-of-Mon-20120116/135228.html
  std::deque<StackNode *> nodesToProcess;

  // Process the stack.
  while (!nodesToProcess.empty()) {
    // Grab the first item off the stack. Set the current generation, remove
    // the node from the stack, and process it.
    StackNode *NodeToProcess = nodesToProcess.back();    // Initialize class members.
    CurrentGeneration = NodeToProcess->currentGeneration();

    // Check if the node needs to be processed.
    if (!NodeToProcess->isProcessed()) {
      // Process the node.
      Changed |= processNode(NodeToProcess->node());
      NodeToProcess->childGeneration(CurrentGeneration);
      NodeToProcess->process();
    } else if (NodeToProcess->childIter() != NodeToProcess->end()) {
      // Push the next child onto the stack.
      DomTreeNode *child = NodeToProcess->nextChild();
      nodesToProcess.push_back(
          new StackNode(AvailableValues, AvailableLoads, AvailableInvariants,
                        AvailableCalls, NodeToProcess->childGeneration(),
                        child, child->begin(), child->end()));
    } else {
      // It has been processed, and there are no more children to process,
      // so delete it and pop it off the stack.
      delete NodeToProcess;
      nodesToProcess.pop_back();
    }
  } // while (!nodes...)

  return Changed;
}

bool EarlyCSE::processNode(DomTreeNode *Node) {
  bool Changed = false;
  BasicBlock *BB = Node->getBlock();

  // See if any instructions in the block can be eliminated.  If so, do it.  If
  // not, add them to AvailableValues.
  for (Instruction &Inst : make_early_inc_range(*BB)) {
    // Dead instructions should just be removed.
    if (isInstructionTriviallyDead(&Inst, &TLI)) {
      LLVM_DEBUG(dbgs() << "EarlyCSE DCE: " << Inst << '\n');
      if (!DebugCounter::shouldExecute(CSECounter)) {
        LLVM_DEBUG(dbgs() << "Skipping due to debug counter\n");
        continue;
      }      salvageKnowledge(&Inst, &AC);
      LLVM_DEBUG(dbgs() << "salvageKnowledge(&Inst, &AC);\n");
      salvageDebugInfo(Inst);
      LLVM_DEBUG(dbgs() << "salvageDebugInfo(Inst);\n");
      removeMSSA(Inst);
      Inst.eraseFromParent();
      Changed = true;
      ++NumSimplify;
      continue;
    }

    // Skip assume intrinsics, they don't really have side effects (although
    // they're marked as such to ensure preservation of control dependencies),
    // and this pass will not bother with its removal. However, we should mark
    // its condition as true for all dominated blocks.
    if (auto *Assume = dyn_cast<AssumeInst>(&Inst)) {
      auto *CondI = dyn_cast<Instruction>(Assume->getArgOperand(0));
      if (CondI && SimpleValue::canHandle(CondI)) {
        LLVM_DEBUG(dbgs() << "EarlyCSE considering assumption: " << Inst
                          << '\n');
        AvailableValues.insert(CondI, ConstantInt::getTrue(BB->getContext()));
      } else
        LLVM_DEBUG(dbgs() << "EarlyCSE skipping assumption: " << Inst << '\n');
      continue;
    }

    // Likewise, noalias intrinsics don't actually write.
    if (match(&Inst,
              m_Intrinsic<Intrinsic::experimental_noalias_scope_decl>())) {
      LLVM_DEBUG(dbgs() << "EarlyCSE skipping noalias intrinsic: " << Inst
                        << '\n');
      continue;
    }

    // Skip sideeffect intrinsics, for the same reason as assume intrinsics.
    if (match(&Inst, m_Intrinsic<Intrinsic::sideeffect>())) {
      LLVM_DEBUG(dbgs() << "EarlyCSE skipping sideeffect: " << Inst << '\n');
      continue;
    }

    // Skip pseudoprobe intrinsics, for the same reason as assume intrinsics.
    if (match(&Inst, m_Intrinsic<Intrinsic::pseudoprobe>())) {
      LLVM_DEBUG(dbgs() << "EarlyCSE skipping pseudoprobe: " << Inst << '\n');
      continue;
    }

    // We can skip all invariant.start intrinsics since they only read memory,
    // and we can forward values across it. For invariant starts without
    // invariant ends, we can use the fact that the invariantness never ends to
    // start a scope in the current generaton which is true for all future
    // generations.  Also, we dont need to consume the last store since the
    // semantics of invariant.start allow us to perform   DSE of the last
    // store, if there was a store following invariant.start. Consider:
    //
    // store 30, i8* p
    // invariant.start(p)
    // store 40, i8* p
    // We can DSE the store to 30, since the store 40 to invariant location p
    // causes undefined behaviour.
    if (match(&Inst, m_Intrinsic<Intrinsic::invariant_start>())) {
      // If there are any uses, the scope might end.
      if (!Inst.use_empty())
        continue;
      MemoryLocation MemLoc =
          MemoryLocation::getForArgument(&cast<CallInst>(Inst), 1, TLI);
      // Don't start a scope if we already have a better one pushed
      if (!AvailableInvariants.count(MemLoc))
        AvailableInvariants.insert(MemLoc, CurrentGeneration);
      continue;
    }

    if (isGuard(&Inst)) {
      if (auto *CondI =
              dyn_cast<Instruction>(cast<CallInst>(Inst).getArgOperand(0))) {
        if (SimpleValue::canHandle(CondI)) {
          // Do we already know the actual value of this condition?
          if (auto *KnownCond = AvailableValues.lookup(CondI)) {
            // Is the condition known to be true?
            if (isa<ConstantInt>(KnownCond) &&
                cast<ConstantInt>(KnownCond)->isOne()) {
              LLVM_DEBUG(dbgs()
                         << "EarlyCSE removing guard: " << Inst << '\n');
              salvageKnowledge(&Inst, &AC);
              removeMSSA(Inst);
              Inst.eraseFromParent();
              Changed = true;
              continue;
            } else
              // Use the known value if it wasn't true.
              cast<CallInst>(Inst).setArgOperand(0, KnownCond);
          }
          // The condition we're on guarding here is true for all dominated
          // locations.
          AvailableValues.insert(CondI, ConstantInt::getTrue(BB->getContext()));
        }
      }

      // Guard intrinsics read all memory, but don't write any memory.
      // Accordingly, don't update the generation but consume the last store (to
      // avoid an incorrect DSE).
      LastStore = nullptr;
      continue;
    }

    // If the instruction can be simplified (e.g. X+0 = X) then replace it with
    // its simpler value.
    if (Value *V = simplifyInstruction(&Inst, SQ)) {
      LLVM_DEBUG(dbgs() << "EarlyCSE Simplify: " << Inst << "  to: " << *V
                        << '\n');
      if (!DebugCounter::shouldExecute(CSECounter)) {
        LLVM_DEBUG(dbgs() << "Skipping due to debug counter\n");
      } else {
        bool Killed = false;
        if (!Inst.use_empty()) {
          Inst.replaceAllUsesWith(V);
          LLVM_DEBUG(dbgs() << "Inst.replaceAllUsesWith(V);\n");
          Changed = true;
        }
        if (isInstructionTriviallyDead(&Inst, &TLI)) {
          salvageKnowledge(&Inst, &AC);
          removeMSSA(Inst);
          Inst.eraseFromParent();
          Changed = true;
          Killed = true;
        }
        if (Changed)
          ++NumSimplify;
        if (Killed)
          continue;
      }
    }

    // If this is a simple instruction that we can value number, process it.
    if (SimpleValue::canHandle(&Inst)) {
      if (auto *CI = dyn_cast<ConstrainedFPIntrinsic>(&Inst)) {
        assert(CI->getExceptionBehavior() != fp::ebStrict &&
               "Unexpected ebStrict from SimpleValue::canHandle()");
        assert((!CI->getRoundingMode() ||
                CI->getRoundingMode() != RoundingMode::Dynamic) &&
               "Unexpected dynamic rounding from SimpleValue::canHandle()");
      }
      // See if the instruction has an available value.  If so, use it.
      if (Value *V = AvailableValues.lookup(&Inst)) {
        LLVM_DEBUG(dbgs() << "EarlyCSE CSE: " << Inst << "  to: " << *V
                          << '\n');
        if (!DebugCounter::shouldExecute(CSECounter)) {
          LLVM_DEBUG(dbgs() << "Skipping due to debug counter\n");
          continue;
        }
        if (auto *I = dyn_cast<Instruction>(V)) {
          // If I being poison triggers UB, there is no need to drop those
          // flags. Otherwise, only retain flags present on both I and Inst.
          // TODO: Currently some fast-math flags are not treated as
          // poison-generating even though they should. Until this is fixed,
          // always retain flags present on both I and Inst for floating point
          // instructions.
          if (isa<FPMathOperator>(I) || (I->hasPoisonGeneratingFlags() && !programUndefinedIfPoison(I)))
            I->andIRFlags(&Inst);
        }
        Inst.replaceAllUsesWith(V);
        salvageKnowledge(&Inst, &AC);
        removeMSSA(Inst);
        Inst.eraseFromParent();
        Changed = true;
        ++NumCSE;
        continue;
      }

      // Otherwise, just remember that this value is available.
      AvailableValues.insert(&Inst, &Inst);
      continue;
    }

    ParseMemoryInst MemInst(&Inst, TTI);
    // If this is a non-volatile load, process it.
    if (MemInst.isValid() && MemInst.isLoad()) {
      // (conservatively) we can't peak past the ordering implied by this
      // operation, but we can add this load to our set of available values
      if (MemInst.isVolatile() || !MemInst.isUnordered()) {
        LastStore = nullptr;
        ++CurrentGeneration;
      }

      if (MemInst.isInvariantLoad()) {
        // If we pass an invariant load, we know that memory location is
        // indefinitely constant from the moment of first dereferenceability.
        // We conservatively treat the invariant_load as that moment.  If we
        // pass a invariant load after already establishing a scope, don't
        // restart it since we want to preserve the earliest point seen.
        auto MemLoc = MemoryLocation::get(&Inst);
        if (!AvailableInvariants.count(MemLoc))
          AvailableInvariants.insert(MemLoc, CurrentGeneration);
      }

      // If we have an available version of this load, and if it is the right
      // generation or the load is known to be from an invariant location,
      // replace this instruction.
      //
      // If either the dominating load or the current load are invariant, then
      // we can assume the current load loads the same value as the dominating
      // load.
      LoadValue InVal = AvailableLoads.lookup(MemInst.getPointerOperand());
      if (Value *Op = getMatchingValue(InVal, MemInst, CurrentGeneration)) {
        LLVM_DEBUG(dbgs() << "EarlyCSE CSE LOAD: " << Inst
                          << "  to: " << *InVal.DefInst << '\n');
        if (!DebugCounter::shouldExecute(CSECounter)) {
          LLVM_DEBUG(dbgs() << "Skipping due to debug counter\n");
          continue;
        }
        if (auto *I = dyn_cast<Instruction>(Op))
          combineMetadataForCSE(I, &Inst, false);
        if (!Inst.use_empty())
          Inst.replaceAllUsesWith(Op);
        salvageKnowledge(&Inst, &AC);
        removeMSSA(Inst);
        Inst.eraseFromParent();
        Changed = true;
        ++NumCSELoad;
        continue;
      }

      // Otherwise, remember that we have this instruction.
      AvailableLoads.insert(MemInst.getPointerOperand(),
                            LoadValue(&Inst, CurrentGeneration,
                                      MemInst.getMatchingId(),
                                      MemInst.isAtomic()));
      LastStore = nullptr;
      continue;
    }

    // If this instruction may read from memory or throw (and potentially read
    // from memory in the exception handler), forget LastStore.  Load/store
    // intrinsics will indicate both a read and a write to memory.  The target
    // may override this (e.g. so that a store intrinsic does not read from
    // memory, and thus will be treated the same as a regular store for
    // commoning purposes).
    if ((Inst.mayReadFromMemory() || Inst.mayThrow()) &&
        !(MemInst.isValid() && !MemInst.mayReadFromMemory()))
      LastStore = nullptr;

    // If this is a read-only call, process it.
    if (CallValue::canHandle(&Inst)) {
      // If we have an available version of this call, and if it is the right
      // generation, replace this instruction.
      std::pair<Instruction *, unsigned> InVal = AvailableCalls.lookup(&Inst);
      if (InVal.first != nullptr &&
          isSameMemGeneration(InVal.second, CurrentGeneration, InVal.first,
                              &Inst)) {
        LLVM_DEBUG(dbgs() << "EarlyCSE CSE CALL: " << Inst
                          << "  to: " << *InVal.first << '\n');
        if (!DebugCounter::shouldExecute(CSECounter)) {
          LLVM_DEBUG(dbgs() << "Skipping due to debug counter\n");
          continue;
        }
        if (!Inst.use_empty())
          Inst.replaceAllUsesWith(InVal.first);
        salvageKnowledge(&Inst, &AC);
        removeMSSA(Inst);
        Inst.eraseFromParent();
        Changed = true;
        ++NumCSECall;
        continue;
      }

      // Otherwise, remember that we have this instruction.
      AvailableCalls.insert(&Inst, std::make_pair(&Inst, CurrentGeneration));
      continue;
    }

    // A release fence requires that all stores complete before it, but does
    // not prevent the reordering of following loads 'before' the fence.  As a
    // result, we don't need to consider it as writing to memory and don't need
    // to advance the generation.  We do need to prevent DSE across the fence,
    // but that's handled above.
    if (auto *FI = dyn_cast<FenceInst>(&Inst))
      if (FI->getOrdering() == AtomicOrdering::Release) {
        assert(Inst.mayReadFromMemory() && "relied on to prevent DSE above");
        continue;
      }

    // write back DSE - If we write back the same value we just loaded from
    // the same location and haven't passed any intervening writes or ordering
    // operations, we can remove the write.  The primary benefit is in allowing
    // the available load table to remain valid and value forward past where
    // the store originally was.
    if (MemInst.isValid() && MemInst.isStore()) {
      LoadValue InVal = AvailableLoads.lookup(MemInst.getPointerOperand());
      if (InVal.DefInst &&
          InVal.DefInst == getMatchingValue(InVal, MemInst, CurrentGeneration)) {
        // It is okay to have a LastStore to a different pointer here if MemorySSA
        // tells us that the load and store are from the same memory generation.
        // In that case, LastStore should keep its present value since we're
        // removing the current store.
        assert((!LastStore ||
                ParseMemoryInst(LastStore, TTI).getPointerOperand() ==
                    MemInst.getPointerOperand() ||
                MSSA) &&
               "can't have an intervening store if not using MemorySSA!");
        LLVM_DEBUG(dbgs() << "EarlyCSE DSE (writeback): " << Inst << '\n');
        if (!DebugCounter::shouldExecute(CSECounter)) {
          LLVM_DEBUG(dbgs() << "Skipping due to debug counter\n");
          continue;
        }
        salvageKnowledge(&Inst, &AC);
        removeMSSA(Inst);
        Inst.eraseFromParent();
        Changed = true;
        ++NumDSE;
        // We can avoid incrementing the generation count since we were able
        // to eliminate this store.
        continue;
      }
    }

    // Okay, this isn't something we can CSE at all.  Check to see if it is
    // something that could modify memory.  If so, our available memory values
    // cannot be used so bump the generation count.
    if (Inst.mayWriteToMemory()) {
      ++CurrentGeneration;

      if (MemInst.isValid() && MemInst.isStore()) {
        // We do a trivial form of DSE if there are two stores to the same
        // location with no intervening loads.  Delete the earlier store.
        if (LastStore) {
          if (overridingStores(ParseMemoryInst(LastStore, TTI), MemInst)) {
            LLVM_DEBUG(dbgs() << "EarlyCSE DEAD STORE: " << *LastStore
                              << "  due to: " << Inst << '\n');
            if (!DebugCounter::shouldExecute(CSECounter)) {
              LLVM_DEBUG(dbgs() << "Skipping due to debug counter\n");
            } else {
              salvageKnowledge(&Inst, &AC);
              removeMSSA(*LastStore);
              LastStore->eraseFromParent();
              LLVM_DEBUG(dbgs() << "LastStore->eraseFromParent();\n");
              Changed = true;
              ++NumDSE;
              LastStore = nullptr;
            }
          }
          // fallthrough - we can exploit information about this store
        }

        // Okay, we just invalidated anything we knew about loaded values.  Try
        // to salvage *something* by remembering that the stored value is a live
        // version of the pointer.  It is safe to forward from volatile stores
        // to non-volatile loads, so we don't have to check for volatility of
        // the store.
        AvailableLoads.insert(MemInst.getPointerOperand(),
                              LoadValue(&Inst, CurrentGeneration,
                                        MemInst.getMatchingId(),
                                        MemInst.isAtomic()));

        // Remember that this was the last unordered store we saw for DSE. We
        // don't yet handle DSE on ordered or volatile stores since we don't
        // have a good way to model the ordering requirement for following
        // passes  once the store is removed.  We could insert a fence, but
        // since fences are slightly stronger than stores in their ordering,
        // it's not clear this is a profitable transform. Another option would
        // be to merge the ordering with that of the post dominating store.
        if (MemInst.isUnordered() && !MemInst.isVolatile())
          LastStore = &Inst;
        else
          LastStore = nullptr;
      }
    }
  }

  return Changed;
}

