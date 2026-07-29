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
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCInstrAnalysis.h"
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
  MachineInstr *Jmp =
      BuildMI(Standalone, DebugLoc(), TII->get(SH::JMP)).addReg(SH::R4);
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
  EXPECT_EQ(4u, TII->getInstSizeInBytes(*Jmp));

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

  MachineBasicBlock *JmpBundleBlock = createBlock();
  Jmp = BuildMI(JmpBundleBlock, DebugLoc(), TII->get(SH::JMP)).addReg(SH::R4);
  Nop = BuildMI(JmpBundleBlock, DebugLoc(), TII->get(SH::NOP));
  MIBundleBuilder(*JmpBundleBlock, Jmp->getIterator(),
                  std::next(Nop->getIterator()));
  finalizeBundle(*JmpBundleBlock, Jmp->getIterator(),
                 std::next(Nop->getIterator()));
  EXPECT_EQ(4u, TII->getInstSizeInBytes(JmpBundleBlock->front()));
}

TEST_F(SHInstrInfoTest, IndirectJumpHasPreciseProperties) {
  const MCInstrDesc &Desc = TII->get(SH::JMP);
  EXPECT_EQ(2u, Desc.getSize());
  EXPECT_EQ(0u, Desc.getNumDefs());
  EXPECT_EQ(1u, Desc.getNumOperands());
  EXPECT_TRUE(Desc.isVariadic());
  EXPECT_TRUE(Desc.isBranch());
  EXPECT_TRUE(Desc.isIndirectBranch());
  EXPECT_TRUE(Desc.isTerminator());
  EXPECT_TRUE(Desc.isBarrier());
  EXPECT_TRUE(Desc.hasDelaySlot());
  EXPECT_FALSE(Desc.isCall());
  EXPECT_FALSE(Desc.isReturn());
  EXPECT_FALSE(Desc.mayLoad());
  EXPECT_FALSE(Desc.mayStore());
  EXPECT_FALSE(Desc.hasUnmodeledSideEffects());
  EXPECT_TRUE(Desc.implicit_uses().empty());
  EXPECT_TRUE(Desc.implicit_defs().empty());
}

TEST_F(SHInstrInfoTest, IndirectJumpIsNotAnalyzedOrRemovedAsDirectBranch) {
  MachineBasicBlock *Dispatch = createBlock();
  MachineBasicBlock *FirstTarget = createBlock();
  MachineBasicBlock *SecondTarget = createBlock();
  Dispatch->addSuccessor(FirstTarget);
  Dispatch->addSuccessor(SecondTarget);
  MachineInstr *Jmp =
      BuildMI(Dispatch, DebugLoc(), TII->get(SH::JMP)).addReg(SH::R4);

  MachineBasicBlock *TBB = nullptr;
  MachineBasicBlock *FBB = nullptr;
  SmallVector<MachineOperand, 4> Condition;
  EXPECT_TRUE(TII->analyzeBranch(*Dispatch, TBB, FBB, Condition));
  EXPECT_EQ(nullptr, TBB);
  EXPECT_EQ(nullptr, FBB);
  EXPECT_TRUE(Condition.empty());
  EXPECT_EQ(nullptr, TII->getBranchDestBlock(*Jmp));
  EXPECT_EQ(0u, TII->removeBranch(*Dispatch));
  EXPECT_EQ(SH::JMP, Dispatch->back().getOpcode());
  EXPECT_EQ(2u, Dispatch->succ_size());
}

