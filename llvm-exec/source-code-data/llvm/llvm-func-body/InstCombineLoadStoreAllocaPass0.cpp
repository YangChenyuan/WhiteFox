Instruction *InstCombinerImpl::visitAllocaInst(AllocaInst &AI) {
  if (auto *I = simplifyAllocaArraySize(*this, AI, DT))
    return I;

  // Check to see if this allocation is only modified by a memcpy/memmove from
  // a memory location whose alignment is equal to or exceeds that of the
  // allocation. If this is the case, we can change all users to use the
  // constant memory location instead.  This is commonly produced by the CFE by
  // constructs like "void foo() { int A[] = {1,2,3,4,5,6,7,8,9...}; }" if 'A'
  // is only subsequently read.
  SmallVector<Instruction *, 4> ToDelete;
  if (MemTransferInst *Copy = isOnlyCopiedFromConstantMemory(AA, &AI, ToDelete)) {
    Value *TheSrc = Copy->getSource();
    Align AllocaAlign = AI.getAlign();
    Align SourceAlign = getOrEnforceKnownAlignment(
      TheSrc, AllocaAlign, DL, &AI, &AC, &DT);
    if (AllocaAlign <= SourceAlign &&
        isDereferenceableForAllocaSize(TheSrc, &AI, DL) &&
        !isa<Instruction>(TheSrc)) {
      // FIXME: Can we sink instructions without violating dominance when TheSrc
      // is an instruction instead of a constant or argument?
      LLVM_DEBUG(dbgs() << "Found alloca equal to global: " << AI << '\n');
      LLVM_DEBUG(dbgs() << "  memcpy = " << *Copy << '\n');
      unsigned SrcAddrSpace = TheSrc->getType()->getPointerAddressSpace();
      auto *DestTy = PointerType::get(AI.getAllocatedType(), SrcAddrSpace);
      if (AI.getAddressSpace() == SrcAddrSpace) {
        for (Instruction *Delete : ToDelete)
          eraseInstFromFunction(*Delete);        Value *Cast = Builder.CreateBitCast(TheSrc, DestTy);
        Instruction *NewI = replaceInstUsesWith(AI, Cast);
        eraseInstFromFunction(*Copy);
        ++NumGlobalCopies;
        return NewI;
      }

      PointerReplacer PtrReplacer(*this, AI, SrcAddrSpace);
      if (PtrReplacer.collectUsers()) {
        for (Instruction *Delete : ToDelete)
          eraseInstFromFunction(*Delete);

        Value *Cast = Builder.CreateBitCast(TheSrc, DestTy);
        PtrReplacer.replacePointer(Cast);
        ++NumGlobalCopies;
      }
    }
  }

  // At last, use the generic allocation site handler to aggressively remove
  // unused allocas.
  return visitAllocSite(AI);
}

