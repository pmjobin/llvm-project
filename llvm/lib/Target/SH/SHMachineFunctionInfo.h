//===-- SHMachineFunctionInfo.h - SH machine function info ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_SH_SHMACHINEFUNCTIONINFO_H

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"
#include <optional>
#include <string>

namespace llvm {

class SHMachineFunctionInfo : public MachineFunctionInfo {
public:
  enum class HandlerKind { Ordinary, Interrupt, TRAPA };

private:
  mutable SmallPtrSet<const MachineInstr *, 4> InlineAsmSizeDiagnostics;
  HandlerKind Handler = HandlerKind::Ordinary;
  bool NoSaveLowRegs = false;
  bool UsesResbank = false;
  std::string StackSwitchSymbol;
  std::optional<unsigned> TrapExitNumber;
  Register SRetReturnReg;
  int VarArgsFrameIndex = 0;
  unsigned VarArgsSaveSize = 0;
  bool HasVarArgsSaveArea = false;
  int VarArgsGPRFrameIndex = 0;
  int VarArgsFPRFrameIndex = 0;
  int VarArgsStackFrameIndex = 0;
  unsigned VarArgsGPRCursor = 0;
  unsigned VarArgsFPRCursor = 0;
  bool HasFPUVarArgsAreas = false;
  bool UsesPICBase = false;
  bool NeedsCallFrameScratch = false;
  bool UseReservedCallFrame = true;
  SmallVector<std::optional<int64_t>, 8> IncomingStackArgumentOffsets;
  uint64_t RealignmentFirstAdjust = 0;
  int ReturnAddressFrameIndex = 0;
  bool HasReturnAddressFrameIndex = false;
  int MACHFrameIndex = 0;
  int MACLFrameIndex = 0;
  int FPULFrameIndex = 0;
  int FPSCRFrameIndex = 0;
  bool SaveMACH = false;
  bool SaveMACL = false;
  bool SaveFPUL = false;
  bool SaveFPSCR = false;
  SmallVector<std::pair<MCRegister, int>, 16> FPRSpills;
  int PICBaseCPI = -1;

public:
  SHMachineFunctionInfo(const Function &F, const TargetSubtargetInfo *) {
    Attribute HandlerAttr = F.getFnAttribute("sh-interrupt-handler");
    if (!HandlerAttr.isValid())
      return;
    StringRef Kind = HandlerAttr.getValueAsString();
    if (Kind == "interrupt")
      Handler = HandlerKind::Interrupt;
    else if (Kind == "trapa")
      Handler = HandlerKind::TRAPA;
    else
      report_fatal_error("SH handler attribute has an invalid value");
    NoSaveLowRegs = F.hasFnAttribute("sh-nosave-low-regs");
    UsesResbank = F.hasFnAttribute("sh-resbank");
    if (Attribute A = F.getFnAttribute("sh-sp-switch"); A.isValid())
      StackSwitchSymbol = A.getValueAsString().str();
    if (Attribute A = F.getFnAttribute("sh-trap-exit"); A.isValid()) {
      unsigned Number;
      if (A.getValueAsString().getAsInteger(10, Number) || Number > 255)
        report_fatal_error("SH trap_exit attribute has an invalid number");
      TrapExitNumber = Number;
    }
  }

  HandlerKind getHandlerKind() const { return Handler; }
  bool isHandler() const { return Handler != HandlerKind::Ordinary; }
  bool isInterruptHandler() const { return Handler == HandlerKind::Interrupt; }
  bool isTRAPAHandler() const { return Handler == HandlerKind::TRAPA; }
  bool noSaveLowRegs() const { return NoSaveLowRegs; }
  bool usesResbank() const { return UsesResbank; }
  bool hasStackSwitch() const { return !StackSwitchSymbol.empty(); }
  StringRef getStackSwitchSymbol() const { return StackSwitchSymbol; }
  bool hasTrapExit() const { return TrapExitNumber.has_value(); }
  unsigned getTrapExitNumber() const { return *TrapExitNumber; }

  bool markInlineAsmSizeDiagnosticReported(const MachineInstr *MI) const {
    return InlineAsmSizeDiagnostics.insert(MI).second;
  }

  Register getSRetReturnReg() const { return SRetReturnReg; }
  void setSRetReturnReg(Register Reg) { SRetReturnReg = Reg; }

