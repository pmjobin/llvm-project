//===- SHInstrInfoTest.cpp - SH instruction information tests ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHInstrInfo.h"
#include "SHBranchRange.h"
#include "SHConstantPoolValue.h"
#include "SHISelLowering.h"
#include "SHSubtarget.h"
#include "SHTargetMachine.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/LiveRegUnits.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineInstrBundle.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSchedule.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetSelect.h"

#include "gtest/gtest.h"
#include <limits>
#include <random>
#include <type_traits>

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
    M->setTargetTriple(TT);
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

TEST_F(SHInstrInfoTest, PhysicalCopiesPreserveRenamingContract) {
  MachineBasicBlock *MBB = createBlock();
  TII->copyPhysReg(*MBB, MBB->end(), DebugLoc(), SH::R4, SH::R5,
                   /*KillSrc=*/true, /*RenamableDest=*/true,
                   /*RenamableSrc=*/true);
  MachineInstr &Copy = MBB->back();
  ASSERT_EQ(SH::MOVrr, Copy.getOpcode());
  EXPECT_TRUE(Copy.getOperand(0).isRenamable());
  EXPECT_TRUE(Copy.getOperand(1).isRenamable());
  EXPECT_TRUE(Copy.getOperand(1).isKill());
  auto Operands = TII->isCopyInstr(Copy);
  ASSERT_TRUE(Operands);
  EXPECT_EQ(&Copy.getOperand(0), Operands->Destination);
  EXPECT_EQ(&Copy.getOperand(1), Operands->Source);

  // A physical ABI constraint stays fixed through COPY expansion.
  TII->copyPhysReg(*MBB, MBB->end(), DebugLoc(), SH::R0, SH::R4,
                   /*KillSrc=*/false, /*RenamableDest=*/false,
                   /*RenamableSrc=*/true);
  EXPECT_FALSE(MBB->back().getOperand(0).isRenamable());
  EXPECT_TRUE(MBB->back().getOperand(1).isRenamable());
}

TEST_F(SHInstrInfoTest, CopyRecognitionKeepsAdditionalContracts) {
  MachineBasicBlock *MBB = createBlock();
  auto AddCopy = [&](Register Dst, Register Src) -> MachineInstr & {
    return *BuildMI(*MBB, MBB->end(), DebugLoc(), TII->get(SH::MOVrr), Dst)
                .addReg(Src);
  };
  EXPECT_FALSE(TII->isCopyInstr(AddCopy(SH::R15, SH::R4)));
  EXPECT_FALSE(TII->isCopyInstr(AddCopy(SH::R4, SH::R15)));
  EXPECT_FALSE(TII->isCopyInstr(AddCopy(SH::MACL, SH::R4)));
  EXPECT_FALSE(TII->isCopyInstr(AddCopy(SH::R4, SH::FPSCR)));

  MachineInstr &Implicit = AddCopy(SH::R4, SH::R5);
  Implicit.addOperand(MachineOperand::CreateReg(SH::TBit, false, true));
  EXPECT_FALSE(TII->isCopyInstr(Implicit));
  MachineInstr &SubReg = AddCopy(SH::R4, SH::R5);
  SubReg.getOperand(1).setSubReg(SH::sub_fpu32_0);
  EXPECT_FALSE(TII->isCopyInstr(SubReg));
  MachineInstr &Undef = AddCopy(SH::R4, SH::R5);
  Undef.getOperand(1).setIsUndef();
  EXPECT_FALSE(TII->isCopyInstr(Undef));
  for (auto Flag : {MachineInstr::FrameSetup, MachineInstr::FrameDestroy,
                    MachineInstr::NoMerge}) {
    MachineInstr &Fixed = AddCopy(SH::R4, SH::R5);
    Fixed.setFlag(Flag);
    EXPECT_FALSE(TII->isCopyInstr(Fixed));
  }
}

TEST_F(SHInstrInfoTest, LateBranchRangeMaximumAlignmentPadding) {
  MF->ensureAlignment(Align(8));
  MachineBasicBlock *Prefix = createBlock();
  MachineBasicBlock *Source = createBlock();
  MachineBasicBlock *Gap = createBlock();
  MachineBasicBlock *Target = createBlock();
  for (unsigned I = 0; I != 3; ++I)
    BuildMI(*Prefix, Prefix->end(), DebugLoc(), TII->get(SH::NOP));
  MachineInstr *Removed = &Prefix->back();
  BuildMI(*Source, Source->end(), DebugLoc(), TII->get(SH::BF)).addMBB(Target);
  for (unsigned I = 0; I != 127; ++I)
    BuildMI(*Gap, Gap->end(), DebugLoc(), TII->get(SH::NOP));
  Target->setAlignment(Align(8), 2);
  auto SizeAfter = [&](const MachineInstr &MI, unsigned Size) {
    return &MI == Removed ? 0u : Size;
  };
  // Before: branch 6, target 264. After removal: branch 4, target 260,
  // because four bytes of padding exceed the maximum skip of two.
  EXPECT_TRUE(preservesSHBranchRanges(*MF, *TII, SizeAfter));
  // Without the skip limit, the target stays at 264 and BF is out of range.
  Target->setAlignment(Align(8), 0);
  EXPECT_FALSE(preservesSHBranchRanges(*MF, *TII, SizeAfter));
}

TEST_F(SHInstrInfoTest, DwarfRegisterMappingsAndInitialFrameState) {
  const MCRegisterInfo &MRI = TM->getMCRegisterInfo();
  const std::pair<MCRegister, int64_t> MappedRegisters[] = {
      {SH::R0, 0},     {SH::R1, 1},    {SH::R2, 2},    {SH::R3, 3},
      {SH::R4, 4},     {SH::R5, 5},    {SH::R6, 6},    {SH::R7, 7},
      {SH::R8, 8},     {SH::R9, 9},    {SH::R10, 10},  {SH::R11, 11},
      {SH::R12, 12},   {SH::R13, 13},  {SH::R14, 14},  {SH::R15, 15},
      {SH::PC, 16},    {SH::PR, 17},   {SH::GBR, 18},  {SH::VBR, 19},
      {SH::MACH, 20},  {SH::MACL, 21}, {SH::SR, 22},   {SH::FPUL, 23},
      {SH::FPSCR, 24}, {SH::FR0, 25},  {SH::FR1, 26},  {SH::FR2, 27},
      {SH::FR3, 28},   {SH::FR4, 29},  {SH::FR5, 30},  {SH::FR6, 31},
      {SH::FR7, 32},   {SH::FR8, 33},  {SH::FR9, 34},  {SH::FR10, 35},
      {SH::FR11, 36},  {SH::FR12, 37}, {SH::FR13, 38}, {SH::FR14, 39},
      {SH::FR15, 40},
  };
  for (auto [Reg, DwarfReg] : MappedRegisters) {
    EXPECT_EQ(DwarfReg, MRI.getDwarfRegNum(Reg, false));
    EXPECT_EQ(DwarfReg, MRI.getDwarfRegNum(Reg, true));
    EXPECT_EQ(Reg, MRI.getLLVMRegNum(DwarfReg, false));
    EXPECT_EQ(Reg, MRI.getLLVMRegNum(DwarfReg, true));
  }

  for (MCRegister Reg :
       {SH::TBit, SH::MBit, SH::QBit, SH::SBit, SH::SSR, SH::SPC, SH::SGR,
        SH::DBR, SH::R0_BANK, SH::R1_BANK, SH::R2_BANK, SH::R3_BANK,
        SH::R4_BANK, SH::R5_BANK, SH::R6_BANK, SH::R7_BANK}) {
    EXPECT_EQ(-1, MRI.getDwarfRegNum(Reg, false));
    EXPECT_EQ(-1, MRI.getDwarfRegNum(Reg, true));
  }
  for (const TargetRegisterClass *RC :
       {&SH::XFRegClass, &SH::DRRegClass, &SH::XDRegClass, &SH::FVRegClass,
        &SH::XMTRXRegRegClass})
    for (MCRegister Reg : *RC) {
      EXPECT_EQ(-1, MRI.getDwarfRegNum(Reg, false));
      EXPECT_EQ(-1, MRI.getDwarfRegNum(Reg, true));
    }
  EXPECT_EQ(std::nullopt, MRI.getLLVMRegNum(999, false));
  EXPECT_EQ(std::nullopt, MRI.getLLVMRegNum(999, true));

  EXPECT_EQ(SH::PR, MRI.getRARegister());
  EXPECT_EQ(
      SH::R15,
      MF->getSubtarget<SHSubtarget>().getRegisterInfo()->getFrameRegister(*MF));

  const MCAsmInfo &MAI = TM->getMCAsmInfo();
  EXPECT_EQ(ExceptionHandling::DwarfCFI, M->getExceptionModel());
  ASSERT_EQ(1u, MAI.getInitialFrameState().size());
  const MCCFIInstruction &InitialCFA = MAI.getInitialFrameState().front();
  EXPECT_EQ(MCCFIInstruction::OpDefCfa, InitialCFA.getOperation());
  EXPECT_EQ(15u, InitialCFA.getRegister());
  EXPECT_EQ(0, InitialCFA.getOffset());
}

TEST_F(SHInstrInfoTest, RegisterUnitsCoverBothHalvesOfDoubleRegisters) {
  LiveRegUnits Units(TII->getRegisterInfo());
  Units.addReg(SH::FR0);
  EXPECT_TRUE(Units.available(SH::FR1));
  Units.addReg(SH::DR0);
  EXPECT_FALSE(Units.available(SH::FR0));
  EXPECT_FALSE(Units.available(SH::FR1));
  EXPECT_TRUE(Units.available(SH::FR2));
  Units.addReg(SH::DR2);
  EXPECT_FALSE(Units.available(SH::FR3));
  Units.clear();
  Units.addReg(SH::DR0);
  Units.addReg(SH::FR0);
  EXPECT_FALSE(Units.available(SH::FR1));
}

