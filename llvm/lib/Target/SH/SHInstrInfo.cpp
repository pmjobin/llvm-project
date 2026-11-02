//===-- SHInstrInfo.cpp - SH instruction information ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHInstrInfo.h"
#include "MCTargetDesc/SHBaseInfo.h"
#include "SHConstantPoolValue.h"
#include "SHFrameLowering.h"
#include "SHInlineAsmSize.h"
#include "SHLiteralPool.h"
#include "SHMachineFunctionInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/CFIInstBuilder.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetMachine.h"
#include <limits>

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "SHGenInstrInfo.inc"

SHInstrInfo::SHInstrInfo(const SHSubtarget &STI)
    : SHGenInstrInfo(STI, RI, SH::ADJCALLSTACKDOWN, SH::ADJCALLSTACKUP), RI() {}

bool SHInstrInfo::isDataSymbolAddress(const MachineInstr &MI) const {
  if (MI.getOpcode() != SH::MOVL_load_pc_island || MI.getNumOperands() < 2 ||
      !MI.getOperand(1).isCPI())
    return false;
  const MachineFunction *MF = MI.getMF();
  if (!MF)
    return false;
  const std::vector<MachineConstantPoolEntry> &Constants =
      MF->getConstantPool()->getConstants();
  unsigned CPI = MI.getOperand(1).getIndex();
  if (CPI >= Constants.size() || !Constants[CPI].isMachineConstantPoolEntry())
    return false;
  const auto *Value =
      static_cast<const SHConstantPoolValue *>(Constants[CPI].Val.MachineCPVal);
  return Value->isGlobalValue() &&
         !Value->getGlobalValue()->getValueType()->isFunctionTy();
}

unsigned SHInstrInfo::getTailMergeSize(const MachineFunction &MF) const {
  if ((MF.getFunction().hasOptSize() || MF.getFunction().hasMinSize()))
    return 1;
  return TargetInstrInfo::getTailMergeSize(MF);
}

bool SHInstrInfo::shouldHoist(const MachineInstr &MI,
                              const MachineLoop *) const {
  const MachineFunction *MF = MI.getMF();
  if (!MF ||
      (!MF->getFunction().hasOptSize() && !MF->getFunction().hasMinSize()))
    return true;
  // On SH, pre-RA invariant hoisting at -Os commonly extends live ranges
  // around loops and costs more spill and constant-pool code than it removes.
  // Preserve post-RA MachineLICM, where virtual-register pressure is settled.
  return !MF->getProperties().hasProperty(
      MachineFunctionProperties::Property::IsSSA);
}

bool SHInstrInfo::analyzeCompare(const MachineInstr &MI, Register &SrcReg,
                                 Register &SrcReg2, int64_t &CmpMask,
                                 int64_t &CmpValue) const {
  if (MI.getOpcode() == SH::TST &&
      MI.getOperand(0).getReg() == MI.getOperand(1).getReg()) {
    SrcReg = MI.getOperand(0).getReg();
    CmpValue = 0;
  } else if (MI.getOpcode() == SH::CMP_EQri_VALUE_PSEUDO &&
             MI.getOperand(1).isImm() && MI.getOperand(1).getImm() == 1) {
    SrcReg = MI.getOperand(0).getReg();
    CmpValue = MI.getOperand(1).getImm();
  } else {
    return false;
  }
  SrcReg2 = Register();
  CmpMask = ~INT64_C(0);
  return true;
}

bool SHInstrInfo::optimizeCompareInstr(MachineInstr &CmpInstr, Register SrcReg,
                                       Register SrcReg2, int64_t CmpMask,
                                       int64_t CmpValue,
                                       const MachineRegisterInfo *MRI) const {
  if (!CmpInstr.getMF()->getSubtarget<SHSubtarget>().hasSH2())
    return false;
  if (!SrcReg.isVirtual() || SrcReg2 || CmpMask != ~INT64_C(0) ||
      (CmpValue != 0 && CmpValue != 1))
    return false;
  MachineInstr *Def = MRI->getUniqueVRegDef(SrcReg);
  bool MaterializedTInverted = false;
  if (Def && Def->getParent() == CmpInstr.getParent() &&
      Def->getOpcode() == SH::XORrr) {
    for (unsigned BooleanOperand : {1U, 2U}) {
      Register BooleanReg = Def->getOperand(BooleanOperand).getReg();
      Register OneReg = Def->getOperand(BooleanOperand == 1 ? 2 : 1).getReg();
      MachineInstr *BooleanDef =
          BooleanReg.isVirtual() ? MRI->getUniqueVRegDef(BooleanReg) : nullptr;
      MachineInstr *OneDef =
          OneReg.isVirtual() ? MRI->getUniqueVRegDef(OneReg) : nullptr;
      if (!BooleanDef || BooleanDef->getOpcode() != SH::MOVT || !OneDef ||
          OneDef->getOpcode() != SH::MOVri || !OneDef->getOperand(1).isImm() ||
          OneDef->getOperand(1).getImm() != 1)
        continue;
      Def = BooleanDef;
      MaterializedTInverted = true;
      break;
    }
  }
  if (Def && Def->getParent() == CmpInstr.getParent() &&
      Def->getOpcode() == SH::MOVT) {
    for (auto I = std::next(Def->getIterator()), E = CmpInstr.getIterator();
         I != E; ++I)
      if (I->readsRegister(SH::TBit, &RI) || I->modifiesRegister(SH::TBit, &RI))
        return false;
    MachineInstr *Branch = nullptr;
    for (auto I = std::next(CmpInstr.getIterator()),
              E = CmpInstr.getParent()->instr_end();
         I != E; ++I) {
      if (!I->readsRegister(SH::TBit, &RI) &&
          !I->modifiesRegister(SH::TBit, &RI))
        continue;
      if (I->readsRegister(SH::TBit, &RI) &&
          !I->modifiesRegister(SH::TBit, &RI))
        Branch = &*I;
      break;
    }
    if (!Branch)
      return false;
    // Identify the branch that can compensate for a net T inversion.
    unsigned InverseOpcode;
    switch (Branch->getOpcode()) {
    default:
      return false;
    case SH::BT:
      InverseOpcode = SH::BF;
      break;
    case SH::BF:
      InverseOpcode = SH::BT;
      break;
    case SH::BT_S:
      InverseOpcode = SH::BF_S;
      break;
    case SH::BF_S:
      InverseOpcode = SH::BT_S;
      break;
    }
    // Comparing a materialized boolean to zero inverts it, while comparing
    // to one preserves it. An intervening XOR by one flips it once more.
    bool ReconstructsOriginalT = MaterializedTInverted == (CmpValue == 0);
    if (ReconstructsOriginalT) {
      Def->clearRegisterKills(SH::TBit, &RI);
      CmpInstr.eraseFromParent();
      return true;
    }
    // Inverting this branch preserves its decision, but leaves the opposite
    // T value for any later consumer. Require a complete proof that the
    // replaced value dies here, including taken edges and carry/borrow uses.
    if (!MRI->tracksLiveness())
      return false;
    for (const MachineBasicBlock *Succ : CmpInstr.getParent()->successors())
      if (Succ->isLiveIn(SH::TBit))
        return false;
    for (auto I = std::next(Branch->getIterator()),
              E = CmpInstr.getParent()->instr_end();
         I != E; ++I) {
      if (I->readsRegister(SH::TBit, &RI))
        return false;
      if (I->modifiesRegister(SH::TBit, &RI))
        break;
    }
    Branch->setDesc(get(InverseOpcode));
    Def->clearRegisterKills(SH::TBit, &RI);
    CmpInstr.eraseFromParent();
    return true;
  }
  if (CmpValue != 0)
    return false;
  MachineInstr *Decrement = Def;
  if (!Decrement || Decrement->getParent() != CmpInstr.getParent() ||
      Decrement->getOpcode() != SH::ADDri ||
      !Decrement->getOperand(2).isImm() ||
      Decrement->getOperand(2).getImm() != -1)
    return false;

  for (auto I = std::next(Decrement->getIterator()), E = CmpInstr.getIterator();
       I != E; ++I)
    if (I->readsRegister(SH::TBit, &RI) || I->modifiesRegister(SH::TBit, &RI))
      return false;

  Decrement->removeOperand(2);
  Decrement->setDesc(get(SH::DT));
  Decrement->addRegisterDefined(SH::TBit);
  CmpInstr.eraseFromParent();
  return true;
}

