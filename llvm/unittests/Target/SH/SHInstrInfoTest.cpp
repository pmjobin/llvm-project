//===- SHInstrInfoTest.cpp - SH instruction information tests ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHInstrInfo.h"
#include "SHISelLowering.h"
#include "SHSubtarget.h"
#include "SHTargetMachine.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineInstrBundle.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetSelect.h"

#include "gtest/gtest.h"
#include <limits>
#include <random>

using namespace llvm;

namespace {

class SHInstrInfoTest : public testing::Test {
protected:
  std::unique_ptr<TargetMachine> TM;
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<Module> M;
  std::unique_ptr<MachineModuleInfo> MMI;
  MachineFunction *MF = nullptr;
  const SHInstrInfo *TII = nullptr;

  static void SetUpTestSuite() {
    LLVMInitializeSHTargetInfo();
    LLVMInitializeSHTarget();
    LLVMInitializeSHTargetMC();
  }

  SHInstrInfoTest() {
    Triple TT("sh-unknown-elf");
    std::string Error;
    const Target *Target = TargetRegistry::lookupTarget(TT, Error);
    if (!Target)
      report_fatal_error(StringRef(Error));

    TM.reset(Target->createTargetMachine(TT, "sh2", "", TargetOptions(),
                                         std::nullopt, std::nullopt,
                                         CodeGenOptLevel::Default));
    Context = std::make_unique<LLVMContext>();
    M = std::make_unique<Module>("SHInstrInfoTest", *Context);
    M->setDataLayout(TM->createDataLayout());
    auto *FType = FunctionType::get(Type::getVoidTy(*Context), false);
    auto *F = Function::Create(FType, GlobalValue::ExternalLinkage, "test", *M);
    MMI = std::make_unique<MachineModuleInfo>(TM.get());
    MF = &MMI->getOrCreateMachineFunction(*F);
    TII = MF->getSubtarget<SHSubtarget>().getInstrInfo();
  }

  MachineBasicBlock *createBlock() {
    MachineBasicBlock *MBB = MF->CreateMachineBasicBlock();
    MF->push_back(MBB);
    return MBB;
  }

