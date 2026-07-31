//===-- SHAsmPrinter.cpp - SH assembly printer ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHAsmPrinter.h"
#include "MCTargetDesc/SHMCAsmInfo.h"
#include "MCTargetDesc/SHMCTargetDesc.h"
#include "SH.h"
#include "SHConstantPoolValue.h"
#include "SHLiteralPool.h"
#include "SHMCInstLower.h"
#include "TargetInfo/SHTargetInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/AsmPrinterAnalysis.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionAnalysisManager.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalIFunc.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {

static bool isSupportedGlobalType(Type *Ty) {
  if (Ty->isIntegerTy(8) || Ty->isIntegerTy(16) || Ty->isIntegerTy(32) ||
      Ty->isIntegerTy(64))
    return true;
  if (const auto *Pointer = dyn_cast<PointerType>(Ty))
    return Pointer->getAddressSpace() == 0;
  if (const auto *Array = dyn_cast<ArrayType>(Ty))
    return isSupportedGlobalType(Array->getElementType());
  const auto *Struct = dyn_cast<StructType>(Ty);
  if (!Struct || Struct->isOpaque())
    return false;
  return llvm::all_of(Struct->elements(), isSupportedGlobalType);
}

static void validateGlobalInitializer(const Constant *C) {
  if (const auto *GV = dyn_cast<GlobalVariable>(C); GV && GV->isThreadLocal())
    report_fatal_error("SH thread-local storage is not supported");
  if (isa<GlobalValue>(C))
    return;
  if (const auto *Expr = dyn_cast<ConstantExpr>(C)) {
    if (Expr->getOpcode() != Instruction::GetElementPtr &&
        Expr->getOpcode() != Instruction::BitCast)
      report_fatal_error("SH global initializer expression is not supported");
  }
  for (const Use &Operand : C->operands())
    validateGlobalInitializer(cast<Constant>(Operand.get()));
}

static void validateGlobalObject(const GlobalObject &GO) {
  if (GO.hasComdat())
    report_fatal_error("SH COMDAT is not supported");
  if (!GO.hasDefaultVisibility() && !GO.hasHiddenVisibility() &&
      !GO.hasProtectedVisibility())
    report_fatal_error("SH symbol visibility is not supported");
  if (GO.getDLLStorageClass() != GlobalValue::DefaultStorageClass)
    report_fatal_error("SH DLL storage classes are not supported");
  if (!GO.isDeclarationForLinker() && !GO.hasExternalLinkage() &&
      !GO.hasInternalLinkage() && !GO.hasPrivateLinkage())
    report_fatal_error(
        "SH weak, linkonce, common, and appending linkage are not supported");
  if (GO.isDeclarationForLinker() && !GO.hasExternalLinkage())
    report_fatal_error("SH weak and linkonce declarations are not supported");
}

static void validateSHModule(const Module &M) {
  if (!M.aliases().empty())
    report_fatal_error("SH global aliases are not supported");
  if (!M.ifuncs().empty())
    report_fatal_error("SH indirect functions are not supported");

  for (const Function &F : M) {
    if (!F.isIntrinsic())
      validateGlobalObject(F);
  }
  for (const GlobalVariable &GV : M.globals()) {
    validateGlobalObject(GV);
    if (GV.getAddressSpace() != 0)
      report_fatal_error("SH nonzero address spaces are not supported");
    if (GV.isThreadLocal())
      report_fatal_error("SH thread-local storage is not supported");
    if (GV.isExternallyInitialized())
      report_fatal_error("SH externally initialized globals are not supported");
    if (GV.isDeclaration())
      continue;
    if (!isSupportedGlobalType(GV.getValueType()))
      report_fatal_error(
          "SH globals only support i8, i16, i32, i64, pointers, fixed arrays, "
          "and fixed structures");
    validateGlobalInitializer(GV.getInitializer());
  }
}