static bool isExpandedPICAddressAdd(const MachineInstr &MI) {
  if (MI.getOpcode() != SH::ADDrr)
    return false;
  const MachineInstr *Load = MI.getPrevNode();
  const MachineInstr *Mova = Load ? Load->getPrevNode() : nullptr;
  return Load && Mova && Load->getOpcode() == SH::MOVL_load_pc &&
         Mova->getOpcode() == SH::MOVA;
}

static bool touchesRestrictedState(const MachineInstr &MI,
                                   const TargetRegisterInfo &TRI) {
  static constexpr MCPhysReg RestrictedState[] = {
      SH::PC,      SH::GBR,     SH::VBR,     SH::SR,      SH::SSR,
      SH::SPC,     SH::SGR,     SH::DBR,     SH::R0_BANK, SH::R1_BANK,
      SH::R2_BANK, SH::R3_BANK, SH::R4_BANK, SH::R5_BANK, SH::R6_BANK,
      SH::R7_BANK, SH::A0,      SH::A1,      SH::X0,      SH::X1,
      SH::Y0,      SH::Y1,      SH::M0,      SH::M1,      SH::DSR,
      SH::RS,      SH::RE,      SH::MOD,
  };
  for (const MachineOperand &MO : MI.operands()) {
    if (!MO.isReg() || !MO.getReg().isPhysical())
      continue;
    for (MCPhysReg State : RestrictedState)
      if (TRI.regsOverlap(MO.getReg(), State))
        return true;
  }
  return false;
}

bool SHInstrInfo::isLegalInDelaySlot(unsigned Opcode,
                                     const SHSubtarget &STI) const {
  if (SHII::isDelaySlotIllegal(get(Opcode)))
    return false;
  // SH-4 defines MOVA in a delayed branch slot as slot-illegal. SH-2 and
  // SH-3 retain the architecturally defined delayed-PC interpretation.
  return Opcode != SH::MOVA || !STI.isMOVAIllegalInDelaySlot();
}

bool SHInstrInfo::isDelaySlotCandidate(const MachineInstr &MI,
                                       const SHSubtarget &STI) const {
  return isLegalInDelaySlot(MI.getOpcode(), STI) &&
         !isExpandedPICAddressAdd(MI) && !touchesRestrictedState(MI, RI) &&
         !MI.isPseudo() && !MI.isMetaInstruction() && !MI.isInlineAsm() &&
         !MI.isCall() && !MI.isReturn() && !MI.isBranch() &&
         !MI.isTerminator() && !MI.isBarrier() &&
         !MI.hasUnmodeledSideEffects() && !MI.mayRaiseFPException();
}

struct SHI32SynthesisStep {
  unsigned Opcode;
  int64_t Immediate = 0;
};

static SmallVector<SHI32SynthesisStep, 8>
planSignedI32Synthesis(int64_t Value) {
  // Signed radix-256 digits keep every ADD immediate legal and bound the plan
  // to four digits without searching an instruction graph.
  SmallVector<int64_t, 4> ReverseDigits;
  while (!isInt<8>(Value)) {
    uint8_t LowByte = static_cast<uint8_t>(Value);
    int64_t Digit =
        LowByte <= 127 ? LowByte : static_cast<int64_t>(LowByte) - 256;
    ReverseDigits.push_back(Digit);
    Value = (Value - Digit) / 256;
  }
  ReverseDigits.push_back(Value);

  SmallVector<int64_t, 4> Digits(ReverseDigits.rbegin(), ReverseDigits.rend());
  SmallVector<SHI32SynthesisStep, 8> Steps;
  Steps.push_back({SH::MOVri, Digits.front()});
  for (unsigned Index = 1; Index != Digits.size();) {
    if (Digits[Index] != 0) {
      Steps.push_back({SH::SHLL8});
      Steps.push_back({SH::ADDri, Digits[Index]});
      ++Index;
      continue;
    }

    unsigned ZeroCount = 0;
    while (Index + ZeroCount != Digits.size() && Digits[Index + ZeroCount] == 0)
      ++ZeroCount;
    for (unsigned Pair = 0; Pair != ZeroCount / 2; ++Pair)
      Steps.push_back({SH::SHLL16});
    if (ZeroCount % 2 != 0)
      Steps.push_back({SH::SHLL8});
    Index += ZeroCount;
  }
  return Steps;
}

static SmallVector<SHI32SynthesisStep, 8> planI32Synthesis(uint32_t Value) {
  int64_t SignedValue = Value <= std::numeric_limits<int32_t>::max()
                            ? static_cast<int64_t>(Value)
                            : static_cast<int64_t>(Value) - (INT64_C(1) << 32);
  SmallVector<SHI32SynthesisStep, 8> Best = planSignedI32Synthesis(SignedValue);

  if (Value > 127 && Value <= std::numeric_limits<uint8_t>::max()) {
    SmallVector<SHI32SynthesisStep, 8> BytePlan = {
        {SH::MOVri, static_cast<int64_t>(Value) - 256}, {SH::EXTUB}};
    if (BytePlan.size() < Best.size())
      Best = std::move(BytePlan);
  }
  if (Value > std::numeric_limits<int16_t>::max() &&
      Value <= std::numeric_limits<uint16_t>::max()) {
    int64_t SignedWord = static_cast<int64_t>(Value) - (INT64_C(1) << 16);
    SmallVector<SHI32SynthesisStep, 8> WordPlan =
        planSignedI32Synthesis(SignedWord);
    WordPlan.push_back({SH::EXTUW});
    if (WordPlan.size() < Best.size())
      Best = std::move(WordPlan);
  }
  return Best;
}

static bool shouldUseI32Literal(const MachineFunction &MF,
                                unsigned SynthesisInstructions) {
  bool OptimizeSize =
      MF.getFunction().hasOptSize() || MF.getFunction().hasMinSize();
  if (!OptimizeSize && MF.getTarget().getOptLevel() == CodeGenOptLevel::None)
    return false;
  // At -Os, prefer a literal to two or more synthesized instructions.  The
  // four-byte data is shared across repeated uses, which more than amortizes
  // its cost in large functions and shortens the dependent instruction chain.
  // -Oz waits until the isolated form also wins, while speed builds retain
  // synthesis until its dependency chain grows to seven instructions.
  unsigned LiteralThreshold = MF.getFunction().hasMinSize() ? 4U
                              : OptimizeSize                ? 2U
                                                            : 7;
  return SynthesisInstructions >= LiteralThreshold;
}

static bool hasMatchingMoveAcrossCall(MachineBasicBlock &MBB,
                                      MachineBasicBlock::iterator Insert,
                                      int64_t Immediate) {
  bool SawMatchingMove = false;
  for (MachineInstr &MI : llvm::make_range(MBB.begin(), Insert)) {
    if (MI.getOpcode() == SH::MOVri && MI.getOperand(1).getImm() == Immediate)
      SawMatchingMove = true;
    else if (SawMatchingMove && MI.isCall())
      return true;
  }
  return false;
}

static void
avoidCrossCallSynthesisCSE(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator Insert, Register DestReg,
                           SmallVectorImpl<SHI32SynthesisStep> &Steps) {
  if (!DestReg.isVirtual() || Steps.empty() ||
      Steps.front().Opcode != SH::MOVri ||
      !hasMatchingMoveAcrossCall(MBB, Insert, Steps.front().Immediate))
    return;

  // Machine CSE can otherwise reuse the first digit across a call, making a
  // cheap constant occupy a callee-saved register. Start from an adjacent
  // signed byte and correct it before shifting. This costs one bounded step
  // but avoids much larger save/restore sequences under register pressure.
  int64_t Original = Steps.front().Immediate;
  for (int64_t Candidate : {Original - 1, Original + 1}) {
    if (!isInt<8>(Candidate) ||
        hasMatchingMoveAcrossCall(MBB, Insert, Candidate))
      continue;
    Steps.front().Immediate = Candidate;
    Steps.insert(Steps.begin() + 1, {SH::ADDri, Original - Candidate});
    return;
  }
}

