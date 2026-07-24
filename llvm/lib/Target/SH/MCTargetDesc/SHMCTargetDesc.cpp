//===-- SHMCTargetDesc.cpp - SH Target Descriptions ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHMCTargetDesc.h"
#include "SHInstPrinter.h"
#include "SHMCAsmInfo.h"
#include "TargetInfo/SHTargetInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "SHGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "SHGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "SHGenRegisterInfo.inc"

static MCAsmInfo *createSHMCAsmInfo(const MCRegisterInfo &MRI, const Triple &TT,
                                    const MCTargetOptions &Options) {
  return new SHMCAsmInfo(TT, Options);
}

static MCInstrInfo *createSHMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitSHMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createSHMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitSHMCRegisterInfo(X, SH::PR);
  return X;
}

static MCSubtargetInfo *createSHMCSubtargetInfo(const Triple &TT, StringRef CPU,
                                                StringRef FS) {
  if (CPU.empty())
    CPU = "sh2";
  return createSHMCSubtargetInfoImpl(TT, CPU, CPU, FS);
}

static MCInstPrinter *createSHMCInstPrinter(const Triple &TT,
                                            unsigned SyntaxVariant,
                                            const MCAsmInfo &MAI,
                                            const MCInstrInfo &MII,
                                            const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0)
    return new SHInstPrinter(MAI, MII, MRI);
  return nullptr;
}

static void registerTargetMC(Target &T) {
  TargetRegistry::RegisterMCAsmInfo(T, createSHMCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createSHMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createSHMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createSHMCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createSHMCInstPrinter);
  TargetRegistry::RegisterMCCodeEmitter(T, createSHMCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(T, createSHMCAsmBackend);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeSHTargetMC() {
  registerTargetMC(getTheSHTarget());
  registerTargetMC(getTheSHLETarget());
}
