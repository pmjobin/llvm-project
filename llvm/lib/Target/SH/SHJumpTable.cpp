//===-- SHJumpTable.cpp - Place and compress inline SH jump tables --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHBranchRange.h"
#include "SHInstrInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/InitializePasses.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/MathExtras.h"
#include <limits>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "sh-jump-tables"

namespace {

static MachineInstr *getReachingDef(MachineInstr &Use, Register Reg,
                                    const TargetRegisterInfo &TRI) {
  MachineBasicBlock &MBB = *Use.getParent();
  for (auto I = Use.getIterator(); I != MBB.begin();) {
    --I;
    if (I->modifiesRegister(Reg, &TRI))
      return &*I;
  }
  return nullptr;
}

static MachineInstr *getInlineTableMOVA(MachineInstr &Branch,
                                        const TargetRegisterInfo &TRI,
                                        MachineInstr **LoadOut = nullptr,
                                        MachineInstr **ScaleOut = nullptr) {
  if (LoadOut)
    *LoadOut = nullptr;
  if (ScaleOut)
    *ScaleOut = nullptr;
  if (Branch.getOpcode() != SH::BRAF || Branch.getNumExplicitOperands() < 2 ||
      !Branch.getOperand(0).isReg() || !Branch.getOperand(1).isJTI())
    return nullptr;
  MachineInstr *Load =
      getReachingDef(Branch, Branch.getOperand(0).getReg(), TRI);
  if (!Load || Load->getOpcode() != SH::MOVL_load_indexed ||
      Load->getNumExplicitOperands() != 2 || !Load->getOperand(0).isReg() ||
      !Load->getOperand(1).isReg() ||
      Load->getOperand(0).getReg() != Branch.getOperand(0).getReg())
    return nullptr;
  MachineInstr *Scale =
      getReachingDef(*Load, Load->getOperand(1).getReg(), TRI);
  if (Scale &&
      (Scale->getOpcode() != SH::SHLL2 ||
       Scale->getNumExplicitOperands() != 2 || !Scale->getOperand(0).isReg() ||
       !Scale->getOperand(1).isReg() ||
       Scale->getOperand(0).getReg() != Load->getOperand(1).getReg()))
    Scale = nullptr;
  MachineInstr *MOVA = getReachingDef(*Load, SH::R0, TRI);
  if (!MOVA || MOVA->getOpcode() != SH::MOVA ||
      MOVA->getNumExplicitOperands() != 2 || !MOVA->getOperand(0).isReg() ||
      MOVA->getOperand(0).getReg() != SH::R0)
    return nullptr;
  if (LoadOut)
    *LoadOut = Load;
  if (ScaleOut)
    *ScaleOut = Scale;
  return MOVA;
}

static bool isTBitLiveAfter(const MachineInstr &MI,
                            const TargetRegisterInfo &TRI) {
  const MachineBasicBlock &MBB = *MI.getParent();
  LivePhysRegs LiveRegs(TRI);
  LiveRegs.addLiveOuts(MBB);
  for (auto I = MBB.rbegin(), E = MachineBasicBlock::const_reverse_iterator(
                                  std::next(MI.getIterator()));
       I != E; ++I)
    LiveRegs.stepBackward(*I);
  return LiveRegs.contains(SH::TBit);
}

class InlineJumpTablesImpl {
public:
  bool run(MachineFunction &MF) const {
    const SHSubtarget &STI = MF.getSubtarget<SHSubtarget>();
    MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();
    if (!STI.hasSH2() || !MJTI ||
        MJTI->getEntryKind() != MachineJumpTableInfo::EK_Inline)
      return false;

    const SHInstrInfo &TII = *STI.getInstrInfo();
    const TargetRegisterInfo &TRI = *STI.getRegisterInfo();
    SmallVector<MachineInstr *, 4> Branches;
    DenseMap<unsigned, SmallVector<MachineInstr *, 2>> TableMOVAs;
    for (MachineBasicBlock &MBB : MF)
      for (MachineInstr &MI : MBB) {
        if (MI.getOpcode() == SH::BRAF && MI.getNumExplicitOperands() == 2 &&
            MI.getOperand(1).isJTI())
          Branches.push_back(&MI);
        if (MI.getOpcode() != SH::MOVA || MI.getNumExplicitOperands() != 2 ||
            !MI.getOperand(0).isReg() || MI.getOperand(0).getReg() != SH::R0 ||
            !MI.getOperand(1).isJTI())
          continue;
        unsigned JTI = MI.getOperand(1).getIndex();
        TableMOVAs[JTI].push_back(&MI);
      }

    for (MachineInstr *Branch : Branches) {
      unsigned JTI = Branch->getOperand(1).getIndex();
      MachineInstr *MOVA = getInlineTableMOVA(*Branch, TRI);
      if (!MOVA) {
        auto MOVAIt = TableMOVAs.find(JTI);
        if (MOVAIt != TableMOVAs.end() && MOVAIt->second.size() == 1)
          MOVA = MOVAIt->second.front();
      }
      if (!MOVA || !MOVA->getOperand(1).isJTI() ||
          static_cast<unsigned>(MOVA->getOperand(1).getIndex()) != JTI)
        reportFatalUsageError(
            "SH inline jump-table dispatch has an unsupported shape");
      if (JTI >= MJTI->getJumpTables().size() ||
          MJTI->getJumpTables()[JTI].MBBs.empty())
        reportFatalUsageError("SH inline jump table has an invalid index");

      MCSymbol *Table = MF.getContext().createNamedTempSymbol("sh_jumptable");
      MCSymbol *Base =
          MF.getContext().createNamedTempSymbol("sh_jumptable_base");
      MOVA->getOperand(1).ChangeToMCSymbol(Table);
      Branch->addOperand(MachineOperand::CreateMCSymbol(Base));

      MachineBasicBlock *Dispatch = Branch->getParent();
      MachineBasicBlock *TableBlock = MF.CreateMachineBasicBlock();
      TableBlock->setAlignment(Align(4));
      TableBlock->setSectionID(Dispatch->getSectionID());
      if (Dispatch->isEndSection()) {
        Dispatch->setIsEndSection(false);
        TableBlock->setIsEndSection();
      }
      MF.insert(std::next(Dispatch->getIterator()), TableBlock);
      BuildMI(*TableBlock, TableBlock->end(), DebugLoc(),
              TII.get(SH::SH_JUMPTABLE))
          .addJumpTableIndex(JTI)
          .addSym(Table)
          .addSym(Base)
          .addImm(4);
    }

    if (Branches.empty())
      return false;
    MF.ensureAlignment(Align(4));
    MF.RenumberBlocks();
    return true;
  }
};

static std::optional<uint64_t>
getConservativeFunctionSize(MachineFunction &MF, const SHInstrInfo &TII) {
  uint64_t Size = 0;
  for (const MachineBasicBlock &MBB : MF) {
    if (MBB.isBeginSection() && !MBB.isEntryBlock())
      return std::nullopt;
    if (Size >
        std::numeric_limits<uint64_t>::max() - (MBB.getAlignment().value() - 1))
      return std::nullopt;
    Size += MBB.getAlignment().value() - 1;
    for (const MachineInstr &MI : MBB) {
      if (MI.isBundledWithPred())
        continue;
      unsigned InstSize = TII.getInstSizeInBytes(MI);
      if (!InstSize && !MI.isMetaInstruction())
        return std::nullopt;
      if (Size > std::numeric_limits<uint64_t>::max() - InstSize)
        return std::nullopt;
      Size += InstSize;
    }
  }
  return Size;
}

class CompressJumpTablesImpl {
public:
  bool run(MachineFunction &MF) const {
    const SHSubtarget &STI = MF.getSubtarget<SHSubtarget>();
    MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();
    if (!STI.hasSH2() || !MJTI ||
        MJTI->getEntryKind() != MachineJumpTableInfo::EK_Inline)
      return false;

    const SHInstrInfo &TII = *STI.getInstrInfo();
    const TargetRegisterInfo &TRI = *STI.getRegisterInfo();
    std::optional<uint64_t> FunctionSize = getConservativeFunctionSize(MF, TII);
    if (!FunctionSize || *FunctionSize > INT16_MAX)
      return false;

    DenseMap<const MCSymbol *, MachineInstr *> Tables;
    for (MachineBasicBlock &MBB : MF)
      for (MachineInstr &MI : MBB)
        if (MI.getOpcode() == SH::SH_JUMPTABLE &&
            MI.getNumExplicitOperands() == 4 && MI.getOperand(2).isMCSymbol())
          Tables.try_emplace(MI.getOperand(2).getMCSymbol(), &MI);

    bool Changed = false;
    for (MachineBasicBlock &MBB : MF) {
      for (MachineInstr &Branch : MBB) {
        if (Branch.getOpcode() != SH::BRAF ||
            Branch.getNumExplicitOperands() != 3 ||
            !Branch.getOperand(1).isJTI() || !Branch.getOperand(2).isMCSymbol())
          continue;
        auto TableIt = Tables.find(Branch.getOperand(2).getMCSymbol());
        if (TableIt == Tables.end())
          continue;
        MachineInstr &Table = *TableIt->second;
        unsigned JTI = Branch.getOperand(1).getIndex();
        if (JTI >= MJTI->getJumpTables().size() ||
            static_cast<unsigned>(Table.getOperand(0).getIndex()) != JTI ||
            Table.getOperand(3).getImm() != 4 ||
            Table.getParent()->getSectionID() != MBB.getSectionID())
          continue;
        if (llvm::any_of(MJTI->getJumpTables()[JTI].MBBs,
                         [&](const MachineBasicBlock *Target) {
                           return Target->getSectionID() != MBB.getSectionID();
                         }))
          continue;

        MachineInstr *Load = nullptr;
        MachineInstr *Scale = nullptr;
        MachineInstr *MOVA = getInlineTableMOVA(Branch, TRI, &Load, &Scale);
        if (!MOVA || !Scale || isTBitLiveAfter(*Scale, TRI) ||
            !MOVA->getOperand(1).isMCSymbol() ||
            MOVA->getOperand(1).getMCSymbol() !=
                Table.getOperand(1).getMCSymbol())
          continue;
        if (Load->memoperands().size() != 1)
          continue;
        MachineMemOperand *MMO = *Load->memoperands_begin();
        const PseudoSourceValue *PSV = MMO->getPseudoValue();
        if (!MMO->isLoad() || MMO->isStore() ||
            MMO->getSize() != LocationSize::precise(4) ||
            MMO->getAlign() < Align(4) || !MMO->isInvariant() ||
            !MMO->isDereferenceable() || !PSV || !PSV->isJumpTable())
          continue;

        // The entry-width bound does not protect unrelated direct branches:
        // shrinking a table can increase their distance across alignment.
        // Check against the current layout so accepted compressions compose.
        if (!preservesSHBranchRanges(
                MF, TII, [&](const MachineInstr &MI, unsigned Size) {
                  return &MI == &Table ? Size / 2 : Size;
                }))
          continue;

        Scale->setDesc(TII.get(SH::SHLL));
        Scale->addOperand(
            MF, MachineOperand::CreateReg(SH::TBit, true, true, false, true));
        Load->setDesc(TII.get(SH::MOVW_load_indexed));
        MachineMemOperand *NarrowMMO =
            MF.getMachineMemOperand(MMO, MMO->getOffset(), 2);
        Load->setMemRefs(MF, {NarrowMMO});
        Table.getOperand(3).setImm(2);
        Changed = true;
      }
    }
    return Changed;
  }
};

class SHInlineJumpTablesLegacy : public MachineFunctionPass {
public:
  static char ID;
  SHInlineJumpTablesLegacy() : MachineFunctionPass(ID) {}
  StringRef getPassName() const override { return "SH Inline Jump Tables"; }
  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().setNoVRegs();
  }
  bool runOnMachineFunction(MachineFunction &MF) override {
    return InlineJumpTablesImpl().run(MF);
  }
};