TEST_F(SHInstrInfoTest, MCAnalysisClassifiesIndirectControlTransfer) {
  Triple TT("sh-unknown-elf");
  std::string Error;
  const Target *Target = TargetRegistry::lookupTarget(TT, Error);
  ASSERT_NE(nullptr, Target) << Error;
  std::unique_ptr<const MCInstrInfo> Info(Target->createMCInstrInfo());
  std::unique_ptr<const MCInstrAnalysis> Analysis(
      Target->createMCInstrAnalysis(Info.get()));
  ASSERT_NE(nullptr, Info);
  ASSERT_NE(nullptr, Analysis);

  MCInst Jmp = MCInstBuilder(SH::JMP).addReg(SH::R4);
  MCInst Jsr = MCInstBuilder(SH::JSR).addReg(SH::R4);
  MCInst Rts = MCInstBuilder(SH::RTS);
  MCInst Bra = MCInstBuilder(SH::BRA).addImm(4);
  MCInst Bt = MCInstBuilder(SH::BT).addImm(4);
  MCInst Bf = MCInstBuilder(SH::BF).addImm(4);

  EXPECT_TRUE(Analysis->isBranch(Jmp));
  EXPECT_TRUE(Analysis->isIndirectBranch(Jmp));
  EXPECT_TRUE(Analysis->isTerminator(Jmp));
  EXPECT_TRUE(Analysis->isBarrier(Jmp));
  EXPECT_FALSE(Analysis->isCall(Jmp));
  EXPECT_FALSE(Analysis->isReturn(Jmp));
  uint64_t Destination = 0;
  EXPECT_FALSE(Analysis->evaluateBranch(Jmp, 0x100, 2, Destination));

  EXPECT_TRUE(Analysis->isCall(Jsr));
  EXPECT_FALSE(Analysis->isIndirectBranch(Jsr));
  EXPECT_TRUE(Analysis->isReturn(Rts));
  EXPECT_FALSE(Analysis->isCall(Rts));
  EXPECT_TRUE(Analysis->isUnconditionalBranch(Bra));
  EXPECT_TRUE(Analysis->isConditionalBranch(Bt));
  EXPECT_TRUE(Analysis->isConditionalBranch(Bf));
}

namespace {

struct SwitchTableModel {
  int32_t Minimum;
  unsigned DefaultDestination;
  SmallVector<unsigned, 32> Entries;
};

static SwitchTableModel
buildSwitchTableModel(ArrayRef<std::pair<int32_t, unsigned>> Cases,
                      unsigned DefaultDestination) {
  assert(!Cases.empty());
  int32_t Minimum = Cases.front().first;
  int32_t Maximum = Cases.front().first;
  for (auto [Value, Destination] : Cases) {
    (void)Destination;
    Minimum = std::min(Minimum, Value);
    Maximum = std::max(Maximum, Value);
  }
  uint64_t Range = static_cast<uint64_t>(static_cast<int64_t>(Maximum) -
                                         static_cast<int64_t>(Minimum));
  assert(Range < 128 && "test model only builds compact switch tables");

  SwitchTableModel Model{
      Minimum, DefaultDestination,
      SmallVector<unsigned, 32>(Range + 1, DefaultDestination)};
  for (auto [Value, Destination] : Cases) {
    uint32_t Index =
        static_cast<uint32_t>(Value) - static_cast<uint32_t>(Minimum);
    Model.Entries[Index] = Destination;
  }
  return Model;
}

static unsigned evaluateSwitchTable(const SwitchTableModel &Model,
                                    int32_t Value) {
  uint32_t Index =
      static_cast<uint32_t>(Value) - static_cast<uint32_t>(Model.Minimum);
  if (Index >= Model.Entries.size())
    return Model.DefaultDestination;
  return Model.Entries[Index];
}

static unsigned
evaluateSwitchCases(ArrayRef<std::pair<int32_t, unsigned>> Cases,
                    unsigned DefaultDestination, int32_t Value) {
  for (auto [CaseValue, Destination] : Cases)
    if (Value == CaseValue)
      return Destination;
  return DefaultDestination;
}

static void verifySwitchTableModel(ArrayRef<std::pair<int32_t, unsigned>> Cases,
                                   unsigned DefaultDestination,
                                   ArrayRef<int32_t> Values) {
  SwitchTableModel Model = buildSwitchTableModel(Cases, DefaultDestination);
  for (int32_t Value : Values)
    EXPECT_EQ(evaluateSwitchCases(Cases, DefaultDestination, Value),
              evaluateSwitchTable(Model, Value))
        << "switch value " << Value << ", minimum " << Model.Minimum;
}

} // namespace

