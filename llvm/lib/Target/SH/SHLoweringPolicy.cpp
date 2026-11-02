//===-- SHLoweringPolicy.cpp - SH scalar lowering policies ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHISelLowering.h"
#include "SHRegisterInfo.h"
#include "SHSubtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

bool SHTargetLowering::isIntDivCheap(EVT VT, AttributeList Attr) const {
  if (VT != MVT::i32 || Attr.hasFnAttr(Attribute::OptimizeForSize) ||
      Attr.hasFnAttr(Attribute::MinSize))
    return false;
  // SH2+ widening multiplies allow LLVM's exact reciprocal division.
  // Preserve SH1 helpers, SH2A hardware division and unoptimized behavior.
  return getTargetMachine().getOptLevel() == CodeGenOptLevel::None ||
         Attr.hasFnAttr(Attribute::OptimizeNone) || !Subtarget.hasSH2() ||
         Subtarget.hasSH2A();
}

bool SHTargetLowering::isLegalAddressingMode(const DataLayout &DL,
                                             const AddrMode &AM, Type *Ty,
                                             unsigned AddrSpace,
                                             Instruction *I) const {
  if (AM.BaseGV || AM.ScalableOffset)
    return false;
  // Inline assembly uses an unknown address space. Always allow its address
  // to be computed in a register, without assuming a displacement is legal.
  if (AddrSpace != 0)
    return AddrSpace == ~0u && AM.HasBaseReg && AM.Scale == 0 &&
           AM.BaseOffs == 0;
  if (AM.Scale != 0)
    return AM.Scale == 1 && AM.BaseOffs == 0;
  if (!AM.HasBaseReg)
    return false;
  if (AM.BaseOffs == 0)
    return true;
  // Scalar FPU accesses can materialize a signed byte offset in R0 and use
  // the indexed form. Let CodeGenPrepare sink this two-instruction address
  // form instead of keeping separate adjusted pointers live across blocks.
  if (Ty && Ty->isFloatTy() && Subtarget.hasSingleFPU() &&
      isInt<8>(AM.BaseOffs))
    return true;
  if (!Ty || AM.BaseOffs < 0)
    return false;

  // Integer displacements are unsigned and scaled by the access width.
  // Floating-point registers only have register-indirect and indexed forms.
  if (Ty->isIntegerTy(8))
    return AM.BaseOffs <= 15;
  if (Ty->isIntegerTy(16))
    return AM.BaseOffs <= 30 && AM.BaseOffs % 2 == 0;
  if (Ty->isPointerTy() || Ty->isIntegerTy(32) ||
      (Ty->isFloatTy() && !Subtarget.hasSingleFPU()))
    return AM.BaseOffs <= 60 && AM.BaseOffs % 4 == 0;
  // Both longword accesses of a legalized doubleword must fit.
  if (Ty->isIntegerTy(64) || (Ty->isDoubleTy() && !Subtarget.hasDoubleFPU()))
    return AM.BaseOffs <= 56 && AM.BaseOffs % 4 == 0;
  return false;
}

bool SHTargetLowering::allowsMisalignedMemoryAccesses(
    EVT VT, unsigned AddrSpace, Align Alignment, MachineMemOperand::Flags Flags,
    unsigned *Fast) const {
  if (Fast)
    *Fast = 0;
  if (VT == MVT::v4f32 && Subtarget.hasFPUVector() && AddrSpace == 0 &&
      Alignment >= Align(4) && !(Flags & MachineMemOperand::MOVolatile)) {
    // The vector transport pseudo chooses two aligned pairs or four words.
    if (Fast)
      *Fast = Alignment >= Align(8);
    return true;
  }
  return false;
}

bool SHTargetLowering::findOptimalMemOpLowering(
    LLVMContext &Context, std::vector<EVT> &MemOps, unsigned Limit,
    const MemOp &Op, unsigned DstAS, unsigned SrcAS, const AttributeList &Attrs,
    EVT *LargestVT) const {
  // IR validation admits only bounded, constant-size volatile intrinsics.
  // Keep their accesses explicit even in size mode: ordinary libc routines
  // do not promise volatile accesses. The native expander retains alignment,
  // overlap and memory flags; ordinary operations use its normal store limit.
  if (Op.isVolatile())
    Limit = ~0U;
  return TargetLowering::findOptimalMemOpLowering(
      Context, MemOps, Limit, Op, DstAS, SrcAS, Attrs, LargestVT);
}

EVT SHTargetLowering::getOptimalMemOpType(
    LLVMContext &Context, const MemOp &Op,
    const AttributeList &FuncAttributes) const {
  // A matrix-sized transfer fits the four ordinary FV registers. The native
  // memmove expander loads all columns before any store, retaining snapshot
  // semantics even when the source and destination overlap. Do not introduce
  // FP registers into handlers, explicit noimplicitfloat or volatile copies.
  if (Subtarget.hasFPUVector() && Op.isMemcpyOrMemmove() && Op.size() == 64 &&
      Op.isAligned(Align(8)) && !Op.isVolatile() &&
      !FuncAttributes.hasFnAttr(Attribute::NoImplicitFloat) &&
      !FuncAttributes.hasFnAttr(Attribute::StrictFP) &&
      !FuncAttributes.hasFnAttr("sh-interrupt-handler"))
    return MVT::v4f32;
  if (Op.isAligned(Align(4)))
    return MVT::i32;
  if (Op.isAligned(Align(2)))
    return MVT::i16;
  return MVT::i8;
}
