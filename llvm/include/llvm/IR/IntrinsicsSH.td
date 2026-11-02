//===- IntrinsicsSH.td - SH intrinsic definitions -----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

let TargetPrefix = "sh" in {
  // Explicit context-management reads must observe writes, including writes
  // performed by an out-of-line wrapper. Do not inherit thread.pointer's
  // invariant contract or change that contract for other targets.
  def int_sh_get_thread_pointer : Intrinsic<[llvm_ptr_ty], [],
      [IntrInaccessibleMemOnly, IntrHasSideEffects]>;
  def int_sh_set_thread_pointer : Intrinsic<[], [llvm_ptr_ty],
      [IntrInaccessibleMemOnly, IntrHasSideEffects]>;
  // Share the explicit graphics FP-state ordering. The setter preserves the
  // current FR/SZ/PR, while replacing the caller-selected nonmode fields.
  def int_sh_get_fpscr : Intrinsic<[llvm_i32_ty], [],
      [IntrInaccessibleMemOnly, IntrHasSideEffects]>;
  def int_sh_set_fpscr : Intrinsic<[], [llvm_i32_ty],
      [IntrInaccessibleMemOnly, IntrHasSideEffects]>;
  // Observable FP status prevents speculation and elimination. These are
  // explicit hardware operations, not substitutes for strict scalar math.
  def int_sh_fsca : Intrinsic<[llvm_float_ty, llvm_float_ty], [llvm_i32_ty],
                             [IntrInaccessibleMemOnly, IntrHasSideEffects]>;
  def int_sh_fsrra : Intrinsic<[llvm_float_ty], [llvm_float_ty],
                              [IntrInaccessibleMemOnly, IntrHasSideEffects]>;
  def int_sh_fipr : Intrinsic<[llvm_float_ty], [llvm_v4f32_ty, llvm_v4f32_ty],
                             [IntrInaccessibleMemOnly, IntrHasSideEffects]>;
  def int_sh_ftrv : Intrinsic<[llvm_v4f32_ty], [llvm_ptr_ty, llvm_v4f32_ty],
                             [IntrRead<[ArgMem, InaccessibleMem]>,
                              IntrWrite<[InaccessibleMem]>, IntrHasSideEffects,
                              NoCapture<ArgIndex<0>>]>;
  def int_sh_ftrv4 : Intrinsic<[], [llvm_ptr_ty, llvm_ptr_ty, llvm_ptr_ty],
                              [IntrInaccessibleMemOrArgMemOnly, IntrHasSideEffects,
                               WriteOnly<ArgIndex<0>>, ReadOnly<ArgIndex<1>>,
                               ReadOnly<ArgIndex<2>>, NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
                               NoCapture<ArgIndex<2>>]>;
  def int_sh_ftrv_array : Intrinsic<[],
      [llvm_ptr_ty, llvm_ptr_ty, llvm_i32_ty, llvm_ptr_ty],
      [IntrInaccessibleMemOrArgMemOnly, IntrHasSideEffects,
       WriteOnly<ArgIndex<0>>, ReadOnly<ArgIndex<1>>, ReadOnly<ArgIndex<3>>,
       NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>, NoCapture<ArgIndex<3>>]>;
  def int_sh_ftrv_compose : Intrinsic<[],
      [llvm_ptr_ty, llvm_ptr_ty, llvm_i32_ty, llvm_ptr_ty, llvm_ptr_ty],
      [IntrInaccessibleMemOrArgMemOnly, IntrHasSideEffects,
       WriteOnly<ArgIndex<0>>, ReadOnly<ArgIndex<1>>, ReadOnly<ArgIndex<3>>,
       ReadOnly<ArgIndex<4>>, NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
       NoCapture<ArgIndex<3>>, NoCapture<ArgIndex<4>>]>;

  // Perform the SH byte test-and-set primitive. The operation observes one
  // byte, returns one exactly when that byte was zero, and stores old | 0x80.
  // It is a monotonic atomic operation; stronger ordering requires a fence.
  def int_sh_tas_b
      : Intrinsic<[llvm_i32_ty], [llvm_ptr_ty],
                  [IntrArgMemOnly, IntrHasSideEffects, IntrWillReturn,
                   NoCapture<ArgIndex<0>>]>;
}
