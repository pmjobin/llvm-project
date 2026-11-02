//===-- SHMCInstLower.cpp - Lower SH MachineInstr to MCInst ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHMCInstLower.h"
#include "SHInstrInfo.h"
#include "SHLiteralPool.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

void SHMCInstLower::lower(const MachineInstr *MI, MCInst &OutMI) const {
  bool IsLongLiteral = MI->getOpcode() == SH::MOVL_load_pc_island;
  bool IsWordLiteral = MI->getOpcode() == SH::MOVW_load_pc_island;
  bool IsInlineJumpTableDispatch =
      MI->getOpcode() == SH::BRAF && MI->getNumExplicitOperands() == 3 &&
      MI->getOperand(0).isReg() && MI->getOperand(1).isJTI() &&
      MI->getOperand(2).isMCSymbol();
  if (MI->getOpcode() == SH::BRAF && MI->getNumExplicitOperands() != 1 &&
      !IsInlineJumpTableDispatch)
    reportFatalUsageError("SH inline jump-table BRAF metadata is malformed");
  OutMI.setOpcode(IsLongLiteral   ? static_cast<unsigned>(SH::MOVL_load_pc)
                  : IsWordLiteral ? static_cast<unsigned>(SH::MOVW_load_pc)
                                  : MI->getOpcode());
  for (unsigned I = 0, E = MI->getNumOperands(); I != E; ++I) {
    const MachineOperand &MO = MI->getOperand(I);
    if ((IsLongLiteral || IsWordLiteral) && I == 2)
      continue;
    switch (MO.getType()) {
    case MachineOperand::MO_Register:
      if (!MO.isImplicit())
        OutMI.addOperand(MCOperand::createReg(MO.getReg()));
      break;
    case MachineOperand::MO_Immediate:
      OutMI.addOperand(MCOperand::createImm(MO.getImm()));
      break;
    case MachineOperand::MO_MachineBasicBlock:
      OutMI.addOperand(MCOperand::createExpr(
          MCSymbolRefExpr::create(MO.getMBB()->getSymbol(), Ctx)));
      break;
    case MachineOperand::MO_GlobalAddress: {
      const GlobalValue &GV = *MO.getGlobal();
      MCSymbol *Symbol =
          GV.hasExternalLinkage() && GV.isDSOLocal() && !GV.isInterposable() &&
                  GV.canBenefitFromLocalAlias()
              ? Printer.getSymbolWithGlobalValueBase(&GV, "$local")
              : Printer.getSymbol(&GV);
      const MCExpr *Expr = MCSymbolRefExpr::create(Symbol, Ctx);
      if (MO.getOffset() != 0)
        Expr = MCBinaryExpr::createAdd(
            Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);
      OutMI.addOperand(MCOperand::createExpr(Expr));
      break;
    }
    case MachineOperand::MO_ConstantPoolIndex: {
      MCSymbol *Symbol;
      if (IsLongLiteral || IsWordLiteral) {
        int64_t Instance = MI->getOperand(2).getImm();
        if (Instance < 0)
          report_fatal_error(
              "SH literal load reached AsmPrinter without an island instance");
        Symbol = getSHLiteralIslandSymbol(Ctx, Printer.getDataLayout(),
                                          Printer.getFunctionNumber(),
                                          MO.getIndex(), Instance);
      } else {
        Symbol = Printer.GetCPISymbol(MO.getIndex());
      }
      const MCExpr *Expr = MCSymbolRefExpr::create(Symbol, Ctx);
      if (MO.getOffset() != 0)
        Expr = MCBinaryExpr::createAdd(
            Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);
      OutMI.addOperand(MCOperand::createExpr(Expr));
      break;
    }
    case MachineOperand::MO_JumpTableIndex: {
      if (MI->getOpcode() == SH::JMP)
        break;
      if (IsInlineJumpTableDispatch && I == 1)
        break;
      const MCExpr *Expr =
          MCSymbolRefExpr::create(Printer.GetJTISymbol(MO.getIndex()), Ctx);
      if (MO.getOffset() != 0)
        Expr = MCBinaryExpr::createAdd(
            Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);
      OutMI.addOperand(MCOperand::createExpr(Expr));
      break;
    }
    case MachineOperand::MO_BlockAddress: {
      const MCExpr *Expr = MCSymbolRefExpr::create(
          Printer.GetBlockAddressSymbol(MO.getBlockAddress()), Ctx);
      if (MO.getOffset() != 0)
        Expr = MCBinaryExpr::createAdd(
            Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);
      OutMI.addOperand(MCOperand::createExpr(Expr));
      break;
    }
    case MachineOperand::MO_MCSymbol:
      if (IsInlineJumpTableDispatch && I == 2)
        break;
      OutMI.addOperand(MCOperand::createExpr(
          MCSymbolRefExpr::create(MO.getMCSymbol(), Ctx)));
      break;
    case MachineOperand::MO_RegisterMask:
      break;
    default:
      llvm_unreachable("unexpected SH machine operand");
    }
  }
}