  SmallVector<MachineOperand, 1> trueCondition() {
    return {MachineOperand::CreateImm(SHCC::TSet)};
  }
};

TEST_F(SHInstrInfoTest, GetInstSizeInBytesAccountsForDelaySlots) {
  MachineBasicBlock *Standalone = createBlock();
  MachineBasicBlock *Target = createBlock();
  MachineInstr *Nop = BuildMI(Standalone, DebugLoc(), TII->get(SH::NOP));
  MachineInstr *Bt =
      BuildMI(Standalone, DebugLoc(), TII->get(SH::BT)).addMBB(Target);
  MachineInstr *Bf =
      BuildMI(Standalone, DebugLoc(), TII->get(SH::BF)).addMBB(Target);
  MachineInstr *Bra =
      BuildMI(Standalone, DebugLoc(), TII->get(SH::BRA)).addMBB(Target);
  MachineInstr *Rts = BuildMI(Standalone, DebugLoc(), TII->get(SH::RTS));
  MachineInstr *Bsr = BuildMI(Standalone, DebugLoc(), TII->get(SH::BSR))
                          .addExternalSymbol("callee");
  MachineInstr *Jsr =
      BuildMI(Standalone, DebugLoc(), TII->get(SH::JSR)).addReg(SH::R4);
  MachineInstr *Add =
      BuildMI(Standalone, DebugLoc(), TII->get(SH::ADDri), SH::R15)
          .addReg(SH::R15)
          .addImm(-4);
  MachineInstr *CallFrameDown =
      BuildMI(Standalone, DebugLoc(), TII->get(SH::ADJCALLSTACKDOWN))
          .addImm(4)
          .addImm(0);
  MachineInstr *CallFrameUp =
      BuildMI(Standalone, DebugLoc(), TII->get(SH::ADJCALLSTACKUP))
          .addImm(4)
          .addImm(0);

  EXPECT_EQ(2u, TII->getInstSizeInBytes(*Nop));
  EXPECT_EQ(2u, TII->getInstSizeInBytes(*Add));
  EXPECT_EQ(0u, TII->getInstSizeInBytes(*CallFrameDown));
  EXPECT_EQ(0u, TII->getInstSizeInBytes(*CallFrameUp));
  EXPECT_EQ(2u, TII->getInstSizeInBytes(*Bt));
  EXPECT_EQ(2u, TII->getInstSizeInBytes(*Bf));
  EXPECT_EQ(4u, TII->getInstSizeInBytes(*Bra));
  EXPECT_EQ(4u, TII->getInstSizeInBytes(*Rts));
  EXPECT_EQ(4u, TII->getInstSizeInBytes(*Bsr));
  EXPECT_EQ(4u, TII->getInstSizeInBytes(*Jsr));

  MachineBasicBlock *BraBundleBlock = createBlock();
  Bra = BuildMI(BraBundleBlock, DebugLoc(), TII->get(SH::BRA)).addMBB(Target);
  Nop = BuildMI(BraBundleBlock, DebugLoc(), TII->get(SH::NOP));
  MIBundleBuilder(*BraBundleBlock, Bra->getIterator(),
                  std::next(Nop->getIterator()));
  finalizeBundle(*BraBundleBlock, Bra->getIterator(),
                 std::next(Nop->getIterator()));
  MachineInstr &BraBundle = BraBundleBlock->front();
  EXPECT_EQ(TargetOpcode::BUNDLE, BraBundle.getOpcode());
  EXPECT_EQ(4u, TII->getInstSizeInBytes(BraBundle));

  MachineBasicBlock *RtsBundleBlock = createBlock();
  Rts = BuildMI(RtsBundleBlock, DebugLoc(), TII->get(SH::RTS));
  Nop = BuildMI(RtsBundleBlock, DebugLoc(), TII->get(SH::NOP));
  MIBundleBuilder(*RtsBundleBlock, Rts->getIterator(),
                  std::next(Nop->getIterator()));
  finalizeBundle(*RtsBundleBlock, Rts->getIterator(),
                 std::next(Nop->getIterator()));
  MachineInstr &RtsBundle = RtsBundleBlock->front();
  EXPECT_EQ(TargetOpcode::BUNDLE, RtsBundle.getOpcode());
  EXPECT_EQ(4u, TII->getInstSizeInBytes(RtsBundle));

  MachineBasicBlock *BsrBundleBlock = createBlock();
  Bsr = BuildMI(BsrBundleBlock, DebugLoc(), TII->get(SH::BSR))
            .addExternalSymbol("callee");
  Nop = BuildMI(BsrBundleBlock, DebugLoc(), TII->get(SH::NOP));
  MIBundleBuilder(*BsrBundleBlock, Bsr->getIterator(),
                  std::next(Nop->getIterator()));
  finalizeBundle(*BsrBundleBlock, Bsr->getIterator(),
                 std::next(Nop->getIterator()));
  EXPECT_EQ(4u, TII->getInstSizeInBytes(BsrBundleBlock->front()));

  MachineBasicBlock *JsrBundleBlock = createBlock();
  Jsr = BuildMI(JsrBundleBlock, DebugLoc(), TII->get(SH::JSR)).addReg(SH::R4);
  Nop = BuildMI(JsrBundleBlock, DebugLoc(), TII->get(SH::NOP));
  MIBundleBuilder(*JsrBundleBlock, Jsr->getIterator(),
                  std::next(Nop->getIterator()));
  finalizeBundle(*JsrBundleBlock, Jsr->getIterator(),
                 std::next(Nop->getIterator()));
  EXPECT_EQ(4u, TII->getInstSizeInBytes(JsrBundleBlock->front()));
}

TEST_F(SHInstrInfoTest, CCallRegisterMaskMatchesABI) {
  const SHRegisterInfo &TRI = TII->getRegisterInfo();
  const uint32_t *Mask = TRI.getCallPreservedMask(*MF, CallingConv::C);

  for (MCRegister Reg :
       {SH::R0, SH::R1, SH::R2, SH::R3, SH::R4, SH::R5, SH::R6, SH::R7, SH::PC,
        SH::PR, SH::TBit, SH::MBit, SH::QBit, SH::MACH, SH::MACL})
    EXPECT_TRUE(MachineOperand::clobbersPhysReg(Mask, Reg));
  for (MCRegister Reg : {SH::R8, SH::R9, SH::R10, SH::R11, SH::R12, SH::R13,
                         SH::R14, SH::R15, SH::GBR, SH::VBR, SH::SR})
    EXPECT_FALSE(MachineOperand::clobbersPhysReg(Mask, Reg));
}

TEST_F(SHInstrInfoTest, ByteAndWordInstructionsHavePreciseProperties) {
  for (unsigned Opcode : {SH::MOVB_load_reg, SH::MOVW_load_reg,
                          SH::MOVB_load_disp, SH::MOVW_load_disp}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_EQ(2u, Desc.getSize());
    EXPECT_TRUE(Desc.mayLoad());
    EXPECT_FALSE(Desc.mayStore());
  }

  for (unsigned Opcode : {SH::MOVB_store_reg, SH::MOVW_store_reg,
                          SH::MOVB_store_disp, SH::MOVW_store_disp}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_EQ(2u, Desc.getSize());
    EXPECT_FALSE(Desc.mayLoad());
    EXPECT_TRUE(Desc.mayStore());
  }

  for (unsigned Opcode : {SH::EXTUB, SH::EXTUW, SH::EXTSB, SH::EXTSW}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_EQ(2u, Desc.getSize());
    EXPECT_FALSE(Desc.mayLoad());
    EXPECT_FALSE(Desc.mayStore());
  }
}

static uint32_t executeConstantShiftPlan(ArrayRef<unsigned> Opcodes,
                                         uint32_t Value) {
  for (unsigned Opcode : Opcodes) {
    switch (Opcode) {
    default:
      llvm_unreachable("unexpected constant shift plan opcode");
    case SH::SHLL:
      Value <<= 1;
      break;
    case SH::SHLL2:
      Value <<= 2;
      break;
    case SH::SHLL8:
      Value <<= 8;
      break;
    case SH::SHLL16:
      Value <<= 16;
      break;
    case SH::SHLR:
      Value >>= 1;
      break;
    case SH::SHLR2:
      Value >>= 2;
      break;
    case SH::SHLR8:
      Value >>= 8;
      break;
    case SH::SHLR16:
      Value >>= 16;
      break;
    case SH::SHAR:
      Value = (Value >> 1) | (Value & 0x80000000);
      break;
    case SH::EXTSB:
      Value = static_cast<uint32_t>(
          static_cast<int32_t>(static_cast<int8_t>(Value)));
      break;
    case SH::EXTSW:
      Value = static_cast<uint32_t>(
          static_cast<int32_t>(static_cast<int16_t>(Value)));
      break;
    }
  }
  return Value;
}

TEST_F(SHInstrInfoTest, ConstantShiftPlansCoverEveryDefinedAmount) {
  constexpr uint32_t Values[] = {0,          1,          0x7fffffff,
                                 0x80000000, 0xdeadbeef, 0xffffffff};
  for (unsigned Amount = 0; Amount != 32; ++Amount) {
    for (unsigned Pseudo : {SH::SHLri, SH::SRLri, SH::SRAri}) {
      SmallVector<unsigned, 16> Plan = SH::planConstantShift(Pseudo, Amount);
      EXPECT_EQ(Amount == 0, Plan.empty());
      for (uint32_t Value : Values) {
        uint32_t Expected;
        if (Pseudo == SH::SHLri)
          Expected = Value << Amount;
        else if (Pseudo == SH::SRLri)
          Expected = Value >> Amount;
        else {
          Expected = Value;
          for (unsigned I = 0; I != Amount; ++I)
            Expected = (Expected >> 1) | (Expected & 0x80000000);
        }
        EXPECT_EQ(Expected, executeConstantShiftPlan(Plan, Value))
            << "amount " << Amount << ", pseudo " << Pseudo;
      }
    }
  }

  EXPECT_EQ((SmallVector<unsigned, 2>{SH::SHLR16, SH::EXTSW}),
            SH::planConstantShift(SH::SRAri, 16));
  EXPECT_EQ((SmallVector<unsigned, 3>{SH::SHLR16, SH::SHLR8, SH::EXTSB}),
            SH::planConstantShift(SH::SRAri, 24));
}

TEST_F(SHInstrInfoTest, IntegerALUAndShiftPropertiesArePrecise) {
  constexpr unsigned RealOpcodes[] = {
      SH::SUBrr, SH::NEG,   SH::ANDrr, SH::ORrr,   SH::XORrr, SH::NOT,
      SH::TST,   SH::DT,    SH::SHLL,  SH::SHLR,   SH::SHAR,  SH::SHLL2,
      SH::SHLR2, SH::SHLL8, SH::SHLR8, SH::SHLL16, SH::SHLR16};
  for (unsigned Opcode : RealOpcodes) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_EQ(2u, Desc.getSize());
    EXPECT_FALSE(Desc.mayLoad());
    EXPECT_FALSE(Desc.mayStore());
  }