void SHInstrInfo::materializeI32(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator Insert,
                                 const DebugLoc &DL, Register DestReg,
                                 uint32_t Value,
                                 MachineInstr::MIFlag Flag) const {
  MachineFunction &MF = *MBB.getParent();
  int64_t SignedValue = SignExtend64<32>(Value);
  if (MF.getSubtarget<SHSubtarget>().hasSH2A()) {
    if (isInt<20>(SignedValue)) {
      BuildMI(MBB, Insert, DL, get(SH::MOVI20), DestReg)
          .addImm(SignedValue)
          .setMIFlag(Flag);
      return;
    }
    if (SignedValue % 256 == 0 && isInt<20>(SignedValue / 256)) {
      BuildMI(MBB, Insert, DL, get(SH::MOVI20S), DestReg)
          .addImm(SignedValue)
          .setMIFlag(Flag);
      return;
    }
  }
  SmallVector<SHI32SynthesisStep, 8> Steps = planI32Synthesis(Value);
  avoidCrossCallSynthesisCSE(MBB, Insert, DestReg, Steps);
  if (shouldUseI32Literal(MF, Steps.size())) {
    bool UseWordLiteral = isInt<16>(SignedValue);
    unsigned LiteralSize = UseWordLiteral ? 2U : 4U;
    Align LiteralAlignment(LiteralSize);
    MF.setAlignment(
        std::max(MF.getAlignment(), Align(SHLiteralIslandAlignment)));
    Type *LiteralType =
        IntegerType::get(MF.getFunction().getContext(), LiteralSize * 8);
    uint64_t LiteralValue = UseWordLiteral ? static_cast<uint16_t>(Value)
                                           : static_cast<uint64_t>(Value);
    const Constant *C = ConstantInt::get(LiteralType, LiteralValue);
    unsigned CPI =
        MF.getConstantPool()->getConstantPoolIndex(C, LiteralAlignment);
    MachineMemOperand *MMO = MF.getMachineMemOperand(
        MachinePointerInfo::getConstantPool(MF),
        MachineMemOperand::MOLoad | MachineMemOperand::MODereferenceable |
            MachineMemOperand::MOInvariant,
        LiteralSize, LiteralAlignment);
    BuildMI(
        MBB, Insert, DL,
        get(UseWordLiteral ? SH::MOVW_load_pc_island : SH::MOVL_load_pc_island),
        DestReg)
        .addConstantPoolIndex(CPI)
        .addImm(SHUnassignedLiteralIsland)
        .addMemOperand(MMO)
        .setMIFlag(Flag);
    return;
  }

  MachineRegisterInfo &MRI = MF.getRegInfo();
  Register Current;
  for (unsigned Index = 0; Index != Steps.size(); ++Index) {
    const SHI32SynthesisStep &Step = Steps[Index];
    bool IsLast = Index + 1 == Steps.size();
    Register Output = DestReg.isPhysical() || IsLast
                          ? DestReg
                          : MRI.createVirtualRegister(&SH::GPRRegClass);
    MachineInstrBuilder Builder =
        BuildMI(MBB, Insert, DL, get(Step.Opcode), Output);
    if (Step.Opcode == SH::MOVri)
      Builder.addImm(Step.Immediate);
    else {
      Builder.addReg(Current);
      if (Step.Opcode == SH::ADDri)
        Builder.addImm(Step.Immediate);
    }
    Builder.setMIFlag(Flag);
    Current = Output;
  }
}

unsigned SHInstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  if (MI.isInlineAsm()) {
    SHInlineAsmSize Size = getSHInlineAsmSize(MI.getOperand(0).getSymbolName());
    if (Size.Kind == SHInlineAsmSizeKind::Indeterminate ||
        Size.Kind == SHInlineAsmSizeKind::Malformed) {
      const MachineFunction &MF = *MI.getParent()->getParent();
      if (MF.getInfo<SHMachineFunctionInfo>()
              ->markInlineAsmSizeDiagnosticReported(&MI))
        MI.emitInlineAsmError("SH inline assembly size cannot be determined: " +
                              Size.Diagnostic);
      return 0;
    }
    if (Size.Bytes > std::numeric_limits<unsigned>::max()) {
      const MachineFunction &MF = *MI.getParent()->getParent();
      if (MF.getInfo<SHMachineFunctionInfo>()
              ->markInlineAsmSizeDiagnosticReported(&MI))
        MI.emitInlineAsmError(
            "SH inline assembly size exceeds the representable range");
      return 0;
    }
    return static_cast<unsigned>(Size.Bytes);
  }

  if (MI.getOpcode() == TargetOpcode::BUNDLE)
    return getInstBundleSize(MI);

  if (MI.getOpcode() == SH::SH_CONSTPOOL_ENTRY)
    return MI.getOperand(2).getImm();
  if (MI.getOpcode() == SH::SH_JUMPTABLE) {
    if (MI.getNumExplicitOperands() != 4 || !MI.getOperand(0).isJTI() ||
        !MI.getOperand(3).isImm())
      reportFatalUsageError("SH inline jump table is malformed");
    int64_t EntrySize = MI.getOperand(3).getImm();
    if (EntrySize != 2 && EntrySize != 4)
      reportFatalUsageError("SH inline jump table has invalid metadata");
    const MachineFunction *MF = MI.getMF();
    const MachineJumpTableInfo *MJTI = MF ? MF->getJumpTableInfo() : nullptr;
    int JTI = MI.getOperand(0).getIndex();
    if (!MJTI || JTI < 0 ||
        static_cast<unsigned>(JTI) >= MJTI->getJumpTables().size())
      reportFatalUsageError("SH inline jump table has an invalid index");
    size_t Entries = MJTI->getJumpTables()[JTI].MBBs.size();
    if (Entries == 0)
      reportFatalUsageError("SH inline jump table has no entries");
    if (Entries >
        std::numeric_limits<unsigned>::max() / static_cast<unsigned>(EntrySize))
      reportFatalUsageError("SH inline jump table size is not representable");
    return Entries * static_cast<unsigned>(EntrySize);
  }
  if (MI.getOpcode() == SH::MOVL_load_pc_island ||
      MI.getOpcode() == SH::MOVW_load_pc_island)
    return SH::PCRelativeLiteralInstructionSize;
  if (MI.getOpcode() == SH::SH_PIC_SETUP ||
      MI.getOpcode() == SH::SH_PIC_ADDRESS)
    return 6;
  if (MI.getOpcode() == SH::SH_PIC_GOTOFF)
    return 4;
  if (MI.getOpcode() == SH::SH_TLS_CALL)
    return 12;
  if (MI.getOpcode() == SH::SH_TLS_IE)
    return 8;
  if (MI.getOpcode() == SH::SH_LONG_BRANCH)
    return 16;
  if (MI.getOpcode() == SH::SH_REL16_BRANCH)
    return 8;

  unsigned Size = MI.getDesc().getSize();
  // SH delay-slot bundles can use the delayed instruction as their header.
  if (MI.isBundledWithSucc() && !MI.isBundledWithPred())
    Size += getInstBundleSize(MI);
  if (MI.getDesc().hasDelaySlot() && !MI.isBundledWithSucc())
    Size += get(SH::NOP).getSize();
  return Size;
}

std::optional<DestSourcePair>
SHInstrInfo::isCopyInstrImpl(const MachineInstr &MI) const {
  if (MI.getOpcode() != SH::MOVrr || MI.getNumOperands() != 2 ||
      MI.isBundled() || MI.getFlags() != MachineInstr::NoFlags)
    return std::nullopt;
  const MachineOperand &Dst = MI.getOperand(0), &Src = MI.getOperand(1);
  if (!Dst.isReg() || !Dst.isDef() || Dst.isImplicit() || Dst.getSubReg() ||
      !Src.isReg() || !Src.isUse() || Src.isImplicit() || Src.isUndef() ||
      Src.getSubReg() || !Dst.getReg().isPhysical() ||
      !Src.getReg().isPhysical() || Dst.getReg() == SH::R15 ||
      Src.getReg() == SH::R15 ||
      !SH::GPRRegClass.contains(Dst.getReg(), Src.getReg()))
    return std::nullopt;
  return DestSourcePair{Dst, Src};
}

void SHInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MI,
                              const DebugLoc &DL, Register DestReg,
                              Register SrcReg, bool KillSrc, bool RenamableDest,
                              bool RenamableSrc) const {
  if (SH::FRRegClass.contains(DestReg, SrcReg)) {
    BuildMI(MBB, MI, DL, get(SH::FMOV_FR), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
    return;
  }
  if (SH::FVRegClass.contains(DestReg, SrcReg)) {
    for (unsigned Sub :
         {SH::sub_fpu32_0, SH::sub_fpu32_1, SH::sub_fpu32_2, SH::sub_fpu32_3})
      BuildMI(MBB, MI, DL, get(SH::FMOV_FR), RI.getSubReg(DestReg, Sub))
          .addReg(RI.getSubReg(SrcReg, Sub), getKillRegState(KillSrc));
    return;
  }
  if (SH::DRRegClass.contains(DestReg, SrcReg)) {
    for (unsigned SubReg : {SH::sub_fpu32_0, SH::sub_fpu32_1}) {
      BuildMI(MBB, MI, DL, get(SH::FMOV_FR), RI.getSubReg(DestReg, SubReg))
          .addReg(RI.getSubReg(SrcReg, SubReg),
                  getKillRegState(KillSrc && SubReg == SH::sub_fpu32_1));
    }
    return;
  }
  if (SH::GPRRegClass.contains(DestReg, SrcReg)) {
    BuildMI(MBB, MI, DL, get(SH::MOVrr))
        .addReg(DestReg, RegState::Define | getRenamableRegState(RenamableDest))
        .addReg(SrcReg,
                getKillRegState(KillSrc) | getRenamableRegState(RenamableSrc));
    return;
  }
  if (SH::GPRRegClass.contains(DestReg)) {
    unsigned Opcode = SrcReg == SH::PR     ? SH::STS_PR
                      : SrcReg == SH::MACH ? SH::STS_MACH
                      : SrcReg == SH::MACL ? SH::STS_MACL
                      : SrcReg == SH::TBit ? SH::MOVT
                                           : 0;
    if (Opcode != 0) {
      BuildMI(MBB, MI, DL, get(Opcode), DestReg);
      return;
    }
  }
  if (SH::GPRRegClass.contains(SrcReg)) {
    unsigned Opcode = DestReg == SH::PR     ? SH::LDS_PR
                      : DestReg == SH::MACH ? SH::LDS_MACH
                      : DestReg == SH::MACL ? SH::LDS_MACL
                                            : 0;
    if (Opcode != 0) {
      BuildMI(MBB, MI, DL, get(Opcode))
          .addReg(SrcReg, getKillRegState(KillSrc));
      return;
    }
  }
  report_fatal_error("SH cannot copy the requested physical registers");
}

bool SHInstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  if (expandGraphicsPseudo(MI))
    return true;
  unsigned Opcode = MI.getOpcode();
  if (Opcode == SH::FIPR_VALUE) {
    MachineBasicBlock &MBB = *MI.getParent();
    const DebugLoc &DL = MI.getDebugLoc();
    Register Result = MI.getOperand(0).getReg();
    Register Scratch = MI.getOperand(1).getReg();
    BuildMI(MBB, MI, DL, get(SH::FIPR), Scratch)
        .addReg(MI.getOperand(2).getReg())
        .addReg(MI.getOperand(3).getReg());
    Register Lane = RI.getSubReg(Scratch, SH::sub_fpu32_3);
    if (Result != Lane) {
      BuildMI(MBB, MI, DL, get(SH::FLDS)).addReg(Lane);
      BuildMI(MBB, MI, DL, get(SH::FSTS), Result);
    }
    MI.eraseFromParent();
    return true;
  }
  if (Opcode == SH::FMAC_FR_PSEUDO) {
    MachineBasicBlock &MBB = *MI.getParent();
    const DebugLoc &DL = MI.getDebugLoc();
    MachineOperand *MulLHS = &MI.getOperand(2);
    MachineOperand *MulRHS = &MI.getOperand(3);
    if (MulRHS->getReg() == SH::FR0 && MulLHS->getReg() != SH::FR0) {
      MachineOperand *Temporary = MulLHS;
      MulLHS = MulRHS;
      MulRHS = Temporary;
    }
    if (MulLHS->getReg() != SH::FR0)
      BuildMI(MBB, MI, DL, get(SH::FMOV_FR), SH::FR0).addReg(MulLHS->getReg());
    MachineInstrBuilder FMAC =
        BuildMI(MBB, MI, DL, get(SH::FMAC_FR), MI.getOperand(0).getReg());
    FMAC.addReg(MI.getOperand(1).getReg(),
                getKillRegState(MI.getOperand(1).isKill()));
    FMAC.addReg(MulRHS->getReg());
    FMAC.setMIFlags(MI.getFlags());
    MI.eraseFromParent();
    return true;
  }
  if (Opcode == SH::FMOV_D_LOAD_SCALAR || Opcode == SH::FMOV_D_STORE_SCALAR) {
    MachineBasicBlock &MBB = *MI.getParent();
    MachineFunction &MF = *MBB.getParent();
    MachineBasicBlock::iterator Insert = MI.getIterator();
    const DebugLoc &DL = MI.getDebugLoc();
    bool IsLoad = Opcode == SH::FMOV_D_LOAD_SCALAR;
    Register FPUReg = MI.getOperand(0).getReg();
    MachineOperand &Base = MI.getOperand(1);
    auto GetFPUWord = [&](unsigned MemoryWord) {
      unsigned PhysicalWord =
          MF.getDataLayout().isBigEndian() ? MemoryWord : 1 - MemoryWord;
      return Register(RI.getSubReg(
          FPUReg, PhysicalWord == 0 ? SH::sub_fpu32_0 : SH::sub_fpu32_1));
    };
    auto AddWordMMOs = [&](MachineInstrBuilder &MIB, unsigned Word) {
      for (const MachineMemOperand *MMO : MI.memoperands())
        MIB.addMemOperand(MF.getMachineMemOperand(MMO, 4 * Word, 4));
    };
    if (IsLoad) {
      MachineInstrBuilder First =
          BuildMI(MBB, Insert, DL, get(SH::FMOV_S_LOAD_ALIAS), GetFPUWord(0));
      First.addReg(Base.getReg()).setMIFlags(MI.getFlags());
      AddWordMMOs(First, 0);
    } else {
      MachineInstrBuilder First =
          BuildMI(MBB, Insert, DL, get(SH::FMOV_S_STORE_ALIAS));
      First
          .addReg(GetFPUWord(0),
                  getKillRegState(MI.getOperand(0).isKill()) |
                      getUndefRegState(MI.getOperand(0).isUndef()))
          .addReg(Base.getReg())
          .setMIFlags(MI.getFlags());
      AddWordMMOs(First, 0);
    }
    bool ReuseIndex = false;
    for (auto I = Insert; I != MBB.begin();) {
      --I;
      if (I->isDebugInstr())
        continue;
      if (!I->modifiesRegister(SH::R0, &RI))
        continue;
      ReuseIndex = I->getOpcode() == SH::MOVri &&
                   I->getOperand(0).getReg() == SH::R0 &&
                   I->getOperand(1).getImm() == 4;
      if (ReuseIndex) {
        // Expansion extends this constant's live range past uses that
        // allocation could kill before the pseudo's implicit R0 def.
        I->clearRegisterDeads(SH::R0);
        for (auto J = std::next(I); J != Insert; ++J)
          J->clearRegisterKills(SH::R0, &RI);
      }
      break;
    }
    if (!ReuseIndex)
      BuildMI(MBB, Insert, DL, get(SH::MOVri), SH::R0).addImm(4);
    if (IsLoad) {
      MachineInstrBuilder Second = BuildMI(
          MBB, Insert, DL, get(SH::FMOV_S_LOAD_INDEXED_ALIAS), GetFPUWord(1));
      Second.addReg(Base.getReg(), getKillRegState(Base.isKill()))
          .setMIFlags(MI.getFlags());
      AddWordMMOs(Second, 1);
    } else {
      MachineInstrBuilder Second =
          BuildMI(MBB, Insert, DL, get(SH::FMOV_S_STORE_INDEXED_ALIAS));
      Second
          .addReg(GetFPUWord(1),
                  getKillRegState(MI.getOperand(0).isKill()) |
                      getUndefRegState(MI.getOperand(0).isUndef()))
          .addReg(Base.getReg(), getKillRegState(Base.isKill()))
          .setMIFlags(MI.getFlags());
      AddWordMMOs(Second, 1);
    }
    MI.eraseFromParent();
    return true;
  }
  if (Opcode == SH::SET_FPSCR_FIELDS) {
    MachineBasicBlock &MBB = *MI.getParent();
    const DebugLoc &DL = MI.getDebugLoc();
    Register Scratch = MI.getOperand(0).getReg();
    Register Mask = MI.getOperand(1).getReg();
    Register Value = MI.getOperand(2).getReg();
    BuildMI(MBB, MI, DL, get(SH::STS_FPSCR), Scratch);
    BuildMI(MBB, MI, DL, get(SH::XORrr), Scratch).addReg(Scratch).addReg(Value);
    BuildMI(MBB, MI, DL, get(SH::MOVri), Mask).addImm(0x38);
    BuildMI(MBB, MI, DL, get(SH::SHLL16), Mask).addReg(Mask);
    BuildMI(MBB, MI, DL, get(SH::ANDrr), Scratch).addReg(Scratch).addReg(Mask);
    BuildMI(MBB, MI, DL, get(SH::XORrr), Scratch).addReg(Scratch).addReg(Value);
    BuildMI(MBB, MI, DL, get(SH::LDS_FPSCR)).addReg(Scratch);
    MI.eraseFromParent();
    return true;
  }
  if (Opcode == SH::MATERIALIZE_FPSCR_MASK) {
    MachineBasicBlock &MBB = *MI.getParent();
    MachineBasicBlock::iterator Insert = MI.getIterator();
    const DebugLoc &DL = MI.getDebugLoc();
    Register Dest = MI.getOperand(0).getReg();
    BuildMI(MBB, Insert, DL, get(SH::MOVri), Dest).add(MI.getOperand(1));
    BuildMI(MBB, Insert, DL, get(SH::SHLL16), Dest).addReg(Dest);
    MI.eraseFromParent();
    return true;
  }
  if (Opcode == SH::CMP_EQri_VALUE_PSEUDO) {
    Register Source = MI.getOperand(0).getReg();
    if (Source != SH::R0)
      BuildMI(*MI.getParent(), MI, MI.getDebugLoc(), get(SH::MOVrr), SH::R0)
          .addReg(Source, getKillRegState(MI.getOperand(0).isKill()))
          .setMIFlags(MI.getFlags());
    BuildMI(*MI.getParent(), MI, MI.getDebugLoc(), get(SH::CMP_EQri))
        .add(MI.getOperand(1))
        .setMIFlags(MI.getFlags());
    MI.eraseFromParent();
    return true;
  }
  if (Opcode == SH::TSTri_PSEUDO) {
    Register Source = MI.getOperand(0).getReg();
    if (Source != SH::R0)
      BuildMI(*MI.getParent(), MI, MI.getDebugLoc(), get(SH::MOVrr), SH::R0)
          .addReg(Source, getKillRegState(MI.getOperand(0).isKill()))
          .setMIFlags(MI.getFlags());
    BuildMI(*MI.getParent(), MI, MI.getDebugLoc(), get(SH::TSTri))
        .add(MI.getOperand(1))
        .setMIFlags(MI.getFlags());
    MI.eraseFromParent();
    return true;
  }
  if (Opcode == SH::RTE_RETURN) {
    MachineBasicBlock &MBB = *MI.getParent();
    if (MBB.getParent()->getInfo<SHMachineFunctionInfo>()->usesResbank())
      BuildMI(MBB, MI, MI.getDebugLoc(), get(SH::RESBANK));
    BuildMI(MBB, MI, MI.getDebugLoc(), get(SH::RTE));
    MI.eraseFromParent();
    return true;
  }
  if (Opcode == SH::SET_FPSCR_MODE || Opcode == SH::TRANSITION_FPSCR_MODE ||
      Opcode == SH::TRANSITION_FPSCR_MODE_REG) {
    MachineBasicBlock &MBB = *MI.getParent();
    MachineBasicBlock::iterator Insert = MI.getIterator();
    const DebugLoc &DL = MI.getDebugLoc();
    if (Opcode == SH::TRANSITION_FPSCR_MODE_REG) {
      BuildMI(MBB, Insert, DL, get(SH::STS_FPSCR), SH::R0);
      BuildMI(MBB, Insert, DL, get(SH::XORrr), SH::R0)
          .addReg(SH::R0)
          .add(MI.getOperand(1));
      BuildMI(MBB, Insert, DL, get(SH::LDS_FPSCR)).addReg(SH::R0);
      MI.eraseFromParent();
      return true;
    }
    if (Opcode == SH::TRANSITION_FPSCR_MODE) {
      int64_t Mask = MI.getOperand(1).getImm();
      bool ReuseMask = false;
      for (auto I = Insert; I != MBB.begin();) {
        --I;
        if (I->isDebugInstr())
          continue;
        if (!I->modifiesRegister(SH::R1, &RI))
          continue;
        if (I->getOpcode() != SH::SHLL16 ||
            I->getOperand(0).getReg() != SH::R1 ||
            I->getOperand(1).getReg() != SH::R1)
          break;
        auto Materialize = I;
        while (Materialize != MBB.begin()) {
          --Materialize;
          if (!Materialize->isDebugInstr())
            break;
        }
        ReuseMask = Materialize->getOpcode() == SH::MOVri &&
                    Materialize->getOperand(0).getReg() == SH::R1 &&
                    Materialize->getOperand(1).getImm() == Mask;
        break;
      }
      BuildMI(MBB, Insert, DL, get(SH::STS_FPSCR), SH::R0);
      if (!ReuseMask) {
        BuildMI(MBB, Insert, DL, get(SH::MOVri), SH::R1).addImm(Mask);
        BuildMI(MBB, Insert, DL, get(SH::SHLL16), SH::R1).addReg(SH::R1);
      }
      BuildMI(MBB, Insert, DL, get(SH::XORrr), SH::R0)
          .addReg(SH::R0)
          .addReg(SH::R1);
      BuildMI(MBB, Insert, DL, get(SH::LDS_FPSCR)).addReg(SH::R0);
      MI.eraseFromParent();
      return true;
    }
    int64_t Mode = MI.getOperand(0).getImm();
    uint32_t ClearMask;
    uint32_t SetMask;
    if (Mode == 0) {
      ClearMask = 1U << 21;
      SetMask = 0;
    } else if (Mode == 1) {
      ClearMask = 7U << 19;
      SetMask = 0;
    } else if (Mode == 2) {
      ClearMask = 7U << 19;
      SetMask = 3U << 19;
    } else if (Mode == 3) {
      ClearMask = 7U << 19;
      SetMask = 1U << 19;
    } else if (Mode == 4) {
      ClearMask = 7U << 19;
      SetMask = 1U << 20;
    } else {
      report_fatal_error("SH FPSCR mode pseudo has an invalid mode");
    }
    BuildMI(MBB, Insert, DL, get(SH::STS_FPSCR), SH::R0);
    BuildMI(MBB, Insert, DL, get(SH::MOVri), SH::R1).addImm(ClearMask >> 16);
    BuildMI(MBB, Insert, DL, get(SH::SHLL16), SH::R1).addReg(SH::R1);
    BuildMI(MBB, Insert, DL, get(SH::NOT), SH::R1).addReg(SH::R1);
    BuildMI(MBB, Insert, DL, get(SH::ANDrr), SH::R0)
        .addReg(SH::R0)
        .addReg(SH::R1);
    if (SetMask != 0) {
      BuildMI(MBB, Insert, DL, get(SH::MOVri), SH::R1).addImm(SetMask >> 16);
      BuildMI(MBB, Insert, DL, get(SH::SHLL16), SH::R1).addReg(SH::R1);
      BuildMI(MBB, Insert, DL, get(SH::ORrr), SH::R0)
          .addReg(SH::R0)
          .addReg(SH::R1);
    }
    BuildMI(MBB, Insert, DL, get(SH::LDS_FPSCR)).addReg(SH::R0);
    MI.eraseFromParent();
    return true;
  }
  if (Opcode == SH::GPR_TO_FR_PSEUDO || Opcode == SH::FR_TO_GPR_PSEUDO ||
      Opcode == SH::GPR_PAIR_TO_DR_PSEUDO ||
      Opcode == SH::DR_TO_GPR_PAIR_PSEUDO || Opcode == SH::I32_TO_F32_PSEUDO ||
      Opcode == SH::I32_TO_F64_PSEUDO || Opcode == SH::F32_TO_I32_PSEUDO ||
      Opcode == SH::F64_TO_I32_PSEUDO || Opcode == SH::F32_TO_F64_PSEUDO ||
      Opcode == SH::F64_TO_F32_PSEUDO) {
    MachineBasicBlock &MBB = *MI.getParent();
    MachineBasicBlock::iterator Insert = MI.getIterator();
    const DebugLoc &DL = MI.getDebugLoc();
    // Preserve the data-use contract without transferring def, implicit or
    // tied-operand flags to the scalar instruction's explicit input.
    auto DataUseFlags = [](const MachineOperand &Src) {
      return getKillRegState(Src.isKill()) | getUndefRegState(Src.isUndef());
    };
    if (Opcode == SH::GPR_TO_FR_PSEUDO) {
      MachineOperand &Src = MI.getOperand(1);
      BuildMI(MBB, Insert, DL, get(SH::LDS_FPUL))
          .addReg(Src.getReg(), DataUseFlags(Src));
      BuildMI(MBB, Insert, DL, get(SH::FSTS), MI.getOperand(0).getReg());
    } else if (Opcode == SH::FR_TO_GPR_PSEUDO) {
      MachineOperand &Src = MI.getOperand(1);
      BuildMI(MBB, Insert, DL, get(SH::FLDS))
          .addReg(Src.getReg(), DataUseFlags(Src));
      BuildMI(MBB, Insert, DL, get(SH::STS_FPUL), MI.getOperand(0).getReg());
    } else if (Opcode == SH::GPR_PAIR_TO_DR_PSEUDO) {
      Register Dest = MI.getOperand(0).getReg();
      bool SameSource = MI.getOperand(1).getReg() == MI.getOperand(2).getReg();
      for (unsigned I = 0; I != 2; ++I) {
        MachineOperand &Src = MI.getOperand(I + 1);
        if (I == 0 || !SameSource) {
          bool KillSource =
              Src.isKill() || (SameSource && MI.getOperand(2).isKill());
          // A shared load must retain either half's defined input. When
          // both are undef, share one choice for the two equal operands.
          bool UndefSource =
              Src.isUndef() && (!SameSource || MI.getOperand(2).isUndef());
          // An undef half's kill does not kill the other half's live,
          // defined value when the two loads are merged.
          if (SameSource && Src.isUndef() != MI.getOperand(2).isUndef())
            KillSource =
                Src.isUndef() ? MI.getOperand(2).isKill() : Src.isKill();
          BuildMI(MBB, Insert, DL, get(SH::LDS_FPUL))
              .addReg(Src.getReg(), getKillRegState(KillSource) |
                                        getUndefRegState(UndefSource));
        }
        BuildMI(MBB, Insert, DL, get(SH::FSTS),
                RI.getSubReg(Dest, I == 0 ? SH::sub_fpu32_0 : SH::sub_fpu32_1));
      }
    } else if (Opcode == SH::DR_TO_GPR_PAIR_PSEUDO) {
      Register Src = MI.getOperand(2).getReg();
      for (unsigned I = 0; I != 2; ++I) {
        BuildMI(MBB, Insert, DL, get(SH::FLDS))
            .addReg(
                RI.getSubReg(Src, I == 0 ? SH::sub_fpu32_0 : SH::sub_fpu32_1),
                getKillRegState(MI.getOperand(2).isKill() && I == 1) |
                    getUndefRegState(MI.getOperand(2).isUndef()));
        BuildMI(MBB, Insert, DL, get(SH::STS_FPUL), MI.getOperand(I).getReg());
      }
    } else if (Opcode == SH::I32_TO_F32_PSEUDO ||
               Opcode == SH::I32_TO_F64_PSEUDO) {
      MachineOperand &Src = MI.getOperand(1);
      BuildMI(MBB, Insert, DL, get(SH::LDS_FPUL))
          .addReg(Src.getReg(), DataUseFlags(Src));
      BuildMI(
          MBB, Insert, DL,
          get(Opcode == SH::I32_TO_F32_PSEUDO ? SH::FLOAT_FR : SH::FLOAT_DR),
          MI.getOperand(0).getReg());
    } else if (Opcode == SH::F32_TO_I32_PSEUDO ||
               Opcode == SH::F64_TO_I32_PSEUDO) {
      MachineOperand &Src = MI.getOperand(1);
      BuildMI(MBB, Insert, DL,
              get(Opcode == SH::F32_TO_I32_PSEUDO ? SH::FTRC_FR : SH::FTRC_DR))
          .addReg(Src.getReg(), DataUseFlags(Src));
      BuildMI(MBB, Insert, DL, get(SH::STS_FPUL), MI.getOperand(0).getReg());
    } else if (Opcode == SH::F32_TO_F64_PSEUDO) {
      MachineOperand &Src = MI.getOperand(1);
      BuildMI(MBB, Insert, DL, get(SH::FLDS))
          .addReg(Src.getReg(), DataUseFlags(Src));
      BuildMI(MBB, Insert, DL, get(SH::FCNVSD), MI.getOperand(0).getReg());
    } else {
      MachineOperand &Src = MI.getOperand(1);
      BuildMI(MBB, Insert, DL, get(SH::FCNVDS))
          .addReg(Src.getReg(), DataUseFlags(Src));
      BuildMI(MBB, Insert, DL, get(SH::FSTS), MI.getOperand(0).getReg());
    }
    MI.eraseFromParent();
    return true;
  }
  if (Opcode == SH::EH_RETURN) {
    MachineBasicBlock &MBB = *MI.getParent();
    MachineBasicBlock::iterator Insert = MI.getIterator();
    const DebugLoc &DL = MI.getDebugLoc();
    BuildMI(MBB, Insert, DL, get(SH::ADDrr), SH::R15)
        .addReg(SH::R15)
        .addReg(SH::R3);
    BuildMI(MBB, Insert, DL, get(SH::LDS_PR))
        .addReg(MI.getOperand(0).getReg(),
                getKillRegState(MI.getOperand(0).isKill()));
    BuildMI(MBB, Insert, DL, get(SH::RTS));
    MBB.erase(Insert);
    return true;
  }
  bool IsLoad = Opcode == SH::MOVB_load_frame || Opcode == SH::MOVW_load_frame;
  bool IsStore =
      Opcode == SH::MOVB_store_frame || Opcode == SH::MOVW_store_frame;
  if (!IsLoad && !IsStore)
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  MachineBasicBlock::iterator Insert = MI.getIterator();
  const DebugLoc &DL = MI.getDebugLoc();
  unsigned RealOpcode;
  if (Opcode == SH::MOVB_load_frame)
    RealOpcode = SH::MOVB_load_disp;
  else if (Opcode == SH::MOVW_load_frame)
    RealOpcode = SH::MOVW_load_disp;
  else if (Opcode == SH::MOVB_store_frame)
    RealOpcode = SH::MOVB_store_disp;
  else
    RealOpcode = SH::MOVW_store_disp;

  if (IsLoad) {
    MachineOperand &Dest = MI.getOperand(0);
    Register DestReg = Dest.getReg();
    MachineInstrBuilder Load = BuildMI(MBB, Insert, DL, get(RealOpcode), SH::R0)
                                   .add(MI.getOperand(1))
                                   .add(MI.getOperand(2))
                                   .setMIFlags(MI.getFlags())
                                   .cloneMemRefs(MI);
    if (DestReg == SH::R0) {
      Load->getOperand(0).setIsDead(Dest.isDead());
    } else {
      // Preserve the allocated operand's renaming provenance. The pseudo's
      // conservative descriptor hides it until this ordinary copy is emitted.
      BuildMI(MBB, Insert, DL, get(SH::MOVrr))
          .add(Dest)
          .addReg(SH::R0, RegState::Kill);
    }
  } else {
    MachineOperand &Src = MI.getOperand(0);
    Register SrcReg = Src.getReg();
    if (SrcReg != SH::R0) {
      MachineInstrBuilder Copy =
          BuildMI(MBB, Insert, DL, get(SH::MOVrr), SH::R0).add(Src);
      // The address is still read by the following store after this copy.
      if (RI.regsOverlap(SrcReg, MI.getOperand(1).getReg()))
        Copy->getOperand(1).setIsKill(false);
    }
    RegState R0State =
        SrcReg == SH::R0 ? getKillRegState(Src.isKill()) : RegState::Kill;
    BuildMI(MBB, Insert, DL, get(RealOpcode))
        .addReg(SH::R0, R0State)
        .add(MI.getOperand(1))
        .add(MI.getOperand(2))
        .setMIFlags(MI.getFlags())
        .cloneMemRefs(MI);
  }

  MI.eraseFromParent();
  return true;
}

