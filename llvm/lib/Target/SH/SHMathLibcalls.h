//===- SHMathLibcalls.h - Scalar libm entry points -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHMATHLIBCALLS_H
#define LLVM_LIB_TARGET_SH_SHMATHLIBCALLS_H

#include "llvm/CodeGen/LibcallLoweringInfo.h"

namespace llvm {

inline void initSHMathLibcalls(LibcallLoweringInfo &Info, bool HasDoubleABI) {
  Info.setLibcallImpl(RTLIB::CEIL_F32, RTLIB::impl_ceilf);
  Info.setLibcallImpl(RTLIB::FLOOR_F32, RTLIB::impl_floorf);
  Info.setLibcallImpl(RTLIB::RINT_F32, RTLIB::impl_rintf);
  Info.setLibcallImpl(RTLIB::TRUNC_F32, RTLIB::impl_truncf);
  Info.setLibcallImpl(RTLIB::LOG10_F32, RTLIB::impl_log10f);
  Info.setLibcallImpl(RTLIB::ROUND_F32, RTLIB::impl_roundf);
  Info.setLibcallImpl(RTLIB::FMIN_F32, RTLIB::impl_fminf);
  Info.setLibcallImpl(RTLIB::FMAX_F32, RTLIB::impl_fmaxf);
  // In the E-float ABI C double is f32. Its public double entry points do not
  // provide an f64 calling convention. Keep those f64 libcalls unavailable.
  if (HasDoubleABI) {
    Info.setLibcallImpl(RTLIB::CEIL_F64, RTLIB::impl_ceil);
    Info.setLibcallImpl(RTLIB::FLOOR_F64, RTLIB::impl_floor);
    Info.setLibcallImpl(RTLIB::RINT_F64, RTLIB::impl_rint);
    Info.setLibcallImpl(RTLIB::TRUNC_F64, RTLIB::impl_trunc);
    Info.setLibcallImpl(RTLIB::LOG10_F64, RTLIB::impl_log10);
    Info.setLibcallImpl(RTLIB::ROUND_F64, RTLIB::impl_round);
    Info.setLibcallImpl(RTLIB::FMIN_F64, RTLIB::impl_fmin);
    Info.setLibcallImpl(RTLIB::FMAX_F64, RTLIB::impl_fmax);
  }
}

} // namespace llvm

#endif
