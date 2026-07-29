//===-- SHLiteralPool.cpp - SH inline literal-island helpers --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHLiteralPool.h"
#include "SHInstrInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSymbol.h"

using namespace llvm;

bool llvm::isSHLiteralIslandBlock(const MachineBasicBlock &MBB) {
  bool HasEntry = false;
  for (const MachineInstr &MI : MBB) {
    if (MI.isMetaInstruction())
      continue;
    if (MI.getOpcode() != SH::SH_CONSTPOOL_ENTRY)
      return false;
    HasEntry = true;
  }
  return HasEntry;
}

MCSymbol *llvm::getSHLiteralIslandSymbol(MCContext &Ctx, const DataLayout &DL,
                                         unsigned FunctionNumber, unsigned CPI,
                                         unsigned Instance) {
  return Ctx.getOrCreateSymbol(Twine(DL.getInternalSymbolPrefix()) + "CPI" +
                               Twine(FunctionNumber) + "_" + Twine(CPI) + "_" +
                               Twine(Instance));
}