Register SHInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                          int &FrameIndex) const {
  if ((MI.getOpcode() == SH::FMOV_S_LOAD_FI ||
       MI.getOpcode() == SH::FMOV_D_LOAD_FI) &&
      MI.getOperand(1).isFI() && MI.getOperand(2).isImm() &&
      MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  if (MI.getOpcode() == SH::MOVL_load_disp && MI.getOperand(1).isFI() &&
      MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  return Register();
}

Register SHInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                         int &FrameIndex) const {
  if ((MI.getOpcode() == SH::FMOV_S_STORE_FI ||
       MI.getOpcode() == SH::FMOV_D_STORE_FI) &&
      MI.getOperand(1).isFI() && MI.getOperand(2).isImm() &&
      MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  if (MI.getOpcode() == SH::MOVL_store_disp && MI.getOperand(1).isFI() &&
      MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  return Register();
}

void SHInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool IsKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
    MachineInstr::MIFlag Flags) const {
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  if (SH::FRRegClass.hasSubClassEq(RC) || SH::DRRegClass.hasSubClassEq(RC) ||
      SH::FVRegClass.hasSubClassEq(RC)) {
    bool IsDouble = SH::DRRegClass.hasSubClassEq(RC);
    bool IsVector = SH::FVRegClass.hasSubClassEq(RC);
    int64_t Size = IsVector ? 16 : IsDouble ? 8 : 4;
    if (MFI.getObjectSize(FrameIndex) != Size ||
        MFI.getObjectAlign(FrameIndex) < Align(4))
      report_fatal_error("SH FPU spill slot has an invalid size or alignment");
    MachineMemOperand *MMO = MF.getMachineMemOperand(
        MachinePointerInfo::getFixedStack(MF, FrameIndex),
        MachineMemOperand::MOStore, Size, Align(4));
    BuildMI(MBB, MI, MBB.findDebugLoc(MI),
            get(IsVector   ? SH::FMOV_V_STORE_FI
                : IsDouble ? SH::FMOV_D_STORE_FI
                           : SH::FMOV_S_STORE_FI))
        .addReg(SrcReg,
                getKillRegState(IsKill && !MF.getRegInfo().isLiveIn(SrcReg)))
        .addFrameIndex(FrameIndex)
        .addImm(0)
        .addMemOperand(MMO)
        .setMIFlag(Flags);
    return;
  }
  if (!SH::GPRRegClass.hasSubClassEq(RC))
    report_fatal_error("SH cannot spill the requested register class");
  if (MFI.getObjectSize(FrameIndex) != 4 ||
      MFI.getObjectAlign(FrameIndex) < Align(4))
    report_fatal_error("SH GPR spill slots must be four-byte aligned words");

  MachineMemOperand *MMO =
      MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FrameIndex),
                              MachineMemOperand::MOStore, 4, Align(4));
  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(SH::MOVL_store_disp))
      .addReg(SrcReg,
              getKillRegState(IsKill && !MF.getRegInfo().isLiveIn(SrcReg)))
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}

void SHInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MI,
                                       Register DestReg, int FrameIndex,
                                       const TargetRegisterClass *RC,
                                       Register VReg, unsigned SubReg,
                                       MachineInstr::MIFlag Flags) const {
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  if ((SH::FRRegClass.hasSubClassEq(RC) || SH::DRRegClass.hasSubClassEq(RC) ||
       SH::FVRegClass.hasSubClassEq(RC)) &&
      SubReg == 0) {
    bool IsDouble = SH::DRRegClass.hasSubClassEq(RC);
    bool IsVector = SH::FVRegClass.hasSubClassEq(RC);
    int64_t Size = IsVector ? 16 : IsDouble ? 8 : 4;
    if (MFI.getObjectSize(FrameIndex) != Size ||
        MFI.getObjectAlign(FrameIndex) < Align(4))
      report_fatal_error("SH FPU spill slot has an invalid size or alignment");
    MachineMemOperand *MMO = MF.getMachineMemOperand(
        MachinePointerInfo::getFixedStack(MF, FrameIndex),
        MachineMemOperand::MOLoad, Size, Align(4));
    BuildMI(MBB, MI, MBB.findDebugLoc(MI),
            get(IsVector   ? SH::FMOV_V_LOAD_FI
                : IsDouble ? SH::FMOV_D_LOAD_FI
                           : SH::FMOV_S_LOAD_FI),
            DestReg)
        .addFrameIndex(FrameIndex)
        .addImm(0)
        .addMemOperand(MMO)
        .setMIFlag(Flags);
    return;
  }
  if (!SH::GPRRegClass.hasSubClassEq(RC) || SubReg != 0)
    report_fatal_error("SH cannot reload the requested register class");
  if (MFI.getObjectSize(FrameIndex) != 4 ||
      MFI.getObjectAlign(FrameIndex) < Align(4))
    report_fatal_error("SH GPR spill slots must be four-byte aligned words");

  MachineMemOperand *MMO =
      MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FrameIndex),
                              MachineMemOperand::MOLoad, 4, Align(4));
  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(SH::MOVL_load_disp), DestReg)
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}

