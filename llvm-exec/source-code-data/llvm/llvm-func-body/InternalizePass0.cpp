bool InternalizePass::internalizeModule(Module &M) {
  bool Changed = false;

  // Mark all functions not in the api as internal.
  IsWasm = Triple(M.getTargetTriple()).isOSBinFormatWasm();
  for (Function &I : M) {
    if (!maybeInternalize(I, ComdatMap))
      continue;
    Changed = true;

  // Mark all global variables with initializers that are not in the api as
  // internal as well.
  for (auto &GV : M.globals()) {
    if (!maybeInternalize(GV, ComdatMap))
      continue;
    Changed = true;

  // Mark all aliases that are not in the api as internal as well.
  for (auto &GA : M.aliases()) {
    if (!maybeInternalize(GA, ComdatMap))
      continue;
    Changed = true;

  return Changed;
}

bool InternalizePass::maybeInternalize(
    GlobalValue &GV, DenseMap<const Comdat *, ComdatInfo> &ComdatMap) {

  SmallString<0> ComdatName;
  if (Comdat *C = GV.getComdat())     if (auto *GO = dyn_cast<GlobalObject>(&GV)) {
      // If a comdat with one member is not externally visible, we can drop it.
      // Otherwise, the comdat can be used to establish dependencies among the
      // group of sections. Thus we have to keep the comdat but switch it to
      // nodeduplicate.
      // Note: nodeduplicate is not necessary for COFF. wasm doesn't support
      // nodeduplicate.
      ComdatInfo &Info = ComdatMap.find(C)->second;
      if (Info.Size == 1)
        GO->setComdat(nullptr);
      else if (!IsWasm)
        C->setSelectionKind(Comdat::NoDeduplicate);
    }

    if (GV.hasLocalLinkage())
      return false;
  } else {
    if (GV.hasLocalLinkage())
      return false;

    if (shouldPreserveGV(GV))
      return false;
  }

  GV.setVisibility(GlobalValue::DefaultVisibility);
  GV.setLinkage(GlobalValue::InternalLinkage);
  return true;
}