  for (unsigned Opcode : {SH::TST, SH::DT, SH::SHLL, SH::SHLR, SH::SHAR})
    EXPECT_TRUE(TII->get(Opcode).hasImplicitDefOfPhysReg(SH::TBit));
  for (unsigned Opcode :
       {SH::SUBrr, SH::NEG, SH::ANDrr, SH::ORrr, SH::XORrr, SH::NOT, SH::SHLL2,
        SH::SHLR2, SH::SHLL8, SH::SHLR8, SH::SHLL16, SH::SHLR16})
    EXPECT_FALSE(TII->get(Opcode).hasImplicitDefOfPhysReg(SH::TBit));

  EXPECT_TRUE(TII->get(SH::TST).isCompare());
  for (unsigned Opcode : {SH::SUBrr, SH::ANDrr, SH::ORrr, SH::XORrr, SH::DT,
                          SH::SHLL, SH::SHLR, SH::SHAR, SH::SHLL2, SH::SHLR2,
                          SH::SHLL8, SH::SHLR8, SH::SHLL16, SH::SHLR16})
    EXPECT_EQ(0, TII->get(Opcode).getOperandConstraint(1, MCOI::TIED_TO));
  for (unsigned Opcode : {SH::NEG, SH::NOT}) {
    EXPECT_EQ(-1, TII->get(Opcode).getOperandConstraint(0, MCOI::TIED_TO));
    EXPECT_EQ(-1, TII->get(Opcode).getOperandConstraint(1, MCOI::TIED_TO));
  }

