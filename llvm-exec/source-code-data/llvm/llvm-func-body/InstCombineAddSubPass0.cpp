Instruction *InstCombinerImpl::visitFSub(BinaryOperator &I) {
  if (Value *V = simplifyFSubInst(I.getOperand(0), I.getOperand(1),
                                  I.getFastMathFlags(),
                                  getSimplifyQuery().getWithInstruction(&I)))
    return replaceInstUsesWith(I, V);

  if (I.hasAllowReassoc() && I.hasNoSignedZeros()) {
    // (Y - X) - Y --> -X
    if (match(Op0, m_FSub(m_Specific(Op1), m_Value(X))))
      return UnaryOperator::CreateFNegFMF(X, &I);    // Y - (X + Y) --> -X
    // Y - (Y + X) --> -X
    if (match(Op1, m_c_FAdd(m_Specific(Op0), m_Value(X))))
      return UnaryOperator::CreateFNegFMF(X, &I);

    // (X * C) - X --> X * (C - 1.0)
    if (match(Op0, m_FMul(m_Specific(Op1), m_Constant(C)))) {
      if (Constant *CSubOne = ConstantFoldBinaryOpOperands(
              Instruction::FSub, C, ConstantFP::get(Ty, 1.0), DL))
        return BinaryOperator::CreateFMulFMF(Op1, CSubOne, &I);
    }
    // X - (X * C) --> X * (1.0 - C)
    if (match(Op1, m_FMul(m_Specific(Op0), m_Constant(C)))) {
      if (Constant *OneSubC = ConstantFoldBinaryOpOperands(
              Instruction::FSub, ConstantFP::get(Ty, 1.0), C, DL))
        return BinaryOperator::CreateFMulFMF(Op0, OneSubC, &I);
    }

    // Reassociate fsub/fadd sequences to create more fadd instructions and
    // reduce dependency chains:
    // ((X - Y) + Z) - Op1 --> (X + Z) - (Y + Op1)
    Value *Z;
    if (match(Op0, m_OneUse(m_c_FAdd(m_OneUse(m_FSub(m_Value(X), m_Value(Y))),
                                     m_Value(Z))))) {
      Value *XZ = Builder.CreateFAddFMF(X, Z, &I);
      Value *YW = Builder.CreateFAddFMF(Y, Op1, &I);
      return BinaryOperator::CreateFSubFMF(XZ, YW, &I);
    }

    auto m_FaddRdx = [](Value *&Sum, Value *&Vec) {
      return m_OneUse(m_Intrinsic<Intrinsic::vector_reduce_fadd>(m_Value(Sum),
                                                                 m_Value(Vec)));
    };
    Value *A0, *A1, *V0, *V1;
    if (match(Op0, m_FaddRdx(A0, V0)) && match(Op1, m_FaddRdx(A1, V1)) &&
        V0->getType() == V1->getType()) {
      // Difference of sums is sum of differences:
      // add_rdx(A0, V0) - add_rdx(A1, V1) --> add_rdx(A0, V0 - V1) - A1
      Value *Sub = Builder.CreateFSubFMF(V0, V1, &I);
      Value *Rdx = Builder.CreateIntrinsic(Intrinsic::vector_reduce_fadd,
                                           {Sub->getType()}, {A0, Sub}, &I);
      return BinaryOperator::CreateFSubFMF(Rdx, A1, &I);
    }

    if (Instruction *F = factorizeFAddFSub(I, Builder))
      return F;

    // TODO: This performs reassociative folds for FP ops. Some fraction of the
    // functionality has been subsumed by simple pattern matching here and in
    // InstSimplify. We should let a dedicated reassociation pass handle more
    // complex pattern matching and remove this from InstCombine.
    if (Value *V = FAddCombine(Builder).simplify(&I))
      return replaceInstUsesWith(I, V);

    // (X - Y) - Op1 --> X - (Y + Op1)
    if (match(Op0, m_OneUse(m_FSub(m_Value(X), m_Value(Y))))) {
      Value *FAdd = Builder.CreateFAddFMF(Y, Op1, &I);
      return BinaryOperator::CreateFSubFMF(X, FAdd, &I);
    }
  }

  return nullptr;
}

