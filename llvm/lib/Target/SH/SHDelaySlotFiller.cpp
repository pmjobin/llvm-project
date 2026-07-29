//===-- SHDelaySlotFiller.cpp - Fill SH delay slots with nops -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHInstrInfo.h"
#include "SHSubtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachinePassManager.h"

using namespace llvm;

#define DEBUG_TYPE "sh-delay-slot-filler"

namespace {

class FillerImpl {
public:
  bool run(MachineFunction &MF) {
    const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
    bool Changed = false;
    for (MachineBasicBlock &MBB : MF) {
      for (MachineBasicBlock::instr_iterator I = MBB.instr_begin();
           I != MBB.instr_end(); ++I) {
        if (!I->getDesc().hasDelaySlot() || I->isBundledWithSucc())
          continue;
        MachineBasicBlock::instr_iterator Slot = std::next(I);
        if (Slot == MBB.instr_end() || Slot->getOpcode() != SH::NOP ||
            Slot->isBundledWithPred()) {
          BuildMI(MBB, Slot, DebugLoc(), TII->get(SH::NOP));
          Slot = std::next(I);
        }
        MIBundleBuilder(MBB, I, std::next(Slot));
        I = Slot;
        Changed = true;
      }
    }
    return Changed;
  }
};

class SHDelaySlotFillerLegacy : public MachineFunctionPass {
public:
  static char ID;
  SHDelaySlotFillerLegacy() : MachineFunctionPass(ID) {}
  StringRef getPassName() const override { return "SH Delay Slot Filler"; }
  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().setNoVRegs();
  }
  bool runOnMachineFunction(MachineFunction &MF) override {
    FillerImpl Impl;
    return Impl.run(MF);
  }
};

char SHDelaySlotFillerLegacy::ID = 0;

} // namespace

INITIALIZE_PASS(SHDelaySlotFillerLegacy, DEBUG_TYPE, "Fill SH delay slots",
                false, false)

FunctionPass *llvm::createSHDelaySlotFillerLegacyPass() {
  return new SHDelaySlotFillerLegacy();
}

PreservedAnalyses
SHDelaySlotFillerPass::run(MachineFunction &MF,
                           MachineFunctionAnalysisManager &MFAM) {
  FillerImpl Impl;
  return Impl.run(MF) ? getMachineFunctionPassPreservedAnalyses()
                            .preserveSet<CFGAnalyses>()
                      : PreservedAnalyses::all();
}
