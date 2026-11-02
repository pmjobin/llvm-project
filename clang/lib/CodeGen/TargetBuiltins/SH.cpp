//===------ SH.cpp - Emit LLVM Code for SH builtins -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CGBuiltin.h"
#include "clang/Basic/TargetBuiltins.h"
#include "llvm/IR/IntrinsicsSH.h"

using namespace clang;
using namespace CodeGen;
using namespace llvm;

Value *CodeGenFunction::EmitSHBuiltinExpr(unsigned BuiltinID,
                                          const CallExpr *E) {
  bool IsGraphics = false;
  switch (BuiltinID) {
  case SH::BI__builtin_sh_fsca:
  case SH::BI__builtin_sh_fsrra:
  case SH::BI__builtin_sh_fipr:
  case SH::BI__builtin_sh_ftrv:
  case SH::BI__builtin_sh_ftrv4:
  case SH::BI__builtin_sh_ftrv_array:
  case SH::BI__builtin_sh_ftrv_compose:
    IsGraphics = true;
    break;
  default:
    break;
  }
  if (IsGraphics && (CurFn->hasFnAttribute("sh-interrupt-handler") ||
                     CurFn->hasFnAttribute(llvm::Attribute::StrictFP))) {
    CGM.ErrorUnsupported(
        E, "SH graphics intrinsic in a handler or strict FP function");
    return nullptr;
  }
  if ((BuiltinID == SH::BI__builtin_sh_get_fpscr ||
       BuiltinID == SH::BI__builtin_sh_set_fpscr) &&
      CurFn->hasFnAttribute("sh-interrupt-handler")) {
    CGM.ErrorUnsupported(E, "SH FPSCR control intrinsic in a handler");
    return nullptr;
  }
  switch (BuiltinID) {
  default:
    return nullptr;
  case SH::BI__builtin_set_thread_pointer:
    return Builder.CreateCall(
        CGM.getIntrinsic(Intrinsic::sh_set_thread_pointer),
        EmitScalarExpr(E->getArg(0)));
  case SH::BI__builtin_sh_get_fpscr:
    return Builder.CreateCall(CGM.getIntrinsic(Intrinsic::sh_get_fpscr));
  case SH::BI__builtin_sh_set_fpscr:
    return Builder.CreateCall(CGM.getIntrinsic(Intrinsic::sh_set_fpscr),
                              EmitScalarExpr(E->getArg(0)));
  case SH::BI__builtin_sh_fsca: {
    Value *Pair = Builder.CreateCall(CGM.getIntrinsic(Intrinsic::sh_fsca),
                                     EmitScalarExpr(E->getArg(0)));
    Value *V = PoisonValue::get(ConvertType(E->getType()));
    V = Builder.CreateInsertElement(V, Builder.CreateExtractValue(Pair, 0),
                                    uint64_t(0));
    return Builder.CreateInsertElement(V, Builder.CreateExtractValue(Pair, 1),
                                       1);
  }
  case SH::BI__builtin_sh_fsrra:
  case SH::BI__builtin_sh_fipr:
  case SH::BI__builtin_sh_ftrv:
  case SH::BI__builtin_sh_ftrv4:
  case SH::BI__builtin_sh_ftrv_array:
  case SH::BI__builtin_sh_ftrv_compose: {
    Intrinsic::ID ID =
        BuiltinID == SH::BI__builtin_sh_fsrra   ? Intrinsic::sh_fsrra
        : BuiltinID == SH::BI__builtin_sh_fipr  ? Intrinsic::sh_fipr
        : BuiltinID == SH::BI__builtin_sh_ftrv  ? Intrinsic::sh_ftrv
        : BuiltinID == SH::BI__builtin_sh_ftrv4 ? Intrinsic::sh_ftrv4
        : BuiltinID == SH::BI__builtin_sh_ftrv_array
            ? Intrinsic::sh_ftrv_array
            : Intrinsic::sh_ftrv_compose;
    SmallVector<Value *, 5> Args;
    SmallVector<std::pair<unsigned, llvm::Align>, 4> Alignments;
    for (const Expr *Arg : E->arguments()) {
      if (Arg->getType()->isPointerType()) {
        Address A = EmitPointerWithAlignment(Arg);
        Alignments.emplace_back(Args.size(), A.getAlignment().getAsAlign());
        Args.push_back(A.emitRawPointer(*this));
      } else {
        Args.push_back(EmitScalarExpr(Arg));
      }
    }
    auto *Call = Builder.CreateCall(CGM.getIntrinsic(ID), Args);
    for (auto [Index, Alignment] : Alignments)
      Call->addParamAttr(Index, llvm::Attribute::getWithAlignment(
                                    getLLVMContext(), Alignment));
    return Call;
  }
  case SH::BI__builtin_sh_tas_b: {
    Value *Address = EmitScalarExpr(E->getArg(0));
    Function *Intrinsic = CGM.getIntrinsic(Intrinsic::sh_tas_b);
    return Builder.CreateCall(Intrinsic, Address);
  }
  }
}
