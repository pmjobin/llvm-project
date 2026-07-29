//===-- SHSubtarget.cpp - SH subtarget information -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHSubtarget.h"
#include "llvm/CodeGen/LibcallLoweringInfo.h"

using namespace llvm;

#define DEBUG_TYPE "sh-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "SHGenSubtargetInfo.inc"

SHSubtarget &SHSubtarget::initializeSubtargetDependencies(StringRef CPU,
                                                          StringRef FS) {
  if (CPU.empty())
    CPU = "sh2";
  ParseSubtargetFeatures(CPU, CPU, FS);
  return *this;
}

SHSubtarget::SHSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                         const TargetMachine &TM)
    : SHGenSubtargetInfo(TT, CPU, CPU, FS),
      InstrInfo(initializeSubtargetDependencies(CPU, FS)), FrameLowering(),
      TLInfo(TM, *this) {}

void SHSubtarget::initLibcallLoweringInfo(LibcallLoweringInfo &Info) const {
  Info.setLibcallImpl(RTLIB::MEMCPY, RTLIB::impl_memcpy);
  Info.setLibcallImpl(RTLIB::MEMMOVE, RTLIB::impl_memmove);
  Info.setLibcallImpl(RTLIB::MEMSET, RTLIB::impl_memset);

  Info.setLibcallImpl(RTLIB::ATOMIC_LOAD, RTLIB::impl___atomic_load);
  Info.setLibcallImpl(RTLIB::ATOMIC_STORE, RTLIB::impl___atomic_store);
  Info.setLibcallImpl(RTLIB::ATOMIC_EXCHANGE, RTLIB::impl___atomic_exchange);
  Info.setLibcallImpl(RTLIB::ATOMIC_COMPARE_EXCHANGE,
                      RTLIB::impl___atomic_compare_exchange);

  Info.setLibcallImpl(RTLIB::ATOMIC_LOAD_4, RTLIB::impl___atomic_load_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_LOAD_8, RTLIB::impl___atomic_load_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_STORE_4, RTLIB::impl___atomic_store_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_STORE_8, RTLIB::impl___atomic_store_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_EXCHANGE_4,
                      RTLIB::impl___atomic_exchange_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_EXCHANGE_8,
                      RTLIB::impl___atomic_exchange_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_COMPARE_EXCHANGE_4,
                      RTLIB::impl___atomic_compare_exchange_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_COMPARE_EXCHANGE_8,
                      RTLIB::impl___atomic_compare_exchange_8);

  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_ADD_4,
                      RTLIB::impl___atomic_fetch_add_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_ADD_8,
                      RTLIB::impl___atomic_fetch_add_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_SUB_4,
                      RTLIB::impl___atomic_fetch_sub_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_SUB_8,
                      RTLIB::impl___atomic_fetch_sub_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_AND_4,
                      RTLIB::impl___atomic_fetch_and_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_AND_8,
                      RTLIB::impl___atomic_fetch_and_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_OR_4,
                      RTLIB::impl___atomic_fetch_or_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_OR_8,
                      RTLIB::impl___atomic_fetch_or_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_XOR_4,
                      RTLIB::impl___atomic_fetch_xor_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_XOR_8,
                      RTLIB::impl___atomic_fetch_xor_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_NAND_4,
                      RTLIB::impl___atomic_fetch_nand_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_NAND_8,
                      RTLIB::impl___atomic_fetch_nand_8);

  for (RTLIB::Libcall Call : {RTLIB::ATOMIC_LOAD_1,
                              RTLIB::ATOMIC_LOAD_2,
                              RTLIB::ATOMIC_LOAD_16,
                              RTLIB::ATOMIC_STORE_1,
                              RTLIB::ATOMIC_STORE_2,
                              RTLIB::ATOMIC_STORE_16,
                              RTLIB::ATOMIC_EXCHANGE_1,
                              RTLIB::ATOMIC_EXCHANGE_2,
                              RTLIB::ATOMIC_EXCHANGE_16,
                              RTLIB::ATOMIC_COMPARE_EXCHANGE_1,
                              RTLIB::ATOMIC_COMPARE_EXCHANGE_2,
                              RTLIB::ATOMIC_COMPARE_EXCHANGE_16,
                              RTLIB::ATOMIC_FETCH_ADD_1,
                              RTLIB::ATOMIC_FETCH_ADD_2,
                              RTLIB::ATOMIC_FETCH_ADD_16,
                              RTLIB::ATOMIC_FETCH_SUB_1,
                              RTLIB::ATOMIC_FETCH_SUB_2,
                              RTLIB::ATOMIC_FETCH_SUB_16,
                              RTLIB::ATOMIC_FETCH_AND_1,
                              RTLIB::ATOMIC_FETCH_AND_2,
                              RTLIB::ATOMIC_FETCH_AND_16,
                              RTLIB::ATOMIC_FETCH_OR_1,
                              RTLIB::ATOMIC_FETCH_OR_2,
                              RTLIB::ATOMIC_FETCH_OR_16,
                              RTLIB::ATOMIC_FETCH_XOR_1,
                              RTLIB::ATOMIC_FETCH_XOR_2,
                              RTLIB::ATOMIC_FETCH_XOR_16,
                              RTLIB::ATOMIC_FETCH_NAND_1,
                              RTLIB::ATOMIC_FETCH_NAND_2,
                              RTLIB::ATOMIC_FETCH_NAND_16})
    Info.setLibcallImpl(Call, RTLIB::Unsupported);
}