static std::optional<SHCC::CondCode> getSHBranchCondition(unsigned Opcode) {
  if (Opcode == SH::BT)
    return SHCC::TSet;
  if (Opcode == SH::BF)
    return SHCC::TClear;
  return std::nullopt;
}

bool SHInstrInfo::analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                                MachineBasicBlock *&FBB,
                                SmallVectorImpl<MachineOperand> &Cond,
                                bool AllowModify) const {
  MachineBasicBlock::iterator I = MBB.end();
  while (I != MBB.begin()) {
    --I;
    if (I->isDebugInstr())
      continue;
    if (!isUnpredicatedTerminator(*I))
      break;
    if (!I->isBranch())
      return true;

    if (I->getOpcode() == SH::BRA) {
      if (!AllowModify) {
        TBB = I->getOperand(0).getMBB();
        continue;
      }

      MBB.erase(std::next(I), MBB.end());
      Cond.clear();
      FBB = nullptr;
      if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
        TBB = nullptr;
        I->eraseFromParent();
        I = MBB.end();
        continue;
      }
      TBB = I->getOperand(0).getMBB();
      continue;
    }

    std::optional<SHCC::CondCode> BranchCode =
        getSHBranchCondition(I->getOpcode());
    if (!BranchCode)
      return true;
    if (Cond.empty()) {
      FBB = TBB;
      TBB = I->getOperand(0).getMBB();
      Cond.push_back(MachineOperand::CreateImm(*BranchCode));
      continue;
    }
    if (Cond.size() != 1 || TBB != I->getOperand(0).getMBB() ||
        Cond[0].getImm() != *BranchCode)
      return true;
  }
  return false;
}

unsigned SHInstrInfo::insertBranch(MachineBasicBlock &MBB,
                                   MachineBasicBlock *TBB,
                                   MachineBasicBlock *FBB,
                                   ArrayRef<MachineOperand> Cond,
                                   const DebugLoc &DL, int *BytesAdded) const {
  assert(TBB && "SH branch must have a destination");
  assert(Cond.size() <= 1 && "invalid SH branch condition");
  if (BytesAdded)
    *BytesAdded = 0;

  if (Cond.empty()) {
    assert(!FBB && "unconditional SH branch has a false destination");
    BuildMI(&MBB, DL, get(SH::BRA)).addMBB(TBB);
    if (BytesAdded)
      *BytesAdded = get(SH::BRA).getSize() + get(SH::NOP).getSize();
    return 1;
  }

  assert(Cond[0].isImm() && "invalid SH branch condition operand");
  unsigned Opcode = Cond[0].getImm() == SHCC::TSet ? SH::BT : SH::BF;
  BuildMI(&MBB, DL, get(Opcode)).addMBB(TBB);
  unsigned Count = 1;
  if (BytesAdded)
    *BytesAdded = get(Opcode).getSize();
  if (FBB) {
    BuildMI(&MBB, DL, get(SH::BRA)).addMBB(FBB);
    ++Count;
    if (BytesAdded)
      *BytesAdded += get(SH::BRA).getSize() + get(SH::NOP).getSize();
  }
  return Count;
}

