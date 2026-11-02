; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/arbitrary-memset.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARBITRARY
; RUN: not llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/arbitrary-memcpy.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARBITRARY
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/arbitrary-memmove.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARBITRARY
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/out-of-domain.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=DOMAIN
; RUN: not llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/negative-constant.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=DOMAIN

; ARBITRARY: error: {{.*}}SH i64 memory intrinsic size is not provably 32-bit
; DOMAIN: error: {{.*}}SH memory intrinsic size exceeds the 32-bit address space

;--- arbitrary-memset.ll
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1 immarg)

define void @arbitrary_memset(ptr %dst, i64 %size) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 0, i64 %size, i1 false)
	ret void
}

;--- arbitrary-memcpy.ll
declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1 immarg)

define void @arbitrary_memcpy(ptr %dst, ptr %src, i64 %size) {
	call void @llvm.memcpy.p0.p0.i64(ptr align 1 %dst, ptr align 1 %src, i64 %size, i1 false)
	ret void
}

;--- arbitrary-memmove.ll
declare void @llvm.memmove.p0.p0.i64(ptr, ptr, i64, i1 immarg)

define void @arbitrary_memmove(ptr %dst, ptr %src, i64 %size) {
	call void @llvm.memmove.p0.p0.i64(ptr align 1 %dst, ptr align 1 %src, i64 %size, i1 false)
	ret void
}

;--- out-of-domain.ll
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1 immarg)

define void @out_of_domain(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 0, i64 4294967296, i1 false)
	ret void
}

;--- negative-constant.ll
declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1 immarg)

define void @negative_constant(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i64(ptr align 1 %dst, ptr align 1 %src, i64 -1, i1 false)
	ret void
}
