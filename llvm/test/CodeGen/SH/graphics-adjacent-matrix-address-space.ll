; RUN: split-file %s %t
; RUN: not llc -mtriple=shle -mcpu=sh4 -verify-machineinstrs %t/source.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=shle -mcpu=sh4 -verify-machineinstrs %t/destination.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=shle -mcpu=sh4 -enable-new-pm=1 -verify-machineinstrs %t/source.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=shle -mcpu=sh4 -enable-new-pm=1 -verify-machineinstrs %t/destination.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=sh -mcpu=sh4 -verify-machineinstrs %t/source.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=sh -mcpu=sh4 -verify-machineinstrs %t/destination.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=sh -mcpu=sh4 -enable-new-pm=1 -verify-machineinstrs %t/source.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=sh -mcpu=sh4 -enable-new-pm=1 -verify-machineinstrs %t/destination.ll -o /dev/null 2>&1 | FileCheck %s

; Unsupported address spaces must reach the target diagnostic before fusion
; constructs a fixed-AS0 intrinsic. This is not an address-space support test.
; CHECK: error: {{.*}}SH nonzero address spaces are not supported
; CHECK-NOT: Assertion

;--- source.ll
declare <4 x float> @llvm.sh.ftrv(ptr, <4 x float>)
define void @source_as1(ptr noalias %out, ptr addrspace(1) noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr addrspace(1) %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr addrspace(1) %in, i32 16
  %v1 = load <4 x float>, ptr addrspace(1) %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

;--- destination.ll
declare <4 x float> @llvm.sh.ftrv(ptr, <4 x float>)
define void @destination_as1(ptr addrspace(1) noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr addrspace(1) %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr addrspace(1) %out, i32 16
  store <4 x float> %r1, ptr addrspace(1) %d1, align 16
  ret void
}