class SHAsmPrinter : public AsmPrinter {
  void emitLiteralIslandEntry(const MachineInstr &MI) {
    unsigned CPI = MI.getOperand(0).getIndex();
    unsigned Instance = MI.getOperand(1).getImm();
    const MachineConstantPoolEntry &Entry =
        MF->getConstantPool()->getConstants()[CPI];
    OutStreamer->emitLabel(getSHLiteralIslandSymbol(
        OutContext, getDataLayout(), getFunctionNumber(), CPI, Instance));
    if (Entry.isMachineConstantPoolEntry())
      emitMachineConstantPoolValue(Entry.Val.MachineCPVal);
    else
      emitGlobalConstant(getDataLayout(), Entry.Val.ConstVal);
  }

public:
  static char ID;
  SHAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}
  StringRef getPassName() const override { return "SH Assembly Printer"; }

  bool doInitialization(Module &M) override {
    validateSHModule(M);
    return AsmPrinter::doInitialization(M);
  }

  void emitFunctionEntryLabel() override {
    AsmPrinter::emitFunctionEntryLabel();
    const Function &F = MF->getFunction();
    if (!F.hasExternalLinkage() || !F.isDSOLocal() || F.isInterposable() ||
        !F.canBenefitFromLocalAlias() || CurrentFnBeginLocal)
      return;
    CurrentFnBeginLocal = getSymbolWithGlobalValueBase(&F, "$local");
    OutStreamer->emitLabel(CurrentFnBeginLocal);
    OutStreamer->emitSymbolAttribute(CurrentFnBeginLocal,
                                     MCSA_ELF_TypeFunction);
  }

  void emitConstantPool() override {}

  void emitMachineConstantPoolValue(MachineConstantPoolValue *MCPV) override {
    const auto *Value = static_cast<const SHConstantPoolValue *>(MCPV);
    MCSymbol *Symbol;
    if (Value->isGlobalValue())
      Symbol = Value->getModifier() == SHConstantPoolValue::Modifier::GOTOFF
                   ? getSymbolPreferLocal(*Value->getGlobalValue())
                   : getSymbol(Value->getGlobalValue());
    else if (Value->isExternalSymbol())
      Symbol = GetExternalSymbolSymbol(Value->getExternalSymbol());
    else if (Value->isJumpTable())
      Symbol = GetJTISymbol(Value->getJumpTableIndex());
    else
      Symbol = GetBlockAddressSymbol(Value->getBlockAddress());
    unsigned Specifier = SH::S_None;
    switch (Value->getModifier()) {
    case SHConstantPoolValue::Modifier::None:
      break;
    case SHConstantPoolValue::Modifier::GOT:
      Specifier = SH::S_GOT;
      break;
    case SHConstantPoolValue::Modifier::GOTOFF:
      Specifier = SH::S_GOTOFF;
      break;
    case SHConstantPoolValue::Modifier::GOTPC:
      Specifier = SH::S_GOTPC;
      break;
    case SHConstantPoolValue::Modifier::PLT:
      Specifier = SH::S_PLT;
      break;
    }
    if (Value->getModifier() == SHConstantPoolValue::Modifier::GOTPC &&
        Symbol->getName() == "_GLOBAL_OFFSET_TABLE_")
      Specifier = SH::S_None;
    const MCExpr *Expr = MCSymbolRefExpr::create(Symbol, Specifier, OutContext);
    if (Value->getAddend() != 0)
      Expr = MCBinaryExpr::createAdd(
          Expr, MCConstantExpr::create(Value->getAddend(), OutContext),
          OutContext);
    OutStreamer->emitValue(Expr, 4);
  }

  void emitInstruction(const MachineInstr *MI) override {
    if (MI->getOpcode() == SH::SH_CONSTPOOL_ENTRY) {
      emitLiteralIslandEntry(*MI);
      return;
    }
    if (MI->getOpcode() == SH::SH_JT_DISPATCH)
      report_fatal_error(
          "SH jump-table dispatch reached final emission without expansion");
    if (MI->getOpcode() == SH::SH_PIC_SETUP ||
        MI->getOpcode() == SH::SH_PIC_ADDRESS)
      report_fatal_error(
          "SH PIC materialization reached final emission without expansion");
    SHMCInstLower Lowering(OutContext, *this);
    MachineBasicBlock::const_instr_iterator I = MI->getIterator();
    MachineBasicBlock::const_instr_iterator E = MI->getParent()->instr_end();
    do {
      MCInst OutMI;
      Lowering.lower(&*I, OutMI);
      EmitToStreamer(*OutStreamer, OutMI);
    } while (++I != E && I->isInsideBundle());
  }
};

char SHAsmPrinter::ID = 0;

} // namespace

INITIALIZE_PASS(SHAsmPrinter, "sh-asm-printer", "SH Assembly Printer", false,
                false)

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeSHAsmPrinter() {
  RegisterAsmPrinter<SHAsmPrinter> X(getTheSHTarget());
  RegisterAsmPrinter<SHAsmPrinter> Y(getTheSHLETarget());
}

PreservedAnalyses SHAsmPrinterBeginPass::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  SHAsmPrinter &Printer = static_cast<SHAsmPrinter &>(
      MAM.getResult<AsmPrinterAnalysis>(M).getPrinter());
  setupModuleAsmPrinter(M, MAM, Printer);
  Printer.doInitialization(M);
  return PreservedAnalyses::all();
}

PreservedAnalyses SHAsmPrinterPass::run(MachineFunction &MF,
                                        MachineFunctionAnalysisManager &MFAM) {
  SHAsmPrinter &Printer = static_cast<SHAsmPrinter &>(
      MFAM.getResult<ModuleAnalysisManagerMachineFunctionProxy>(MF)
          .getCachedResult<AsmPrinterAnalysis>(*MF.getFunction().getParent())
          ->getPrinter());
  setupMachineFunctionAsmPrinter(MFAM, MF, Printer);
  Printer.runOnMachineFunction(MF);
  return PreservedAnalyses::all();
}

PreservedAnalyses SHAsmPrinterEndPass::run(Module &M,
                                           ModuleAnalysisManager &MAM) {
  SHAsmPrinter &Printer = static_cast<SHAsmPrinter &>(
      MAM.getResult<AsmPrinterAnalysis>(M).getPrinter());
  setupModuleAsmPrinter(M, MAM, Printer);
  Printer.doFinalization(M);
  return PreservedAnalyses::all();
}