TEST_F(SHInstrInfoTest, CanonicalProcessorFeatureSetsAndSchedulingAreExact) {
  const FeatureBitset &SH2Features = TM->getMCSubtargetInfo().getFeatureBits();
  EXPECT_TRUE(SH2Features[SH::FeatureSH2]);
  for (unsigned Feature :
       {SH::FeatureDynamicShift, SH::FeatureSBitControl,
        SH::FeatureExceptionState, SH::FeatureBankedGPRs, SH::FeatureMMU,
        SH::FeaturePrefetch, SH::FeatureSH4DebugState, SH::FeatureSH4Cache,
        SH::FeatureMOVAIllegalInDelaySlot, SH::FeatureST40300,
        SH::FeatureSingleFPU, SH::FeatureFSQRT, SH::FeatureFPUSizeMode,
        SH::FeatureDoubleFPU, SH::FeatureFPURegisterBank, SH::FeatureFPUVector,
        SH::FeatureFPUSingleMode})
    EXPECT_FALSE(SH2Features[Feature]);

  Triple TT("sh-unknown-elf");
  std::string Error;
  const Target *Target = TargetRegistry::lookupTarget(TT, Error);
  ASSERT_NE(nullptr, Target) << Error;
  std::unique_ptr<TargetMachine> SH1TM(
      Target->createTargetMachine(TT, "sh1", "", TargetOptions(), std::nullopt,
                                  std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH1TM);
  const MCSubtargetInfo &SH1STI = SH1TM->getMCSubtargetInfo();
  const FeatureBitset &SH1Features = SH1STI.getFeatureBits();
  for (unsigned Feature :
       {SH::FeatureSH2, SH::FeatureDynamicShift, SH::FeatureSBitControl,
        SH::FeatureExceptionState, SH::FeatureBankedGPRs, SH::FeatureMMU,
        SH::FeaturePrefetch, SH::FeatureSH4DebugState, SH::FeatureSH4Cache,
        SH::FeatureMOVAIllegalInDelaySlot, SH::FeatureST40300,
        SH::FeatureSingleFPU, SH::FeatureFSQRT, SH::FeatureFPUSizeMode,
        SH::FeatureDoubleFPU, SH::FeatureFPURegisterBank, SH::FeatureFPUVector,
        SH::FeatureFPUSingleMode})
    EXPECT_FALSE(SH1Features[Feature]);
  EXPECT_EQ(1u, SH1STI.getSchedModel().IssueWidth);
  EXPECT_FALSE(SH1STI.getSchedModel().CompleteModel);
  EXPECT_NE(&TM->getMCSubtargetInfo().getSchedModel(), &SH1STI.getSchedModel());

  std::unique_ptr<TargetMachine> SH3TM(
      Target->createTargetMachine(TT, "sh3", "", TargetOptions(), std::nullopt,
                                  std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH3TM);
  const MCSubtargetInfo &SH3STI = SH3TM->getMCSubtargetInfo();
  const FeatureBitset &SH3Features = SH3STI.getFeatureBits();
  for (unsigned Feature :
       {SH::FeatureSH2, SH::FeatureDynamicShift, SH::FeatureSBitControl,
        SH::FeatureExceptionState, SH::FeatureBankedGPRs, SH::FeatureMMU,
        SH::FeaturePrefetch})
    EXPECT_TRUE(SH3Features[Feature]);
  for (unsigned Feature :
       {SH::FeatureSH4DebugState, SH::FeatureSH4Cache,
        SH::FeatureMOVAIllegalInDelaySlot, SH::FeatureST40300,
        SH::FeatureSingleFPU, SH::FeatureFSQRT, SH::FeatureFPUSizeMode,
        SH::FeatureDoubleFPU, SH::FeatureFPURegisterBank, SH::FeatureFPUVector,
        SH::FeatureFPUSingleMode})
    EXPECT_FALSE(SH3Features[Feature]);
  EXPECT_EQ(1u, SH3STI.getSchedModel().IssueWidth);
  EXPECT_FALSE(SH3STI.getSchedModel().CompleteModel);
  EXPECT_NE(&TM->getMCSubtargetInfo().getSchedModel(), &SH3STI.getSchedModel());

  std::unique_ptr<TargetMachine> SH4TM(
      Target->createTargetMachine(TT, "sh4", "", TargetOptions(), std::nullopt,
                                  std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH4TM);
  const MCSubtargetInfo &SH4STI = SH4TM->getMCSubtargetInfo();
  const FeatureBitset &SH4Features = SH4STI.getFeatureBits();
  for (unsigned Feature :
       {SH::FeatureSH2, SH::FeatureDynamicShift, SH::FeatureSBitControl,
        SH::FeatureExceptionState, SH::FeatureBankedGPRs, SH::FeatureMMU,
        SH::FeaturePrefetch, SH::FeatureSH4DebugState, SH::FeatureSH4Cache,
        SH::FeatureMOVAIllegalInDelaySlot, SH::FeatureSingleFPU,
        SH::FeatureFSQRT, SH::FeatureFPUSizeMode, SH::FeatureDoubleFPU,
        SH::FeatureFPURegisterBank, SH::FeatureFPUVector})
    EXPECT_TRUE(SH4Features[Feature]);
  EXPECT_EQ(2u, SH4STI.getSchedModel().IssueWidth);
  EXPECT_FALSE(SH4STI.getSchedModel().CompleteModel);
  EXPECT_NE(&SH3STI.getSchedModel(), &SH4STI.getSchedModel());
  EXPECT_FALSE(SH4Features[SH::FeatureFPUSingleMode]);
  EXPECT_FALSE(SH4Features[SH::FeatureST40300]);
}

TEST_F(SHInstrInfoTest, SH2AArchitectureAndTuneAreOrthogonal) {
  EXPECT_TRUE(SH::isValidCPUName("sh2a"));
  EXPECT_TRUE(SH::isValidTuneCPUName("sh2a"));
  EXPECT_TRUE(SH::isTuneCompatible(SH::CPUKind::SH2, SH::TuneCPUKind::SH2A));

  Triple TT("sh-unknown-elf");
  std::string Error;
  const Target *Target = TargetRegistry::lookupTarget(TT, Error);
  ASSERT_NE(nullptr, Target) << Error;
  std::unique_ptr<TargetMachine> SH2ATM(
      Target->createTargetMachine(TT, "sh2a", "", TargetOptions(), std::nullopt,
                                  std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH2ATM);
  const MCSubtargetInfo &SH2ASTI = SH2ATM->getMCSubtargetInfo();
  const FeatureBitset &SH2AFeatures = SH2ASTI.getFeatureBits();
  for (unsigned Feature :
       {SH::FeatureSH2, SH::FeatureSH2A, SH::FeatureDynamicShift,
        SH::FeaturePrefetch, SH::FeatureSingleFPU, SH::FeatureFSQRT,
        SH::FeatureFPUSizeMode, SH::FeatureDoubleFPU})
    EXPECT_TRUE(SH2AFeatures[Feature]);
  for (unsigned Feature :
       {SH::FeatureSBitControl, SH::FeatureExceptionState,
        SH::FeatureBankedGPRs, SH::FeatureMMU, SH::FeatureSH4DebugState,
        SH::FeatureSH4Cache, SH::FeatureMOVAIllegalInDelaySlot,
        SH::FeatureST40300, SH::FeatureFPURegisterBank, SH::FeatureFPUVector,
        SH::FeatureFPUSingleMode})
    EXPECT_FALSE(SH2AFeatures[Feature]);
  EXPECT_EQ(2u, SH2ASTI.getSchedModel().IssueWidth);
  EXPECT_EQ(0u, SH2ASTI.getSchedModel().MicroOpBufferSize);
  EXPECT_FALSE(SH2ASTI.getSchedModel().CompleteModel);

  std::unique_ptr<TargetMachine> SH2TM(
      Target->createTargetMachine(TT, "sh2", "", TargetOptions(), std::nullopt,
                                  std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH2TM);
  LLVMContext LocalContext;
  Module LocalModule("SH2ATuneOnly", LocalContext);
  LocalModule.setDataLayout(SH2TM->createDataLayout());
  Function *F = Function::Create(
      FunctionType::get(Type::getVoidTy(LocalContext), false),
      GlobalValue::ExternalLinkage, "sh2_tuned_for_sh2a", LocalModule);
  F->addFnAttr("target-cpu", "sh2");
  F->addFnAttr("tune-cpu", "sh2a");
  const auto *TuneOnly =
      static_cast<const SHSubtarget *>(SH2TM->getSubtargetImpl(*F));
  ASSERT_NE(nullptr, TuneOnly);
  EXPECT_EQ("sh2", TuneOnly->getCPU());
  EXPECT_EQ("sh2a", TuneOnly->getTuneCPU());
  EXPECT_FALSE(TuneOnly->hasSH2A());
  EXPECT_EQ(TM->getMCSubtargetInfo().getFeatureBits(),
            TuneOnly->getFeatureBits());
  EXPECT_EQ(2u, TuneOnly->getSchedModel().IssueWidth);
  EXPECT_EQ(SH2ASTI.getSchedModel().ProcID, TuneOnly->getSchedModel().ProcID);
}

TEST_F(SHInstrInfoTest, SH4AArchitectureAndTuneAreOrthogonal) {
  EXPECT_TRUE(SH::isValidCPUName("sh4a"));
  EXPECT_TRUE(SH::isValidTuneCPUName("sh4a"));
  EXPECT_TRUE(SH::isTuneCompatible(SH::CPUKind::SH2, SH::TuneCPUKind::SH4A));
  EXPECT_FALSE(
      SH::isTuneCompatible(SH::CPUKind::SH4A, SH::TuneCPUKind::ST40300));

  Triple TT("sh-unknown-elf");
  std::string Error;
  const Target *Target = TargetRegistry::lookupTarget(TT, Error);
  ASSERT_NE(nullptr, Target) << Error;
  std::unique_ptr<TargetMachine> SH4ATM(
      Target->createTargetMachine(TT, "sh4a", "", TargetOptions(), std::nullopt,
                                  std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH4ATM);
  const MCSubtargetInfo &SH4ASTI = SH4ATM->getMCSubtargetInfo();
  const FeatureBitset &SH4AFeatures = SH4ASTI.getFeatureBits();
  for (unsigned Feature :
       {SH::FeatureSH2, SH::FeatureDynamicShift, SH::FeatureSBitControl,
        SH::FeatureExceptionState, SH::FeatureBankedGPRs, SH::FeatureMMU,
        SH::FeaturePrefetch, SH::FeatureSH4DebugState, SH::FeatureSH4Cache,
        SH::FeatureMOVAIllegalInDelaySlot, SH::FeatureSH4ACommon,
        SH::FeatureSH4A, SH::FeatureSingleFPU, SH::FeatureFSQRT,
        SH::FeatureFPUSizeMode, SH::FeatureDoubleFPU,
        SH::FeatureFPURegisterBank, SH::FeatureFPUVector})
    EXPECT_TRUE(SH4AFeatures[Feature]);
  EXPECT_FALSE(SH4AFeatures[SH::FeatureST40300]);
  EXPECT_FALSE(SH4AFeatures[SH::FeatureSH2A]);
  EXPECT_FALSE(SH4AFeatures[SH::FeatureFPUSingleMode]);
  EXPECT_EQ(2u, SH4ASTI.getSchedModel().IssueWidth);
  EXPECT_FALSE(SH4ASTI.getSchedModel().CompleteModel);

  std::unique_ptr<TargetMachine> SH2TM(
      Target->createTargetMachine(TT, "sh2", "", TargetOptions(), std::nullopt,
                                  std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH2TM);
  LLVMContext LocalContext;
  Module LocalModule("SH4ATuneOnly", LocalContext);
  LocalModule.setDataLayout(SH2TM->createDataLayout());
  Function *F = Function::Create(
      FunctionType::get(Type::getVoidTy(LocalContext), false),
      GlobalValue::ExternalLinkage, "sh2_tuned_for_sh4a", LocalModule);
  F->addFnAttr("target-cpu", "sh2");
  F->addFnAttr("tune-cpu", "sh4a");
  const auto *TuneOnly =
      static_cast<const SHSubtarget *>(SH2TM->getSubtargetImpl(*F));
  ASSERT_NE(nullptr, TuneOnly);
  EXPECT_EQ("sh2", TuneOnly->getCPU());
  EXPECT_EQ("sh4a", TuneOnly->getTuneCPU());
  EXPECT_FALSE(TuneOnly->hasSH4ACommon());
  EXPECT_FALSE(TuneOnly->hasSH4A());
  EXPECT_FALSE(TuneOnly->hasST40300());
  EXPECT_EQ(TM->getMCSubtargetInfo().getFeatureBits(),
            TuneOnly->getFeatureBits());
  EXPECT_EQ(SH4ASTI.getSchedModel().ProcID, TuneOnly->getSchedModel().ProcID);

  auto CheckSchedule = [&](unsigned Opcode, int ExpectedLatency,
                           std::initializer_list<std::pair<StringRef, unsigned>>
                               ExpectedResources,
                           bool SingleIssue = false) {
    unsigned SchedClass = SH4ATM->getMCInstrInfo()->get(Opcode).getSchedClass();
    const MCSchedClassDesc *Desc =
        SH4ASTI.getSchedModel().getSchedClassDesc(SchedClass);
    ASSERT_NE(nullptr, Desc);
    EXPECT_EQ(ExpectedLatency,
              SH4ASTI.getSchedModel().computeInstrLatency(SH4ASTI, *Desc));
    EXPECT_EQ(SingleIssue, Desc->BeginGroup);
    EXPECT_EQ(SingleIssue, Desc->EndGroup);
    ASSERT_EQ(ExpectedResources.size(), Desc->NumWriteProcResEntries);
    const MCWriteProcResEntry *Resource = SH4ASTI.getWriteProcResBegin(Desc);
    unsigned Index = 0;
    for (const auto &[Name, Release] : ExpectedResources) {
      EXPECT_EQ(Name,
                StringRef(SH4ASTI.getSchedModel()
                              .getProcResource(Resource[Index].ProcResourceIdx)
                              ->Name));
      EXPECT_EQ(Release, Resource[Index].ReleaseAtCycle);
      ++Index;
    }
  };
  CheckSchedule(SH::MOVUA_L, 3, {{"SH4ALS", 1}, {"SH4AMemory", 2}});
  CheckSchedule(SH::ICBI, 13, {{"SH4ACO", 16}}, true);
  CheckSchedule(SH::PREFI, 10, {{"SH4ACO", 13}}, true);
  CheckSchedule(SH::SYNCO, 1, {{"SH4ACO", 1}}, true);
  CheckSchedule(SH::FSRRA, 5, {{"SH4AFEX", 1}});
  CheckSchedule(SH::FSCA, 7, {{"SH4AFEX", 3}});
  CheckSchedule(SH::FPCHG, 3, {{"SH4AFEX", 1}, {"SH4AFLS", 1}});
}

TEST_F(SHInstrInfoTest, SH4ALDSPArchitectureStateAndTuneAreExact) {
  EXPECT_TRUE(SH::isValidCPUName("sh4al-dsp"));
  EXPECT_TRUE(SH::isValidTuneCPUName("sh4al-dsp"));
  EXPECT_TRUE(
      SH::isTuneCompatible(SH::CPUKind::SH3, SH::TuneCPUKind::SH4ALDSP));
  EXPECT_FALSE(
      SH::isTuneCompatible(SH::CPUKind::SH4ALDSP, SH::TuneCPUKind::ST40300));
  EXPECT_EQ(SH::CPUKind::SH4ALDSP, SH::parseCPU("sh4al-dsp"));
  EXPECT_EQ("sh4al-dsp",
            SH::getGNUAssemblerISA("sh4al-dsp", SH::FPUProfile::Default, false,
                                   true));

  Triple TT("sh-unknown-elf");
  std::string Error;
  const Target *Target = TargetRegistry::lookupTarget(TT, Error);
  ASSERT_NE(nullptr, Target) << Error;
  std::unique_ptr<TargetMachine> SH4ALTM(Target->createTargetMachine(
      TT, "sh4al-dsp", "", TargetOptions(), std::nullopt, std::nullopt,
      CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH4ALTM);
  const MCSubtargetInfo &SH4ALSTI = SH4ALTM->getMCSubtargetInfo();
  const FeatureBitset &SH4ALFeatures = SH4ALSTI.getFeatureBits();
  for (unsigned Feature :
       {SH::FeatureSH2, SH::FeatureDynamicShift, SH::FeatureSBitControl,
        SH::FeatureExceptionState, SH::FeatureBankedGPRs, SH::FeatureMMU,
        SH::FeaturePrefetch, SH::FeatureSH4DebugState, SH::FeatureSH4Cache,
        SH::FeatureMOVAIllegalInDelaySlot, SH::FeatureSH4ACommon,
        SH::FeatureSH4A, SH::FeatureDSP, SH::FeatureSH4ALDSP})
    EXPECT_TRUE(SH4ALFeatures[Feature]);
  for (unsigned Feature :
       {SH::FeatureSH2A, SH::FeatureST40300, SH::FeatureSingleFPU,
        SH::FeatureFSQRT, SH::FeatureFPUSizeMode, SH::FeatureDoubleFPU,
        SH::FeatureFPURegisterBank, SH::FeatureFPUVector,
        SH::FeatureFPUSingleMode})
    EXPECT_FALSE(SH4ALFeatures[Feature]);
  EXPECT_EQ(2u, SH4ALSTI.getSchedModel().IssueWidth);
  EXPECT_EQ(0u, SH4ALSTI.getSchedModel().MicroOpBufferSize);
  EXPECT_FALSE(SH4ALSTI.getSchedModel().CompleteModel);

  std::unique_ptr<TargetMachine> SH3TM(
      Target->createTargetMachine(TT, "sh3", "", TargetOptions(), std::nullopt,
                                  std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH3TM);
  LLVMContext TuneContext;
  Module TuneModule("SH4ALDSPTuneOnly", TuneContext);
  TuneModule.setDataLayout(SH3TM->createDataLayout());
  Function *TuneFunction = Function::Create(
      FunctionType::get(Type::getVoidTy(TuneContext), false),
      GlobalValue::ExternalLinkage, "sh3_tuned_for_sh4al_dsp", TuneModule);
  TuneFunction->addFnAttr("target-cpu", "sh3");
  TuneFunction->addFnAttr("tune-cpu", "sh4al-dsp");
  const auto *TuneOnly =
      static_cast<const SHSubtarget *>(SH3TM->getSubtargetImpl(*TuneFunction));
  ASSERT_NE(nullptr, TuneOnly);
  EXPECT_EQ("sh3", TuneOnly->getCPU());
  EXPECT_EQ("sh4al-dsp", TuneOnly->getTuneCPU());
  EXPECT_FALSE(TuneOnly->hasDSP());
  EXPECT_FALSE(TuneOnly->hasSH4ALDSP());
  EXPECT_FALSE(TuneOnly->hasSH4A());
  EXPECT_EQ(SH3TM->getMCSubtargetInfo().getFeatureBits(),
            TuneOnly->getFeatureBits());
  EXPECT_EQ(SH4ALSTI.getSchedModel().ProcID, TuneOnly->getSchedModel().ProcID);

  LLVMContext LocalContext;
  Module LocalModule("SH4ALDSPState", LocalContext);
  LocalModule.setDataLayout(SH4ALTM->createDataLayout());
  Function *F =
      Function::Create(FunctionType::get(Type::getVoidTy(LocalContext), false),
                       GlobalValue::ExternalLinkage, "state", LocalModule);
  MachineModuleInfo LocalMMI(SH4ALTM.get());
  MachineFunction &LocalMF = LocalMMI.getOrCreateMachineFunction(*F);
  const SHSubtarget &ST = LocalMF.getSubtarget<SHSubtarget>();
  const SHRegisterInfo &TRI = *ST.getRegisterInfo();
  EXPECT_TRUE(ST.hasDSP());
  EXPECT_TRUE(ST.hasSH4ALDSP());
  EXPECT_TRUE(ST.hasSH4A());
  EXPECT_FALSE(ST.hasSingleFPU());
  EXPECT_EQ(SH::A0L, TRI.getSubReg(SH::A0, SH::sub_dsp_lo32));
  EXPECT_EQ(SH::A0G, TRI.getSubReg(SH::A0, SH::sub_dsp_guard8));
  EXPECT_EQ(SH::A1L, TRI.getSubReg(SH::A1, SH::sub_dsp_lo32));
  EXPECT_EQ(SH::A1G, TRI.getSubReg(SH::A1, SH::sub_dsp_guard8));
  EXPECT_TRUE(TRI.regsOverlap(SH::A0, SH::A0L));
  EXPECT_TRUE(TRI.regsOverlap(SH::A0, SH::A0G));
  EXPECT_FALSE(TRI.regsOverlap(SH::A0, SH::A1));
  for (const TargetRegisterClass *RC :
       {&SH::DSPAccumulatorRegClass, &SH::DSPGuardRegClass,
        &SH::DSPData32RegClass, &SH::DSPDataRegClass, &SH::DSPXSourceRegClass,
        &SH::DSPYSourceRegClass, &SH::DSPMulXSourceRegClass,
        &SH::DSPMulYSourceRegClass, &SH::DSPMulDestinationRegClass,
        &SH::DSPMoveDataRegClass, &SH::DSRRegRegClass, &SH::RSRegRegClass,
        &SH::RERegRegClass, &SH::MODRegRegClass})
    EXPECT_FALSE(RC->isAllocatable());
  BitVector Reserved = TRI.getReservedRegs(LocalMF);
  const uint32_t *CallMask = TRI.getCallPreservedMask(LocalMF, CallingConv::C);
  for (MCRegister Reg :
       {SH::A0, SH::A1, SH::A0L, SH::A1L, SH::A0G, SH::A1G, SH::X0, SH::X1,
        SH::Y0, SH::Y1, SH::M0, SH::M1, SH::DSR, SH::RS, SH::RE, SH::MOD}) {
    EXPECT_TRUE(Reserved.test(Reg));
    EXPECT_TRUE(MachineOperand::clobbersPhysReg(CallMask, Reg));
  }

  const SHInstrInfo &LocalTII = *ST.getInstrInfo();
  EXPECT_EQ(2u, LocalTII.get(SH::CLRMXY).getSize());
  EXPECT_EQ(2u, LocalTII.get(SH::DSP_DDT).getSize());
  EXPECT_EQ(2u, LocalTII.get(SH::DSP_SDT).getSize());
  EXPECT_EQ(4u, LocalTII.get(SH::DSP_PPI).getSize());
  EXPECT_TRUE(LocalTII.get(SH::LDC_MOD).hasImplicitDefOfPhysReg(SH::MOD));
  EXPECT_TRUE(is_contained(LocalTII.get(SH::STC_MOD).implicit_uses(), SH::MOD));
  EXPECT_TRUE(LocalTII.get(SH::LDS_A0).hasImplicitDefOfPhysReg(SH::A0));
  EXPECT_TRUE(is_contained(LocalTII.get(SH::STS_A0).implicit_uses(), SH::A0));
  for (unsigned Opcode : {SH::CLRMXY, SH::SETDMX, SH::SETDMY}) {
    EXPECT_TRUE(is_contained(LocalTII.get(Opcode).implicit_uses(), SH::SR));
    EXPECT_TRUE(LocalTII.get(Opcode).hasImplicitDefOfPhysReg(SH::SR));
  }
  for (unsigned Opcode : {SH::SETRC_REG, SH::SETRC_IMM}) {
    EXPECT_TRUE(is_contained(LocalTII.get(Opcode).implicit_uses(), SH::SR));
    EXPECT_TRUE(is_contained(LocalTII.get(Opcode).implicit_uses(), SH::RS));
    EXPECT_TRUE(is_contained(LocalTII.get(Opcode).implicit_uses(), SH::RE));
    EXPECT_TRUE(LocalTII.get(Opcode).hasImplicitDefOfPhysReg(SH::SR));
    EXPECT_FALSE(LocalTII.get(Opcode).hasImplicitDefOfPhysReg(SH::RS));
    EXPECT_FALSE(LocalTII.get(Opcode).hasImplicitDefOfPhysReg(SH::RE));
  }
  for (unsigned Opcode : {SH::LDRC_REG, SH::LDRC_IMM}) {
    EXPECT_TRUE(is_contained(LocalTII.get(Opcode).implicit_uses(), SH::SR));
    EXPECT_TRUE(is_contained(LocalTII.get(Opcode).implicit_uses(), SH::RS));
    EXPECT_TRUE(is_contained(LocalTII.get(Opcode).implicit_uses(), SH::RE));
    EXPECT_TRUE(LocalTII.get(Opcode).hasImplicitDefOfPhysReg(SH::SR));
    EXPECT_TRUE(LocalTII.get(Opcode).hasImplicitDefOfPhysReg(SH::RE));
  }
  EXPECT_TRUE(LocalTII.get(SH::DSP_DDT).mayLoad());
  EXPECT_TRUE(LocalTII.get(SH::DSP_DDT).mayStore());
  EXPECT_TRUE(LocalTII.get(SH::DSP_DDT).hasUnmodeledSideEffects());
  EXPECT_TRUE(is_contained(LocalTII.get(SH::DSP_DDT).implicit_uses(), SH::DSR));
  EXPECT_TRUE(is_contained(LocalTII.get(SH::DSP_DDT).implicit_uses(), SH::SR));
  EXPECT_TRUE(LocalTII.get(SH::DSP_DDT).hasImplicitDefOfPhysReg(SH::DSR));
  EXPECT_TRUE(
      is_contained(LocalTII.get(SH::DSP_PPI).implicit_uses(), SH::TBit));
  EXPECT_TRUE(LocalTII.get(SH::DSP_PPI).hasImplicitDefOfPhysReg(SH::TBit));
  for (unsigned Opcode :
       {SH::CLRMXY, SH::LDC_MOD, SH::LDC_L_MOD, SH::LDS_A0, SH::LDS_L_A0,
        SH::STC_MOD, SH::STC_L_MOD, SH::STS_A0, SH::STS_L_A0, SH::DSP_DDT,
        SH::DSP_SDT, SH::DSP_PPI}) {
    EXPECT_FALSE(SHII::isDelaySlotIllegal(LocalTII.get(Opcode)))
        << LocalTII.getName(Opcode);
    EXPECT_TRUE(LocalTII.isLegalInDelaySlot(Opcode, ST))
        << LocalTII.getName(Opcode);
  }
  for (unsigned Opcode : {SH::LDRS, SH::LDRE}) {
    EXPECT_TRUE(SHII::isDelaySlotIllegal(LocalTII.get(Opcode)))
        << LocalTII.getName(Opcode);
    EXPECT_FALSE(LocalTII.isLegalInDelaySlot(Opcode, ST))
        << LocalTII.getName(Opcode);
  }
  MachineBasicBlock *DSPMBB = LocalMF.CreateMachineBasicBlock();
  LocalMF.push_back(DSPMBB);
  MachineInstr *ClearDMXY =
      BuildMI(DSPMBB, DebugLoc(), LocalTII.get(SH::CLRMXY));
  MachineInstr *LoadMOD =
      BuildMI(DSPMBB, DebugLoc(), LocalTII.get(SH::LDC_MOD)).addReg(SH::R0);
  MachineInstr *DSPTransfer =
      BuildMI(DSPMBB, DebugLoc(), LocalTII.get(SH::DSP_DDT)).addImm(0);
  for (MachineInstr *MI : {ClearDMXY, LoadMOD, DSPTransfer})
    EXPECT_FALSE(LocalTII.isDelaySlotCandidate(*MI, ST))
        << LocalTII.getName(MI->getOpcode());

  auto CheckSchedule = [&](unsigned Opcode, int ExpectedLatency,
                           std::initializer_list<StringRef> ExpectedResources,
                           bool SingleIssue) {
    unsigned SchedClass = LocalTII.get(Opcode).getSchedClass();
    const MCSchedClassDesc *Desc =
        SH4ALSTI.getSchedModel().getSchedClassDesc(SchedClass);
    ASSERT_NE(nullptr, Desc);
    EXPECT_EQ(ExpectedLatency,
              SH4ALSTI.getSchedModel().computeInstrLatency(SH4ALSTI, *Desc));
    EXPECT_EQ(SingleIssue, Desc->BeginGroup);
    EXPECT_EQ(SingleIssue, Desc->EndGroup);
    ASSERT_EQ(ExpectedResources.size(), Desc->NumWriteProcResEntries);
    const MCWriteProcResEntry *Resource = SH4ALSTI.getWriteProcResBegin(Desc);
    unsigned Index = 0;
    for (StringRef Name : ExpectedResources)
      EXPECT_EQ(Name, StringRef(SH4ALSTI.getSchedModel()
                                    .getProcResource(
                                        Resource[Index++].ProcResourceIdx)
                                    ->Name));
  };
  CheckSchedule(SH::LDRC_REG, 2, {"SH4ALDSP_CO"}, true);
  CheckSchedule(SH::DSP_DDT, 1,
                {"SH4ALDSP_DSP_LS", "SH4ALDSP_LS", "SH4ALDSP_Memory"}, false);
  CheckSchedule(SH::DSP_PPI, 1,
                {"SH4ALDSP_DSP_EX", "SH4ALDSP_DSP_LS", "SH4ALDSP_DSP_Mult"},
                true);
}

TEST_F(SHInstrInfoTest, ST40TuneModelsAreOrthogonalToSH4Architecture) {
  EXPECT_FALSE(SH::isValidCPUName("st40-100"));
  EXPECT_FALSE(SH::isValidCPUName("st40-200"));
  EXPECT_FALSE(SH::isValidCPUName("st40-300"));
  EXPECT_FALSE(SH::isValidCPUName("st40-340"));
  EXPECT_FALSE(SH::isValidCPUName("st40-400"));
  EXPECT_FALSE(SH::isValidCPUName("st40-500"));
  EXPECT_TRUE(SH::isValidTuneCPUName("st40-100"));
  EXPECT_TRUE(SH::isValidTuneCPUName("st40-200"));
  EXPECT_TRUE(SH::isValidTuneCPUName("st40-300"));
  EXPECT_TRUE(SH::isValidTuneCPUName("st40-340"));
  EXPECT_TRUE(SH::isValidTuneCPUName("st40-400"));
  EXPECT_TRUE(SH::isValidTuneCPUName("st40-500"));
  EXPECT_TRUE(SH::isTuneCompatible(SH::CPUKind::SH4, SH::TuneCPUKind::ST40100));
  EXPECT_TRUE(SH::isTuneCompatible(SH::CPUKind::SH4, SH::TuneCPUKind::ST40200));
  EXPECT_TRUE(SH::isTuneCompatible(SH::CPUKind::SH4, SH::TuneCPUKind::ST40300));
  EXPECT_TRUE(SH::isTuneCompatible(SH::CPUKind::SH4, SH::TuneCPUKind::ST40340));
  EXPECT_TRUE(SH::isTuneCompatible(SH::CPUKind::SH4, SH::TuneCPUKind::ST40400));
  EXPECT_TRUE(SH::isTuneCompatible(SH::CPUKind::SH4, SH::TuneCPUKind::ST40500));
  EXPECT_FALSE(
      SH::isTuneCompatible(SH::CPUKind::SH3, SH::TuneCPUKind::ST40100));

  Triple TT("sh-unknown-elf");
  std::string Error;
  const Target *Target = TargetRegistry::lookupTarget(TT, Error);
  ASSERT_NE(nullptr, Target) << Error;
  std::unique_ptr<TargetMachine> SH4TM(
      Target->createTargetMachine(TT, "sh4", "", TargetOptions(), std::nullopt,
                                  std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH4TM);
  LLVMContext LocalContext;
  Module LocalModule("ST40TuneModels", LocalContext);
  LocalModule.setDataLayout(SH4TM->createDataLayout());
  auto CreateFunction = [&](StringRef Name, StringRef TuneCPU) {
    Function *F = Function::Create(
        FunctionType::get(Type::getVoidTy(LocalContext), false),
        GlobalValue::ExternalLinkage, Name, LocalModule);
    F->addFnAttr("target-cpu", "sh4");
    F->addFnAttr("tune-cpu", TuneCPU);
    return F;
  };
  Function *GenericFunction = CreateFunction("generic", "sh4");
  Function *ST40100Function = CreateFunction("st40_100", "st40-100");
  Function *ST40200Function = CreateFunction("st40_200", "st40-200");
  Function *ST40300TuneFunction = CreateFunction("st40_300_tune", "st40-300");
  Function *ST40340Function = CreateFunction("st40_340", "st40-340");
  Function *ST40400Function = CreateFunction("st40_400", "st40-400");
  Function *ST40500Function = CreateFunction("st40_500", "st40-500");
  Function *ST40300ArchitectureFunction =
      CreateFunction("st40_300_arch", "st40-100");
  ST40300ArchitectureFunction->addFnAttr("target-features", "+st40-300");
  const auto *Generic = static_cast<const SHSubtarget *>(
      SH4TM->getSubtargetImpl(*GenericFunction));
  const auto *ST40100 = static_cast<const SHSubtarget *>(
      SH4TM->getSubtargetImpl(*ST40100Function));
  const auto *ST40200 = static_cast<const SHSubtarget *>(
      SH4TM->getSubtargetImpl(*ST40200Function));
  const auto *ST40300Tune = static_cast<const SHSubtarget *>(
      SH4TM->getSubtargetImpl(*ST40300TuneFunction));
  const auto *ST40340 = static_cast<const SHSubtarget *>(
      SH4TM->getSubtargetImpl(*ST40340Function));
  const auto *ST40400 = static_cast<const SHSubtarget *>(
      SH4TM->getSubtargetImpl(*ST40400Function));
  const auto *ST40500 = static_cast<const SHSubtarget *>(
      SH4TM->getSubtargetImpl(*ST40500Function));
  const auto *ST40300Architecture = static_cast<const SHSubtarget *>(
      SH4TM->getSubtargetImpl(*ST40300ArchitectureFunction));
  ASSERT_NE(nullptr, Generic);
  ASSERT_NE(nullptr, ST40100);
  ASSERT_NE(nullptr, ST40200);
  ASSERT_NE(nullptr, ST40300Tune);
  ASSERT_NE(nullptr, ST40340);
  ASSERT_NE(nullptr, ST40400);
  ASSERT_NE(nullptr, ST40500);
  ASSERT_NE(nullptr, ST40300Architecture);
  EXPECT_EQ("sh4", Generic->getCPU());
  EXPECT_EQ("sh4", ST40100->getCPU());
  EXPECT_EQ("sh4", ST40200->getCPU());
  EXPECT_EQ("sh4", ST40300Tune->getCPU());
  EXPECT_EQ("sh4", ST40340->getCPU());
  EXPECT_EQ("sh4", ST40400->getCPU());
  EXPECT_EQ("sh4", ST40500->getCPU());
  EXPECT_EQ("sh4", ST40300Architecture->getCPU());
  EXPECT_EQ("sh4", Generic->getTuneCPU());
  EXPECT_EQ("st40-100", ST40100->getTuneCPU());
  EXPECT_EQ("st40-200", ST40200->getTuneCPU());
  EXPECT_EQ("st40-300", ST40300Tune->getTuneCPU());
  EXPECT_EQ("st40-340", ST40340->getTuneCPU());
  EXPECT_EQ("st40-400", ST40400->getTuneCPU());
  EXPECT_EQ("st40-500", ST40500->getTuneCPU());
  EXPECT_EQ("st40-100", ST40300Architecture->getTuneCPU());
  EXPECT_EQ(Generic->getFeatureBits(), ST40100->getFeatureBits());
  EXPECT_EQ(Generic->getFeatureBits(), ST40200->getFeatureBits());
  EXPECT_EQ(Generic->getFeatureBits(), ST40300Tune->getFeatureBits());
  EXPECT_EQ(Generic->getFeatureBits(), ST40340->getFeatureBits());
  EXPECT_EQ(Generic->getFeatureBits(), ST40400->getFeatureBits());
  EXPECT_EQ(Generic->getFeatureBits(), ST40500->getFeatureBits());
  EXPECT_FALSE(ST40300Tune->hasST40300());
  EXPECT_FALSE(ST40340->hasST40300());
  EXPECT_FALSE(ST40400->hasST40300());
  EXPECT_FALSE(ST40500->hasST40300());
  EXPECT_TRUE(ST40300Architecture->hasST40300());
  EXPECT_TRUE(ST40300Architecture->hasSH4ACommon());
  EXPECT_FALSE(ST40300Architecture->hasSH4A());
  EXPECT_NE(Generic->getFeatureBits(), ST40300Architecture->getFeatureBits());
  EXPECT_NE(Generic->getSchedModel().ProcID, ST40100->getSchedModel().ProcID);
  EXPECT_NE(Generic->getSchedModel().ProcID, ST40200->getSchedModel().ProcID);
  EXPECT_NE(ST40100->getSchedModel().ProcID, ST40200->getSchedModel().ProcID);
  EXPECT_NE(Generic->getSchedModel().ProcID,
            ST40300Tune->getSchedModel().ProcID);
  EXPECT_EQ(ST40300Tune->getSchedModel().ProcID,
            ST40340->getSchedModel().ProcID);
  EXPECT_EQ(ST40200->getSchedModel().ProcID, ST40400->getSchedModel().ProcID);
  EXPECT_EQ(ST40200->getSchedModel().ProcID, ST40500->getSchedModel().ProcID);
  for (const SHSubtarget *ST : {ST40100, ST40200, ST40340, ST40400, ST40500}) {
    EXPECT_EQ(2u, ST->getSchedModel().IssueWidth);
    EXPECT_EQ(0u, ST->getSchedModel().MicroOpBufferSize);
    EXPECT_FALSE(ST->getSchedModel().CompleteModel);
  }
  EXPECT_EQ(ST40100->getSchedModel().ProcID,
            ST40300Architecture->getSchedModel().ProcID);
  EXPECT_EQ(2u, ST40300Tune->getSchedModel().IssueWidth);
  EXPECT_EQ(0u, ST40300Tune->getSchedModel().MicroOpBufferSize);
  EXPECT_FALSE(ST40300Tune->getSchedModel().CompleteModel);
  auto GetST40300Resource = [&](StringRef Name) {
    for (unsigned Index = 1;
         Index < ST40300Tune->getSchedModel().getNumProcResourceKinds();
         ++Index) {
      const MCProcResourceDesc *Resource =
          ST40300Tune->getSchedModel().getProcResource(Index);
      if (Name == Resource->Name)
        return Resource;
    }
    return static_cast<const MCProcResourceDesc *>(nullptr);
  };
  const MCProcResourceDesc *ALUResource = GetST40300Resource("ST40300ALU");
  const MCProcResourceDesc *LSResource = GetST40300Resource("ST40300LS");
  const MCProcResourceDesc *FPUResource = GetST40300Resource("ST40300FPU");
  ASSERT_NE(nullptr, ALUResource);
  ASSERT_NE(nullptr, LSResource);
  ASSERT_NE(nullptr, FPUResource);
  EXPECT_EQ(2u, ALUResource->NumUnits);
  EXPECT_EQ(1u, LSResource->NumUnits);
  EXPECT_EQ(1u, FPUResource->NumUnits);

  auto CheckSchedule = [](const SHSubtarget &ST, unsigned Opcode,
                          int ExpectedLatency, unsigned ExpectedRelease,
                          bool SingleIssue, StringRef ExpectedResource) {
    unsigned SchedClass = ST.getInstrInfo()->get(Opcode).getSchedClass();
    const MCSchedClassDesc *Desc =
        ST.getSchedModel().getSchedClassDesc(SchedClass);
    ASSERT_NE(nullptr, Desc);
    EXPECT_EQ(ExpectedLatency,
              ST.getSchedModel().computeInstrLatency(ST, *Desc));
    EXPECT_EQ(SingleIssue, Desc->BeginGroup);
    EXPECT_EQ(SingleIssue, Desc->EndGroup);
    ASSERT_EQ(1u, Desc->NumWriteProcResEntries);
    const MCWriteProcResEntry *Resource = ST.getWriteProcResBegin(Desc);
    EXPECT_EQ(ExpectedRelease, Resource->ReleaseAtCycle);
    EXPECT_EQ(
        ExpectedResource,
        ST.getSchedModel().getProcResource(Resource->ProcResourceIdx)->Name);
  };
  for (const auto &[ST, Prefix] :
       {std::pair{ST40100, "ST40100"}, std::pair{ST40200, "ST40200"}}) {
    CheckSchedule(*ST, SH::ADDrr, 1, 1, false, std::string(Prefix) + "EX");
    CheckSchedule(*ST, SH::MOVrr, 0, 1, false, std::string(Prefix) + "MT");
    CheckSchedule(*ST, SH::TST, 1, 1, false, std::string(Prefix) + "MT");
    CheckSchedule(*ST, SH::MOVL_load_reg, 2, 1, false,
                  std::string(Prefix) + "LS");
    CheckSchedule(*ST, SH::MUL_L, 4, 2, true, std::string(Prefix) + "CO");
    CheckSchedule(*ST, SH::MAC_W, 4, 2, true, std::string(Prefix) + "CO");
    CheckSchedule(*ST, SH::BRA, 2, 1, false, std::string(Prefix) + "BR");
    CheckSchedule(*ST, SH::FADD_FR, 3, 1, false, std::string(Prefix) + "FE");
    CheckSchedule(*ST, SH::FMUL_FR, 3, 1, false, std::string(Prefix) + "FE");
    CheckSchedule(*ST, SH::FADD_DR, 8, 6, false, std::string(Prefix) + "FE");
    CheckSchedule(*ST, SH::FTRV, 7, 4, false, std::string(Prefix) + "FE");
    const MCSchedClassDesc *ShiftDesc = ST->getSchedModel().getSchedClassDesc(
        ST->getInstrInfo()->get(SH::SHAD).getSchedClass());
    ASSERT_NE(nullptr, ShiftDesc);
    ASSERT_EQ(1u, ShiftDesc->NumReadAdvanceEntries);
    EXPECT_EQ(
        -1, ST->getReadAdvanceCycles(
                ShiftDesc, 1,
                ST->getWriteLatencyEntry(ST->getSchedModel().getSchedClassDesc(
                                             ST->getInstrInfo()
                                                 ->get(SH::MOVL_load_reg)
                                                 .getSchedClass()),
                                         0)
                    ->WriteResourceID));
  }

  auto CheckST40300Schedule =
      [&](unsigned Opcode, int ExpectedLatency,
          std::initializer_list<std::pair<StringRef, unsigned>>
              ExpectedResources,
          bool SingleIssue = false) {
        unsigned SchedClass =
            ST40300Tune->getInstrInfo()->get(Opcode).getSchedClass();
        const MCSchedClassDesc *Desc =
            ST40300Tune->getSchedModel().getSchedClassDesc(SchedClass);
        ASSERT_NE(nullptr, Desc);
        EXPECT_EQ(ExpectedLatency,
                  ST40300Tune->getSchedModel().computeInstrLatency(*ST40300Tune,
                                                                   *Desc));
        EXPECT_EQ(SingleIssue, Desc->BeginGroup);
        EXPECT_EQ(SingleIssue, Desc->EndGroup);
        ASSERT_EQ(ExpectedResources.size(), Desc->NumWriteProcResEntries);
        const MCWriteProcResEntry *Resource =
            ST40300Tune->getWriteProcResBegin(Desc);
        unsigned Index = 0;
        for (const auto &[Name, Release] : ExpectedResources) {
          EXPECT_EQ(Name, StringRef(ST40300Tune->getSchedModel()
                                        .getProcResource(
                                            Resource[Index].ProcResourceIdx)
                                        ->Name));
          EXPECT_EQ(Release, Resource[Index].ReleaseAtCycle);
          ++Index;
        }
      };
  CheckST40300Schedule(SH::MOVrr, 0, {{"ST40300ALU", 1}});
  CheckST40300Schedule(SH::ADDrr, 1, {{"ST40300ALU", 1}});
  CheckST40300Schedule(SH::MOVL_load_reg, 3, {{"ST40300LS", 1}});
  CheckST40300Schedule(SH::MUL_L, 2, {{"ST40300MUL", 1}});
  CheckST40300Schedule(SH::MAC_W, 5, {{"ST40300LS", 2}, {"ST40300MUL", 2}});
  CheckST40300Schedule(SH::BRA, 4, {{"ST40300BR", 1}});
  CheckST40300Schedule(SH::JSR, 4, {{"ST40300BR", 1}});
  CheckST40300Schedule(SH::RTS, 4, {{"ST40300BR", 1}});
  CheckST40300Schedule(SH::RTE, 9, {{"ST40300BR", 9}}, true);
  CheckST40300Schedule(SH::FADD_FR, 6, {{"ST40300FPU", 1}});
  CheckST40300Schedule(SH::FCMP_EQ_FR, 3, {{"ST40300FPU", 1}});
  CheckST40300Schedule(SH::FDIV_FR, 19,
                       {{"ST40300FDS", 16}, {"ST40300FPU", 1}});
  CheckST40300Schedule(SH::FMUL_DR, 9, {{"ST40300FPU", 4}});
  CheckST40300Schedule(SH::FDIV_DR, 35,
                       {{"ST40300FDS", 32}, {"ST40300FPU", 1}});
  CheckST40300Schedule(SH::FTRV, 9, {{"ST40300FPU", 4}});
  CheckST40300Schedule(SH::ICBI, 8, {{"ST40300ALL", 7}, {"ST40300LS", 1}},
                       true);
  CheckST40300Schedule(SH::SYNCO, 1, {{"ST40300ALL", 1}, {"ST40300LS", 1}},
                       true);
  CheckST40300Schedule(SH::FPCHG, 1, {{"ST40300FPT", 1}, {"ST40300FPU", 1}});
}

TEST_F(SHInstrInfoTest, SH4FPUProfilesComposeCPUFeaturesAndABI) {
  Triple TT("sh-unknown-elf");
  std::string Error;
  const Target *Target = TargetRegistry::lookupTarget(TT, Error);
  ASSERT_NE(nullptr, Target) << Error;
  struct Profile {
    const char *Features;
    const char *ABI;
    bool Single;
    bool Double;
    bool SingleMode;
  };
  const Profile Profiles[] = {
      {"-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-"
       "vector,-fpu-single-mode",
       "gnu", false, false, false},
      {"+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,+fpu-register-bank,+fpu-"
       "vector,+fpu-single-mode",
       "gnu-hard-float-single", true, true, true},
      {"+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-"
       "vector,+fpu-single-mode",
       "gnu-e-float", true, false, true},
  };
  for (const Profile &P : Profiles) {
    TargetOptions Options;
    Options.MCOptions.ABIName = P.ABI;
    std::unique_ptr<TargetMachine> LocalTM(Target->createTargetMachine(
        TT, "sh4", P.Features, Options, std::nullopt, std::nullopt,
        CodeGenOptLevel::Default));
    ASSERT_NE(nullptr, LocalTM);
    LLVMContext LocalContext;
    Module LocalModule("SH4FPUProfile", LocalContext);
    LocalModule.setDataLayout(LocalTM->createDataLayout());
    Function *F = Function::Create(
        FunctionType::get(Type::getVoidTy(LocalContext), false),
        GlobalValue::ExternalLinkage, "profile", LocalModule);
    const auto *STI =
        static_cast<const SHSubtarget *>(LocalTM->getSubtargetImpl(*F));
    ASSERT_NE(nullptr, STI);
    EXPECT_EQ(P.Single, STI->hasSingleFPU());
    EXPECT_EQ(P.Double, STI->hasDoubleFPU());
    EXPECT_EQ(P.SingleMode, STI->hasFPUSingleMode());
    EXPECT_EQ(llvm::SH::parseABI(P.ABI), STI->getABI());
  }
}

TEST_F(SHInstrInfoTest, SH4IntegerSchedulingClassesAreExact) {
  Triple TT("sh-unknown-elf");
  std::string Error;
  const Target *Target = TargetRegistry::lookupTarget(TT, Error);
  ASSERT_NE(nullptr, Target) << Error;
  std::unique_ptr<TargetMachine> SH4TM(
      Target->createTargetMachine(TT, "sh4", "", TargetOptions(), std::nullopt,
                                  std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH4TM);
  const MCSubtargetInfo &STI = SH4TM->getMCSubtargetInfo();
  const MCSchedModel &Model = STI.getSchedModel();
  EXPECT_EQ(2u, Model.IssueWidth);
  EXPECT_FALSE(Model.CompleteModel);

  auto CheckLatency = [&](unsigned Opcode, int Expected) {
    const MCSchedClassDesc *Class =
        Model.getSchedClassDesc(TII->get(Opcode).getSchedClass());
    ASSERT_NE(nullptr, Class);
    ASSERT_TRUE(Class->isValid());
    ASSERT_EQ(1u, Class->NumWriteProcResEntries);
    ASSERT_EQ(1u, Class->NumWriteLatencyEntries);
    EXPECT_EQ(Expected, STI.getWriteLatencyEntry(Class, 0)->Cycles)
        << TII->getName(Opcode);
  };
  CheckLatency(SH::MOVrr, 0);
  CheckLatency(SH::MOVri, 1);
  CheckLatency(SH::MOVL_load_reg, 2);
  CheckLatency(SH::BT, 2);
  CheckLatency(SH::JSR, 3);
  CheckLatency(SH::MAC_L, 4);
  CheckLatency(SH::RTE, 5);
  CheckLatency(SH::TRAPA, 7);
}

TEST_F(SHInstrInfoTest, MOVADelaySlotLegalityIsSubtargetSpecific) {
  const SHSubtarget &SH2STI = MF->getSubtarget<SHSubtarget>();
  EXPECT_TRUE(TII->isLegalInDelaySlot(SH::MOVA, SH2STI));
  MachineBasicBlock *MBB = createBlock();
  MachineInstr *Mova = BuildMI(MBB, DebugLoc(), TII->get(SH::MOVA), SH::R0);
  MachineInstr *Nop = BuildMI(MBB, DebugLoc(), TII->get(SH::NOP));
  MachineInstr *Branch = BuildMI(MBB, DebugLoc(), TII->get(SH::BRA));
  EXPECT_TRUE(TII->isDelaySlotCandidate(*Mova, SH2STI));
  EXPECT_TRUE(TII->isDelaySlotCandidate(*Nop, SH2STI));
  EXPECT_FALSE(TII->isDelaySlotCandidate(*Branch, SH2STI));

  Triple TT("sh-unknown-elf");
  std::string Error;
  const Target *Target = TargetRegistry::lookupTarget(TT, Error);
  ASSERT_NE(nullptr, Target) << Error;
  for (const char *CPU : {"sh1", "sh3", "sh4"}) {
    std::unique_ptr<TargetMachine> LocalTM(
        Target->createTargetMachine(TT, CPU, "", TargetOptions(), std::nullopt,
                                    std::nullopt, CodeGenOptLevel::Default));
    ASSERT_NE(nullptr, LocalTM);
    LLVMContext LocalContext;
    Module LocalModule("MOVA", LocalContext);
    LocalModule.setDataLayout(LocalTM->createDataLayout());
    Function *F = Function::Create(
        FunctionType::get(Type::getVoidTy(LocalContext), false),
        GlobalValue::ExternalLinkage, CPU, LocalModule);
    const auto *STI =
        static_cast<const SHSubtarget *>(LocalTM->getSubtargetImpl(*F));
    ASSERT_NE(nullptr, STI);
    EXPECT_EQ(StringRef(CPU) != "sh4",
              STI->getInstrInfo()->isLegalInDelaySlot(SH::MOVA, *STI));
    EXPECT_EQ(StringRef(CPU) != "sh4",
              STI->getInstrInfo()->isDelaySlotCandidate(*Mova, *STI));
    EXPECT_TRUE(STI->getInstrInfo()->isLegalInDelaySlot(SH::NOP, *STI));
    EXPECT_TRUE(STI->getInstrInfo()->isDelaySlotCandidate(*Nop, *STI));
  }

  for (auto [CPU, Features] : {std::pair{"sh4", "-sh4-debug-state"},
                               std::pair{"sh2", "+mova-slot-illegal"}}) {
    std::unique_ptr<TargetMachine> LocalTM(Target->createTargetMachine(
        TT, CPU, Features, TargetOptions(), std::nullopt, std::nullopt,
        CodeGenOptLevel::Default));
    ASSERT_NE(nullptr, LocalTM);
    LLVMContext LocalContext;
    Module LocalModule("MOVAFeatureComposition", LocalContext);
    LocalModule.setDataLayout(LocalTM->createDataLayout());
    Function *F = Function::Create(
        FunctionType::get(Type::getVoidTy(LocalContext), false),
        GlobalValue::ExternalLinkage, "composed", LocalModule);
    const auto *STI =
        static_cast<const SHSubtarget *>(LocalTM->getSubtargetImpl(*F));
    ASSERT_NE(nullptr, STI);
    EXPECT_FALSE(STI->getInstrInfo()->isLegalInDelaySlot(SH::MOVA, *STI));
  }

  std::unique_ptr<TargetMachine> ST40300TM(Target->createTargetMachine(
      TT, "sh4", "+st40-300", TargetOptions(), std::nullopt, std::nullopt,
      CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, ST40300TM);
  LLVMContext ST40300Context;
  Module ST40300Module("ST40300DelaySlots", ST40300Context);
  ST40300Module.setDataLayout(ST40300TM->createDataLayout());
  Function *ST40300Function = Function::Create(
      FunctionType::get(Type::getVoidTy(ST40300Context), false),
      GlobalValue::ExternalLinkage, "st40_300", ST40300Module);
  const auto *ST40300 = static_cast<const SHSubtarget *>(
      ST40300TM->getSubtargetImpl(*ST40300Function));
  ASSERT_NE(nullptr, ST40300);
  MachineInstr *ICBI =
      BuildMI(MBB, DebugLoc(), TII->get(SH::ICBI)).addReg(SH::R0);
  MachineInstr *SYNCO = BuildMI(MBB, DebugLoc(), TII->get(SH::SYNCO));
  MachineInstr *FPCHG = BuildMI(MBB, DebugLoc(), TII->get(SH::FPCHG));
  EXPECT_FALSE(ST40300->getInstrInfo()->isLegalInDelaySlot(SH::ICBI, *ST40300));
  EXPECT_TRUE(ST40300->getInstrInfo()->isLegalInDelaySlot(SH::SYNCO, *ST40300));
  EXPECT_TRUE(ST40300->getInstrInfo()->isLegalInDelaySlot(SH::FPCHG, *ST40300));
  EXPECT_FALSE(ST40300->getInstrInfo()->isDelaySlotCandidate(*ICBI, *ST40300));
  EXPECT_FALSE(ST40300->getInstrInfo()->isDelaySlotCandidate(*SYNCO, *ST40300));
  EXPECT_FALSE(ST40300->getInstrInfo()->isDelaySlotCandidate(*FPCHG, *ST40300));

  std::unique_ptr<TargetMachine> SH4ATM(
      Target->createTargetMachine(TT, "sh4a", "", TargetOptions(), std::nullopt,
                                  std::nullopt, CodeGenOptLevel::Default));
  ASSERT_NE(nullptr, SH4ATM);
  LLVMContext SH4AContext;
  Module SH4AModule("SH4ADelaySlots", SH4AContext);
  SH4AModule.setDataLayout(SH4ATM->createDataLayout());
  Function *SH4AFunction =
      Function::Create(FunctionType::get(Type::getVoidTy(SH4AContext), false),
                       GlobalValue::ExternalLinkage, "sh4a", SH4AModule);
  const auto *SH4A =
      static_cast<const SHSubtarget *>(SH4ATM->getSubtargetImpl(*SH4AFunction));
  ASSERT_NE(nullptr, SH4A);
  MachineInstr *PREFI =
      BuildMI(MBB, DebugLoc(), TII->get(SH::PREFI)).addReg(SH::R0);
  MachineInstr *MOVUA =
      BuildMI(MBB, DebugLoc(), TII->get(SH::MOVUA_L)).addReg(SH::R0);
  EXPECT_FALSE(SH4A->getInstrInfo()->isLegalInDelaySlot(SH::ICBI, *SH4A));
  EXPECT_FALSE(SH4A->getInstrInfo()->isLegalInDelaySlot(SH::PREFI, *SH4A));
  EXPECT_TRUE(SH4A->getInstrInfo()->isLegalInDelaySlot(SH::MOVUA_L, *SH4A));
  EXPECT_TRUE(SH4A->getInstrInfo()->isLegalInDelaySlot(SH::SYNCO, *SH4A));
  EXPECT_TRUE(SH4A->getInstrInfo()->isLegalInDelaySlot(SH::FPCHG, *SH4A));
  EXPECT_FALSE(SH4A->getInstrInfo()->isDelaySlotCandidate(*PREFI, *SH4A));
  EXPECT_TRUE(SH4A->getInstrInfo()->isDelaySlotCandidate(*MOVUA, *SH4A));
  EXPECT_FALSE(SH4A->getInstrInfo()->isDelaySlotCandidate(*SYNCO, *SH4A));
  EXPECT_FALSE(SH4A->getInstrInfo()->isDelaySlotCandidate(*FPCHG, *SH4A));
}

TEST_F(SHInstrInfoTest, ST40300InstructionsHavePreciseProperties) {
  auto CheckNoIntegerState = [&](const MCInstrDesc &Desc) {
    for (MCRegister Reg : {SH::TBit, SH::MACH, SH::MACL, SH::FPUL}) {
      EXPECT_FALSE(is_contained(Desc.implicit_uses(), Reg));
      EXPECT_FALSE(Desc.hasImplicitDefOfPhysReg(Reg));
    }
  };

  const MCInstrDesc &ICBI = TII->get(SH::ICBI);
  EXPECT_EQ(2u, ICBI.getSize());
  EXPECT_EQ(0u, ICBI.getNumDefs());
  EXPECT_EQ(1u, ICBI.getNumOperands());
  EXPECT_FALSE(ICBI.mayLoad());
  EXPECT_FALSE(ICBI.mayStore());
  EXPECT_TRUE(ICBI.hasUnmodeledSideEffects());
  EXPECT_TRUE(SHII::isDelaySlotIllegal(ICBI));
  EXPECT_TRUE(ICBI.implicit_uses().empty());
  EXPECT_TRUE(ICBI.implicit_defs().empty());
  CheckNoIntegerState(ICBI);

  const MCInstrDesc &SYNCO = TII->get(SH::SYNCO);
  EXPECT_EQ(2u, SYNCO.getSize());
  EXPECT_EQ(0u, SYNCO.getNumDefs());
  EXPECT_EQ(0u, SYNCO.getNumOperands());
  EXPECT_FALSE(SYNCO.mayLoad());
  EXPECT_FALSE(SYNCO.mayStore());
  EXPECT_TRUE(SYNCO.hasUnmodeledSideEffects());
  EXPECT_FALSE(SHII::isDelaySlotIllegal(SYNCO));
  EXPECT_TRUE(SYNCO.implicit_uses().empty());
  EXPECT_TRUE(SYNCO.implicit_defs().empty());
  CheckNoIntegerState(SYNCO);

  const MCInstrDesc &FPCHG = TII->get(SH::FPCHG);
  EXPECT_EQ(2u, FPCHG.getSize());
  EXPECT_EQ(0u, FPCHG.getNumDefs());
  EXPECT_EQ(0u, FPCHG.getNumOperands());
  EXPECT_FALSE(FPCHG.mayLoad());
  EXPECT_FALSE(FPCHG.mayStore());
  EXPECT_TRUE(FPCHG.hasUnmodeledSideEffects());
  EXPECT_FALSE(SHII::isDelaySlotIllegal(FPCHG));
  EXPECT_TRUE(is_contained(FPCHG.implicit_uses(), SH::FPSCR));
  EXPECT_TRUE(FPCHG.hasImplicitDefOfPhysReg(SH::FPSCR));
  EXPECT_TRUE(SHII::hasFPURequirement(FPCHG));
  CheckNoIntegerState(FPCHG);
}

TEST_F(SHInstrInfoTest, SH4AInstructionsHaveDescriptorOwnedSizesAndState) {
  for (unsigned Opcode :
       {SH::MOVCO_L, SH::MOVLI_L, SH::MOVUA_L, SH::MOVUA_L_POSTINC, SH::LDC_SGR,
        SH::LDC_L_SGR, SH::PREFI, SH::FSRRA, SH::FSCA})
    EXPECT_EQ(2u, TII->get(Opcode).getSize()) << TII->getName(Opcode);
  EXPECT_TRUE(TII->get(SH::MOVCO_L).mayStore());
  EXPECT_TRUE(TII->get(SH::MOVCO_L).hasImplicitDefOfPhysReg(SH::TBit));
  EXPECT_TRUE(TII->get(SH::MOVLI_L).mayLoad());
  EXPECT_TRUE(TII->get(SH::MOVLI_L).hasImplicitDefOfPhysReg(SH::R0));
  EXPECT_TRUE(TII->get(SH::MOVUA_L).mayLoad());
  EXPECT_TRUE(TII->get(SH::MOVUA_L_POSTINC).mayLoad());
  EXPECT_TRUE(TII->get(SH::LDC_SGR).hasImplicitDefOfPhysReg(SH::SGR));
  EXPECT_TRUE(TII->get(SH::LDC_L_SGR).hasImplicitDefOfPhysReg(SH::SGR));
  EXPECT_TRUE(TII->get(SH::LDC_L_SGR).mayLoad());
  EXPECT_TRUE(SHII::isDelaySlotIllegal(TII->get(SH::ICBI)));
  EXPECT_TRUE(SHII::isDelaySlotIllegal(TII->get(SH::LDC_SGR)));
  EXPECT_TRUE(SHII::isDelaySlotIllegal(TII->get(SH::LDC_L_SGR)));
  EXPECT_TRUE(SHII::isDelaySlotIllegal(TII->get(SH::PREFI)));
  EXPECT_FALSE(SHII::isDelaySlotIllegal(TII->get(SH::SYNCO)));
  for (unsigned Opcode : {SH::FSRRA, SH::FSCA, SH::FPCHG}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_TRUE(SHII::hasFPURequirement(Desc)) << TII->getName(Opcode);
    EXPECT_TRUE(is_contained(Desc.implicit_uses(), SH::FPSCR))
        << TII->getName(Opcode);
  }
  EXPECT_TRUE(is_contained(TII->get(SH::FSCA).implicit_uses(), SH::FPUL));
}

TEST_F(SHInstrInfoTest, SH2AInstructionSizesAndDelaySlotMetadataAreExact) {
  const unsigned WideOpcodes[] = {
      SH::MOVI20,
      SH::MOVI20S,
      SH::MOVB_LOAD_DISP12,
      SH::MOVUB_LOAD_DISP12,
      SH::MOVW_LOAD_DISP12,
      SH::MOVUW_LOAD_DISP12,
      SH::MOVL_LOAD_DISP12,
      SH::MOVB_STORE_DISP12,
      SH::MOVW_STORE_DISP12,
      SH::MOVL_STORE_DISP12,
      SH::BAND_B,
      SH::BANDNOT_B,
      SH::BOR_B,
      SH::BORNOT_B,
      SH::BXOR_B,
      SH::BLD_B,
      SH::BLDNOT_B,
      SH::BCLR_B,
      SH::BSET_B,
      SH::BST_B,
      SH::FMOV_S_LOAD_DISP12,
      SH::FMOV_S_STORE_DISP12,
      SH::FMOV_D_LOAD_DISP12,
      SH::FMOV_D_STORE_DISP12,
      SH::DSP_PPI,
  };
  for (unsigned Opcode : WideOpcodes) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_EQ(4u, Desc.getSize()) << TII->getName(Opcode);
    EXPECT_EQ(Opcode != SH::DSP_PPI, SHII::isDelaySlotIllegal(Desc))
        << TII->getName(Opcode);
  }

  unsigned WideInstructionCount = 0;
  for (unsigned Opcode = 0; Opcode != SH::INSTRUCTION_LIST_END; ++Opcode)
    if (TII->get(Opcode).getSize() == 4)
      ++WideInstructionCount;
  EXPECT_EQ(std::size(WideOpcodes), WideInstructionCount);
  for (unsigned Opcode : {SH::DIVS, SH::DIVU, SH::RESBANK}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_EQ(2u, Desc.getSize()) << TII->getName(Opcode);
    EXPECT_TRUE(SHII::isDelaySlotIllegal(Desc)) << TII->getName(Opcode);
  }
}

TEST_F(SHInstrInfoTest, FPUInstructionsHaveDeliberateStateRequirements) {
  unsigned Classified = 0;
  for (unsigned Opcode = 0; Opcode != SH::INSTRUCTION_LIST_END; ++Opcode) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    bool TouchesFPUState = is_contained(Desc.implicit_uses(), SH::FPSCR) ||
                           is_contained(Desc.implicit_uses(), SH::FPUL) ||
                           Desc.hasImplicitDefOfPhysReg(SH::FPSCR) ||
                           Desc.hasImplicitDefOfPhysReg(SH::FPUL);
    if (!TouchesFPUState)
      continue;
    EXPECT_TRUE(SHII::hasFPURequirement(Desc)) << TII->getName(Opcode);
    ++Classified;
  }
  EXPECT_GT(Classified, 0u);

  EXPECT_EQ(SHII::FPURequirement::PrimaryBank,
            SHII::getFPURequirement(TII->get(SH::FLDS)));
  EXPECT_EQ(SHII::FPURequirement::Single,
            SHII::getFPURequirement(TII->get(SH::FADD_FR)));
  EXPECT_EQ(SHII::FPURequirement::Double,
            SHII::getFPURequirement(TII->get(SH::FMOV_DR_DR)));
  EXPECT_EQ(SHII::FPURequirement::DoubleArithmetic,
            SHII::getFPURequirement(TII->get(SH::FADD_DR)));
  for (unsigned Opcode : {SH::FIPR, SH::FTRV, SH::FSCA, SH::FSRRA,
                          SH::FSCA_VALUE})
    EXPECT_EQ(SHII::FPURequirement::SingleArithmetic,
              SHII::getFPURequirement(TII->get(Opcode)))
        << TII->getName(Opcode);
  for (unsigned Opcode :
       {SH::XMTRX_LOAD, SH::XMTRX_STORE, SH::FTRV_MEM, SH::FTRV4_MEM,
        SH::FTRV_ARRAY, SH::FTRV_COMPOSE, SH::GRAPHICS_BEGIN, SH::GRAPHICS_END,
        SH::XMTRX_FROM_FVS, SH::FMOV_P_COPY_XD, SH::FMOV_V_LOAD_PAIR,
        SH::FMOV_V_STORE_PAIR, SH::FMOV_P_LOAD_DR, SH::FMOV_P_LOAD_XD,
        SH::FMOV_P_STORE_DR, SH::FMOV_P_STORE_XD})
    EXPECT_EQ(SHII::FPURequirement::PairedSingle,
              SHII::getFPURequirement(TII->get(Opcode)))
        << TII->getName(Opcode);
  EXPECT_EQ(SHII::FPURequirement::Single,
            SHII::getFPURequirement(TII->get(SH::GRAPHICS_SCALAR)));
  for (unsigned Opcode : {SH::FMAC_FR, SH::LDS_FPSCR})
    EXPECT_EQ(SHII::FPURequirement::None,
              SHII::getFPURequirement(TII->get(Opcode)))
        << TII->getName(Opcode);
}

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
  unsigned CFIIndex =
      MF->addFrameInst(MCCFIInstruction::cfiDefCfaOffset(nullptr, 4));
  MachineInstr *CFI =
      BuildMI(Standalone, DebugLoc(), TII->get(TargetOpcode::CFI_INSTRUCTION))
          .addCFIIndex(CFIIndex);

  EXPECT_EQ(2u, TII->getInstSizeInBytes(*Nop));
  EXPECT_EQ(2u, TII->getInstSizeInBytes(*Add));
  EXPECT_EQ(0u, TII->getInstSizeInBytes(*CFI));
  EXPECT_FALSE(CFI->getDebugLoc());
  EXPECT_FALSE(CFI->isBundledWithPred());
  EXPECT_FALSE(CFI->isBundledWithSucc());
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
  EXPECT_EQ(4u, TII->getInstSizeInBytes(*Bra));
  finalizeBundle(*BraBundleBlock, Bra->getIterator(),
                 std::next(Nop->getIterator()));
  MachineInstr &BraBundle = BraBundleBlock->front();
  EXPECT_EQ(TargetOpcode::BUNDLE, BraBundle.getOpcode());
  EXPECT_EQ(4u, TII->getInstSizeInBytes(BraBundle));

  MachineBasicBlock *WideBundleBlock = createBlock();
  Bra = BuildMI(WideBundleBlock, DebugLoc(), TII->get(SH::BRA)).addMBB(Target);
  MachineInstr *Wide =
      BuildMI(WideBundleBlock, DebugLoc(), TII->get(SH::MOVI20), SH::R0)
          .addImm(1);
  MIBundleBuilder(*WideBundleBlock, Bra->getIterator(),
                  std::next(Wide->getIterator()));
  finalizeBundle(*WideBundleBlock, Bra->getIterator(),
                 std::next(Wide->getIterator()));
  EXPECT_EQ(6u, TII->getInstSizeInBytes(WideBundleBlock->front()));

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

TEST_F(SHInstrInfoTest, AtomicPrimitivesHavePreciseProperties) {
  const MCInstrDesc &Tas = TII->get(SH::TAS_B);
  EXPECT_EQ(2u, Tas.getSize());
  EXPECT_EQ(0u, Tas.getNumDefs());
  EXPECT_EQ(1u, Tas.getNumOperands());
  EXPECT_TRUE(Tas.mayLoad());
  EXPECT_TRUE(Tas.mayStore());
  EXPECT_FALSE(Tas.hasDelaySlot());
  EXPECT_FALSE(Tas.isCall());
  EXPECT_FALSE(Tas.isBranch());
  EXPECT_TRUE(llvm::is_contained(Tas.implicit_defs(), SH::TBit));
  EXPECT_TRUE(Tas.implicit_uses().empty());

  const MCInstrDesc &Movt = TII->get(SH::MOVT);
  EXPECT_EQ(2u, Movt.getSize());
  EXPECT_EQ(1u, Movt.getNumDefs());
  EXPECT_EQ(1u, Movt.getNumOperands());
  EXPECT_FALSE(Movt.mayLoad());
  EXPECT_FALSE(Movt.mayStore());
  EXPECT_FALSE(Movt.hasDelaySlot());
  EXPECT_TRUE(llvm::is_contained(Movt.implicit_uses(), SH::TBit));
  EXPECT_TRUE(Movt.implicit_defs().empty());
}

TEST_F(SHInstrInfoTest, CompletedSH2StateAndMemoryPropertiesArePrecise) {
  const MCInstrDesc &Mac = TII->get(SH::MAC_L);
  EXPECT_TRUE(Mac.mayLoad());
  EXPECT_FALSE(Mac.mayStore());
  for (MCRegister Reg : {SH::SR, SH::MACH, SH::MACL})
    EXPECT_TRUE(llvm::is_contained(Mac.implicit_uses(), Reg));
  for (MCRegister Reg : {SH::MACH, SH::MACL})
    EXPECT_TRUE(llvm::is_contained(Mac.implicit_defs(), Reg));

  const MCInstrDesc &Div1 = TII->get(SH::DIV1);
  for (MCRegister Reg : {SH::MBit, SH::QBit, SH::TBit})
    EXPECT_TRUE(llvm::is_contained(Div1.implicit_uses(), Reg));
  EXPECT_FALSE(llvm::is_contained(Div1.implicit_defs(), SH::MBit));
  for (MCRegister Reg : {SH::QBit, SH::TBit})
    EXPECT_TRUE(llvm::is_contained(Div1.implicit_defs(), Reg));

  const MCInstrDesc &GBRLogic = TII->get(SH::AND_B_GBR);
  EXPECT_TRUE(GBRLogic.mayLoad());
  EXPECT_TRUE(GBRLogic.mayStore());
  EXPECT_TRUE(llvm::is_contained(GBRLogic.implicit_uses(), SH::R0));
  EXPECT_TRUE(llvm::is_contained(GBRLogic.implicit_uses(), SH::GBR));
  EXPECT_FALSE(llvm::is_contained(GBRLogic.implicit_defs(), SH::R0));

  const MCInstrDesc &IndexedLoad = TII->get(SH::MOVB_load_indexed);
  const MCInstrDesc &IndexedStore = TII->get(SH::MOVW_store_indexed);
  EXPECT_TRUE(IndexedLoad.mayLoad());
  EXPECT_FALSE(IndexedLoad.mayStore());
  EXPECT_TRUE(llvm::is_contained(IndexedLoad.implicit_uses(), SH::R0));
  EXPECT_FALSE(IndexedStore.mayLoad());
  EXPECT_TRUE(IndexedStore.mayStore());
  EXPECT_TRUE(llvm::is_contained(IndexedStore.implicit_uses(), SH::R0));
}

TEST_F(SHInstrInfoTest, CompletedSH2ControlPropertiesArePrecise) {
  MachineBasicBlock *MBB = createBlock();
  for (unsigned Opcode : {SH::BT_S, SH::BF_S, SH::BRAF, SH::BSRF, SH::RTE}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_TRUE(Desc.hasDelaySlot()) << TII->getName(Opcode);
    MachineInstr *MI = BuildMI(MBB, DebugLoc(), Desc);
    EXPECT_EQ(4u, TII->getInstSizeInBytes(*MI)) << TII->getName(Opcode);
  }

  const MCInstrDesc &Rte = TII->get(SH::RTE);
  EXPECT_TRUE(Rte.isReturn());
  EXPECT_TRUE(Rte.isTerminator());
  EXPECT_TRUE(Rte.isBarrier());
  EXPECT_TRUE(Rte.mayLoad());
  EXPECT_FALSE(Rte.mayStore());
  EXPECT_TRUE(llvm::is_contained(Rte.implicit_uses(), SH::R15));
  for (MCRegister Reg : {SH::R15, SH::SR, SH::MBit, SH::QBit, SH::TBit})
    EXPECT_TRUE(llvm::is_contained(Rte.implicit_defs(), Reg));

  const MCInstrDesc &Trapa = TII->get(SH::TRAPA);
  EXPECT_TRUE(Trapa.isCall());
  EXPECT_FALSE(Trapa.hasDelaySlot());
  EXPECT_TRUE(Trapa.mayLoad());
  EXPECT_TRUE(Trapa.mayStore());
  EXPECT_TRUE(Trapa.hasUnmodeledSideEffects());
  for (MCRegister Reg :
       {SH::R15, SH::SR, SH::MBit, SH::QBit, SH::TBit, SH::VBR})
    EXPECT_TRUE(llvm::is_contained(Trapa.implicit_uses(), Reg));
  EXPECT_TRUE(llvm::is_contained(Trapa.implicit_defs(), SH::R15));
}

TEST_F(SHInstrInfoTest, CompletedSHTBitDependenciesAreExact) {
  auto OpcodesWithTBit = [&](bool Definitions) {
    SmallVector<unsigned> Result;
    for (unsigned Opcode = 0; Opcode != SH::INSTRUCTION_LIST_END; ++Opcode) {
      const MCInstrDesc &Desc = TII->get(Opcode);
      if (Desc.isPseudo())
        continue;
      ArrayRef<MCPhysReg> Registers =
          Definitions ? Desc.implicit_defs() : Desc.implicit_uses();
      if (llvm::is_contained(Registers, SH::TBit))
        Result.push_back(Opcode);
    }
    llvm::sort(Result);
    return Result;
  };

  SmallVector<unsigned> ExpectedUses = {
      SH::TRAPA,     SH::TRAP_EXIT_RETURN,
      SH::MOVT,      SH::STC_SR,
      SH::DIV1,      SH::ROTCL,
      SH::ROTCR,     SH::ADDC,
      SH::SUBC,      SH::NEGC,
      SH::BT,        SH::BF,
      SH::BT_S,      SH::BF_S,
      SH::STC_L_SR,  SH::MOVRT,
      SH::NOTT,      SH::BAND_B,
      SH::BANDNOT_B, SH::BOR_B,
      SH::BORNOT_B,  SH::BXOR_B,
      SH::BST_B,     SH::BST,
      SH::DSP_PPI,
  };
  SmallVector<unsigned> ExpectedDefs = {
      SH::CLRT,       SH::SETT,       SH::RTE,        SH::TAS_B,
      SH::MOVCO_L,    SH::LDC_SR,     SH::CMP_EQri,   SH::TSTri,
      SH::CMP_PL,     SH::CMP_PZ,     SH::CMP_STR,    SH::DT,
      SH::SHLL,       SH::SHLR,       SH::SHAR,       SH::SHAL,
      SH::DIV0U,      SH::DIV0S,      SH::DIV1,       SH::ROTCL,
      SH::ROTCR,      SH::ROTL,       SH::ROTR,       SH::ADDC,
      SH::SUBC,       SH::NEGC,       SH::ADDV,       SH::SUBV,
      SH::CMP_EQ,     SH::CMP_HS,     SH::CMP_GE,     SH::CMP_HI,
      SH::CMP_GT,     SH::TST,        SH::LDC_L_SR,   SH::TST_B_GBR,
      SH::FCMP_EQ_FR, SH::FCMP_GT_FR, SH::FCMP_EQ_DR, SH::FCMP_GT_DR,
      SH::NOTT,       SH::BAND_B,     SH::BANDNOT_B,  SH::BOR_B,
      SH::BORNOT_B,   SH::BXOR_B,     SH::BLD_B,      SH::BLDNOT_B,
      SH::BLD,        SH::DSP_PPI,
  };
  llvm::sort(ExpectedUses);
  llvm::sort(ExpectedDefs);
  EXPECT_EQ(ExpectedUses, OpcodesWithTBit(false));
  EXPECT_EQ(ExpectedDefs, OpcodesWithTBit(true));
}

TEST_F(SHInstrInfoTest, SBitDependenciesAreExact) {
  auto OpcodesWithSBit = [&](bool Definitions) {
    SmallVector<unsigned> Result;
    for (unsigned Opcode = 0; Opcode != SH::INSTRUCTION_LIST_END; ++Opcode) {
      const MCInstrDesc &Desc = TII->get(Opcode);
      if (Desc.isPseudo())
        continue;
      ArrayRef<MCPhysReg> Registers =
          Definitions ? Desc.implicit_defs() : Desc.implicit_uses();
      if (llvm::is_contained(Registers, SH::SBit))
        Result.push_back(Opcode);
    }
    llvm::sort(Result);
    return Result;
  };

  SmallVector<unsigned> ExpectedUses = {
      SH::TRAPA, SH::TRAP_EXIT_RETURN, SH::MAC_L,
      SH::MAC_W, SH::STC_SR,           SH::STC_L_SR,
  };
  SmallVector<unsigned> ExpectedDefs = {
      SH::CLRS, SH::SETS, SH::RTE, SH::LDC_SR, SH::LDC_L_SR,
  };
  llvm::sort(ExpectedUses);
  llvm::sort(ExpectedDefs);
  EXPECT_EQ(ExpectedUses, OpcodesWithSBit(false));
  EXPECT_EQ(ExpectedDefs, OpcodesWithSBit(true));
}

TEST_F(SHInstrInfoTest, CompletedSH2DelaySlotSetIsExact) {
  SmallVector<unsigned> Actual;
  for (unsigned Opcode = 0; Opcode != SH::INSTRUCTION_LIST_END; ++Opcode) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    if (!Desc.isPseudo() && Desc.hasDelaySlot())
      Actual.push_back(Opcode);
  }
  SmallVector<unsigned> Expected = {
      SH::RTE, SH::BT_S, SH::BF_S, SH::BRA, SH::BRAF,     SH::BSR,
      SH::JMP, SH::JSR,  SH::BSRF, SH::RTS, SH::TAIL_JMP,
  };
  llvm::sort(Actual);
  llvm::sort(Expected);
  EXPECT_EQ(Expected, Actual);
}

TEST_F(SHInstrInfoTest, IndirectCallMetadata) {
  const MCInstrDesc &Ordinary = TII->get(SH::JSR);
  EXPECT_TRUE(Ordinary.isCall());
  EXPECT_TRUE(Ordinary.hasDelaySlot());
  EXPECT_FALSE(Ordinary.isReturn());
  EXPECT_EQ(2u, Ordinary.getSize());
  EXPECT_FALSE(Ordinary.mayLoad());
  EXPECT_FALSE(Ordinary.mayStore());
  EXPECT_TRUE(llvm::is_contained(Ordinary.implicit_defs(), SH::PR));
  MachineBasicBlock *MBB = createBlock();
  MachineInstr *Call =
      BuildMI(*MBB, MBB->end(), DebugLoc(), Ordinary).addReg(SH::R2);
  EXPECT_EQ(4u, TII->getInstSizeInBytes(*Call));
}

TEST_F(SHInstrInfoTest, CompletedSH2MemoryEffectsArePrecise) {
  for (unsigned Opcode : {
           SH::RTE,
           SH::LDC_L_SR,
           SH::LDC_L_GBR,
           SH::LDC_L_VBR,
           SH::LDS_L_MACH,
           SH::LDS_L_MACL,
           SH::MOVB_load_postinc,
           SH::MOVW_load_postinc,
           SH::MOVL_load_postinc,
           SH::MOVB_load_indexed,
           SH::MOVW_load_indexed,
           SH::MOVW_load_pc,
           SH::MOVB_load_gbr,
           SH::MOVW_load_gbr,
           SH::MOVL_load_gbr,
           SH::TST_B_GBR,
           SH::MAC_L,
           SH::MAC_W,
       }) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_TRUE(Desc.mayLoad()) << TII->getName(Opcode);
    EXPECT_FALSE(Desc.mayStore()) << TII->getName(Opcode);
  }
  for (unsigned Opcode : {
           SH::STC_L_SR,
           SH::STC_L_GBR,
           SH::STC_L_VBR,
           SH::STS_L_MACH,
           SH::STS_L_MACL,
           SH::MOVB_store_predec,
           SH::MOVW_store_predec,
           SH::MOVL_store_predec,
           SH::MOVB_store_indexed,
           SH::MOVW_store_indexed,
           SH::MOVL_store_indexed,
           SH::MOVB_store_gbr,
           SH::MOVW_store_gbr,
           SH::MOVL_store_gbr,
       }) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_FALSE(Desc.mayLoad()) << TII->getName(Opcode);
    EXPECT_TRUE(Desc.mayStore()) << TII->getName(Opcode);
  }
  for (unsigned Opcode :
       {SH::TRAPA, SH::TAS_B, SH::AND_B_GBR, SH::OR_B_GBR, SH::XOR_B_GBR}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_TRUE(Desc.mayLoad()) << TII->getName(Opcode);
    EXPECT_TRUE(Desc.mayStore()) << TII->getName(Opcode);
  }
}

TEST_F(SHInstrInfoTest, CompletedSH2SpecialRegisterTransfersArePrecise) {
  struct Transfer {
    unsigned Opcode;
    MCRegister State;
    bool ReadsState;
    bool MayLoad;
    bool MayStore;
  };
  const Transfer Transfers[] = {
      {SH::STC_SR, SH::SR, true, false, false},
      {SH::STC_GBR, SH::GBR, true, false, false},
      {SH::STC_VBR, SH::VBR, true, false, false},
      {SH::STC_L_SR, SH::SR, true, false, true},
      {SH::STC_L_GBR, SH::GBR, true, false, true},
      {SH::STC_L_VBR, SH::VBR, true, false, true},
      {SH::LDC_SR, SH::SR, false, false, false},
      {SH::LDC_GBR, SH::GBR, false, false, false},
      {SH::LDC_VBR, SH::VBR, false, false, false},
      {SH::LDC_L_SR, SH::SR, false, true, false},
      {SH::LDC_L_GBR, SH::GBR, false, true, false},
      {SH::LDC_L_VBR, SH::VBR, false, true, false},
      {SH::STS_MACH, SH::MACH, true, false, false},
      {SH::STS_MACL, SH::MACL, true, false, false},
      {SH::STS_PR, SH::PR, true, false, false},
      {SH::STS_L_MACH, SH::MACH, true, false, true},
      {SH::STS_L_MACL, SH::MACL, true, false, true},
      {SH::STS_L_PR, SH::PR, true, false, true},
      {SH::LDS_MACH, SH::MACH, false, false, false},
      {SH::LDS_MACL, SH::MACL, false, false, false},
      {SH::LDS_PR, SH::PR, false, false, false},
      {SH::LDS_L_MACH, SH::MACH, false, true, false},
      {SH::LDS_L_MACL, SH::MACL, false, true, false},
      {SH::LDS_L_PR, SH::PR, false, true, false},
  };
  for (const Transfer &Item : Transfers) {
    const MCInstrDesc &Desc = TII->get(Item.Opcode);
    EXPECT_EQ(Item.MayLoad, Desc.mayLoad()) << TII->getName(Item.Opcode);
    EXPECT_EQ(Item.MayStore, Desc.mayStore()) << TII->getName(Item.Opcode);
    EXPECT_EQ(Item.ReadsState,
              llvm::is_contained(Desc.implicit_uses(), Item.State))
        << TII->getName(Item.Opcode);
    EXPECT_EQ(!Item.ReadsState,
              llvm::is_contained(Desc.implicit_defs(), Item.State))
        << TII->getName(Item.Opcode);
  }
  for (unsigned Opcode : {SH::STC_SR, SH::STC_L_SR})
    for (MCRegister Reg : {SH::MBit, SH::QBit, SH::TBit})
      EXPECT_TRUE(llvm::is_contained(TII->get(Opcode).implicit_uses(), Reg));
  for (unsigned Opcode : {SH::LDC_SR, SH::LDC_L_SR})
    for (MCRegister Reg : {SH::MBit, SH::QBit, SH::TBit})
      EXPECT_TRUE(TII->get(Opcode).hasImplicitDefOfPhysReg(Reg));
}

TEST_F(SHInstrInfoTest, GenericAtomicsAreNeverNativelyLockFree) {
  const SHSubtarget &ST = MF->getSubtarget<SHSubtarget>();
  const TargetLowering *TLI = ST.getTargetLowering();
  ASSERT_NE(nullptr, TLI);
  EXPECT_EQ(0u, TLI->getMaxAtomicSizeInBitsSupported());

  const std::pair<RTLIB::Libcall, RTLIB::LibcallImpl> Supported[] = {
      {RTLIB::ATOMIC_LOAD, RTLIB::impl___atomic_load},
      {RTLIB::ATOMIC_STORE, RTLIB::impl___atomic_store},
      {RTLIB::ATOMIC_EXCHANGE, RTLIB::impl___atomic_exchange},
      {RTLIB::ATOMIC_COMPARE_EXCHANGE, RTLIB::impl___atomic_compare_exchange},
      {RTLIB::ATOMIC_LOAD_1, RTLIB::impl___atomic_load_1},
      {RTLIB::ATOMIC_LOAD_2, RTLIB::impl___atomic_load_2},
      {RTLIB::ATOMIC_LOAD_4, RTLIB::impl___atomic_load_4},
      {RTLIB::ATOMIC_LOAD_8, RTLIB::impl___atomic_load_8},
      {RTLIB::ATOMIC_STORE_1, RTLIB::impl___atomic_store_1},
      {RTLIB::ATOMIC_STORE_2, RTLIB::impl___atomic_store_2},
      {RTLIB::ATOMIC_STORE_4, RTLIB::impl___atomic_store_4},
      {RTLIB::ATOMIC_STORE_8, RTLIB::impl___atomic_store_8},
      {RTLIB::ATOMIC_EXCHANGE_1, RTLIB::impl___atomic_exchange_1},
      {RTLIB::ATOMIC_EXCHANGE_2, RTLIB::impl___atomic_exchange_2},
      {RTLIB::ATOMIC_EXCHANGE_4, RTLIB::impl___atomic_exchange_4},
      {RTLIB::ATOMIC_EXCHANGE_8, RTLIB::impl___atomic_exchange_8},
      {RTLIB::ATOMIC_COMPARE_EXCHANGE_1,
       RTLIB::impl___atomic_compare_exchange_1},
      {RTLIB::ATOMIC_COMPARE_EXCHANGE_2,
       RTLIB::impl___atomic_compare_exchange_2},
      {RTLIB::ATOMIC_COMPARE_EXCHANGE_4,
       RTLIB::impl___atomic_compare_exchange_4},
      {RTLIB::ATOMIC_COMPARE_EXCHANGE_8,
       RTLIB::impl___atomic_compare_exchange_8},
      {RTLIB::ATOMIC_FETCH_ADD_1, RTLIB::impl___atomic_fetch_add_1},
      {RTLIB::ATOMIC_FETCH_ADD_2, RTLIB::impl___atomic_fetch_add_2},
      {RTLIB::ATOMIC_FETCH_ADD_4, RTLIB::impl___atomic_fetch_add_4},
      {RTLIB::ATOMIC_FETCH_ADD_8, RTLIB::impl___atomic_fetch_add_8},
      {RTLIB::ATOMIC_FETCH_SUB_1, RTLIB::impl___atomic_fetch_sub_1},
      {RTLIB::ATOMIC_FETCH_SUB_2, RTLIB::impl___atomic_fetch_sub_2},
      {RTLIB::ATOMIC_FETCH_SUB_4, RTLIB::impl___atomic_fetch_sub_4},
      {RTLIB::ATOMIC_FETCH_SUB_8, RTLIB::impl___atomic_fetch_sub_8},
      {RTLIB::ATOMIC_FETCH_AND_1, RTLIB::impl___atomic_fetch_and_1},
      {RTLIB::ATOMIC_FETCH_AND_2, RTLIB::impl___atomic_fetch_and_2},
      {RTLIB::ATOMIC_FETCH_AND_4, RTLIB::impl___atomic_fetch_and_4},
      {RTLIB::ATOMIC_FETCH_AND_8, RTLIB::impl___atomic_fetch_and_8},
      {RTLIB::ATOMIC_FETCH_OR_1, RTLIB::impl___atomic_fetch_or_1},
      {RTLIB::ATOMIC_FETCH_OR_2, RTLIB::impl___atomic_fetch_or_2},
      {RTLIB::ATOMIC_FETCH_OR_4, RTLIB::impl___atomic_fetch_or_4},
      {RTLIB::ATOMIC_FETCH_OR_8, RTLIB::impl___atomic_fetch_or_8},
      {RTLIB::ATOMIC_FETCH_XOR_1, RTLIB::impl___atomic_fetch_xor_1},
      {RTLIB::ATOMIC_FETCH_XOR_2, RTLIB::impl___atomic_fetch_xor_2},
      {RTLIB::ATOMIC_FETCH_XOR_4, RTLIB::impl___atomic_fetch_xor_4},
      {RTLIB::ATOMIC_FETCH_XOR_8, RTLIB::impl___atomic_fetch_xor_8},
      {RTLIB::ATOMIC_FETCH_NAND_1, RTLIB::impl___atomic_fetch_nand_1},
      {RTLIB::ATOMIC_FETCH_NAND_2, RTLIB::impl___atomic_fetch_nand_2},
      {RTLIB::ATOMIC_FETCH_NAND_4, RTLIB::impl___atomic_fetch_nand_4},
      {RTLIB::ATOMIC_FETCH_NAND_8, RTLIB::impl___atomic_fetch_nand_8},
  };
  for (auto [Call, Impl] : Supported)
    EXPECT_EQ(Impl, TLI->getLibcallImpl(Call));

  const RTLIB::Libcall Unsupported[] = {
      RTLIB::ATOMIC_LOAD_16,      RTLIB::ATOMIC_STORE_16,
      RTLIB::ATOMIC_EXCHANGE_16,  RTLIB::ATOMIC_COMPARE_EXCHANGE_16,
      RTLIB::ATOMIC_FETCH_ADD_16, RTLIB::ATOMIC_FETCH_SUB_16,
      RTLIB::ATOMIC_FETCH_AND_16, RTLIB::ATOMIC_FETCH_OR_16,
      RTLIB::ATOMIC_FETCH_XOR_16, RTLIB::ATOMIC_FETCH_NAND_16,
  };
  for (RTLIB::Libcall Call : Unsupported)
    EXPECT_EQ(RTLIB::Unsupported, TLI->getLibcallImpl(Call));
}

TEST_F(SHInstrInfoTest, VariableShiftLibcallsUseTargetRuntimeMapping) {
  const TargetLowering *TLI =
      MF->getSubtarget<SHSubtarget>().getTargetLowering();
  ASSERT_NE(nullptr, TLI);
  EXPECT_EQ(RTLIB::impl___ashlsi3, TLI->getLibcallImpl(RTLIB::SHL_I32));
  EXPECT_EQ(RTLIB::impl___lshrsi3, TLI->getLibcallImpl(RTLIB::SRL_I32));
  EXPECT_EQ(RTLIB::impl___ashrsi3, TLI->getLibcallImpl(RTLIB::SRA_I32));
}

TEST_F(SHInstrInfoTest, ArithmeticShiftLibcallClobbersCountRegister) {
  const SHRegisterInfo &TRI = TII->getRegisterInfo();
  const uint32_t *ShiftMask = TRI.getShiftLibcallPreservedMask(false);
  const uint32_t *AShrMask = TRI.getShiftLibcallPreservedMask(true);
  EXPECT_FALSE(MachineOperand::clobbersPhysReg(ShiftMask, SH::R5));
  EXPECT_TRUE(MachineOperand::clobbersPhysReg(AShrMask, SH::R5));
  // The contracts differ only in r5, including all register aliases.
  for (unsigned Reg = 1; Reg != TRI.getNumRegs(); ++Reg) {
    if (Reg != SH::R5)
      EXPECT_EQ(MachineOperand::clobbersPhysReg(ShiftMask, Reg),
                MachineOperand::clobbersPhysReg(AShrMask, Reg))
          << TRI.getName(Reg);
  }
  for (MCRegister Reg : {SH::R0, SH::PR, SH::TBit})
    EXPECT_TRUE(MachineOperand::clobbersPhysReg(AShrMask, Reg));
  for (MCRegister Reg :
       {SH::R1,   SH::R2,    SH::R3,   SH::R4,   SH::R6,  SH::R7,
        SH::R8,   SH::R9,    SH::R10,  SH::R11,  SH::R12, SH::R13,
        SH::R14,  SH::R15,   SH::FR0,  SH::FR15, SH::XF0, SH::XF15,
        SH::FPUL, SH::FPSCR, SH::MACH, SH::MACL})
    EXPECT_FALSE(MachineOperand::clobbersPhysReg(AShrMask, Reg));
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
       {SH::R0,      SH::R1,      SH::R2,      SH::R3,      SH::R4,
        SH::R5,      SH::R6,      SH::R7,      SH::PC,      SH::PR,
        SH::TBit,    SH::MBit,    SH::QBit,    SH::SBit,    SH::MACH,
        SH::MACL,    SH::SSR,     SH::SPC,     SH::SGR,     SH::DBR,
        SH::R0_BANK, SH::R1_BANK, SH::R2_BANK, SH::R3_BANK, SH::R4_BANK,
        SH::R5_BANK, SH::R6_BANK, SH::R7_BANK, SH::FPUL,    SH::FPSCR,
        SH::FR0,     SH::FR15,    SH::XF0,     SH::XF15,    SH::DR0,
        SH::DR14,    SH::XD0,     SH::XD14,    SH::FV0,     SH::FV12,
        SH::XMTRX})
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

TEST_F(SHInstrInfoTest, MOVAHasPreciseProperties) {
  const MCInstrDesc &Desc = TII->get(SH::MOVA);
  EXPECT_EQ(2u, Desc.getSize());
  EXPECT_EQ(1u, Desc.getNumDefs());
  EXPECT_EQ(2u, Desc.getNumOperands());
  EXPECT_FALSE(Desc.mayLoad());
  EXPECT_FALSE(Desc.mayStore());
  EXPECT_FALSE(Desc.isBranch());
  EXPECT_FALSE(Desc.isCall());
  EXPECT_FALSE(Desc.hasDelaySlot());
  EXPECT_TRUE(Desc.implicit_uses().empty());
  EXPECT_TRUE(Desc.implicit_defs().empty());
}

TEST_F(SHInstrInfoTest, TLSInstructionsHavePreciseProperties) {
  const MCInstrDesc &ThreadPointer = TII->get(SH::STC_GBR);
  EXPECT_EQ(2u, ThreadPointer.getSize());
  EXPECT_EQ(1u, ThreadPointer.getNumDefs());
  EXPECT_EQ(1u, ThreadPointer.getNumOperands());
  EXPECT_FALSE(ThreadPointer.mayLoad());
  EXPECT_FALSE(ThreadPointer.mayStore());
  EXPECT_FALSE(ThreadPointer.isBranch());
  EXPECT_FALSE(ThreadPointer.isCall());
  EXPECT_FALSE(ThreadPointer.hasDelaySlot());
  EXPECT_TRUE(is_contained(ThreadPointer.implicit_uses(), SH::GBR));
  EXPECT_TRUE(ThreadPointer.implicit_defs().empty());
  for (MCRegister Reg :
       {SH::PR, SH::TBit, SH::MBit, SH::QBit, SH::MACH, SH::MACL})
    EXPECT_FALSE(ThreadPointer.hasImplicitDefOfPhysReg(Reg));

  const MCInstrDesc &IndexedLoad = TII->get(SH::MOVL_load_indexed);
  EXPECT_EQ(2u, IndexedLoad.getSize());
  EXPECT_EQ(1u, IndexedLoad.getNumDefs());
  EXPECT_EQ(2u, IndexedLoad.getNumOperands());
  EXPECT_TRUE(IndexedLoad.mayLoad());
  EXPECT_FALSE(IndexedLoad.mayStore());
  EXPECT_FALSE(IndexedLoad.isBranch());
  EXPECT_FALSE(IndexedLoad.isCall());
  EXPECT_FALSE(IndexedLoad.hasDelaySlot());
  EXPECT_TRUE(is_contained(IndexedLoad.implicit_uses(), SH::R0));
  EXPECT_TRUE(IndexedLoad.implicit_defs().empty());
}

TEST_F(SHInstrInfoTest, PICConstantPoolIdentityIncludesModifierAndAddend) {
  const GlobalValue *GV = M->getFunction("test");
  std::unique_ptr<SHConstantPoolValue> GOT(
      SHConstantPoolValue::create(GV, 0, SHConstantPoolValue::Modifier::GOT));
  std::unique_ptr<SHConstantPoolValue> GOTOFF(SHConstantPoolValue::create(
      GV, 0, SHConstantPoolValue::Modifier::GOTOFF));
  std::unique_ptr<SHConstantPoolValue> PLT(
      SHConstantPoolValue::create(GV, 0, SHConstantPoolValue::Modifier::PLT));
  std::unique_ptr<SHConstantPoolValue> GOTWithAddend(
      SHConstantPoolValue::create(GV, 4, SHConstantPoolValue::Modifier::GOT));
  std::unique_ptr<SHConstantPoolValue> SameGOT(
      SHConstantPoolValue::create(GV, 0, SHConstantPoolValue::Modifier::GOT));

  EXPECT_TRUE(GOT->equals(*SameGOT));
  EXPECT_FALSE(GOT->equals(*GOTOFF));
  EXPECT_FALSE(GOT->equals(*PLT));
  EXPECT_FALSE(GOT->equals(*GOTWithAddend));

  std::unique_ptr<SHConstantPoolValue> TLSValues[] = {
      std::unique_ptr<SHConstantPoolValue>(SHConstantPoolValue::create(
          GV, 0, SHConstantPoolValue::Modifier::TLSGD)),
      std::unique_ptr<SHConstantPoolValue>(SHConstantPoolValue::create(
          GV, 0, SHConstantPoolValue::Modifier::TLSLDM)),
      std::unique_ptr<SHConstantPoolValue>(SHConstantPoolValue::create(
          GV, 0, SHConstantPoolValue::Modifier::DTPOFF)),
      std::unique_ptr<SHConstantPoolValue>(SHConstantPoolValue::create(
          GV, 0, SHConstantPoolValue::Modifier::GOTTPOFF)),
      std::unique_ptr<SHConstantPoolValue>(SHConstantPoolValue::create(
          GV, 0, SHConstantPoolValue::Modifier::TPOFF))};
  for (unsigned LHS = 0; LHS != std::size(TLSValues); ++LHS)
    for (unsigned RHS = 0; RHS != std::size(TLSValues); ++RHS)
      EXPECT_EQ(LHS == RHS, TLSValues[LHS]->equals(*TLSValues[RHS]));

  std::unique_ptr<SHConstantPoolValue> SameTLSGD(
      SHConstantPoolValue::create(GV, 0, SHConstantPoolValue::Modifier::TLSGD));
  std::unique_ptr<SHConstantPoolValue> DTPOFFWithAddend(
      SHConstantPoolValue::create(GV, 4,
                                  SHConstantPoolValue::Modifier::DTPOFF));
  EXPECT_TRUE(TLSValues[0]->equals(*SameTLSGD));
  EXPECT_FALSE(TLSValues[2]->equals(*DTPOFFWithAddend));
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

static uint32_t executeDynamicShift(unsigned Opcode, uint32_t Value,
                                    int32_t Count) {
  if (Count >= 0)
    return Value << (static_cast<uint32_t>(Count) & 31);
  unsigned Amount = ((~static_cast<uint32_t>(Count)) & 31) + 1;
  if (Amount == 32)
    return Opcode == SH::SHAD && (Value & 0x80000000) ? 0xffffffff : 0;
  if (Opcode == SH::SHLD)
    return Value >> Amount;
  for (unsigned Index = 0; Index != Amount; ++Index)
    Value = (Value >> 1) | (Value & 0x80000000);
  return Value;
}

TEST_F(SHInstrInfoTest, DynamicShiftPlansCoverEveryDefinedAmount) {
  constexpr uint32_t Values[] = {
      0, 1, 0xffffffff, 0x80000000, 0x5aa5c33c, 0xdeadbeef,
  };
  for (unsigned Amount = 0; Amount != 32; ++Amount) {
    for (unsigned Pseudo : {SH::SHLrr, SH::SRLrr, SH::SRArr}) {
      SH::DynamicShiftPlan Plan = SH::planDynamicShift(Pseudo);
      int32_t Count = Plan.NegateCount ? -static_cast<int32_t>(Amount)
                                       : static_cast<int32_t>(Amount);
      for (uint32_t Value : Values) {
        uint32_t Expected;
        if (Pseudo == SH::SHLrr)
          Expected = Value << Amount;
        else if (Pseudo == SH::SRLrr)
          Expected = Value >> Amount;
        else {
          Expected = Value;
          for (unsigned Index = 0; Index != Amount; ++Index)
            Expected = (Expected >> 1) | (Expected & 0x80000000);
        }
        EXPECT_EQ(Expected, executeDynamicShift(Plan.Opcode, Value, Count))
            << "amount " << Amount << ", pseudo " << Pseudo;
      }
    }
  }
  EXPECT_EQ(SH::SHLD, SH::planDynamicShift(SH::SHLrr).Opcode);
  EXPECT_FALSE(SH::planDynamicShift(SH::SHLrr).NegateCount);
  EXPECT_EQ(SH::SHLD, SH::planDynamicShift(SH::SRLrr).Opcode);
  EXPECT_TRUE(SH::planDynamicShift(SH::SRLrr).NegateCount);
  EXPECT_EQ(SH::SHAD, SH::planDynamicShift(SH::SRArr).Opcode);
  EXPECT_TRUE(SH::planDynamicShift(SH::SRArr).NegateCount);
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
        SH::SHL64_PSEUDO, SH::SRL64_PSEUDO, SH::SRA64_PSEUDO})
    EXPECT_EQ(0u, TII->get(Opcode).getSize());
}

TEST_F(SHInstrInfoTest, SH3InstructionsHavePreciseProperties) {
  for (unsigned Opcode : {SH::SHAD, SH::SHLD}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_EQ(2u, Desc.getSize());
    EXPECT_EQ(1u, Desc.getNumDefs());
    EXPECT_EQ(3u, Desc.getNumOperands());
    EXPECT_EQ(0, Desc.getOperandConstraint(1, MCOI::TIED_TO));
    EXPECT_FALSE(Desc.mayLoad());
    EXPECT_FALSE(Desc.mayStore());
    EXPECT_FALSE(Desc.hasUnmodeledSideEffects());
  }

  for (unsigned Opcode : {SH::CLRS, SH::SETS}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_TRUE(Desc.hasImplicitDefOfPhysReg(SH::SBit));
    EXPECT_FALSE(Desc.hasUnmodeledSideEffects());
  }
  EXPECT_TRUE(TII->get(SH::LDTLB).hasUnmodeledSideEffects());
  EXPECT_TRUE(TII->get(SH::PREF).hasUnmodeledSideEffects());
  EXPECT_FALSE(TII->get(SH::PREF).mayLoad());
  EXPECT_FALSE(TII->get(SH::PREF).mayStore());

  for (unsigned Opcode : {SH::LDC_L_SSR, SH::LDC_L_SPC, SH::LDC_L_BANK}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_TRUE(Desc.mayLoad());
    EXPECT_FALSE(Desc.mayStore());
    EXPECT_FALSE(Desc.hasUnmodeledSideEffects());
  }
  for (unsigned Opcode : {SH::STC_L_SSR, SH::STC_L_SPC, SH::STC_L_BANK}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_FALSE(Desc.mayLoad());
    EXPECT_TRUE(Desc.mayStore());
    EXPECT_FALSE(Desc.hasUnmodeledSideEffects());
  }
  EXPECT_TRUE(
      llvm::is_contained(TII->get(SH::LDC_SSR).implicit_defs(), SH::SSR));
  EXPECT_TRUE(
      llvm::is_contained(TII->get(SH::LDC_SPC).implicit_defs(), SH::SPC));
  EXPECT_TRUE(
      llvm::is_contained(TII->get(SH::STC_SSR).implicit_uses(), SH::SSR));
  EXPECT_TRUE(
      llvm::is_contained(TII->get(SH::STC_SPC).implicit_uses(), SH::SPC));
  EXPECT_EQ(0, TII->get(SH::LDC_L_BANK).getOperandConstraint(2, MCOI::TIED_TO));
  EXPECT_EQ(0, TII->get(SH::STC_L_BANK).getOperandConstraint(2, MCOI::TIED_TO));
}

TEST_F(SHInstrInfoTest, SH4InstructionsHavePreciseProperties) {
  for (unsigned Opcode : {SH::LDC_DBR, SH::LDC_L_DBR})
    EXPECT_TRUE(TII->get(Opcode).hasImplicitDefOfPhysReg(SH::DBR));
  for (unsigned Opcode : {SH::STC_DBR, SH::STC_L_DBR})
    EXPECT_TRUE(is_contained(TII->get(Opcode).implicit_uses(), SH::DBR));
  for (unsigned Opcode : {SH::STC_SGR, SH::STC_L_SGR})
    EXPECT_TRUE(is_contained(TII->get(Opcode).implicit_uses(), SH::SGR));
  EXPECT_TRUE(TII->get(SH::LDC_L_DBR).mayLoad());
  EXPECT_TRUE(TII->get(SH::STC_L_DBR).mayStore());
  EXPECT_TRUE(TII->get(SH::STC_L_SGR).mayStore());

  const MCInstrDesc &Movca = TII->get(SH::MOVCA_L);
  EXPECT_FALSE(Movca.mayLoad());
  EXPECT_TRUE(Movca.mayStore());
  EXPECT_TRUE(Movca.hasUnmodeledSideEffects());
  EXPECT_TRUE(is_contained(Movca.implicit_uses(), SH::R0));
  const MCInstrDesc &Ocbi = TII->get(SH::OCBI);
  EXPECT_FALSE(Ocbi.mayLoad());
  EXPECT_FALSE(Ocbi.mayStore());
  EXPECT_TRUE(Ocbi.hasUnmodeledSideEffects());
  for (unsigned Opcode : {SH::OCBP, SH::OCBWB}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_TRUE(Desc.mayLoad());
    EXPECT_TRUE(Desc.mayStore());
    EXPECT_TRUE(Desc.hasUnmodeledSideEffects());
  }

  for (unsigned Opcode : {SH::FMOV_FR, SH::FADD_FR, SH::FSUB_FR, SH::FMUL_FR,
                          SH::FDIV_FR, SH::FSQRT_FR, SH::FMOV_DR_DR,
                          SH::FADD_DR, SH::FSQRT_DR, SH::FIPR, SH::FTRV})
    EXPECT_TRUE(is_contained(TII->get(Opcode).implicit_uses(), SH::FPSCR));
  for (unsigned Opcode :
       {SH::FMOV_S_LOAD, SH::FMOV_S_LOAD_POSTINC, SH::FMOV_S_LOAD_INDEXED,
        SH::FMOV_D_LOAD_DR, SH::FMOV_D_LOAD_POSTINC_DR,
        SH::FMOV_D_LOAD_INDEXED_DR, SH::FMOV_D_LOAD_XD,
        SH::FMOV_D_LOAD_POSTINC_XD, SH::FMOV_D_LOAD_INDEXED_XD, SH::LDS_L_FPSCR,
        SH::LDS_L_FPUL}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_TRUE(Desc.mayLoad()) << TII->getName(Opcode);
    EXPECT_FALSE(Desc.mayStore()) << TII->getName(Opcode);
  }
  for (unsigned Opcode :
       {SH::FMOV_S_STORE, SH::FMOV_S_STORE_PREDEC, SH::FMOV_S_STORE_INDEXED,
        SH::FMOV_D_STORE_DR, SH::FMOV_D_STORE_PREDEC_DR,
        SH::FMOV_D_STORE_INDEXED_DR, SH::FMOV_D_STORE_XD,
        SH::FMOV_D_STORE_PREDEC_XD, SH::FMOV_D_STORE_INDEXED_XD,
        SH::STS_L_FPSCR, SH::STS_L_FPUL}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_FALSE(Desc.mayLoad()) << TII->getName(Opcode);
    EXPECT_TRUE(Desc.mayStore()) << TII->getName(Opcode);
  }
  for (unsigned Opcode :
       {SH::FABS_FR, SH::FNEG_FR, SH::FADD_FR, SH::FSUB_FR, SH::FMUL_FR,
        SH::FMAC_FR, SH::FDIV_FR, SH::FSQRT_FR, SH::FIPR, SH::FTRV, SH::FABS_DR,
        SH::FNEG_DR, SH::FADD_DR, SH::FSUB_DR, SH::FMUL_DR, SH::FDIV_DR,
        SH::FSQRT_DR})
    EXPECT_EQ(0, TII->get(Opcode).getOperandConstraint(1, MCOI::TIED_TO))
        << TII->getName(Opcode);
  for (unsigned Opcode :
       {SH::FCMP_EQ_FR, SH::FCMP_GT_FR, SH::FCMP_EQ_DR, SH::FCMP_GT_DR})
    EXPECT_TRUE(TII->get(Opcode).hasImplicitDefOfPhysReg(SH::TBit));
  EXPECT_TRUE(is_contained(TII->get(SH::FMAC_FR).implicit_uses(), SH::FR0));
  EXPECT_TRUE(is_contained(TII->get(SH::FTRV).implicit_uses(), SH::XMTRX));
  for (unsigned Opcode : {SH::FLDS, SH::FTRC_FR, SH::FTRC_DR, SH::FCNVDS})
    EXPECT_TRUE(TII->get(Opcode).hasImplicitDefOfPhysReg(SH::FPUL));
  for (unsigned Opcode : {SH::FSTS, SH::FLOAT_FR, SH::FLOAT_DR, SH::FCNVSD})
    EXPECT_TRUE(is_contained(TII->get(Opcode).implicit_uses(), SH::FPUL));
  for (unsigned Opcode : {SH::FRCHG, SH::FSCHG}) {
    const MCInstrDesc &Desc = TII->get(Opcode);
    EXPECT_TRUE(is_contained(Desc.implicit_uses(), SH::FPSCR));
    EXPECT_TRUE(Desc.hasImplicitDefOfPhysReg(SH::FPSCR));
    EXPECT_TRUE(Desc.hasUnmodeledSideEffects());
  }
}

TEST_F(SHInstrInfoTest, SH4FPUViewsUseDistinctStorage) {
  const SHRegisterInfo &TRI = TII->getRegisterInfo();
  EXPECT_EQ(SH::FR0, TRI.getSubReg(SH::DR0, SH::sub_fpu32_0));
  EXPECT_EQ(SH::FR1, TRI.getSubReg(SH::DR0, SH::sub_fpu32_1));
  EXPECT_EQ(SH::FR14, TRI.getSubReg(SH::DR14, SH::sub_fpu32_0));
  EXPECT_EQ(SH::FR15, TRI.getSubReg(SH::DR14, SH::sub_fpu32_1));
  EXPECT_EQ(SH::XF0, TRI.getSubReg(SH::XD0, SH::sub_fpu32_0));
  EXPECT_EQ(SH::XF15, TRI.getSubReg(SH::XD14, SH::sub_fpu32_1));
  EXPECT_EQ(SH::FR8, TRI.getSubReg(SH::FV8, SH::sub_fpu32_0));
  EXPECT_EQ(SH::FR11, TRI.getSubReg(SH::FV8, SH::sub_fpu32_3));
  EXPECT_EQ(SH::XF0, TRI.getSubReg(SH::XMTRX, SH::sub_fpu32_0));
  EXPECT_EQ(SH::XF15, TRI.getSubReg(SH::XMTRX, SH::sub_fpu32_15));
  const MCRegister DRs[] = {SH::DR0, SH::DR2,  SH::DR4,  SH::DR6,
                            SH::DR8, SH::DR10, SH::DR12, SH::DR14};
  const MCRegister FRs[] = {SH::FR0,  SH::FR1,  SH::FR2,  SH::FR3,
                            SH::FR4,  SH::FR5,  SH::FR6,  SH::FR7,
                            SH::FR8,  SH::FR9,  SH::FR10, SH::FR11,
                            SH::FR12, SH::FR13, SH::FR14, SH::FR15};
  for (unsigned I = 0; I != std::size(DRs); ++I) {
    EXPECT_TRUE(TRI.regsOverlap(DRs[I], FRs[I * 2]));
    EXPECT_TRUE(TRI.regsOverlap(DRs[I], FRs[I * 2 + 1]));
    if (I != 0)
      EXPECT_FALSE(TRI.regsOverlap(DRs[I], FRs[I * 2 - 1]));
    if (I + 1 != std::size(DRs))
      EXPECT_FALSE(TRI.regsOverlap(DRs[I], FRs[I * 2 + 2]));
  }
  EXPECT_TRUE(TRI.regsOverlap(SH::XD0, SH::XF0));
  EXPECT_TRUE(TRI.regsOverlap(SH::FV0, SH::FR3));
  EXPECT_TRUE(TRI.regsOverlap(SH::XMTRX, SH::XF15));
  EXPECT_FALSE(TRI.regsOverlap(SH::FR0, SH::XF0));
  EXPECT_FALSE(TRI.regsOverlap(SH::DR0, SH::XD0));
  EXPECT_TRUE(SH::FRRegClass.isAllocatable());
  EXPECT_TRUE(SH::DRRegClass.isAllocatable());
  EXPECT_TRUE(SH::FVRegClass.isAllocatable());
  EXPECT_EQ(4u, TRI.getSpillSize(SH::FRRegClass));
  EXPECT_EQ(Align(4), TRI.getSpillAlign(SH::FRRegClass));
  EXPECT_EQ(8u, TRI.getSpillSize(SH::DRRegClass));
  EXPECT_EQ(Align(4), TRI.getSpillAlign(SH::DRRegClass));
  EXPECT_EQ(16u, TRI.getSpillSize(SH::FVRegClass));
  EXPECT_EQ(Align(4), TRI.getSpillAlign(SH::FVRegClass));
  for (MCRegister Reg : {SH::FR12, SH::FR13, SH::FR14, SH::FR15, SH::DR12,
                         SH::DR14})
    EXPECT_TRUE(TRI.regsOverlap(SH::FV12, Reg));
  for (MCRegister Reg : {SH::FR11, SH::DR10, SH::XF12, SH::XD12, SH::XMTRX})
    EXPECT_FALSE(TRI.regsOverlap(SH::FV12, Reg));
  for (const TargetRegisterClass *RC :
       {&SH::FR0RegRegClass, &SH::XFRegClass, &SH::XDRegClass,
        &SH::XMTRXRegRegClass})
    EXPECT_FALSE(RC->isAllocatable());
}

TEST_F(SHInstrInfoTest, StateRegistersAreReservedAndUnallocatable) {
  const SHRegisterInfo &TRI = TII->getRegisterInfo();
  BitVector Reserved = TRI.getReservedRegs(*MF);
  EXPECT_FALSE(Reserved.test(SH::R12));
  for (MCRegister Reg :
       {SH::MBit, SH::QBit, SH::TBit, SH::SBit, SH::SSR, SH::SPC, SH::SGR,
        SH::DBR, SH::FPUL, SH::FPSCR, SH::R0_BANK, SH::R1_BANK, SH::R2_BANK,
        SH::R3_BANK, SH::R4_BANK, SH::R5_BANK, SH::R6_BANK, SH::R7_BANK}) {
    EXPECT_TRUE(Reserved.test(Reg));
    EXPECT_FALSE(SH::GPRRegClass.contains(Reg));
  }
  for (MCRegister Reg : SH::BankedGPRRegClass)
    EXPECT_FALSE(SH::GPRRegClass.contains(Reg));
  for (const TargetRegisterClass *RC :
       {&SH::FRRegClass, &SH::XFRegClass, &SH::DRRegClass, &SH::XDRegClass,
        &SH::FVRegClass, &SH::XMTRXRegRegClass})
    for (MCRegister Reg : *RC) {
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

TEST_F(SHInstrInfoTest, BranchOffsetRangesMatchEncodings) {
  for (unsigned Opcode : {SH::BT, SH::BF}) {
    EXPECT_TRUE(TII->isBranchOffsetInRange(Opcode, -252));
    EXPECT_FALSE(TII->isBranchOffsetInRange(Opcode, -254));
    EXPECT_TRUE(TII->isBranchOffsetInRange(Opcode, 258));
    EXPECT_FALSE(TII->isBranchOffsetInRange(Opcode, 260));
  }

  EXPECT_TRUE(TII->isBranchOffsetInRange(SH::BRA, -4092));
  EXPECT_FALSE(TII->isBranchOffsetInRange(SH::BRA, -4094));
  EXPECT_TRUE(TII->isBranchOffsetInRange(SH::BRA, 4098));
  EXPECT_FALSE(TII->isBranchOffsetInRange(SH::BRA, 4100));
  EXPECT_FALSE(TII->isBranchOffsetInRange(SH::BRA, 1));
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

TEST_F(SHInstrInfoTest, MemoryAccessPlanPreservesExactByteImages) {
  auto loadChunk = [](ArrayRef<uint8_t> Bytes, unsigned Offset, unsigned Width,
                      bool IsLittleEndian) {
    uint32_t Value = 0;
    for (unsigned Byte = 0; Byte != Width; ++Byte) {
      unsigned Shift = 8 * (IsLittleEndian ? Byte : Width - 1 - Byte);
      Value |= static_cast<uint32_t>(Bytes[Offset + Byte]) << Shift;
    }
    return Value;
  };
  auto storeChunk = [](MutableArrayRef<uint8_t> Bytes, unsigned Offset,
                       unsigned Width, uint32_t Value, bool IsLittleEndian) {
    for (unsigned Byte = 0; Byte != Width; ++Byte) {
      unsigned Shift = 8 * (IsLittleEndian ? Byte : Width - 1 - Byte);
      Bytes[Offset + Byte] = static_cast<uint8_t>(Value >> Shift);
    }
  };
  auto copyWithPlan = [&](MutableArrayRef<uint8_t> Bytes, unsigned Destination,
                          unsigned Source, unsigned Size, Align Alignment,
                          bool IsLittleEndian, bool IsMove) {
    SmallVector<unsigned, 16> Widths = SH::planMemoryAccesses(Size, Alignment);
    SmallVector<uint32_t, 16> Loaded;
    unsigned Offset = 0;
    if (IsMove)
      for (unsigned Width : Widths) {
        Loaded.push_back(
            loadChunk(Bytes, Source + Offset, Width, IsLittleEndian));
        Offset += Width;
      }
    Offset = 0;
    for (auto [Index, Width] : enumerate(Widths)) {
      uint32_t Value =
          IsMove ? Loaded[Index]
                 : loadChunk(Bytes, Source + Offset, Width, IsLittleEndian);
      storeChunk(Bytes, Destination + Offset, Width, Value, IsLittleEndian);
      Offset += Width;
    }
  };
  auto memsetWithPlan =
      [&](MutableArrayRef<uint8_t> Bytes, unsigned Destination, unsigned Size,
          uint8_t ByteValue, Align Alignment, bool IsLittleEndian) {
        unsigned Offset = 0;
        for (unsigned Width : SH::planMemoryAccesses(Size, Alignment)) {
          uint32_t Value = 0;
          for (unsigned Byte = 0; Byte != Width; ++Byte) {
            unsigned Shift = 8 * (IsLittleEndian ? Byte : Width - 1 - Byte);
            Value |= static_cast<uint32_t>(ByteValue) << Shift;
          }
          storeChunk(Bytes, Destination + Offset, Width, Value, IsLittleEndian);
          Offset += Width;
        }
      };
  auto referenceMove = [](MutableArrayRef<uint8_t> Bytes, unsigned Destination,
                          unsigned Source, unsigned Size) {
    SmallVector<uint8_t, 64> SourceImage(Bytes.slice(Source, Size).begin(),
                                         Bytes.slice(Source, Size).end());
    llvm::copy(SourceImage, Bytes.begin() + Destination);
  };

  constexpr unsigned RequiredSizes[] = {0,  1,  2,  3,  4,  5,  7, 8,
                                        12, 15, 16, 17, 28, 59, 60};
  const Align Alignments[] = {Align(1), Align(2), Align(4)};
  for (unsigned Size : RequiredSizes)
    for (Align Alignment : Alignments)
      for (bool IsLittleEndian : {false, true}) {
        SmallVector<uint8_t, 192> Initial(192);
        for (unsigned I = 0; I != Initial.size(); ++I)
          Initial[I] = static_cast<uint8_t>(I * 37 + Size);

        SmallVector<uint8_t, 192> Actual = Initial;
        SmallVector<uint8_t, 192> Expected = Initial;
        copyWithPlan(Actual, 96, 0, Size, Alignment, IsLittleEndian, false);
        llvm::copy(ArrayRef(Expected).slice(0, Size), Expected.begin() + 96);
        EXPECT_EQ(Expected, Actual);

        for (auto [Destination, Source] :
             {std::pair(12u, 8u), std::pair(8u, 12u), std::pair(8u, 8u)}) {
          Actual = Initial;
          Expected = Initial;
          copyWithPlan(Actual, Destination, Source, Size, Alignment,
                       IsLittleEndian, true);
          referenceMove(Expected, Destination, Source, Size);
          EXPECT_EQ(Expected, Actual);
        }

        Actual = Initial;
        Expected = Initial;
        memsetWithPlan(Actual, 64, Size, 0xa5, Alignment, IsLittleEndian);
        llvm::fill(MutableArrayRef(Expected).slice(64, Size), 0xa5);
        EXPECT_EQ(Expected, Actual);
      }

  std::mt19937 Generator(0x53484313);
  for (unsigned Scenario = 0; Scenario != 10000; ++Scenario) {
    unsigned Size = Generator() % 61;
    Align Alignment = Align(1u << (Generator() % 3));
    bool IsLittleEndian = Generator() & 1;
    SmallVector<uint8_t, 256> Initial(256);
    for (uint8_t &Byte : Initial)
      Byte = static_cast<uint8_t>(Generator());

    unsigned Source = Generator() % 65;
    unsigned Destination = 128 + Generator() % 65;
    SmallVector<uint8_t, 256> Actual = Initial;
    SmallVector<uint8_t, 256> Expected = Initial;
    copyWithPlan(Actual, Destination, Source, Size, Alignment, IsLittleEndian,
                 false);
    llvm::copy(ArrayRef(Expected).slice(Source, Size),
               Expected.begin() + Destination);
    EXPECT_EQ(Expected, Actual);

    Source = Generator() % (257 - Size);
    Destination = Generator() % (257 - Size);
    Actual = Initial;
    Expected = Initial;
    copyWithPlan(Actual, Destination, Source, Size, Alignment, IsLittleEndian,
                 true);
    referenceMove(Expected, Destination, Source, Size);
    EXPECT_EQ(Expected, Actual);

    Destination = Generator() % (257 - Size);
    uint8_t ByteValue = static_cast<uint8_t>(Generator());
    Actual = Initial;
    Expected = Initial;
    memsetWithPlan(Actual, Destination, Size, ByteValue, Alignment,
                   IsLittleEndian);
    llvm::fill(MutableArrayRef(Expected).slice(Destination, Size), ByteValue);
    EXPECT_EQ(Expected, Actual);
  }
}

TEST(SHVarArgsModelTest, SavedRegisterAndOverflowImagesRoundTrip) {
  struct Argument {
    SmallVector<uint8_t, 12> Bytes;
  };
  auto appendArgument = [](SmallVectorImpl<SmallVector<uint8_t, 4>> &Slots,
                           ArrayRef<uint8_t> Bytes, bool IsLittleEndian) {
    for (unsigned Offset = 0; Offset < Bytes.size(); Offset += 4) {
      unsigned ValidBytes =
          std::min(4u, static_cast<unsigned>(Bytes.size() - Offset));
      SmallVector<uint8_t, 4> Slot(4, 0);
      unsigned SlotOffset =
          !IsLittleEndian && ValidBytes != 4 ? 4 - ValidBytes : 0;
      llvm::copy(Bytes.slice(Offset, ValidBytes), Slot.begin() + SlotOffset);
      Slots.push_back(std::move(Slot));
    }
  };
  auto readArgument = [](ArrayRef<SmallVector<uint8_t, 4>> Slots,
                         unsigned &Cursor, unsigned Size, bool IsLittleEndian) {
    SmallVector<uint8_t, 12> Result;
    for (unsigned Offset = 0; Offset < Size; Offset += 4) {
      unsigned ValidBytes = std::min(4u, Size - Offset);
      unsigned SlotOffset =
          !IsLittleEndian && ValidBytes != 4 ? 4 - ValidBytes : 0;
      llvm::append_range(
          Result, ArrayRef(Slots[Cursor++]).slice(SlotOffset, ValidBytes));
    }
    return Result;
  };

  std::mt19937 Generator(0x53484314);
  constexpr unsigned AggregateSizes[] = {1, 2, 3, 5, 6, 7, 8, 9, 12};
  for (bool IsLittleEndian : {false, true}) {
    for (unsigned Scenario = 0; Scenario != 10000; ++Scenario) {
      unsigned FixedCursor = Generator() % 5;
      unsigned FixedStackWords = FixedCursor == 4 ? Generator() % 4 : 0;
      unsigned ArgumentCount = 1 + Generator() % 12;
      SmallVector<Argument, 12> Arguments;
      for (unsigned I = 0; I != ArgumentCount; ++I) {
        unsigned Kind = Generator() % 3;
        unsigned Size =
            Kind == 0 ? 4
            : Kind == 1
                ? 8
                : AggregateSizes[Generator() % std::size(AggregateSizes)];
        Argument Arg;
        for (unsigned Byte = 0; Byte != Size; ++Byte)
          Arg.Bytes.push_back(static_cast<uint8_t>(Generator()));
        Arguments.push_back(std::move(Arg));
      }

      SmallVector<SmallVector<uint8_t, 4>, 32> RegisterAndStackSlots;
      for (unsigned I = 0; I != FixedCursor; ++I)
        RegisterAndStackSlots.push_back(SmallVector<uint8_t, 4>(4, 0));
      for (unsigned I = 0; I != FixedStackWords; ++I)
        RegisterAndStackSlots.push_back(SmallVector<uint8_t, 4>(4, 0));
      for (const Argument &Arg : Arguments)
        appendArgument(RegisterAndStackSlots, Arg.Bytes, IsLittleEndian);
      RegisterAndStackSlots.resize(
          std::max(4u, static_cast<unsigned>(RegisterAndStackSlots.size())),
          SmallVector<uint8_t, 4>(4, 0));

      SmallVector<SmallVector<uint8_t, 4>, 32> VaImage;
      if (FixedCursor < 4)
        llvm::append_range(VaImage, ArrayRef(RegisterAndStackSlots)
                                        .slice(FixedCursor, 4 - FixedCursor));
      unsigned FirstOverflow = std::max(4u, FixedCursor) + FixedStackWords;
      llvm::append_range(
          VaImage, ArrayRef(RegisterAndStackSlots).drop_front(FirstOverflow));

      unsigned Cursor = 0;
      for (const Argument &Arg : Arguments) {
        ASSERT_LE(Cursor + divideCeil(Arg.Bytes.size(), 4u), VaImage.size())
            << "scenario " << Scenario << " endian " << IsLittleEndian;
        SmallVector<uint8_t, 12> Extracted =
            readArgument(VaImage, Cursor, Arg.Bytes.size(), IsLittleEndian);
        EXPECT_EQ(Arg.Bytes, Extracted)
            << "scenario " << Scenario << " endian " << IsLittleEndian;
      }
      unsigned ExpectedWords = 0;
      for (const Argument &Arg : Arguments)
        ExpectedWords += divideCeil(Arg.Bytes.size(), 4u);
      EXPECT_EQ(ExpectedWords, Cursor);
    }
  }
}

enum class AtomicModelOperation {
  Load,
  Store,
  Exchange,
  Add,
  Sub,
  And,
  Nand,
  Or,
  Xor,
  SignedMax,
  SignedMin,
  UnsignedMax,
  UnsignedMin,
  CompareExchange,
};

template <typename T> struct AtomicShimResult {
  T Old;
  T Memory;
  T Expected;
  bool Success;
  unsigned SuccessOrder;
  unsigned FailureOrder;
};

template <typename T>
static AtomicShimResult<T>
runAtomicShim(AtomicModelOperation Operation, T Memory, T Operand, T Expected,
              bool Weak, bool SpuriousFailure, unsigned SuccessOrder,
              unsigned FailureOrder) {
  T Old = Memory;
  bool Success = false;
  using SignedT = std::make_signed_t<T>;
  switch (Operation) {
  case AtomicModelOperation::Load:
    break;
  case AtomicModelOperation::Store:
    Memory = Operand;
    break;
  case AtomicModelOperation::Exchange:
    Memory = Operand;
    break;
  case AtomicModelOperation::Add:
    Memory = Old + Operand;
    break;
  case AtomicModelOperation::Sub:
    Memory = Old - Operand;
    break;
  case AtomicModelOperation::And:
    Memory = Old & Operand;
    break;
  case AtomicModelOperation::Nand:
    Memory = ~(Old & Operand);
    break;
  case AtomicModelOperation::Or:
    Memory = Old | Operand;
    break;
  case AtomicModelOperation::Xor:
    Memory = Old ^ Operand;
    break;
  case AtomicModelOperation::SignedMax:
    Memory = static_cast<SignedT>(Old) > static_cast<SignedT>(Operand)
                 ? Old
                 : Operand;
    break;
  case AtomicModelOperation::SignedMin:
    Memory = static_cast<SignedT>(Old) < static_cast<SignedT>(Operand)
                 ? Old
                 : Operand;
    break;
  case AtomicModelOperation::UnsignedMax:
    Memory = std::max(Old, Operand);
    break;
  case AtomicModelOperation::UnsignedMin:
    Memory = std::min(Old, Operand);
    break;
  case AtomicModelOperation::CompareExchange:
    Success = Old == Expected && !(Weak && SpuriousFailure);
    if (Success)
      Memory = Operand;
    else
      Expected = Old;
    break;
  }
  return {Old, Memory, Expected, Success, SuccessOrder, FailureOrder};
}

template <typename T> static void testAtomicRuntimeModel(uint64_t Seed) {
  std::mt19937_64 Generator(Seed);
  constexpr AtomicModelOperation Operations[] = {
      AtomicModelOperation::Load,        AtomicModelOperation::Store,
      AtomicModelOperation::Exchange,    AtomicModelOperation::Add,
      AtomicModelOperation::Sub,         AtomicModelOperation::And,
      AtomicModelOperation::Nand,        AtomicModelOperation::Or,
      AtomicModelOperation::Xor,         AtomicModelOperation::SignedMax,
      AtomicModelOperation::SignedMin,   AtomicModelOperation::UnsignedMax,
      AtomicModelOperation::UnsignedMin, AtomicModelOperation::CompareExchange,
  };
  constexpr unsigned Orders[] = {0, 2, 3, 4, 5};
  for (unsigned Scenario = 0; Scenario != 10000; ++Scenario) {
    AtomicModelOperation Operation =
        Operations[Generator() % std::size(Operations)];
    T Initial = static_cast<T>(Generator());
    T Operand = static_cast<T>(Generator());
    T Expected = Scenario % 3 == 0 ? Initial : static_cast<T>(Generator());
    bool Weak = Scenario % 2 != 0;
    bool SpuriousFailure = Scenario % 17 == 0;
    unsigned SuccessOrder = Orders[Generator() % std::size(Orders)];
    unsigned FailureOrder = Orders[Generator() % std::size(Orders)];

    T ReferenceMemory = Initial;
    T ReferenceExpected = Expected;
    bool ReferenceSuccess = false;
    using SignedT = std::make_signed_t<T>;
    switch (Operation) {
    case AtomicModelOperation::Load:
      break;
    case AtomicModelOperation::Store:
    case AtomicModelOperation::Exchange:
      ReferenceMemory = Operand;
      break;
    case AtomicModelOperation::Add:
      ReferenceMemory = Initial + Operand;
      break;
    case AtomicModelOperation::Sub:
      ReferenceMemory = Initial - Operand;
      break;
    case AtomicModelOperation::And:
      ReferenceMemory = Initial & Operand;
      break;
    case AtomicModelOperation::Nand:
      ReferenceMemory = ~(Initial & Operand);
      break;
    case AtomicModelOperation::Or:
      ReferenceMemory = Initial | Operand;
      break;
    case AtomicModelOperation::Xor:
      ReferenceMemory = Initial ^ Operand;
      break;
    case AtomicModelOperation::SignedMax:
      if (static_cast<SignedT>(Initial) < static_cast<SignedT>(Operand))
        ReferenceMemory = Operand;
      break;
    case AtomicModelOperation::SignedMin:
      if (static_cast<SignedT>(Initial) > static_cast<SignedT>(Operand))
        ReferenceMemory = Operand;
      break;
    case AtomicModelOperation::UnsignedMax:
      if (Initial < Operand)
        ReferenceMemory = Operand;
      break;
    case AtomicModelOperation::UnsignedMin:
      if (Initial > Operand)
        ReferenceMemory = Operand;
      break;
    case AtomicModelOperation::CompareExchange:
      ReferenceSuccess = Initial == Expected && !(Weak && SpuriousFailure);
      if (ReferenceSuccess)
        ReferenceMemory = Operand;
      else
        ReferenceExpected = Initial;
      break;
    }

    AtomicShimResult<T> Result =
        runAtomicShim(Operation, Initial, Operand, Expected, Weak,
                      SpuriousFailure, SuccessOrder, FailureOrder);
    EXPECT_EQ(Initial, Result.Old) << "scenario " << Scenario;
    EXPECT_EQ(ReferenceMemory, Result.Memory) << "scenario " << Scenario;
    EXPECT_EQ(ReferenceExpected, Result.Expected) << "scenario " << Scenario;
    EXPECT_EQ(ReferenceSuccess, Result.Success) << "scenario " << Scenario;
    EXPECT_EQ(SuccessOrder, Result.SuccessOrder) << "scenario " << Scenario;
    EXPECT_EQ(FailureOrder, Result.FailureOrder) << "scenario " << Scenario;
  }
}

TEST(SHAtomicRuntimeModelTest, I32AndI64OperationsMatchIndependentModel) {
  testAtomicRuntimeModel<uint32_t>(0x534843150032);
  testAtomicRuntimeModel<uint64_t>(0x534843150064);
}

enum class PICRelocationKind { GOTPC, GOTOFF, GOT32, PLT32 };

static int64_t evaluatePICRelocation(PICRelocationKind Kind, int64_t Symbol,
                                     int64_t Addend, int64_t Place, int64_t GOT,
                                     int64_t GOTEntry, int64_t PLTEntry) {
  switch (Kind) {
  case PICRelocationKind::GOTPC:
    return GOT + Addend - Place;
  case PICRelocationKind::GOTOFF:
    return Symbol + Addend - GOT;
  case PICRelocationKind::GOT32:
    return GOTEntry + Addend - GOT;
  case PICRelocationKind::PLT32:
    return PLTEntry + Addend - Place;
  }
  llvm_unreachable("unknown PIC relocation kind");
}

static uint32_t decodeBigEndian(const uint8_t Bytes[4]) {
  return uint32_t(Bytes[0]) << 24 | uint32_t(Bytes[1]) << 16 |
         uint32_t(Bytes[2]) << 8 | uint32_t(Bytes[3]);
}

static uint32_t decodeLittleEndian(const uint8_t Bytes[4]) {
  return uint32_t(Bytes[3]) << 24 | uint32_t(Bytes[2]) << 16 |
         uint32_t(Bytes[1]) << 8 | uint32_t(Bytes[0]);
}

TEST(SHPICRelocationModelTest,
     RelocationsMatchIndependentLayoutAndEndianModel) {
  constexpr PICRelocationKind Kinds[] = {
      PICRelocationKind::GOTPC, PICRelocationKind::GOTOFF,
      PICRelocationKind::GOT32, PICRelocationKind::PLT32};
  std::mt19937_64 Generator(0x534843170000);
  auto RandomAddress = [&]() {
    return int64_t(Generator() % UINT64_C(0x70000000)) + 0x10000;
  };
  auto RandomAddend = [&]() {
    return int64_t(Generator() % UINT64_C(0x200001)) - 0x100000;
  };

  for (PICRelocationKind Kind : Kinds) {
    for (unsigned Scenario = 0; Scenario != 10000; ++Scenario) {
      SCOPED_TRACE(Scenario);
      int64_t Symbol = RandomAddress();
      int64_t Addend = RandomAddend();
      int64_t Place = RandomAddress() & ~INT64_C(3);
      int64_t GOT = RandomAddress() & ~INT64_C(3);
      int64_t GOTEntry = RandomAddress() & ~INT64_C(3);
      int64_t PLTEntry = RandomAddress() & ~INT64_C(3);
      int64_t Value = evaluatePICRelocation(Kind, Symbol, Addend, Place, GOT,
                                            GOTEntry, PLTEntry);

      switch (Kind) {
      case PICRelocationKind::GOTPC:
        EXPECT_EQ(GOT + Addend, Place + Value);
        break;
      case PICRelocationKind::GOTOFF:
        EXPECT_EQ(Symbol + Addend, GOT + Value);
        break;
      case PICRelocationKind::GOT32:
        EXPECT_EQ(GOTEntry + Addend, GOT + Value);
        break;
      case PICRelocationKind::PLT32:
        EXPECT_EQ(PLTEntry + Addend, Place + Value);
        break;
      }

      uint32_t Word = static_cast<uint32_t>(Value);
      uint8_t BigEndian[4] = {uint8_t(Word >> 24), uint8_t(Word >> 16),
                              uint8_t(Word >> 8), uint8_t(Word)};
      uint8_t LittleEndian[4] = {uint8_t(Word), uint8_t(Word >> 8),
                                 uint8_t(Word >> 16), uint8_t(Word >> 24)};
      EXPECT_EQ(Word, decodeBigEndian(BigEndian));
      EXPECT_EQ(Word, decodeLittleEndian(LittleEndian));

      if (Kind == PICRelocationKind::GOTPC ||
          Kind == PICRelocationKind::PLT32) {
        int64_t ClonePlace = RandomAddress() & ~INT64_C(3);
        int64_t CloneValue = evaluatePICRelocation(
            Kind, Symbol, Addend, ClonePlace, GOT, GOTEntry, PLTEntry);
        EXPECT_EQ(Value + Place - ClonePlace, CloneValue);
        int64_t Target = Kind == PICRelocationKind::GOTPC ? GOT : PLTEntry;
        EXPECT_EQ(Target + Addend, ClonePlace + CloneValue);
      }
    }
  }
}

enum class TLSRelocationKind {
  GeneralDynamic,
  LocalDynamic,
  LocalOffset,
  InitialExec,
  LocalExec
};

static int64_t evaluateTLSRelocation(TLSRelocationKind Kind, int64_t Symbol,
                                     int64_t Addend, int64_t GOT,
                                     int64_t GDEntry, int64_t LDEntry,
                                     int64_t IEEntry, int64_t TLSBase,
                                     int64_t TCBSize) {
  switch (Kind) {
  case TLSRelocationKind::GeneralDynamic:
    return GDEntry - GOT;
  case TLSRelocationKind::LocalDynamic:
    return LDEntry - GOT;
  case TLSRelocationKind::LocalOffset:
    return Symbol + Addend - TLSBase;
  case TLSRelocationKind::InitialExec:
    return IEEntry - GOT;
  case TLSRelocationKind::LocalExec:
    return Symbol + Addend - TLSBase + TCBSize;
  }
  llvm_unreachable("unknown TLS relocation kind");
}

TEST(SHTLSRelocationModelTest,
     RelocationsMatchIndependentLayoutAddendAndEndianModel) {
  constexpr TLSRelocationKind Kinds[] = {
      TLSRelocationKind::GeneralDynamic, TLSRelocationKind::LocalDynamic,
      TLSRelocationKind::LocalOffset, TLSRelocationKind::InitialExec,
      TLSRelocationKind::LocalExec};
  std::mt19937_64 Generator(0x534843180000);
  auto RandomAddress = [&]() {
    return int64_t(Generator() % UINT64_C(0x70000000)) + 0x10000;
  };
  auto RandomAddend = [&]() {
    return int64_t(Generator() % UINT64_C(0x200001)) - 0x100000;
  };

  for (TLSRelocationKind Kind : Kinds) {
    for (unsigned Scenario = 0; Scenario != 10000; ++Scenario) {
      SCOPED_TRACE(Scenario);
      bool IsDynamic = Generator() & 1;
      int64_t Symbol = RandomAddress();
      int64_t Addend = Kind == TLSRelocationKind::LocalOffset ||
                               Kind == TLSRelocationKind::LocalExec
                           ? RandomAddend()
                           : 0;
      int64_t GOT = RandomAddress() & ~INT64_C(3);
      int64_t GDEntry = RandomAddress() & ~INT64_C(3);
      int64_t LDEntry = RandomAddress() & ~INT64_C(3);
      int64_t IEEntry = RandomAddress() & ~INT64_C(3);
      int64_t TLSBase = RandomAddress() & ~INT64_C(7);
      int64_t TCBSize = alignTo(INT64_C(8), INT64_C(1) << (Generator() % 5));
      int64_t Place = RandomAddress() & ~INT64_C(3);
      int64_t Value = evaluateTLSRelocation(Kind, Symbol, Addend, GOT, GDEntry,
                                            LDEntry, IEEntry, TLSBase, TCBSize);

      switch (Kind) {
      case TLSRelocationKind::GeneralDynamic:
        EXPECT_EQ(GDEntry, GOT + Value);
        break;
      case TLSRelocationKind::LocalDynamic:
        EXPECT_EQ(LDEntry, GOT + Value);
        break;
      case TLSRelocationKind::LocalOffset:
        EXPECT_EQ(Symbol + Addend, TLSBase + Value);
        break;
      case TLSRelocationKind::InitialExec:
        EXPECT_EQ(IEEntry, GOT + Value);
        break;
      case TLSRelocationKind::LocalExec:
        EXPECT_EQ(Symbol + Addend + TCBSize, TLSBase + Value);
        break;
      }

      uint32_t Word = static_cast<uint32_t>(Value);
      uint8_t BigEndian[4] = {uint8_t(Word >> 24), uint8_t(Word >> 16),
                              uint8_t(Word >> 8), uint8_t(Word)};
      uint8_t LittleEndian[4] = {uint8_t(Word), uint8_t(Word >> 8),
                                 uint8_t(Word >> 16), uint8_t(Word >> 24)};
      EXPECT_EQ(Word, decodeBigEndian(BigEndian));
      EXPECT_EQ(Word, decodeLittleEndian(LittleEndian));

      int64_t ClonePlace = RandomAddress() & ~INT64_C(3);
      if (ClonePlace == Place)
        ClonePlace += 4;
      EXPECT_EQ(Value,
                evaluateTLSRelocation(Kind, Symbol, Addend, GOT, GDEntry,
                                      LDEntry, IEEntry, TLSBase, TCBSize));
      EXPECT_NE(Place, ClonePlace);
      int64_t MovedGOT = GOT + (IsDynamic ? 0x4000 : -0x4000);
      int64_t MovedGDEntry = GDEntry + (IsDynamic ? 0x8000 : -0x8000);
      int64_t MovedLDEntry = LDEntry + (IsDynamic ? 0xc000 : -0xc000);
      int64_t MovedIEEntry = IEEntry + (IsDynamic ? 0x10000 : -0x10000);
      int64_t Moved =
          evaluateTLSRelocation(Kind, Symbol, Addend, MovedGOT, MovedGDEntry,
                                MovedLDEntry, MovedIEEntry, TLSBase, TCBSize);
      if (Kind == TLSRelocationKind::LocalOffset ||
          Kind == TLSRelocationKind::LocalExec)
        EXPECT_EQ(Value, Moved);
      else
        EXPECT_NE(Value, Moved);
    }
  }
}

} // namespace
