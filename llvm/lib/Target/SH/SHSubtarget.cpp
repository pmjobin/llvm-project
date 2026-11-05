// SH subtarget information.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include "SHSubtarget.h"
#include "SHMathLibcalls.h"
#include "llvm/TargetParser/SHTargetParser.h"

using namespace llvm;

#define DEBUG_TYPE "sh-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "SHGenSubtargetInfo.inc"

SHSubtarget &SHSubtarget::initializeSubtargetDependencies(StringRef CPU,
                                                          StringRef TuneCPU,
                                                          StringRef FS) {
  if (CPU.empty())
    CPU = SH::DefaultCPU;
  if (TuneCPU.empty())
    TuneCPU = CPU;
  ParseSubtargetFeatures(CPU, TuneCPU, FS);
  return *this;
}

SHSubtarget::SHSubtarget(const Triple &TT, StringRef CPU, StringRef TuneCPU,
                         StringRef FS, SH::ABIKind ABI, bool OptSize,
                         const TargetMachine &TM)
    : SHGenSubtargetInfo(TT, CPU, TuneCPU, FS), ABI(ABI), OptSize(OptSize),
      InstrInfo(initializeSubtargetDependencies(CPU, TuneCPU, FS)),
      FrameLowering(), TLInfo(TM, *this) {}

