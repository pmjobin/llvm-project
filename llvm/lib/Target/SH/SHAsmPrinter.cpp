//===-- SHAsmPrinter.cpp - SH assembly printer ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHAsmPrinter.h"
#include "MCTargetDesc/SHInstPrinter.h"
#include "MCTargetDesc/SHMCAsmInfo.h"
#include "MCTargetDesc/SHMCTargetDesc.h"
#include "MCTargetDesc/SHTargetStreamer.h"
#include "SH.h"
#include "SHConstantPoolValue.h"
#include "SHIRValidation.h"
#include "SHInlineAsmSize.h"
#include "SHLiteralPool.h"
#include "SHMCInstLower.h"
#include "SHMachineFunctionInfo.h"
#include "SHRegisterInfo.h"
#include "SHSubtarget.h"
#include "SHTargetMachine.h"
#include "TargetInfo/SHTargetInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/AsmPrinterAnalysis.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionAnalysisManager.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalIFunc.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/SaveAndRestore.h"
#include "llvm/Target/TargetMachine.h"
#include <limits>

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {

static bool isSupportedGlobalType(Type *Ty, bool IsTLS) {
  if (Ty->isIntegerTy(1) || Ty->isIntegerTy(8) || Ty->isIntegerTy(16) ||
      Ty->isIntegerTy(32) || Ty->isIntegerTy(64) || Ty->isFloatTy() ||
      Ty->isDoubleTy())
    return true;
  if (isSupportedSHVectorType(Ty))
    return true;
  if (const auto *Pointer = dyn_cast<PointerType>(Ty))
    return Pointer->getAddressSpace() == 0;
  if (const auto *Array = dyn_cast<ArrayType>(Ty))
    return isSupportedGlobalType(Array->getElementType(), IsTLS);
  const auto *Struct = dyn_cast<StructType>(Ty);
  if (!Struct || Struct->isOpaque())
    return false;
  return llvm::all_of(Struct->elements(), [IsTLS](Type *Element) {
    return isSupportedGlobalType(Element, IsTLS);
  });
}

static void validateInitializer(const Constant *C) {
  if (const auto *Pointer = dyn_cast<PointerType>(C->getType());
      Pointer && Pointer->getAddressSpace() != 0)
    reportFatalUsageError("SH initializer pointers require address space zero");
  if (const auto *GV = dyn_cast<GlobalVariable>(C)) {
    if (GV->isThreadLocal())
      reportFatalUsageError(
          "SH global initializer cannot reference a TLS symbol");
    return;
  }
  if (isa<GlobalValue>(C))
    return;
  if (isa<DSOLocalEquivalent>(C))
    reportFatalUsageError(
        "SH dso_local_equivalent initializer is not supported");
  for (const Use &Operand : C->operands())
    if (const auto *OperandC = dyn_cast<Constant>(Operand.get()))
      validateInitializer(OperandC);
}

static void validateGlobalObject(const GlobalObject &GO) {
  const auto *GV = dyn_cast<GlobalVariable>(&GO);
  if (!GO.hasDefaultVisibility() && !GO.hasHiddenVisibility() &&
      !GO.hasProtectedVisibility())
    reportFatalUsageError("SH symbol visibility is not supported");
  if (GO.getDLLStorageClass() != GlobalValue::DefaultStorageClass)
    reportFatalUsageError("SH DLL storage classes are not supported");
  if (GO.hasAppendingLinkage() &&
      (!GV ||
       (GV->getName() != "llvm.global_ctors" &&
        GV->getName() != "llvm.global_dtors" && GV->getName() != "llvm.used" &&
        GV->getSection() != "llvm.metadata")))
    reportFatalUsageError(
        "SH appending linkage is not supported for this global");
  if (!GO.isDeclarationForLinker() && !GO.hasExternalLinkage() &&
      !GO.hasInternalLinkage() && !GO.hasPrivateLinkage() &&
      !GO.hasWeakLinkage() && !GO.hasLinkOnceLinkage() &&
      !GO.hasAppendingLinkage() && !GO.hasCommonLinkage())
    reportFatalUsageError("SH global linkage is not supported");
  if (GO.isDeclarationForLinker() && !GO.hasExternalLinkage() &&
      !GO.hasExternalWeakLinkage() && !GO.hasAvailableExternallyLinkage())
    reportFatalUsageError("SH global declaration linkage is not supported");
}

static void validateGlobalAlias(const GlobalAlias &GA) {
  if (!GA.hasDefaultVisibility() && !GA.hasHiddenVisibility() &&
      !GA.hasProtectedVisibility())
    reportFatalUsageError("SH symbol visibility is not supported");
  if (GA.getDLLStorageClass() != GlobalValue::DefaultStorageClass)
    reportFatalUsageError("SH DLL storage classes are not supported");
  if (GA.getAddressSpace() != 0)
    reportFatalUsageError("SH nonzero address spaces are not supported");
  if (!GA.hasExternalLinkage() && !GA.hasInternalLinkage() &&
      !GA.hasPrivateLinkage() && !GA.hasWeakLinkage() &&
      !GA.hasLinkOnceLinkage())
    reportFatalUsageError("SH global alias linkage is not supported");
}

static void validateSHModule(const Module &M, const TargetMachine &TM) {
  bool IsFDPIC = static_cast<const SHTargetMachine &>(TM).isFDPIC();
  for (const GlobalAlias &GA : M.aliases())
    validateGlobalAlias(GA);
  if (!M.ifuncs().empty())
    reportFatalUsageError("SH indirect functions are not supported");

  for (const Function &F : M) {
    if (!F.isIntrinsic())
      validateGlobalObject(F);
  }
  bool HasTLS = false;
  for (const GlobalVariable &GV : M.globals()) {
    validateGlobalObject(GV);
    if (GV.getAddressSpace() != 0)
      reportFatalUsageError("SH nonzero address spaces are not supported");
    HasTLS |= GV.isThreadLocal();
    if (GV.isThreadLocal() && !isSupportedGlobalType(GV.getValueType(), true))
      reportFatalUsageError(
          "SH TLS globals only support i1, i8, i16, i32, i64, float, double, "
          "pointers, native float2/float4 vectors, fixed arrays, and fixed "
          "structures");
    if (GV.isDeclaration())
      continue;
    if (!GV.isThreadLocal() && !isSupportedGlobalType(GV.getValueType(), false))
      reportFatalUsageError(
          "SH globals only support i1, i8, i16, i32, i64, float, double, "
          "pointers, native float2/float4 vectors, fixed arrays, and fixed "
          "structures");
    validateInitializer(GV.getInitializer());
  }
  const Triple &TT = TM.getTargetTriple();
  bool IsGenericELF = TT.getOS() == Triple::UnknownOS &&
                      TT.getEnvironment() == Triple::UnknownEnvironment;
  bool IsGNULinux = TT.isOSLinux() && TT.isGNUEnvironment();
  if (HasTLS && !IsGenericELF && !IsGNULinux && !(IsFDPIC && TT.isMuslFDPIC()))
    reportFatalUsageError(
        "SH native ELF TLS supports only unknown-elf and linux-gnu triples");
}

class SHAsmPrinter : public AsmPrinter {
  std::optional<Align> CurrentGlobalAlignment;
  bool EmittingGlobalAlias = false;

  bool requiresUnalignedDirective(unsigned Size, uint64_t Offset) const {
    return CurrentGlobalAlignment && Size > 1 &&
           commonAlignment(*CurrentGlobalAlignment, Offset) < Align(Size);
  }

  void emitUnalignedDataValue(const MCExpr *Value, unsigned Size) {
    if (auto *TargetStreamer = OutStreamer->getTargetStreamer()) {
      static_cast<SHTargetStreamer *>(TargetStreamer)
          ->emitUnalignedDataValue(Value, Size);
      return;
    }
    OutStreamer->emitValue(Value, Size);
  }

  bool emitUnalignedDataSequential(const ConstantDataSequential *CDS,
                                   uint64_t Offset) {
    if (CDS->isString())
      return false;
    unsigned ElementSize = CDS->getElementByteSize();
    if ((ElementSize != 2 && ElementSize != 4 && ElementSize != 8) ||
        !requiresUnalignedDirective(ElementSize, Offset))
      return false;

    for (unsigned I = 0; I != CDS->getNumElements(); ++I) {
      uint64_t Bits =
          CDS->getElementType()->isFloatingPointTy()
              ? CDS->getElementAsAPFloat(I).bitcastToAPInt().getZExtValue()
              : CDS->getElementAsInteger(I);
      emitUnalignedDataValue(MCConstantExpr::create(Bits, OutContext),
                             ElementSize);
    }
    return true;
  }

  void emitLiteralIslandEntry(const MachineInstr &MI) {
    unsigned CPI = MI.getOperand(0).getIndex();
    unsigned Instance = MI.getOperand(1).getImm();
    const MachineConstantPoolEntry &Entry =
        MF->getConstantPool()->getConstants()[CPI];
    SaveAndRestore Guard(
        CurrentGlobalAlignment,
        std::optional<Align>(Align(MI.getOperand(3).getImm())));
    OutStreamer->emitLabel(getSHLiteralIslandSymbol(
        OutContext, getDataLayout(), getFunctionNumber(), CPI, Instance));
    if (Entry.isMachineConstantPoolEntry())
      emitMachineConstantPoolValue(Entry.Val.MachineCPVal);
    else
      emitGlobalConstant(getDataLayout(), Entry.Val.ConstVal);
  }

  void emitInlineJumpTable(const MachineInstr &MI) {
    if (MI.getNumExplicitOperands() != 4 || !MI.getOperand(0).isJTI() ||
        !MI.getOperand(1).isMCSymbol() || !MI.getOperand(2).isMCSymbol() ||
        !MI.getOperand(3).isImm())
      reportFatalUsageError("SH inline jump table is malformed");
    int JTI = MI.getOperand(0).getIndex();
    int64_t EntrySize = MI.getOperand(3).getImm();
    const MachineJumpTableInfo *MJTI = MF->getJumpTableInfo();
    if (!MJTI || MJTI->getEntryKind() != MachineJumpTableInfo::EK_Inline ||
        JTI < 0 || static_cast<unsigned>(JTI) >= MJTI->getJumpTables().size() ||
        MJTI->getJumpTables()[JTI].MBBs.empty() ||
        (EntrySize != 2 && EntrySize != 4))
      reportFatalUsageError("SH inline jump table has invalid metadata");

    MCSymbol *Table = MI.getOperand(1).getMCSymbol();
    const MCSymbol *Base = MI.getOperand(2).getMCSymbol();
    OutStreamer->emitLabel(Table);
    OutStreamer->emitDataRegion(EntrySize == 2 ? MCDR_DataRegionJT16
                                               : MCDR_DataRegionJT32);
    for (const MachineBasicBlock *Target : MJTI->getJumpTables()[JTI].MBBs) {
      const MCExpr *Expr = MCBinaryExpr::createSub(
          MCSymbolRefExpr::create(Target->getSymbol(), OutContext),
          MCSymbolRefExpr::create(Base, OutContext), OutContext);
      Expr = MCBinaryExpr::createSub(
          Expr, MCConstantExpr::create(4, OutContext), OutContext);
      OutStreamer->emitValue(Expr, static_cast<unsigned>(EntrySize));
    }
    OutStreamer->emitDataRegion(MCDR_DataRegionEnd);
  }

  void emitLongBranch(const MachineInstr &MI) {
    if (MI.getNumExplicitOperands() != 3 || !MI.getOperand(0).isReg() ||
        !SH::GPRNoR0RegClass.contains(MI.getOperand(0).getReg()) ||
        !MI.getOperand(1).isMBB() || !MI.getOperand(2).isImm() ||
        (MI.getOperand(2).getImm() != 0 && MI.getOperand(2).getImm() != 1))
      reportFatalUsageError("SH long branch pseudo is malformed");

    MCRegister Scratch = MI.getOperand(0).getReg();
    MCSymbol *Literal = OutContext.createTempSymbol("sh_long_branch", true);
    const MCExpr *LiteralExpr = MCSymbolRefExpr::create(Literal, OutContext);

    MCInst MOVA;
    MOVA.setOpcode(SH::MOVA);
    MOVA.addOperand(MCOperand::createReg(SH::R0));
    MOVA.addOperand(MCOperand::createExpr(LiteralExpr));
    EmitToStreamer(*OutStreamer, MOVA);

    MCInst Load;
    Load.setOpcode(SH::MOVL_load_pc);
    Load.addOperand(MCOperand::createReg(Scratch));
    Load.addOperand(MCOperand::createExpr(LiteralExpr));
    EmitToStreamer(*OutStreamer, Load);

    MCInst Add;
    Add.setOpcode(SH::ADDrr);
    Add.addOperand(MCOperand::createReg(Scratch));
    Add.addOperand(MCOperand::createReg(Scratch));
    Add.addOperand(MCOperand::createReg(SH::R0));
    EmitToStreamer(*OutStreamer, Add);

    MCInst Jump;
    Jump.setOpcode(SH::JMP);
    Jump.addOperand(MCOperand::createReg(Scratch));
    EmitToStreamer(*OutStreamer, Jump);
    MCInst Nop;
    Nop.setOpcode(SH::NOP);
    EmitToStreamer(*OutStreamer, Nop);

    OutStreamer->emitCodeAlignment(Align(4), getSubtargetInfo());
    OutStreamer->emitLabel(Literal);
    const MCExpr *TargetExpr = MCSymbolRefExpr::create(
        MI.getOperand(1).getMBB()->getSymbol(), OutContext);
    OutStreamer->emitValue(
        MCBinaryExpr::createSub(TargetExpr, LiteralExpr, OutContext), 4);
    if (MI.getOperand(2).getImm())
      OutStreamer->emitCFIRestoreState(SMLoc());
  }

  void emitRelativeBranch(const MachineInstr &MI) {
    if (MI.getNumExplicitOperands() != 3 || !MI.getOperand(0).isReg() ||
        !SH::GPRRegClass.contains(MI.getOperand(0).getReg()) ||
        !MI.getOperand(1).isMBB() || !MI.getOperand(2).isImm() ||
        (MI.getOperand(2).getImm() != 0 && MI.getOperand(2).getImm() != 1))
      reportFatalUsageError("SH relative branch pseudo is malformed");

    MCRegister Scratch = MI.getOperand(0).getReg();
    MCSymbol *Literal = OutContext.createTempSymbol("sh_relative_branch", true);
    const MCExpr *LiteralExpr = MCSymbolRefExpr::create(Literal, OutContext);

    MCInst Load;
    Load.setOpcode(SH::MOVW_load_pc);
    Load.addOperand(MCOperand::createReg(Scratch));
    Load.addOperand(MCOperand::createExpr(LiteralExpr));
    EmitToStreamer(*OutStreamer, Load);

    MCInst Branch;
    Branch.setOpcode(SH::BRAF);
    Branch.addOperand(MCOperand::createReg(Scratch));
    EmitToStreamer(*OutStreamer, Branch);
    MCInst Nop;
    Nop.setOpcode(SH::NOP);
    EmitToStreamer(*OutStreamer, Nop);

    OutStreamer->emitLabel(Literal);
    const MCExpr *TargetExpr = MCSymbolRefExpr::create(
        MI.getOperand(1).getMBB()->getSymbol(), OutContext);
    OutStreamer->emitValue(
        MCBinaryExpr::createSub(TargetExpr, LiteralExpr, OutContext), 2);
    if (MI.getOperand(2).getImm())
      OutStreamer->emitCFIRestoreState(SMLoc());
  }

public:
  static char ID;
  SHAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  const MCExpr *lowerConstant(const Constant *CV,
                              const Constant *BaseCV = nullptr,
                              uint64_t Offset = 0) override {
    if (static_cast<const SHTargetMachine &>(TM).isFDPIC() &&
        !EmittingGlobalAlias) {
      const Value *Stripped = CV->stripPointerCasts();
      const auto *GV = dyn_cast<GlobalValue>(Stripped);
      bool IsFunction = isa_and_nonnull<Function>(GV) ||
                        (isa_and_nonnull<GlobalAlias>(GV) &&
                         isa_and_nonnull<Function>(
                             cast<GlobalAlias>(GV)->getAliaseeObject()));
      if (IsFunction) {
        const MCExpr *Expr =
            MCSymbolRefExpr::create(getSymbol(GV), SH::S_FUNCDESC, OutContext);
        return Expr;
      }
    }
    return AsmPrinter::lowerConstant(CV, BaseCV, Offset);
  }

  void emitGlobalAlias(const Module &M, const GlobalAlias &GA) override {
    SaveAndRestore Guard(EmittingGlobalAlias, true);
    AsmPrinter::emitGlobalAlias(M, GA);
  }
  StringRef getPassName() const override { return "SH Assembly Printer"; }

  bool runOnMachineFunction(MachineFunction &MF) override {
    bool InvalidSize = false;
    for (const MachineBasicBlock &MBB : MF) {
      for (const MachineInstr &MI : MBB) {
        if (!MI.isInlineAsm())
          continue;
        SHInlineAsmSize Size =
            getSHInlineAsmSize(MI.getOperand(0).getSymbolName());
        std::string Diagnostic;
        if (Size.Kind == SHInlineAsmSizeKind::Indeterminate ||
            Size.Kind == SHInlineAsmSizeKind::Malformed)
          Diagnostic = "SH inline assembly size cannot be determined: " +
                       Size.Diagnostic;
        else if (Size.Bytes > std::numeric_limits<unsigned>::max())
          Diagnostic =
              "SH inline assembly size exceeds the representable range";
        else
          continue;
        InvalidSize = true;
        if (MF.getInfo<SHMachineFunctionInfo>()
                ->markInlineAsmSizeDiagnosticReported(&MI))
          MI.emitInlineAsmError(Diagnostic);
      }
    }
    if (InvalidSize)
      return false;
    return AsmPrinter::runOnMachineFunction(MF);
  }

  void emitFunctionBodyStart() override {
    if (auto *TargetStreamer = OutStreamer->getTargetStreamer())
      static_cast<SHTargetStreamer *>(TargetStreamer)
          ->emitMachine(MF->getSubtarget<SHSubtarget>());
    AsmPrinter::emitFunctionBodyStart();
  }

  void emitGlobalVariable(const GlobalVariable *GV) override {
    std::optional<Align> Alignment;
    if (GV->hasInitializer())
      Alignment = getGVAlignment(GV, GV->getDataLayout());
    SaveAndRestore Guard(CurrentGlobalAlignment, Alignment);
    AsmPrinter::emitGlobalVariable(GV);
  }

  bool emitGlobalConstantValue(const DataLayout &DL, const Constant *CV,
                               const Constant *BaseCV,
                               uint64_t Offset) override {
    if (isSupportedSHVectorType(CV->getType())) {
      SmallVector<const Constant *, 4> Elements;
      unsigned Count = cast<FixedVectorType>(CV->getType())->getNumElements();
      for (unsigned I = 0; I != Count; ++I) {
        const Constant *Element = CV->getAggregateElement(I);
        if (!Element)
          return false;
        Elements.push_back(Element);
      }
      // A splat can itself be a ConstantFP: do not treat its scalar APFloat
      // as a value of the whole vector's size. All native lanes are four
      // bytes. Preserve their effective alignment inside packed aggregates,
      // including when generic recursion starts each scalar at offset zero.
      SaveAndRestore Guard(
          CurrentGlobalAlignment,
          CurrentGlobalAlignment
              ? std::optional<Align>(commonAlignment(*CurrentGlobalAlignment,
                                                     Offset))
              : std::nullopt);
      for (const Constant *Element : Elements)
        emitGlobalConstant(DL, Element);
      return true;
    }
    if (const auto *CDS = dyn_cast<ConstantDataSequential>(CV))
      return emitUnalignedDataSequential(CDS, Offset);

    uint64_t Size = DL.getTypeStoreSize(CV->getType());
    if ((Size != 2 && Size != 4 && Size != 8) ||
        !requiresUnalignedDirective(Size, Offset))
      return false;

    const MCExpr *Value = nullptr;
    if (const auto *CI = dyn_cast<ConstantInt>(CV))
      Value = MCConstantExpr::create(CI->getZExtValue(), OutContext);
    else if (const auto *CB = dyn_cast<ConstantByte>(CV))
      Value = MCConstantExpr::create(CB->getZExtValue(), OutContext);
    else if (const auto *CFP = dyn_cast<ConstantFP>(CV))
      Value = MCConstantExpr::create(
          CFP->getValueAPF().bitcastToAPInt().getZExtValue(), OutContext);
    else if (isa<ConstantPointerNull>(CV))
      Value = MCConstantExpr::create(0, OutContext);
    else if (isa<GlobalValue>(CV) || isa<ConstantExpr>(CV))
      Value = lowerConstant(CV, BaseCV, Offset);
    else
      return false;

    emitUnalignedDataValue(Value, Size);
    return true;
  }

  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       const char *ExtraCode, raw_ostream &OS) override {
    if (ExtraCode && ExtraCode[0]) {
      if (ExtraCode[0] != 'O' || ExtraCode[1] != '\0')
        return AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, OS);
      const MachineOperand &MO = MI->getOperand(OpNo);
      if (MO.isImm()) {
        OS << MO.getImm();
        return false;
      }
      if (MO.isGlobal()) {
        PrintSymbolOperand(MO, OS);
        return false;
      }
      return true;
    }
    const MachineOperand &MO = MI->getOperand(OpNo);
    if (MO.isReg()) {
      OS << SHInstPrinter::getRegisterName(MO.getReg());
      return false;
    }
    if (MO.isImm()) {
      OS << MO.getImm();
      return false;
    }
    if (MO.isGlobal()) {
      PrintSymbolOperand(MO, OS);
      return false;
    }
    return true;
  }

  bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo,
                             const char *ExtraCode, raw_ostream &OS) override {
    const MachineOperand &MO = MI->getOperand(OpNo);
    if (!MO.isReg())
      return true;
    unsigned ByteOffset = 0;
    if (ExtraCode && ExtraCode[0]) {
      if (ExtraCode[1] != '\0' ||
          (ExtraCode[0] != 'R' && ExtraCode[0] != 'S' && ExtraCode[0] != 'T'))
        return true;
      if (ExtraCode[0] == 'T')
        ByteOffset = 4;
      else if (ExtraCode[0] == 'R')
        ByteOffset = getDataLayout().isLittleEndian() ? 0 : 4;
      else
        ByteOffset = getDataLayout().isLittleEndian() ? 4 : 0;
    }
    const InlineAsm::Flag Flags(MI->getOperand(OpNo - 1).getImm());
    if (Flags.getMemoryConstraintID() == InlineAsm::ConstraintCode::p) {
      if (ExtraCode && ExtraCode[0])
        return true;
      OS << SHInstPrinter::getRegisterName(MO.getReg());
      return false;
    }
    if (ByteOffset == 0)
      OS << '@' << SHInstPrinter::getRegisterName(MO.getReg());
    else
      OS << "@(" << ByteOffset << ','
         << SHInstPrinter::getRegisterName(MO.getReg()) << ')';
    return false;
  }

  bool doInitialization(Module &M) override {
    validateSHModule(M, TM);
    bool Changed = AsmPrinter::doInitialization(M);
    for (const GlobalVariable &GV : M.globals())
      if (GV.isThreadLocal() && GV.isDeclaration())
        OutStreamer->emitSymbolAttribute(getSymbol(&GV), MCSA_ELF_TypeTLS);
    return Changed;
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
      Symbol = (Value->getModifier() == SHConstantPoolValue::Modifier::GOTOFF ||
                Value->getModifier() ==
                    SHConstantPoolValue::Modifier::GOTOFFFUNCDESC)
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
    case SHConstantPoolValue::Modifier::PCREL:
      break;
    case SHConstantPoolValue::Modifier::GOTFUNCDESC:
      Specifier = SH::S_GOTFUNCDESC;
      break;
    case SHConstantPoolValue::Modifier::GOTOFFFUNCDESC:
      Specifier = SH::S_GOTOFFFUNCDESC;
      break;
    case SHConstantPoolValue::Modifier::FUNCDESC:
      Specifier = SH::S_FUNCDESC;
      break;
    case SHConstantPoolValue::Modifier::TLSGD:
      Specifier = SH::S_TLSGD;
      break;
    case SHConstantPoolValue::Modifier::TLSLDM:
      Specifier = SH::S_TLSLDM;
      break;
    case SHConstantPoolValue::Modifier::DTPOFF:
      Specifier = SH::S_DTPOFF;
      break;
    case SHConstantPoolValue::Modifier::GOTTPOFF:
      Specifier = SH::S_GOTTPOFF;
      break;
    case SHConstantPoolValue::Modifier::TPOFF:
      Specifier = SH::S_TPOFF;
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
    if (Value->getModifier() == SHConstantPoolValue::Modifier::PCREL) {
      const MCSymbol *Base = Value->getPCRelativeBase();
      if (!Base) {
        MCSymbol *LiteralBase = OutContext.createTempSymbol("sh_pcrel", true);
        OutStreamer->emitLabel(LiteralBase);
        Base = LiteralBase;
      }
      Expr = MCBinaryExpr::createSub(
          Expr, MCSymbolRefExpr::create(Base, OutContext), OutContext);
    }
    if (Value->getModifier() == SHConstantPoolValue::Modifier::PLT &&
        Value->getPCRelativeBase()) {
      MCSymbol *LiteralBase =
          OutContext.createTempSymbol("sh_fdpic_call_literal", true);
      OutStreamer->emitLabel(LiteralBase);
      const MCExpr *Bias = MCBinaryExpr::createSub(
          MCSymbolRefExpr::create(Value->getPCRelativeBase(), OutContext),
          MCSymbolRefExpr::create(LiteralBase, OutContext), OutContext);
      Expr = MCBinaryExpr::createSub(Expr, Bias, OutContext);
    }
    OutStreamer->emitValue(Expr, 4);
  }

  void emitInstruction(const MachineInstr *MI) override {
    if (MI->getOpcode() == SH::SH_CONSTPOOL_ENTRY) {
      emitLiteralIslandEntry(*MI);
      return;
    }
    if (MI->getOpcode() == SH::SH_JUMPTABLE) {
      emitInlineJumpTable(*MI);
      return;
    }
    if (MI->getOpcode() == SH::SH_JT_DISPATCH)
      reportFatalUsageError(
          "SH jump-table dispatch reached final emission without expansion");
    if (MI->getOpcode() == SH::SH_PIC_SETUP ||
        MI->getOpcode() == SH::SH_PIC_ADDRESS ||
        MI->getOpcode() == SH::SH_PIC_GOTOFF)
      report_fatal_error(
          "SH PIC materialization reached final emission without expansion");
    if (MI->getOpcode() == SH::SH_TLS_CALL || MI->getOpcode() == SH::SH_TLS_IE)
      report_fatal_error(
          "SH TLS materialization reached final emission without expansion");
    if (MI->getOpcode() == SH::SH_LONG_BRANCH) {
      emitLongBranch(*MI);
      return;
    }
    if (MI->getOpcode() == SH::SH_REL16_BRANCH) {
      emitRelativeBranch(*MI);
      return;
    }
    const MCSymbol *FDPICCallBase = nullptr;
    if (MI->getOpcode() == SH::BSRF)
      for (const MachineOperand &MO : MI->operands())
        if (MO.isMCSymbol()) {
          if (FDPICCallBase)
            report_fatal_error(
                "SH FDPIC direct call has multiple PC-relative bases");
          FDPICCallBase = MO.getMCSymbol();
        }
    SHMCInstLower Lowering(OutContext, *this);
    MachineBasicBlock::const_instr_iterator I = MI->getIterator();
    MachineBasicBlock::const_instr_iterator E = MI->getParent()->instr_end();
    do {
      if (I->getOpcode() == SH::BRAF && I->getNumExplicitOperands() != 1) {
        if (I->getNumExplicitOperands() != 3 || !I->getOperand(0).isReg() ||
            !I->getOperand(1).isJTI() || !I->getOperand(2).isMCSymbol())
          reportFatalUsageError(
              "SH inline jump-table BRAF metadata is malformed");
        OutStreamer->emitLabel(I->getOperand(2).getMCSymbol());
      }
      MCInst OutMI;
      Lowering.lower(&*I, OutMI);
      EmitToStreamer(*OutStreamer, OutMI);
    } while (++I != E && I->isInsideBundle());
    if (FDPICCallBase)
      OutStreamer->emitLabel(const_cast<MCSymbol *>(FDPICCallBase));
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