  EXPECT_FALSE(TII->get(SH::SUBrr).isCommutable());
  for (unsigned Opcode : {SH::ANDrr, SH::ORrr, SH::XORrr})
    EXPECT_TRUE(TII->get(Opcode).isCommutable());
}

TEST_F(SHInstrInfoTest, MultiplyAndDivideInstructionsHavePreciseProperties) {
  constexpr unsigned RealOpcodes[] = {SH::MUL_L, SH::STS_MACL, SH::DIV0U,
                                      SH::DIV0S, SH::DIV1,     SH::ROTCL,
                                      SH::ADDC,  SH::SUBC};
  for (unsigned Opcode : RealOpcodes) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_EQ(2u, Desc.getSize());
    EXPECT_FALSE(Desc.mayLoad());
    EXPECT_FALSE(Desc.mayStore());
    EXPECT_FALSE(Desc.hasUnmodeledSideEffects());
  }

  const MCInstrDesc &Multiply = TII->get(SH::MUL_L);
  EXPECT_TRUE(Multiply.isCommutable());
  EXPECT_TRUE(Multiply.hasImplicitDefOfPhysReg(SH::MACL));
  EXPECT_FALSE(Multiply.hasImplicitDefOfPhysReg(SH::MACH));
  EXPECT_FALSE(Multiply.hasImplicitDefOfPhysReg(SH::TBit));

  const MCInstrDesc &Extract = TII->get(SH::STS_MACL);
  EXPECT_EQ(1u, Extract.getNumDefs());
  EXPECT_TRUE(is_contained(Extract.implicit_uses(), SH::MACL));
  EXPECT_FALSE(is_contained(Extract.implicit_uses(), SH::MACH));
  EXPECT_FALSE(is_contained(Extract.implicit_uses(), SH::TBit));

  for (MCRegister Reg : {SH::MBit, SH::QBit, SH::TBit}) {
    EXPECT_TRUE(TII->get(SH::DIV0U).hasImplicitDefOfPhysReg(Reg));
    EXPECT_TRUE(TII->get(SH::DIV0S).hasImplicitDefOfPhysReg(Reg));
  }

  const MCInstrDesc &Div1 = TII->get(SH::DIV1);
  EXPECT_TRUE(is_contained(Div1.implicit_uses(), SH::MBit));
  EXPECT_FALSE(Div1.hasImplicitDefOfPhysReg(SH::MBit));
  for (MCRegister Reg : {SH::QBit, SH::TBit}) {
    EXPECT_TRUE(is_contained(Div1.implicit_uses(), Reg));
    EXPECT_TRUE(Div1.hasImplicitDefOfPhysReg(Reg));
  }

  for (unsigned Opcode : {SH::ROTCL, SH::ADDC, SH::SUBC}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_TRUE(is_contained(Desc.implicit_uses(), SH::TBit));
    EXPECT_TRUE(Desc.hasImplicitDefOfPhysReg(SH::TBit));
    EXPECT_FALSE(Desc.isCommutable());
    EXPECT_EQ(0, Desc.getOperandConstraint(1, MCOI::TIED_TO));
  }
  EXPECT_EQ(0, Div1.getOperandConstraint(1, MCOI::TIED_TO));
  EXPECT_FALSE(Div1.isCommutable());

  for (unsigned Opcode :
       {SH::MUL32_PSEUDO, SH::UDIV32_PSEUDO, SH::UREM32_PSEUDO,
        SH::UDIVREM32_PSEUDO, SH::SDIV32_PSEUDO, SH::SREM32_PSEUDO,
        SH::SDIVREM32_PSEUDO})
    EXPECT_EQ(0u, TII->get(Opcode).getSize());
}