void SHSubtarget::initLibcallLoweringInfo(LibcallLoweringInfo &Info) const {
  Info.setLibcallImpl(RTLIB::MUL_I32, RTLIB::impl___mulsi3);
  Info.setLibcallImpl(RTLIB::MUL_I64, RTLIB::impl___muldi3);
  Info.setLibcallImpl(RTLIB::SDIV_I32, RTLIB::impl___divsi3);
  Info.setLibcallImpl(RTLIB::UDIV_I32, RTLIB::impl___udivsi3);
  Info.setLibcallImpl(RTLIB::SREM_I32, RTLIB::impl___modsi3);
  Info.setLibcallImpl(RTLIB::UREM_I32, RTLIB::impl___umodsi3);
  Info.setLibcallImpl(RTLIB::SDIVREM_I32, RTLIB::impl___divmodsi4);
  Info.setLibcallImpl(RTLIB::UDIVREM_I32, RTLIB::impl___udivmodsi4);
  Info.setLibcallImpl(RTLIB::SHL_I32, RTLIB::impl___ashlsi3);
  Info.setLibcallImpl(RTLIB::SRL_I32, RTLIB::impl___lshrsi3);
  Info.setLibcallImpl(RTLIB::SRA_I32, RTLIB::impl___ashrsi3);
  Info.setLibcallImpl(RTLIB::SDIV_I64, RTLIB::impl___divdi3);
  Info.setLibcallImpl(RTLIB::UDIV_I64, RTLIB::impl___udivdi3);
  Info.setLibcallImpl(RTLIB::SREM_I64, RTLIB::impl___moddi3);
  Info.setLibcallImpl(RTLIB::UREM_I64, RTLIB::impl___umoddi3);
  Info.setLibcallImpl(RTLIB::ADD_F32, RTLIB::impl___addsf3);
  Info.setLibcallImpl(RTLIB::ADD_F64, RTLIB::impl___adddf3);
  Info.setLibcallImpl(RTLIB::SUB_F32, RTLIB::impl___subsf3);
  Info.setLibcallImpl(RTLIB::SUB_F64, RTLIB::impl___subdf3);
  Info.setLibcallImpl(RTLIB::MUL_F32, RTLIB::impl___mulsf3);
  Info.setLibcallImpl(RTLIB::MUL_F64, RTLIB::impl___muldf3);
  Info.setLibcallImpl(RTLIB::DIV_F32, RTLIB::impl___divsf3);
  Info.setLibcallImpl(RTLIB::DIV_F64, RTLIB::impl___divdf3);
  Info.setLibcallImpl(RTLIB::REM_F32, RTLIB::impl_fmodf);
  Info.setLibcallImpl(RTLIB::REM_F64, RTLIB::impl_fmod);
  Info.setLibcallImpl(RTLIB::SQRT_F32, RTLIB::impl_sqrtf);
  Info.setLibcallImpl(RTLIB::SQRT_F64, RTLIB::impl_sqrt);
  initSHMathLibcalls(Info, !usesEFloatABI());
  Info.setLibcallImpl(RTLIB::FPEXT_F32_F64, RTLIB::impl___extendsfdf2);
  Info.setLibcallImpl(RTLIB::FPROUND_F64_F32, RTLIB::impl___truncdfsf2);
  Info.setLibcallImpl(RTLIB::FPTOSINT_F32_I32, RTLIB::impl___fixsfsi);
  Info.setLibcallImpl(RTLIB::FPTOSINT_F32_I64, RTLIB::impl___fixsfdi);
  Info.setLibcallImpl(RTLIB::FPTOSINT_F64_I32, RTLIB::impl___fixdfsi);
  Info.setLibcallImpl(RTLIB::FPTOSINT_F64_I64, RTLIB::impl___fixdfdi);
  Info.setLibcallImpl(RTLIB::FPTOUINT_F32_I32, RTLIB::impl___fixunssfsi);
  Info.setLibcallImpl(RTLIB::FPTOUINT_F32_I64, RTLIB::impl___fixunssfdi);
  Info.setLibcallImpl(RTLIB::FPTOUINT_F64_I32, RTLIB::impl___fixunsdfsi);
  Info.setLibcallImpl(RTLIB::FPTOUINT_F64_I64, RTLIB::impl___fixunsdfdi);
  Info.setLibcallImpl(RTLIB::SINTTOFP_I32_F32, RTLIB::impl___floatsisf);
  Info.setLibcallImpl(RTLIB::SINTTOFP_I32_F64, RTLIB::impl___floatsidf);
  Info.setLibcallImpl(RTLIB::SINTTOFP_I64_F32, RTLIB::impl___floatdisf);
  Info.setLibcallImpl(RTLIB::SINTTOFP_I64_F64, RTLIB::impl___floatdidf);
  Info.setLibcallImpl(RTLIB::UINTTOFP_I32_F32, RTLIB::impl___floatunsisf);
  Info.setLibcallImpl(RTLIB::UINTTOFP_I32_F64, RTLIB::impl___floatunsidf);
  Info.setLibcallImpl(RTLIB::UINTTOFP_I64_F32, RTLIB::impl___floatundisf);
  Info.setLibcallImpl(RTLIB::UINTTOFP_I64_F64, RTLIB::impl___floatundidf);
  // GNU/compiler-rt comparison helpers return a signed comparison result, not
  // a boolean. Preserve the predicate so legalization compares it with zero.
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_OEQ_F32, RTLIB::impl___eqsf2);
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_OEQ_F64, RTLIB::impl___eqdf2);
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_UNE_F32, RTLIB::impl___nesf2);
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_UNE_F64, RTLIB::impl___nedf2);
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_OGE_F32, RTLIB::impl___gesf2);
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_OGE_F64, RTLIB::impl___gedf2);
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_OLT_F32, RTLIB::impl___ltsf2);
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_OLT_F64, RTLIB::impl___ltdf2);
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_OLE_F32, RTLIB::impl___lesf2);
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_OLE_F64, RTLIB::impl___ledf2);
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_OGT_F32, RTLIB::impl___gtsf2);
  Info.setLibcallImpl(RTLIB::FCMP3_PRED_OGT_F64, RTLIB::impl___gtdf2);
  Info.setLibcallImpl(RTLIB::UO_F32, RTLIB::impl___unordsf2);
  Info.setLibcallImpl(RTLIB::UO_F64, RTLIB::impl___unorddf2);
  if (!hasSH2())
    Info.setLibcallImpl(RTLIB::SDIV_I32, RTLIB::impl___sdivsi3);
  if (usesEFloatABI()) {
    Info.setLibcallImpl(RTLIB::REM_F64, RTLIB::impl_fmodl);
    Info.setLibcallImpl(RTLIB::SQRT_F64, RTLIB::impl_sqrtl);
  }
  Info.setLibcallImpl(RTLIB::MEMCPY, RTLIB::impl_memcpy);
  Info.setLibcallImpl(RTLIB::MEMMOVE, RTLIB::impl_memmove);
  Info.setLibcallImpl(RTLIB::MEMSET, RTLIB::impl_memset);
  Info.setLibcallImpl(RTLIB::UNWIND_RESUME, RTLIB::impl__Unwind_Resume);
  Info.setLibcallImpl(RTLIB::STACK_CHECK_GUARD, RTLIB::impl___stack_chk_guard);
  Info.setLibcallImpl(RTLIB::STACKPROTECTOR_CHECK_FAIL,
                      RTLIB::impl___stack_chk_fail);

  Info.setLibcallImpl(RTLIB::ATOMIC_LOAD, RTLIB::impl___atomic_load);
  Info.setLibcallImpl(RTLIB::ATOMIC_STORE, RTLIB::impl___atomic_store);
  Info.setLibcallImpl(RTLIB::ATOMIC_EXCHANGE, RTLIB::impl___atomic_exchange);
  Info.setLibcallImpl(RTLIB::ATOMIC_COMPARE_EXCHANGE,
                      RTLIB::impl___atomic_compare_exchange);

  Info.setLibcallImpl(RTLIB::ATOMIC_LOAD_1, RTLIB::impl___atomic_load_1);
  Info.setLibcallImpl(RTLIB::ATOMIC_LOAD_2, RTLIB::impl___atomic_load_2);
  Info.setLibcallImpl(RTLIB::ATOMIC_LOAD_4, RTLIB::impl___atomic_load_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_LOAD_8, RTLIB::impl___atomic_load_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_STORE_1, RTLIB::impl___atomic_store_1);
  Info.setLibcallImpl(RTLIB::ATOMIC_STORE_2, RTLIB::impl___atomic_store_2);
  Info.setLibcallImpl(RTLIB::ATOMIC_STORE_4, RTLIB::impl___atomic_store_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_STORE_8, RTLIB::impl___atomic_store_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_EXCHANGE_1,
                      RTLIB::impl___atomic_exchange_1);
  Info.setLibcallImpl(RTLIB::ATOMIC_EXCHANGE_2,
                      RTLIB::impl___atomic_exchange_2);
  Info.setLibcallImpl(RTLIB::ATOMIC_EXCHANGE_4,
                      RTLIB::impl___atomic_exchange_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_EXCHANGE_8,
                      RTLIB::impl___atomic_exchange_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_COMPARE_EXCHANGE_1,
                      RTLIB::impl___atomic_compare_exchange_1);
  Info.setLibcallImpl(RTLIB::ATOMIC_COMPARE_EXCHANGE_2,
                      RTLIB::impl___atomic_compare_exchange_2);
  Info.setLibcallImpl(RTLIB::ATOMIC_COMPARE_EXCHANGE_4,
                      RTLIB::impl___atomic_compare_exchange_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_COMPARE_EXCHANGE_8,
                      RTLIB::impl___atomic_compare_exchange_8);

  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_ADD_1,
                      RTLIB::impl___atomic_fetch_add_1);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_ADD_2,
                      RTLIB::impl___atomic_fetch_add_2);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_ADD_4,
                      RTLIB::impl___atomic_fetch_add_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_ADD_8,
                      RTLIB::impl___atomic_fetch_add_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_SUB_1,
                      RTLIB::impl___atomic_fetch_sub_1);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_SUB_2,
                      RTLIB::impl___atomic_fetch_sub_2);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_SUB_4,
                      RTLIB::impl___atomic_fetch_sub_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_SUB_8,
                      RTLIB::impl___atomic_fetch_sub_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_AND_1,
                      RTLIB::impl___atomic_fetch_and_1);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_AND_2,
                      RTLIB::impl___atomic_fetch_and_2);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_AND_4,
                      RTLIB::impl___atomic_fetch_and_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_AND_8,
                      RTLIB::impl___atomic_fetch_and_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_OR_1,
                      RTLIB::impl___atomic_fetch_or_1);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_OR_2,
                      RTLIB::impl___atomic_fetch_or_2);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_OR_4,
                      RTLIB::impl___atomic_fetch_or_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_OR_8,
                      RTLIB::impl___atomic_fetch_or_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_XOR_1,
                      RTLIB::impl___atomic_fetch_xor_1);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_XOR_2,
                      RTLIB::impl___atomic_fetch_xor_2);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_XOR_4,
                      RTLIB::impl___atomic_fetch_xor_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_XOR_8,
                      RTLIB::impl___atomic_fetch_xor_8);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_NAND_1,
                      RTLIB::impl___atomic_fetch_nand_1);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_NAND_2,
                      RTLIB::impl___atomic_fetch_nand_2);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_NAND_4,
                      RTLIB::impl___atomic_fetch_nand_4);
  Info.setLibcallImpl(RTLIB::ATOMIC_FETCH_NAND_8,
                      RTLIB::impl___atomic_fetch_nand_8);

  for (RTLIB::Libcall Call :
       {RTLIB::ATOMIC_LOAD_16, RTLIB::ATOMIC_STORE_16,
        RTLIB::ATOMIC_EXCHANGE_16, RTLIB::ATOMIC_COMPARE_EXCHANGE_16,
        RTLIB::ATOMIC_FETCH_ADD_16, RTLIB::ATOMIC_FETCH_SUB_16,
        RTLIB::ATOMIC_FETCH_AND_16, RTLIB::ATOMIC_FETCH_OR_16,
        RTLIB::ATOMIC_FETCH_XOR_16, RTLIB::ATOMIC_FETCH_NAND_16})
    Info.setLibcallImpl(Call, RTLIB::Unsupported);
}