class SHCompressJumpTablesLegacy : public MachineFunctionPass {
public:
  static char ID;
  SHCompressJumpTablesLegacy() : MachineFunctionPass(ID) {}
  StringRef getPassName() const override { return "SH Compress Jump Tables"; }
  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().setNoVRegs().setTracksLiveness();
  }
  bool runOnMachineFunction(MachineFunction &MF) override {
    return CompressJumpTablesImpl().run(MF);
  }
};

char SHInlineJumpTablesLegacy::ID = 0;
char SHCompressJumpTablesLegacy::ID = 0;

} // namespace

INITIALIZE_PASS(SHInlineJumpTablesLegacy, "sh-inline-jump-tables",
                "Place inline SH jump tables", false, false)
INITIALIZE_PASS(SHCompressJumpTablesLegacy, "sh-compress-jump-tables",
                "Compress inline SH jump tables", false, false)

FunctionPass *llvm::createSHInlineJumpTablesLegacyPass() {
  return new SHInlineJumpTablesLegacy();
}

FunctionPass *llvm::createSHCompressJumpTablesLegacyPass() {
  return new SHCompressJumpTablesLegacy();
}

PreservedAnalyses
SHInlineJumpTablesPass::run(MachineFunction &MF,
                            MachineFunctionAnalysisManager &MFAM) {
  return InlineJumpTablesImpl().run(MF) ? PreservedAnalyses::none()
                                        : PreservedAnalyses::all();
}

PreservedAnalyses
SHCompressJumpTablesPass::run(MachineFunction &MF,
                              MachineFunctionAnalysisManager &MFAM) {
  return CompressJumpTablesImpl().run(MF) ? PreservedAnalyses::none()
                                          : PreservedAnalyses::all();
}