TEST_F(SHInstrInfoTest, SwitchTableModelMatchesSwitchSemantics) {
  constexpr std::pair<int32_t, unsigned> NegativeCases[] = {
      {-4, 1}, {-3, 2}, {-1, 2}, {2, 3}};
  constexpr int32_t NegativeValues[] = {-6, -5, -4, -3, -2, -1, 0, 1, 2, 3};
  verifySwitchTableModel(NegativeCases, 9, NegativeValues);

  constexpr std::pair<int32_t, unsigned> LowBoundaryCases[] = {
      {std::numeric_limits<int32_t>::min(), 1},
      {std::numeric_limits<int32_t>::min() + 2, 2},
      {std::numeric_limits<int32_t>::min() + 5, 1}};
  constexpr int32_t LowBoundaryValues[] = {
      std::numeric_limits<int32_t>::min(),
      std::numeric_limits<int32_t>::min() + 1,
      std::numeric_limits<int32_t>::min() + 5,
      std::numeric_limits<int32_t>::min() + 6,
      std::numeric_limits<int32_t>::max()};
  verifySwitchTableModel(LowBoundaryCases, 7, LowBoundaryValues);

  constexpr std::pair<int32_t, unsigned> HighBoundaryCases[] = {
      {std::numeric_limits<int32_t>::max() - 5, 1},
      {std::numeric_limits<int32_t>::max() - 2, 2},
      {std::numeric_limits<int32_t>::max(), 3}};
  constexpr int32_t HighBoundaryValues[] = {
      std::numeric_limits<int32_t>::min(),
      std::numeric_limits<int32_t>::max() - 6,
      std::numeric_limits<int32_t>::max() - 5,
      std::numeric_limits<int32_t>::max() - 4,
      std::numeric_limits<int32_t>::max()};
  verifySwitchTableModel(HighBoundaryCases, 7, HighBoundaryValues);

  constexpr std::pair<int32_t, unsigned> PromotedI8Cases[] = {
      {static_cast<int8_t>(-128), 1},
      {static_cast<int8_t>(-126), 2},
      {static_cast<int8_t>(-123), 1}};
  constexpr int32_t PromotedI8Values[] = {
      static_cast<int8_t>(-128), static_cast<int8_t>(-127),
      static_cast<int8_t>(-123), static_cast<int8_t>(127)};
  verifySwitchTableModel(PromotedI8Cases, 8, PromotedI8Values);

  constexpr std::pair<int32_t, unsigned> PromotedI16Cases[] = {
      {static_cast<int16_t>(-32768), 1},
      {static_cast<int16_t>(-32766), 2},
      {static_cast<int16_t>(-32763), 1}};
  constexpr int32_t PromotedI16Values[] = {
      static_cast<int16_t>(-32768), static_cast<int16_t>(-32767),
      static_cast<int16_t>(-32763), static_cast<int16_t>(32767)};
  verifySwitchTableModel(PromotedI16Cases, 8, PromotedI16Values);
}

