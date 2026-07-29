//===- IntrinsicsSH.td - SH intrinsic definitions -----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

let TargetPrefix = "sh" in {
  // Perform the SH byte test-and-set primitive. The operation observes one
  // byte, returns one exactly when that byte was zero, and stores old | 0x80.
  // It is a monotonic atomic operation; stronger ordering requires a fence.
  def int_sh_tas_b
      : Intrinsic<[llvm_i32_ty], [llvm_ptr_ty],
                  [IntrArgMemOnly, IntrHasSideEffects, IntrWillReturn,
                   NoCapture<ArgIndex<0>>]>;
}
