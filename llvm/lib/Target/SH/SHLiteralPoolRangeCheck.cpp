//===-- SHLiteralPoolRangeCheck.cpp - Validate trailing literal pools -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHInstrInfo.h"
#include "SHLiteralPool.h"
#include "SHSubtarget.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include <limits>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "sh-literal-pool-range-check"

namespace {

struct LiteralUse {
  uint64_t Offset;
  unsigned CPI;
};

static uint64_t checkedAdd(uint64_t LHS, uint64_t RHS) {
  if (LHS > std::numeric_limits<uint64_t>::max() - RHS)
    report_fatal_error("SH literal pool code layout overflow");
  return LHS + RHS;
}

static uint64_t alignBlockOffset(uint64_t Offset, const MachineBasicBlock &MBB,
                                 Align FunctionAlignment) {
  Align Alignment = MBB.getAlignment();
  if (Alignment > FunctionAlignment)
    report_fatal_error(
        "SH literal pool cannot validate a basic-block alignment greater than "
        "the function alignment");
  uint64_t Aligned = alignTo(Offset, Alignment);
  uint64_t Padding = Aligned - Offset;
  unsigned MaxPadding = MBB.getMaxBytesForAlignment();
  return MaxPadding != 0 && Padding > MaxPadding ? Offset : Aligned;
}

static std::optional<bool> hasPoolBarrier(const MachineBasicBlock &MBB) {
  auto I = MBB.rbegin();
  while (I != MBB.rend() && I->isMetaInstruction())
    ++I;
  if (I == MBB.rend())
    return std::nullopt;

  while (I->isBundledWithPred()) {
    ++I;
    if (I == MBB.rend())
      return false;
  }

  if (!I->isBundle())
    return I->isBarrier();

  bool InBundle = false;
  for (const MachineInstr &MI : MBB) {
    if (&MI == &*I) {
      InBundle = true;
      continue;
    }
    if (!InBundle)
      continue;
    if (!MI.isBundledWithPred())
      break;
    if (MI.isBarrier())
      return true;
  }
  return false;
}

static bool hasSafePoolBarrier(const MachineFunction &MF) {
  for (auto I = MF.rbegin(), E = MF.rend(); I != E; ++I) {
    std::optional<bool> Barrier = hasPoolBarrier(*I);
    if (Barrier)
      return *Barrier;
    if (!I->pred_empty())
      return false;
  }
  return false;
}

class RangeCheckImpl {
public:
  void run(MachineFunction &MF) const {
    const auto &Constants = MF.getConstantPool()->getConstants();
    if (Constants.empty())
      return;

    if (MF.getAlignment() < Align(4))
      report_fatal_error(
          "SH literal pool function alignment must be at least four");
    if (MF.empty() || !hasSafePoolBarrier(MF))
      report_fatal_error("SH literal pool would be reachable by fallthrough");

    const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
    SmallVector<LiteralUse, 8> Uses;
    uint64_t Offset = 0;

    for (const MachineBasicBlock &MBB : MF) {
      if (MBB.isBeginSection() && !MBB.isEntryBlock())
        report_fatal_error(
            "SH literal pools do not support basic-block sections");
      Offset = alignBlockOffset(Offset, MBB, MF.getAlignment());

      for (const MachineInstr &MI : MBB) {
        if (MI.isBundledWithPred())
          continue;
        if (MI.isInlineAsm())
          report_fatal_error(
              "SH literal pool layout does not support inline assembly");
        if (MI.getOpcode() == SH::MOVL_load_pc) {
          if (!MI.getOperand(1).isCPI())
            report_fatal_error(
                "SH PC-relative literal load must reference a constant pool");
          Uses.push_back(
              {Offset, static_cast<unsigned>(MI.getOperand(1).getIndex())});
        }

        unsigned Size = TII.getInstSizeInBytes(MI);
        if (Size == 0 && !MI.isMetaInstruction())
          report_fatal_error(
              "SH literal pool layout encountered an unexpanded instruction");
        Offset = checkedAdd(Offset, Size);
      }
    }

    uint64_t PoolStart = alignTo(Offset, Align(4));
    SHLiteralPoolLayout Layout = computeSHLiteralPoolLayout(MF);
    for (const LiteralUse &Use : Uses) {
      if (Use.CPI >= Layout.Entries.size())
        report_fatal_error(
            "SH literal load has an invalid constant-pool index");
      uint64_t EntryOffset =
          checkedAdd(PoolStart, Layout.Entries[Use.CPI].Offset);
      uint64_t Base = (Use.Offset & ~UINT64_C(3)) + 4;
      int64_t Distance;
      if (EntryOffset >= Base) {
        uint64_t UnsignedDistance = EntryOffset - Base;
        if (UnsignedDistance >
            static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
          report_fatal_error("SH literal pool code layout overflow");
        Distance = static_cast<int64_t>(UnsignedDistance);
      } else {
        uint64_t UnsignedDistance = Base - EntryOffset;
        if (UnsignedDistance >
            static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
          report_fatal_error("SH literal pool code layout overflow");
        Distance = -static_cast<int64_t>(UnsignedDistance);
      }

      if (Distance < 0)
        report_fatal_error(Twine("SH literal pool entry is behind the "
                                 "instruction: function ") +
                           MF.getName() + ", constant-pool index " +
                           Twine(Use.CPI) + ", instruction offset " +
                           Twine(Use.Offset) + ", pool-entry offset " +
                           Twine(EntryOffset) + ", distance " +
                           Twine(Distance));
      if ((Distance & 3) != 0)
        report_fatal_error(Twine("SH literal pool entry is misaligned: "
                                 "function ") +
                           MF.getName() + ", constant-pool index " +
                           Twine(Use.CPI) + ", instruction offset " +
                           Twine(Use.Offset) + ", pool-entry offset " +
                           Twine(EntryOffset) + ", distance " +
                           Twine(Distance));
      if (Distance > 1020)
        report_fatal_error(Twine("SH literal pool entry is out of range: "
                                 "function ") +
                           MF.getName() + ", constant-pool index " +
                           Twine(Use.CPI) + ", instruction offset " +
                           Twine(Use.Offset) + ", pool-entry offset " +
                           Twine(EntryOffset) + ", distance " +
                           Twine(Distance) + ", allowed range 0..1020");
    }
  }
};

class SHLiteralPoolRangeCheckLegacy : public MachineFunctionPass {
public:
  static char ID;
  SHLiteralPoolRangeCheckLegacy() : MachineFunctionPass(ID) {}
  StringRef getPassName() const override {
    return "SH Literal Pool Range Check";
  }
  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().setNoVRegs();
  }
  bool runOnMachineFunction(MachineFunction &MF) override {
    RangeCheckImpl().run(MF);
    return false;
  }
};

char SHLiteralPoolRangeCheckLegacy::ID = 0;

} // namespace

INITIALIZE_PASS(SHLiteralPoolRangeCheckLegacy, DEBUG_TYPE,
                "Validate SH trailing literal-pool ranges", false, true)

FunctionPass *llvm::createSHLiteralPoolRangeCheckLegacyPass() {
  return new SHLiteralPoolRangeCheckLegacy();
}

PreservedAnalyses
SHLiteralPoolRangeCheckPass::run(MachineFunction &MF,
                                 MachineFunctionAnalysisManager &MFAM) {
  RangeCheckImpl().run(MF);
  return PreservedAnalyses::all();
}