unsigned SHInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                   int *BytesRemoved) const {
  if (BytesRemoved)
    *BytesRemoved = 0;
  unsigned Count = 0;

  while (!MBB.empty()) {
    MachineBasicBlock::iterator I = MBB.end();
    do {
      --I;
    } while (I != MBB.begin() && I->isDebugInstr());

    if (I->getOpcode() == SH::NOP && I->isBundledWithPred()) {
      MachineBasicBlock::iterator Slot = I;
      --I;
      if (I->getOpcode() != SH::BRA || !I->isBundledWithSucc())
        break;
      if (BytesRemoved)
        *BytesRemoved += get(SH::BRA).getSize() + get(SH::NOP).getSize();
      MBB.erase(I, std::next(Slot));
      ++Count;
      continue;
    }

    if (I->getOpcode() != SH::BRA && I->getOpcode() != SH::BT &&
        I->getOpcode() != SH::BF)
      break;
    if (BytesRemoved)
      *BytesRemoved += I->getOpcode() == SH::BRA
                           ? get(SH::BRA).getSize() + get(SH::NOP).getSize()
                           : I->getDesc().getSize();
    I->eraseFromParent();
    ++Count;
  }
  return Count;
}

bool SHInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  if (Cond.size() != 1 || !Cond[0].isImm())
    return true;
  if (Cond[0].getImm() == SHCC::TSet)
    Cond[0].setImm(SHCC::TClear);
  else if (Cond[0].getImm() == SHCC::TClear)
    Cond[0].setImm(SHCC::TSet);
  else
    return true;
  return false;
}

MachineBasicBlock *
SHInstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
  if (MI.getOpcode() == SH::JMP)
    return nullptr;
  assert((MI.getOpcode() == SH::BRA || MI.getOpcode() == SH::BT ||
          MI.getOpcode() == SH::BF || MI.getOpcode() == SH::SH_LONG_BRANCH ||
          MI.getOpcode() == SH::SH_REL16_BRANCH) &&
         "not an SH direct branch");
  return MI
      .getOperand(MI.getOpcode() == SH::SH_LONG_BRANCH ||
                          MI.getOpcode() == SH::SH_REL16_BRANCH
                      ? 1
                      : 0)
      .getMBB();
}

bool SHInstrInfo::isBranchOffsetInRange(unsigned BranchOpc,
                                        int64_t BrOffset) const {
  if (BranchOpc == SH::BT || BranchOpc == SH::BF)
    return SH::isBranchOffsetInRange(BrOffset,
                                     SH::ConditionalBranchDisplacementBits);
  if (BranchOpc == SH::BRA)
    return SH::isBranchOffsetInRange(BrOffset,
                                     SH::UnconditionalBranchDisplacementBits);
  if (BranchOpc == SH::SH_LONG_BRANCH || BranchOpc == SH::SH_REL16_BRANCH)
    return true;
  llvm_unreachable("not an SH direct branch");
}

void SHInstrInfo::insertIndirectBranch(MachineBasicBlock &MBB,
                                       MachineBasicBlock &NewDestBB,
                                       MachineBasicBlock &RestoreBB,
                                       const DebugLoc &DL, int64_t BrOffset,
                                       RegScavenger *RS) const {
  assert(MBB.empty() && "SH long branch block must be empty");
  assert(RestoreBB.empty() && "SH long branch restore block must be empty");
  if (!isInt<32>(BrOffset))
    report_fatal_error("SH long branch displacement exceeds signed 32 bits");

  MachineFunction &MF = *MBB.getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  // Leave one displacement bit of headroom for source growth and restore
  // blocks inserted later in branch relaxation.
  bool UseRelativeBranch = isInt<15>(BrOffset);
  bool SaveR0 = !UseRelativeBranch;
  bool SaveScratch = true;
  Register Scratch;
  if (RS) {
    RS->enterBasicBlockEnd(MBB);
    if (!UseRelativeBranch)
      SaveR0 = RS->isRegUsed(SH::R0);
    Scratch = RS->FindUnusedReg(UseRelativeBranch ? &SH::GPRRegClass
                                                  : &SH::GPRNoR0RegClass);
    SaveScratch = !Scratch;
  }
  if (!Scratch) {
    const TargetRegisterClass &ScratchClass =
        UseRelativeBranch ? SH::GPRRegClass : SH::GPRNoR0RegClass;
    for (Register Candidate : ScratchClass) {
      if (!MRI.isReserved(Candidate)) {
        Scratch = Candidate;
        break;
      }
    }
  }
  if (!Scratch)
    report_fatal_error("SH long branch has no usable scratch register");
  if (RS) {
    if (!UseRelativeBranch)
      RS->setRegUsed(SH::R0);
    RS->setRegUsed(Scratch);
  }

  SmallVector<Register, 2> SavedRegs;
  if (SaveR0)
    SavedRegs.push_back(SH::R0);
  if (SaveScratch)
    SavedRegs.push_back(Scratch);
  MachineBasicBlock *JumpTarget = SavedRegs.empty() ? &NewDestBB : &RestoreBB;
  bool TrackSPForCFA =
      MF.needsFrameMoves() &&
      !MF.getSubtarget<SHSubtarget>().getFrameLowering()->hasFP(MF);
  CFIInstBuilder BranchCFI(MBB, MBB.end(), MachineInstr::NoFlags);
  if (!SavedRegs.empty() && TrackSPForCFA)
    BranchCFI.insertCFIInst(MCCFIInstruction::createRememberState(nullptr));
  int64_t StackOffset = 0;
  for (Register SavedReg : SavedRegs) {
    BuildMI(MBB, MBB.end(), DL, get(SH::ADDri), SH::R15)
        .addReg(SH::R15)
        .addImm(-4);
    StackOffset -= 4;
    if (TrackSPForCFA)
      BranchCFI.buildDefCFAOffset(MF.getFrameInfo().getStackSize() -
                                  StackOffset);
    MachineMemOperand *MMO =
        MF.getMachineMemOperand(MachinePointerInfo::getStack(MF, StackOffset),
                                MachineMemOperand::MOStore, 4, Align(4));
    BuildMI(MBB, MBB.end(), DL, get(SH::MOVL_store_reg))
        .addReg(SavedReg, RegState::Kill)
        .addReg(SH::R15)
        .addMemOperand(MMO);
  }
  BuildMI(MBB, MBB.end(), DL,
          get(UseRelativeBranch ? SH::SH_REL16_BRANCH : SH::SH_LONG_BRANCH),
          Scratch)
      .addMBB(JumpTarget)
      .addImm(!SavedRegs.empty() && TrackSPForCFA)
      ->getOperand(0)
      .setIsDead();

  if (SavedRegs.empty())
    return;

  CFIInstBuilder RestoreCFI(RestoreBB, RestoreBB.end(), MachineInstr::NoFlags);
  if (TrackSPForCFA)
    RestoreCFI.buildDefCFAOffset(MF.getFrameInfo().getStackSize() -
                                 StackOffset);
  for (Register SavedReg : reverse(SavedRegs)) {
    MachineMemOperand *MMO =
        MF.getMachineMemOperand(MachinePointerInfo::getStack(MF, StackOffset),
                                MachineMemOperand::MOLoad, 4, Align(4));
    BuildMI(RestoreBB, RestoreBB.end(), DL, get(SH::MOVL_load_reg), SavedReg)
        .addReg(SH::R15)
        .addMemOperand(MMO);
    BuildMI(RestoreBB, RestoreBB.end(), DL, get(SH::ADDri), SH::R15)
        .addReg(SH::R15)
        .addImm(4);
    StackOffset += 4;
    if (TrackSPForCFA)
      RestoreCFI.buildDefCFAOffset(MF.getFrameInfo().getStackSize() -
                                   StackOffset);
  }
}
