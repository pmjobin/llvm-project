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

  for (MCRegister Reg : {SH::R0, SH::R1, SH::R2, SH::R3, SH::R4, SH::R5, SH::R6,
                         SH::R7, SH::PR, SH::TBit, SH::MACH, SH::MACL})
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
