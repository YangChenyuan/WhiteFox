static Function *promoteArguments(Function *F, FunctionAnalysisManager &FAM,
                                  unsigned MaxElements, bool IsRecursive) {
  // Don't perform argument promotion for naked functions; otherwise we can end
  // up removing parameters that are seemingly 'not used' as they are referred
  // to in the assembly.
  if (F->hasFnAttribute(Attribute::Naked))
    return nullptr;

  // Make sure that it is local to this module.
  if (!F->hasLocalLinkage())
    return nullptr;

  // Don't promote arguments for variadic functions. Adding, removing, or
  // changing non-pack parameters can change the classification of pack
  // parameters. Frontends encode that classification at the call site in the
  // IR, while in the callee the classification is determined dynamically based
  // on the number of registers consumed so far.
  if (F->isVarArg())
    return nullptr;

  // Don't transform functions that receive inallocas, as the transformation may
  // not be safe depending on calling convention.
  if (F->getAttributes().hasAttrSomewhere(Attribute::InAlloca))
    return nullptr;

  // First check: see if there are any pointer arguments!  If not, quick exit.
  SmallVector<Argument *, 16> PointerArgs;
  for (Argument &I : F->args())
    if (I.getType()->isPointerTy())
      PointerArgs.push_back(&I);
  if (PointerArgs.empty())
    return nullptr;

  // Second check: make sure that all callers are direct callers.  We can't
  // transform functions that have indirect callers.  Also see if the function
  // is self-recursive.
  for (Use &U : F->uses()) {
    CallBase *CB = dyn_cast<CallBase>(U.getUser());
    // Must be a direct call.
    if (CB == nullptr || !CB->isCallee(&U) ||
        CB->getFunctionType() != F->getFunctionType())
      return nullptr;    // Can't change signature of musttail callee
    if (CB->isMustTailCall())
      return nullptr;

    if (CB->getFunction() == F)
      IsRecursive = true;
  }

  // Can't change signature of musttail caller
  // FIXME: Support promoting whole chain of musttail functions
  for (BasicBlock &BB : *F)
    if (BB.getTerminatingMustTailCall())
      return nullptr;

  // No promotable pointer arguments.
  if (ArgsToPromote.empty())
    return nullptr;

  if (NumArgsAfterPromote > TTI.getMaxNumArgs())
    return nullptr;

  return doPromotion(F, FAM, ArgsToPromote);
}

PreservedAnalyses ArgumentPromotionPass::run(LazyCallGraph::SCC &C,
                                             CGSCCAnalysisManager &AM,
                                             LazyCallGraph &CG,
                                             CGSCCUpdateResult &UR) {
  bool Changed = false, LocalChange;

  // Iterate until we stop promoting from this SCC.
  do {
    LocalChange = false;    FunctionAnalysisManager &FAM =
        AM.getResult<FunctionAnalysisManagerCGSCCProxy>(C, CG).getManager();

    bool IsRecursive = C.size() > 1;
    for (LazyCallGraph::Node &N : C) {
      Function &OldF = N.getFunction();
      Function *NewF = promoteArguments(&OldF, FAM, MaxElements, IsRecursive);
      if (!NewF)
        continue;
      LocalChange = true;

      // Directly substitute the functions in the call graph. Note that this
      // requires the old function to be completely dead and completely
      // replaced by the new function. It does no call graph updates, it merely
      // swaps out the particular function mapped to a particular node in the
      // graph.
      C.getOuterRefSCC().replaceNodeFunction(N, *NewF);
      LLVM_DEBUG(dbgs() << "C.getOuterRefSCC().replaceNodeFunction(N, *NewF);\n");
      FAM.clear(OldF, OldF.getName());
      OldF.eraseFromParent();

      PreservedAnalyses FuncPA;
      FuncPA.preserveSet<CFGAnalyses>();
      for (auto *U : NewF->users()) {
        auto *UserF = cast<CallBase>(U)->getFunction();
        FAM.invalidate(*UserF, FuncPA);
      }
    }

    Changed |= LocalChange;
  } while (LocalChange);

  PreservedAnalyses PA;
  // We've cleared out analyses for deleted functions.
  PA.preserve<FunctionAnalysisManagerCGSCCProxy>();
  // We've manually invalidated analyses for functions we've modified.
  PA.preserveSet<AllAnalysesOn<Function>>();
  return PA;
}