TEST_F(SHInstrInfoTest, DivisionStateRegistersAreReservedAndUnallocatable) {
  const SHRegisterInfo &TRI = TII->getRegisterInfo();
  BitVector Reserved = TRI.getReservedRegs(*MF);
  for (MCRegister Reg : {SH::MBit, SH::QBit, SH::TBit}) {
    EXPECT_TRUE(Reserved.test(Reg));
    EXPECT_FALSE(SH::GPRRegClass.contains(Reg));
  }
}

namespace {

struct SHDivisionState {
  uint32_t Partial = 0;
  uint32_t Quotient = 0;
  uint32_t Divisor = 0;
  bool M = false;
  bool Q = false;
  bool T = false;

  void rotcl(uint32_t &Value) {
    bool OldT = T;
    T = (Value >> 31) != 0;
    Value = (Value << 1) | static_cast<uint32_t>(OldT);
  }

  void div1() {
    bool OldQ = Q;
    bool ShiftedMSB = (Partial >> 31) != 0;
    Partial = (Partial << 1) | static_cast<uint32_t>(T);

    bool CarryOrBorrow;
    if (OldQ == M) {
      CarryOrBorrow = Partial < Divisor;
      Partial -= Divisor;
    } else {
      uint32_t Before = Partial;
      Partial += Divisor;
      CarryOrBorrow = Partial < Before;
    }

    Q = ShiftedMSB ^ CarryOrBorrow ^ M;
    T = Q == M;
  }
};

static bool applySubc(uint32_t Source, uint32_t &Dest, bool CarryIn) {
  uint64_t Subtrahend =
      static_cast<uint64_t>(Source) + static_cast<uint64_t>(CarryIn);
  bool Borrow = static_cast<uint64_t>(Dest) < Subtrahend;
  Dest = static_cast<uint32_t>(static_cast<uint64_t>(Dest) - Subtrahend);
  return Borrow;
}

static bool applyAddc(uint32_t Source, uint32_t &Dest, bool CarryIn) {
  uint64_t Sum = static_cast<uint64_t>(Dest) + Source + CarryIn;
  Dest = static_cast<uint32_t>(Sum);
  return (Sum >> 32) != 0;
}

static uint32_t simulateUnsignedQuotient(uint32_t Dividend, uint32_t Divisor) {
  SHDivisionState State;
  State.Quotient = Dividend;
  State.Divisor = Divisor;
  for (unsigned I = 0; I != 32; ++I) {
    State.rotcl(State.Quotient);
    State.div1();
  }
  State.rotcl(State.Quotient);
  return State.Quotient;
}

static uint32_t simulateSignedQuotient(uint32_t Dividend, uint32_t Divisor) {
  SHDivisionState State;
  State.Quotient = Dividend;
  State.Divisor = Divisor;

  uint32_t SignProbe = Dividend;
  State.T = (SignProbe >> 31) != 0;
  SignProbe <<= 1;

  State.Partial = Dividend;
  State.T = applySubc(State.Partial, State.Partial, State.T);
  uint32_t Zero = SignProbe ^ SignProbe;
  State.T = applySubc(Zero, State.Quotient, State.T);

  State.Q = (State.Partial >> 31) != 0;
  State.M = (State.Divisor >> 31) != 0;
  State.T = State.M != State.Q;
  for (unsigned I = 0; I != 32; ++I) {
    State.rotcl(State.Quotient);
    State.div1();
  }
  State.rotcl(State.Quotient);
  State.T = applyAddc(Zero, State.Quotient, State.T);
  return State.Quotient;
}

static void verifyUnsignedDivision(uint32_t Dividend, uint32_t Divisor) {
  ASSERT_NE(0u, Divisor);
  uint32_t Quotient = simulateUnsignedQuotient(Dividend, Divisor);
  uint32_t Remainder = Dividend - Divisor * Quotient;
  EXPECT_EQ(Dividend / Divisor, Quotient);
  EXPECT_EQ(Dividend % Divisor, Remainder);
  EXPECT_EQ(static_cast<uint64_t>(Dividend),
            static_cast<uint64_t>(Quotient) * Divisor + Remainder);
  EXPECT_LT(Remainder, Divisor);
}

static void verifySignedDivision(int32_t Dividend, int32_t Divisor) {
  ASSERT_NE(0, Divisor);
  ASSERT_FALSE(Dividend == std::numeric_limits<int32_t>::min() &&
               Divisor == -1);
  int64_t WideDividend = Dividend;
  int64_t WideDivisor = Divisor;
  int64_t ExpectedQuotient = WideDividend / WideDivisor;
  int64_t ExpectedRemainder = WideDividend % WideDivisor;
  int32_t Quotient = static_cast<int32_t>(simulateSignedQuotient(
      static_cast<uint32_t>(Dividend), static_cast<uint32_t>(Divisor)));
  int32_t Remainder = static_cast<int32_t>(static_cast<uint32_t>(Dividend) -
                                           static_cast<uint32_t>(Divisor) *
                                               static_cast<uint32_t>(Quotient));

  EXPECT_EQ(ExpectedQuotient, Quotient);
  EXPECT_EQ(ExpectedRemainder, Remainder);
  EXPECT_EQ(WideDividend,
            static_cast<int64_t>(Quotient) * WideDivisor + Remainder);
  EXPECT_LT(std::abs(static_cast<int64_t>(Remainder)), std::abs(WideDivisor));
  EXPECT_TRUE(Remainder == 0 || (Remainder < 0) == (Dividend < 0));
}

} // namespace