  bool hasVarArgsSaveArea() const { return HasVarArgsSaveArea; }
  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  unsigned getVarArgsSaveSize() const { return VarArgsSaveSize; }
  void setVarArgsSaveArea(int FrameIndex, unsigned SaveSize) {
    VarArgsFrameIndex = FrameIndex;
    VarArgsSaveSize = SaveSize;
    HasVarArgsSaveArea = true;
  }
  bool hasFPUVarArgsAreas() const { return HasFPUVarArgsAreas; }
  int getVarArgsGPRFrameIndex() const { return VarArgsGPRFrameIndex; }
  int getVarArgsFPRFrameIndex() const { return VarArgsFPRFrameIndex; }
  int getVarArgsStackFrameIndex() const { return VarArgsStackFrameIndex; }
  unsigned getVarArgsGPRCursor() const { return VarArgsGPRCursor; }
  unsigned getVarArgsFPRCursor() const { return VarArgsFPRCursor; }
  void setFPUVarArgsAreas(int GPRFrameIndex, int FPRFrameIndex,
                          int StackFrameIndex, unsigned GPRCursor,
                          unsigned FPRCursor) {
    VarArgsGPRFrameIndex = GPRFrameIndex;
    VarArgsFPRFrameIndex = FPRFrameIndex;
    VarArgsStackFrameIndex = StackFrameIndex;
    VarArgsGPRCursor = GPRCursor;
    VarArgsFPRCursor = FPRCursor;
    HasFPUVarArgsAreas = true;
    HasVarArgsSaveArea = true;
  }

  bool usesPICBase() const { return UsesPICBase; }
  bool needsCallFrameScratch() const { return NeedsCallFrameScratch; }
  void setNeedsCallFrameScratch() { NeedsCallFrameScratch = true; }
  bool useReservedCallFrame() const { return UseReservedCallFrame; }
  void setUseReservedCallFrame(bool Value) { UseReservedCallFrame = Value; }
  void setIncomingStackArgumentOffset(unsigned ArgNo, int64_t Offset) {
    if (IncomingStackArgumentOffsets.size() <= ArgNo)
      IncomingStackArgumentOffsets.resize(ArgNo + 1);
    IncomingStackArgumentOffsets[ArgNo] = Offset;
  }
  std::optional<int64_t> getIncomingStackArgumentOffset(unsigned ArgNo) const {
    return ArgNo < IncomingStackArgumentOffsets.size()
               ? IncomingStackArgumentOffsets[ArgNo]
               : std::nullopt;
  }
  uint64_t getRealignmentFirstAdjust() const { return RealignmentFirstAdjust; }
  void setRealignmentFirstAdjust(uint64_t Size) {
    RealignmentFirstAdjust = Size;
  }
  bool hasReturnAddressFrameIndex() const { return HasReturnAddressFrameIndex; }
  int getReturnAddressFrameIndex() const { return ReturnAddressFrameIndex; }
  void setReturnAddressFrameIndex(int FrameIndex) {
    ReturnAddressFrameIndex = FrameIndex;
    HasReturnAddressFrameIndex = true;
  }
  bool savesMACH() const { return SaveMACH; }
  bool savesMACL() const { return SaveMACL; }
  void setNeedsMACHSave() { SaveMACH = true; }
  void setNeedsMACLSave() { SaveMACL = true; }
  int getMACHFrameIndex() const { return MACHFrameIndex; }
  int getMACLFrameIndex() const { return MACLFrameIndex; }
  void setMACHFrameIndex(int FrameIndex) {
    MACHFrameIndex = FrameIndex;
    SaveMACH = true;
  }
  void setMACLFrameIndex(int FrameIndex) {
    MACLFrameIndex = FrameIndex;
    SaveMACL = true;
  }
  bool savesFPUL() const { return SaveFPUL; }
  bool savesFPSCR() const { return SaveFPSCR; }
  void setNeedsFPULSave() { SaveFPUL = true; }
  void setNeedsFPSCRSave() { SaveFPSCR = true; }
  int getFPULFrameIndex() const { return FPULFrameIndex; }
  int getFPSCRFrameIndex() const { return FPSCRFrameIndex; }
  void setFPULFrameIndex(int FrameIndex) {
    FPULFrameIndex = FrameIndex;
    SaveFPUL = true;
  }
  void setFPSCRFrameIndex(int FrameIndex) {
    FPSCRFrameIndex = FrameIndex;
    SaveFPSCR = true;
  }
  bool hasFPRSpills() const { return !FPRSpills.empty(); }
  ArrayRef<std::pair<MCRegister, int>> getFPRSpills() const {
    return FPRSpills;
  }
  void addFPRSpill(MCRegister Reg, int FrameIndex) {
    FPRSpills.emplace_back(Reg, FrameIndex);
  }
  int getPICBaseCPI() const { return PICBaseCPI; }
  void setPICBaseCPI(unsigned CPI) {
    PICBaseCPI = CPI;
    UsesPICBase = true;
  }
};

} // namespace llvm

#endif
