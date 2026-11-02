; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-member.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/scalable-member.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/byval-zero.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=BYVAL-ZERO
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/byval-address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=BYVAL-AS
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/aggregate-cc.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CC
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/aggregate-select.ll -o /dev/null
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/multiple-sret.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MULTIPLE-SRET
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/volatile-dynamic.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VOLATILE-DYNAMIC
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/volatile-large-memcpy.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VOLATILE-60
; RUN: not llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs %t/volatile-large-memcpy.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VOLATILE-60
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/volatile-large-memmove.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VOLATILE-16
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memory-address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MEMORY-AS
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memcpy-inline.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INLINE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memcpy-inline-large.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INLINE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memset-inline.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INLINE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memset-inline-large.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INLINE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memcpy-atomic.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ATOMIC

; ARGUMENT: error: {{.*}}SH function argument type is not supported by the ABI
; BYVAL-ZERO: error: {{.*}}SH zero-sized byval arguments are not supported
; BYVAL-AS: LLVM ERROR: SH nonzero address spaces are not supported
; CC: error: {{.*}}SH only supports the C and fast calling conventions
; MULTIPLE-SRET: Cannot have multiple 'sret' parameters!
; VOLATILE-DYNAMIC: error: {{.*}}SH volatile memory intrinsics require a constant size
; VOLATILE-60: error: {{.*}}SH volatile memory intrinsic size cannot exceed 60 bytes
; VOLATILE-16: error: {{.*}}SH volatile memory intrinsic size cannot exceed 16 bytes
; MEMORY-AS: LLVM ERROR: SH nonzero address spaces are not supported
; INLINE: error: {{.*}}SH atomic and inline-only memory intrinsics are not supported
; ATOMIC: error: {{.*}}SH atomic memory intrinsics are not supported

;--- vector-member.ll
%V = type { <2 x i32> }

define void @vector_member(%V %value) {
	ret void
}

;--- scalable-member.ll
%SV = type { <vscale x 2 x i32> }

define void @scalable_member(%SV %value) {
	ret void
}

;--- byval-zero.ll
%B0 = type [0 x i32]

declare void @take0(ptr byval(%B0) align 1)

define void @byval_zero(ptr %source) {
	call void @take0(ptr byval(%B0) align 1 %source)
	ret void
}

;--- byval-address-space.ll
%B4 = type [1 x i32]

@as1_source = addrspace(1) global %B4 zeroinitializer

declare void @take_as1(ptr addrspace(1) byval(%B4) align 1)

define void @byval_address_space() {
	call void @take_as1(ptr addrspace(1) byval(%B4) align 1 @as1_source)
	ret void
}

;--- aggregate-cc.ll
%S8 = type { i32, i32 }

define void @aggregate_cc(ptr %callee, %S8 %value) {
	call coldcc void %callee(%S8 %value)
	ret void
}

;--- aggregate-select.ll
%S8 = type { i32, i32 }

define %S8 @aggregate_select(%S8 %left, %S8 %right) {
	%result = select i1 true, %S8 %left, %S8 %right
	ret %S8 %result
}

;--- multiple-sret.ll
%S4 = type { i32 }

define void @multiple_sret(ptr sret(%S4) %first, ptr sret(%S4) %second) {
	ret void
}

;--- volatile-dynamic.ll
declare void @llvm.memcpy.p0.p0.i32(ptr, ptr, i32, i1 immarg)

define void @volatile_dynamic(ptr %dst, ptr %src, i32 %size) {
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 %dst, ptr align 1 %src, i32 %size, i1 true)
	ret void
}

;--- volatile-large-memcpy.ll
declare void @llvm.memcpy.p0.p0.i32(ptr, ptr, i32, i1 immarg)

define void @volatile_large_memcpy(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 %dst, ptr align 1 %src, i32 61, i1 true)
	ret void
}

;--- volatile-large-memmove.ll
declare void @llvm.memmove.p0.p0.i32(ptr, ptr, i32, i1 immarg)

define void @volatile_large_memmove(ptr %dst, ptr %src) {
	call void @llvm.memmove.p0.p0.i32(ptr align 1 %dst, ptr align 1 %src, i32 17, i1 true)
	ret void
}

;--- memory-address-space.ll
declare void @llvm.memcpy.p1.p1.i32(ptr addrspace(1), ptr addrspace(1), i32, i1 immarg)

@as1_dst = addrspace(1) global [4 x i8] zeroinitializer
@as1_src = addrspace(1) global [4 x i8] zeroinitializer

define void @memory_address_space() {
	call void @llvm.memcpy.p1.p1.i32(ptr addrspace(1) align 1 @as1_dst, ptr addrspace(1) align 1 @as1_src, i32 4, i1 false)
	ret void
}

;--- memcpy-inline.ll
declare void @llvm.memcpy.inline.p0.p0.i32(ptr, ptr, i32 immarg, i1 immarg)

define void @memcpy_inline(ptr %dst, ptr %src) {
	call void @llvm.memcpy.inline.p0.p0.i32(ptr align 1 %dst, ptr align 1 %src, i32 4, i1 false)
	ret void
}

;--- memcpy-inline-large.ll
declare void @llvm.memcpy.inline.p0.p0.i32(ptr, ptr, i32 immarg, i1 immarg)

define void @memcpy_inline_large(ptr %dst, ptr %src) {
	call void @llvm.memcpy.inline.p0.p0.i32(ptr align 1 %dst, ptr align 1 %src, i32 61, i1 false)
	ret void
}

;--- memset-inline.ll
declare void @llvm.memset.inline.p0.i32(ptr, i8, i32 immarg, i1 immarg)

define void @memset_inline(ptr %dst) {
	call void @llvm.memset.inline.p0.i32(ptr align 1 %dst, i8 90, i32 4, i1 false)
	ret void
}

;--- memset-inline-large.ll
declare void @llvm.memset.inline.p0.i32(ptr, i8, i32 immarg, i1 immarg)

define void @memset_inline_large(ptr %dst) {
	call void @llvm.memset.inline.p0.i32(ptr align 1 %dst, i8 90, i32 61, i1 false)
	ret void
}

;--- memcpy-atomic.ll
declare void @llvm.memcpy.element.unordered.atomic.p0.p0.i32(ptr, ptr, i32, i32 immarg)

define void @memcpy_atomic(ptr %dst, ptr %src) {
	call void @llvm.memcpy.element.unordered.atomic.p0.p0.i32(ptr align 1 %dst, ptr align 1 %src, i32 61, i32 1)
	ret void
}