TEST_F(SHInstrInfoTest, DivisionSequencesMatchArchitecturalSemantics) {
  constexpr std::pair<uint32_t, uint32_t> UnsignedCases[] = {
      {0, 1},
      {1, 1},
      {1, 2},
      {2, 1},
      {0xffffffff, 1},
      {0xffffffff, 0xffffffff},
      {0xffffffff, 2},
      {0x80000000, 3},
      {0x80000000, 0x7fffffff},
      {0x7fffffff, 0x80000000},
      {7, 31},
      {0x87654321, 0xf0000001}};
  for (auto [Dividend, Divisor] : UnsignedCases)
    verifyUnsignedDivision(Dividend, Divisor);

  constexpr std::pair<int32_t, int32_t> SignedCases[] = {
      {0, 1},
      {1, 1},
      {-1, 1},
      {1, -1},
      {-1, -1},
      {std::numeric_limits<int32_t>::min(), 1},
      {std::numeric_limits<int32_t>::min(), 2},
      {std::numeric_limits<int32_t>::min(),
       std::numeric_limits<int32_t>::min()},
      {std::numeric_limits<int32_t>::max(), -1},
      {-1, std::numeric_limits<int32_t>::min()},
      {-7, 3},
      {7, -3},
      {-7, -3}};
  for (auto [Dividend, Divisor] : SignedCases)
    verifySignedDivision(Dividend, Divisor);

  std::mt19937 Generator(0x53484347);
  for (unsigned I = 0; I != 10000; ++I) {
    uint32_t Dividend = Generator();
    uint32_t Divisor = Generator();
    if (Divisor == 0)
      Divisor = 1;
    verifyUnsignedDivision(Dividend, Divisor);
  }
  for (unsigned I = 0; I != 10000; ++I) {
    int32_t Dividend = static_cast<int32_t>(Generator());
    int32_t Divisor = static_cast<int32_t>(Generator());
    if (Divisor == 0)
      Divisor = 1;
    if (Dividend == std::numeric_limits<int32_t>::min() && Divisor == -1)
      Divisor = 1;
    verifySignedDivision(Dividend, Divisor);
  }
}