TEST_F(SHInstrInfoTest, RandomizedSwitchTableModelMatchesIndependentOracle) {
  std::mt19937 Generator(0x53484a54);
  for (unsigned Iteration = 0; Iteration != 10000; ++Iteration) {
    int32_t Minimum = static_cast<int32_t>(Generator() % 2000001) - 1000000;
    unsigned Range = 5 + Generator() % 28;
    unsigned CaseCount = 4 + Generator() % std::min(12u, Range - 3);
    unsigned DefaultDestination = Generator() % 8;
    bool Used[33] = {};
    SmallVector<std::pair<int32_t, unsigned>, 16> Cases;
    while (Cases.size() != CaseCount) {
      unsigned Offset = Generator() % (Range + 1);
      if (Used[Offset])
        continue;
      Used[Offset] = true;
      Cases.emplace_back(Minimum + static_cast<int32_t>(Offset),
                         Generator() % 8);
    }

    int32_t Value;
    switch (Iteration % 4) {
    case 0:
      Value = Minimum - 1 - static_cast<int32_t>(Generator() % 16);
      break;
    case 1:
      Value = Minimum + static_cast<int32_t>(Range) + 1 +
              static_cast<int32_t>(Generator() % 16);
      break;
    default:
      Value = Minimum + static_cast<int32_t>(Generator() % (Range + 1));
      break;
    }

    SwitchTableModel Model = buildSwitchTableModel(Cases, DefaultDestination);
    EXPECT_EQ(evaluateSwitchCases(Cases, DefaultDestination, Value),
              evaluateSwitchTable(Model, Value))
        << "randomized iteration " << Iteration;
  }
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

TEST_F(SHInstrInfoTest, PCLiteralLoadHasPreciseProperties) {
  const MCInstrDesc &Desc = TII->get(SH::MOVL_load_pc);
  EXPECT_EQ(2u, Desc.getSize());
  EXPECT_EQ(1u, Desc.getNumDefs());
  EXPECT_EQ(2u, Desc.getNumOperands());
  EXPECT_TRUE(Desc.mayLoad());
  EXPECT_FALSE(Desc.mayStore());
  EXPECT_FALSE(Desc.isBranch());
  EXPECT_FALSE(Desc.isCall());
  EXPECT_FALSE(Desc.hasDelaySlot());
  EXPECT_TRUE(Desc.implicit_uses().empty());
  EXPECT_TRUE(Desc.implicit_defs().empty());
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
  constexpr unsigned RealOpcodes[] = {
      SH::CLRT, SH::MUL_L, SH::STS_MACL, SH::DIV0U, SH::DIV0S,
      SH::DIV1, SH::ROTCL, SH::ROTCR,    SH::ADDC,  SH::SUBC};
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

  const MCInstrDesc &ClearT = TII->get(SH::CLRT);
  EXPECT_EQ(0u, ClearT.getNumOperands());
  EXPECT_TRUE(ClearT.hasImplicitDefOfPhysReg(SH::TBit));
  EXPECT_TRUE(ClearT.implicit_uses().empty());

  for (unsigned Opcode : {SH::ROTCL, SH::ROTCR, SH::ADDC, SH::SUBC}) {
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
        SH::SDIVREM32_PSEUDO, SH::ADDC_LO_PSEUDO, SH::SUBC_LO_PSEUDO,
        SH::SHL64_PSEUDO, SH::SRL64_PSEUDO, SH::SRA64_PSEUDO,
        SH::BR_CC64_PSEUDO})
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

namespace {

struct PairedI64 {
  uint32_t Lo;
  uint32_t Hi;

  static PairedI64 split(uint64_t Value) {
    return {static_cast<uint32_t>(Value), static_cast<uint32_t>(Value >> 32)};
  }

  uint64_t join() const {
    return static_cast<uint64_t>(Lo) | (static_cast<uint64_t>(Hi) << 32);
  }
};

static bool operator==(PairedI64 LHS, PairedI64 RHS) {
  return LHS.Lo == RHS.Lo && LHS.Hi == RHS.Hi;
}

static PairedI64 addPairs(PairedI64 LHS, PairedI64 RHS) {
  PairedI64 Result;
  Result.Lo = LHS.Lo + RHS.Lo;
  uint32_t Carry = Result.Lo < LHS.Lo;
  Result.Hi = LHS.Hi + RHS.Hi + Carry;
  return Result;
}

static PairedI64 subtractPairs(PairedI64 LHS, PairedI64 RHS) {
  PairedI64 Result;
  Result.Lo = LHS.Lo - RHS.Lo;
  uint32_t Borrow = LHS.Lo < RHS.Lo;
  Result.Hi = LHS.Hi - RHS.Hi - Borrow;
  return Result;
}

static uint32_t arithmeticShiftRight32(uint32_t Value, unsigned Amount) {
  if (Amount == 0)
    return Value;
  uint32_t Shifted = Value >> Amount;
  if ((Value & 0x80000000U) != 0)
    Shifted |= ~uint32_t(0) << (32 - Amount);
  return Shifted;
}

static PairedI64 shiftLeftPairs(PairedI64 Value, unsigned Amount) {
  if (Amount == 0)
    return Value;
  if (Amount < 32)
    return {Value.Lo << Amount,
            (Value.Hi << Amount) | (Value.Lo >> (32 - Amount))};
  if (Amount == 32)
    return {0, Value.Lo};
  return {0, Value.Lo << (Amount - 32)};
}

static PairedI64 shiftRightLogicalPairs(PairedI64 Value, unsigned Amount) {
  if (Amount == 0)
    return Value;
  if (Amount < 32)
    return {(Value.Lo >> Amount) | (Value.Hi << (32 - Amount)),
            Value.Hi >> Amount};
  if (Amount == 32)
    return {Value.Hi, 0};
  return {Value.Hi >> (Amount - 32), 0};
}

static PairedI64 shiftRightArithmeticPairs(PairedI64 Value, unsigned Amount) {
  if (Amount == 0)
    return Value;
  uint32_t Sign = arithmeticShiftRight32(Value.Hi, 31);
  if (Amount < 32)
    return {(Value.Lo >> Amount) | (Value.Hi << (32 - Amount)),
            arithmeticShiftRight32(Value.Hi, Amount)};
  if (Amount == 32)
    return {Value.Hi, Sign};
  return {arithmeticShiftRight32(Value.Hi, Amount - 32), Sign};
}

static PairedI64 shiftLeftPairsIteratively(PairedI64 Value, unsigned Amount) {
  for (unsigned I = 0; I != Amount; ++I) {
    bool Carry = (Value.Lo >> 31) != 0;
    Value.Lo <<= 1;
    Value.Hi = (Value.Hi << 1) | static_cast<uint32_t>(Carry);
  }
  return Value;
}

static PairedI64 shiftRightLogicalPairsIteratively(PairedI64 Value,
                                                   unsigned Amount) {
  for (unsigned I = 0; I != Amount; ++I) {
    bool Carry = (Value.Hi & 1) != 0;
    Value.Hi >>= 1;
    Value.Lo = (Value.Lo >> 1) | (static_cast<uint32_t>(Carry) << 31);
  }
  return Value;
}

static PairedI64 shiftRightArithmeticPairsIteratively(PairedI64 Value,
                                                      unsigned Amount) {
  for (unsigned I = 0; I != Amount; ++I) {
    bool Carry = (Value.Hi & 1) != 0;
    Value.Hi = arithmeticShiftRight32(Value.Hi, 1);
    Value.Lo = (Value.Lo >> 1) | (static_cast<uint32_t>(Carry) << 31);
  }
  return Value;
}

static bool signedLessThan64(uint64_t LHS, uint64_t RHS) {
  bool LHSSign = (LHS >> 63) != 0;
  bool RHSSign = (RHS >> 63) != 0;
  return LHSSign != RHSSign ? LHSSign : LHS < RHS;
}

static bool comparePairs(PairedI64 LHS, PairedI64 RHS, ISD::CondCode CC) {
  bool Equal = LHS == RHS;
  bool UnsignedLess = LHS.Hi != RHS.Hi ? LHS.Hi < RHS.Hi : LHS.Lo < RHS.Lo;
  bool SignedLess;
  bool LHSSign = (LHS.Hi >> 31) != 0;
  bool RHSSign = (RHS.Hi >> 31) != 0;
  if (LHSSign != RHSSign)
    SignedLess = LHSSign;
  else
    SignedLess = LHS.Hi != RHS.Hi ? LHS.Hi < RHS.Hi : LHS.Lo < RHS.Lo;

  switch (CC) {
  default:
    llvm_unreachable("unexpected i64 comparison predicate");
  case ISD::SETEQ:
    return Equal;
  case ISD::SETNE:
    return !Equal;
  case ISD::SETLT:
    return SignedLess;
  case ISD::SETLE:
    return SignedLess || Equal;
  case ISD::SETGT:
    return !SignedLess && !Equal;
  case ISD::SETGE:
    return !SignedLess;
  case ISD::SETULT:
    return UnsignedLess;
  case ISD::SETULE:
    return UnsignedLess || Equal;
  case ISD::SETUGT:
    return !UnsignedLess && !Equal;
  case ISD::SETUGE:
    return !UnsignedLess;
  }
}

static bool evaluateI64EqualityComparisonCFG(PairedI64 LHS, PairedI64 RHS,
                                             ISD::CondCode CC) {
  assert((CC == ISD::SETEQ || CC == ISD::SETNE) &&
         "expected equality comparison predicate");
  if (LHS.Hi != RHS.Hi)
    return CC == ISD::SETNE;
  return CC == ISD::SETEQ ? LHS.Lo == RHS.Lo : LHS.Lo != RHS.Lo;
}

static void verifyPairedI64(uint64_t LHSValue, uint64_t RHSValue) {
  PairedI64 LHS = PairedI64::split(LHSValue);
  PairedI64 RHS = PairedI64::split(RHSValue);
  EXPECT_EQ(LHSValue, LHS.join());
  EXPECT_EQ(RHSValue, RHS.join());
  EXPECT_EQ(LHSValue + RHSValue, addPairs(LHS, RHS).join());
  EXPECT_EQ(LHSValue - RHSValue, subtractPairs(LHS, RHS).join());
  EXPECT_EQ(uint64_t(0) - LHSValue,
            subtractPairs(PairedI64::split(0), LHS).join());
  EXPECT_EQ(LHSValue & RHSValue,
            (PairedI64{LHS.Lo & RHS.Lo, LHS.Hi & RHS.Hi}.join()));
  EXPECT_EQ(LHSValue | RHSValue,
            (PairedI64{LHS.Lo | RHS.Lo, LHS.Hi | RHS.Hi}.join()));
  EXPECT_EQ(LHSValue ^ RHSValue,
            (PairedI64{LHS.Lo ^ RHS.Lo, LHS.Hi ^ RHS.Hi}.join()));
  EXPECT_EQ(~LHSValue, (PairedI64{~LHS.Lo, ~LHS.Hi}.join()));

  for (ISD::CondCode CC :
       {ISD::SETEQ, ISD::SETNE, ISD::SETLT, ISD::SETLE, ISD::SETGT, ISD::SETGE,
        ISD::SETULT, ISD::SETULE, ISD::SETUGT, ISD::SETUGE}) {
    bool Expected;
    switch (CC) {
    default:
      llvm_unreachable("unexpected i64 comparison predicate");
    case ISD::SETEQ:
      Expected = LHSValue == RHSValue;
      break;
    case ISD::SETNE:
      Expected = LHSValue != RHSValue;
      break;
    case ISD::SETLT:
      Expected = signedLessThan64(LHSValue, RHSValue);
      break;
    case ISD::SETLE:
      Expected = signedLessThan64(LHSValue, RHSValue) || LHSValue == RHSValue;
      break;
    case ISD::SETGT:
      Expected = signedLessThan64(RHSValue, LHSValue);
      break;
    case ISD::SETGE:
      Expected = signedLessThan64(RHSValue, LHSValue) || LHSValue == RHSValue;
      break;
    case ISD::SETULT:
      Expected = LHSValue < RHSValue;
      break;
    case ISD::SETULE:
      Expected = LHSValue <= RHSValue;
      break;
    case ISD::SETUGT:
      Expected = LHSValue > RHSValue;
      break;
    case ISD::SETUGE:
      Expected = LHSValue >= RHSValue;
      break;
    }
    EXPECT_EQ(Expected, comparePairs(LHS, RHS, CC))
        << "predicate " << CC << ", lhs " << LHSValue << ", rhs " << RHSValue;
  }

  uint32_t LittleEndianWords[] = {LHS.Lo, LHS.Hi};
  uint32_t BigEndianWords[] = {LHS.Hi, LHS.Lo};
  EXPECT_EQ(LHSValue,
            (PairedI64{LittleEndianWords[0], LittleEndianWords[1]}.join()));
  EXPECT_EQ(LHSValue, (PairedI64{BigEndianWords[1], BigEndianWords[0]}.join()));

  EXPECT_EQ(LHS.Lo, static_cast<uint32_t>(LHSValue));
  EXPECT_EQ((PairedI64{LHS.Lo, 0}),
            PairedI64::split(static_cast<uint64_t>(LHS.Lo)));
  uint32_t Sign = arithmeticShiftRight32(LHS.Lo, 31);
  EXPECT_EQ((PairedI64{LHS.Lo, Sign}),
            PairedI64::split(static_cast<uint64_t>(LHS.Lo) |
                             (static_cast<uint64_t>(Sign) << 32)));

  for (unsigned Amount = 0; Amount != 64; ++Amount) {
    uint64_t ExpectedLeft = LHSValue << Amount;
    uint64_t ExpectedLogicalRight = LHSValue >> Amount;
    uint64_t ExpectedArithmeticRight;
    if ((LHSValue >> 63) == 0)
      ExpectedArithmeticRight = ExpectedLogicalRight;
    else if (Amount == 0)
      ExpectedArithmeticRight = LHSValue;
    else
      ExpectedArithmeticRight =
          ExpectedLogicalRight | (~uint64_t(0) << (64 - Amount));

    EXPECT_EQ(ExpectedLeft, shiftLeftPairs(LHS, Amount).join());
    EXPECT_EQ(ExpectedLogicalRight, shiftRightLogicalPairs(LHS, Amount).join());
    EXPECT_EQ(ExpectedArithmeticRight,
              shiftRightArithmeticPairs(LHS, Amount).join());
    EXPECT_EQ(ExpectedLeft, shiftLeftPairsIteratively(LHS, Amount).join());
    EXPECT_EQ(ExpectedLogicalRight,
              shiftRightLogicalPairsIteratively(LHS, Amount).join());
    EXPECT_EQ(ExpectedArithmeticRight,
              shiftRightArithmeticPairsIteratively(LHS, Amount).join());
  }
}

} // namespace

TEST_F(SHInstrInfoTest, PairedI64ModelMatchesScalarSemantics) {
  constexpr uint64_t Values[] = {
      0,
      1,
      ~uint64_t(0),
      uint64_t(1) << 63,
      (uint64_t(1) << 63) - 1,
      0x00000000ffffffffULL,
      0xffffffff00000000ULL,
      0x123456789abcdef0ULL,
      0x8000000000000001ULL,
      0xdeadbeef01234567ULL,
  };
  for (uint64_t LHS : Values)
    for (uint64_t RHS : Values)
      verifyPairedI64(LHS, RHS);

  std::mt19937 Generator(0x53484339);
  for (unsigned I = 0; I != 10000; ++I) {
    uint64_t LHS = static_cast<uint64_t>(Generator()) |
                   (static_cast<uint64_t>(Generator()) << 32);
    uint64_t RHS = static_cast<uint64_t>(Generator()) |
                   (static_cast<uint64_t>(Generator()) << 32);
    verifyPairedI64(LHS, RHS);
  }
}

TEST_F(SHInstrInfoTest, I64EqualityComparisonCFGRouting) {
  struct ComparisonCase {
    PairedI64 LHS;
    PairedI64 RHS;
    bool Equal;
    bool NotEqual;
  };
  constexpr ComparisonCase Cases[] = {
      {{0x01234567, 0x89abcdef}, {0x01234567, 0x89abcdef}, true, false},
      {{0x01234567, 0x89abcdef}, {0x76543210, 0x89abcdef}, false, true},
      {{0x01234567, 0x89abcdef}, {0x01234567, 0xfedcba98}, false, true},
      {{0x01234567, 0x89abcdef}, {0x76543210, 0xfedcba98}, false, true},
  };

  for (const ComparisonCase &Test : Cases) {
    EXPECT_EQ(Test.Equal,
              evaluateI64EqualityComparisonCFG(Test.LHS, Test.RHS, ISD::SETEQ));
    EXPECT_EQ(Test.NotEqual,
              evaluateI64EqualityComparisonCFG(Test.LHS, Test.RHS, ISD::SETNE));
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
