; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/float-member.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-member.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/scalable-member.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/byval-align-eight.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=BYVAL-ALIGN
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/byval-zero.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=BYVAL-ZERO
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/byval-too-large.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=FRAME
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/byval-address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=BYVAL-AS
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/aggregate-cc.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/aggregate-select.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SELECT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/aggregate-align-eight.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=STACK-ALIGN
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/multiple-sret.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MULTIPLE-SRET
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/volatile-dynamic.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VOLATILE-DYNAMIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/volatile-large-memcpy.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VOLATILE-60
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/volatile-large-memmove.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VOLATILE-16
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memory-address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MEMORY-AS
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memcpy-inline.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INLINE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memcpy-inline-large.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INLINE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memset-inline.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INLINE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memset-inline-large.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INLINE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memcpy-atomic.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ATOMIC

; ARGUMENT: LLVM ERROR: SH function arguments must be scalar i32, i64, or pointers
; BYVAL-ALIGN: LLVM ERROR: SH byval alignment greater than 4 requires unsupported stack realignment
; BYVAL-ZERO: LLVM ERROR: SH zero-sized byval arguments are not supported
; FRAME: LLVM ERROR: SH outgoing call frame size cannot exceed 60 bytes
; BYVAL-AS: LLVM ERROR: SH nonzero address spaces are not supported
; CC: LLVM ERROR: SH only supports the C calling convention
; SELECT: LLVM ERROR: SH select is not supported
; STACK-ALIGN: LLVM ERROR: SH stack object alignment cannot exceed 4 bytes
; MULTIPLE-SRET: Cannot have multiple 'sret' parameters!
; VOLATILE-DYNAMIC: LLVM ERROR: SH volatile memory intrinsics require a constant size
; VOLATILE-60: LLVM ERROR: SH volatile memory intrinsic size cannot exceed 60 bytes
; VOLATILE-16: LLVM ERROR: SH volatile memory intrinsic size cannot exceed 16 bytes
; MEMORY-AS: LLVM ERROR: SH nonzero address spaces are not supported
; INLINE: LLVM ERROR: SH atomic and inline-only memory intrinsics are not supported
; ATOMIC: LLVM ERROR: SH intrinsics are not supported

;--- float-member.ll
%F = type { float }

define void @float_member(%F %value) {
	ret void
}

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

;--- byval-align-eight.ll
%B8 = type [8 x i8]

declare void @take8(ptr byval(%B8) align 8)

define void @byval_align_eight(ptr %source) {
	call void @take8(ptr byval(%B8) align 8 %source)
	ret void
}

;--- byval-zero.ll
%B0 = type [0 x i8]

declare void @take0(ptr byval(%B0) align 1)

define void @byval_zero(ptr %source) {
	call void @take0(ptr byval(%B0) align 1 %source)
	ret void
}

;--- byval-too-large.ll
%B77 = type [77 x i8]

declare void @take77(ptr byval(%B77) align 1)

define void @byval_too_large(ptr %source) {
	call void @take77(ptr byval(%B77) align 1 %source)
	ret void
}

;--- byval-address-space.ll
%B4 = type [4 x i8]

@as1_source = addrspace(1) global %B4 zeroinitializer

declare void @take_as1(ptr addrspace(1) byval(%B4) align 1)

define void @byval_address_space() {
	call void @take_as1(ptr addrspace(1) byval(%B4) align 1 @as1_source)
	ret void
}

;--- aggregate-cc.ll
%S8 = type { i32, i32 }

define void @aggregate_cc(ptr %callee, %S8 %value) {
	call fastcc void %callee(%S8 %value)
	ret void
}

;--- aggregate-select.ll
%S8 = type { i32, i32 }

define %S8 @aggregate_select(%S8 %left, %S8 %right) {
	%result = select i1 true, %S8 %left, %S8 %right
	ret %S8 %result
}

;--- aggregate-align-eight.ll
%S8 = type { i32, i32 }

define void @aggregate_align_eight() {
	%value = alloca %S8, align 8
	store %S8 zeroinitializer, ptr %value, align 8
	ret void
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