TEST_F(SHInstrInfoTest, InsertBranchReportsFinalEmittedSize) {
  MachineBasicBlock *Unconditional = createBlock();
  MachineBasicBlock *UnconditionalTarget = createBlock();
  int BytesAdded = -1;
  EXPECT_EQ(1u, TII->insertBranch(*Unconditional, UnconditionalTarget, nullptr,
                                  {}, DebugLoc(), &BytesAdded));
  EXPECT_EQ(4, BytesAdded);

  MachineBasicBlock *Conditional = createBlock();
  MachineBasicBlock *ConditionalTarget = createBlock();
  SmallVector<MachineOperand, 1> Condition = trueCondition();
  EXPECT_EQ(1u, TII->insertBranch(*Conditional, ConditionalTarget, nullptr,
                                  Condition, DebugLoc(), &BytesAdded));
  EXPECT_EQ(2, BytesAdded);

  MachineBasicBlock *ConditionalAndUnconditional = createBlock();
  MachineBasicBlock *TrueTarget = createBlock();
  MachineBasicBlock *FalseTarget = createBlock();
  EXPECT_EQ(2u,
            TII->insertBranch(*ConditionalAndUnconditional, TrueTarget,
                              FalseTarget, Condition, DebugLoc(), &BytesAdded));
  EXPECT_EQ(6, BytesAdded);
}

TEST_F(SHInstrInfoTest, RemoveBranchReportsFinalEmittedSize) {
  MachineBasicBlock *StandaloneBra = createBlock();
  MachineBasicBlock *Target = createBlock();
  BuildMI(StandaloneBra, DebugLoc(), TII->get(SH::BRA)).addMBB(Target);
  int BytesRemoved = -1;
  EXPECT_EQ(1u, TII->removeBranch(*StandaloneBra, &BytesRemoved));
  EXPECT_EQ(4, BytesRemoved);

  MachineBasicBlock *Conditional = createBlock();
  BuildMI(Conditional, DebugLoc(), TII->get(SH::BT)).addMBB(Target);
  EXPECT_EQ(1u, TII->removeBranch(*Conditional, &BytesRemoved));
  EXPECT_EQ(2, BytesRemoved);

  MachineBasicBlock *ConditionalAndBra = createBlock();
  BuildMI(ConditionalAndBra, DebugLoc(), TII->get(SH::BF)).addMBB(Target);
  BuildMI(ConditionalAndBra, DebugLoc(), TII->get(SH::BRA)).addMBB(Target);
  EXPECT_EQ(2u, TII->removeBranch(*ConditionalAndBra, &BytesRemoved));
  EXPECT_EQ(6, BytesRemoved);

  MachineBasicBlock *BundledBra = createBlock();
  MachineInstr *Bra =
      BuildMI(BundledBra, DebugLoc(), TII->get(SH::BRA)).addMBB(Target);
  MachineInstr *Nop = BuildMI(BundledBra, DebugLoc(), TII->get(SH::NOP));
  MIBundleBuilder(*BundledBra, Bra->getIterator(),
                  std::next(Nop->getIterator()));
  EXPECT_EQ(1u, TII->removeBranch(*BundledBra, &BytesRemoved));
  EXPECT_EQ(4, BytesRemoved);
  EXPECT_TRUE(BundledBra->empty());

  MachineBasicBlock *ConditionalAndBundledBra = createBlock();
  BuildMI(ConditionalAndBundledBra, DebugLoc(), TII->get(SH::BT))
      .addMBB(Target);
  Bra = BuildMI(ConditionalAndBundledBra, DebugLoc(), TII->get(SH::BRA))
            .addMBB(Target);
  Nop = BuildMI(ConditionalAndBundledBra, DebugLoc(), TII->get(SH::NOP));
  MIBundleBuilder(*ConditionalAndBundledBra, Bra->getIterator(),
                  std::next(Nop->getIterator()));
  EXPECT_EQ(2u, TII->removeBranch(*ConditionalAndBundledBra, &BytesRemoved));
  EXPECT_EQ(6, BytesRemoved);
  EXPECT_TRUE(ConditionalAndBundledBra->empty());
}

} // namespace
