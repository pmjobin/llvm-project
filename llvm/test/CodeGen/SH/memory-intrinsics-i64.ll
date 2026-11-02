; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=COMMON
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=COMMON
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-before=finalize-isel %s -o - | FileCheck %s --check-prefix=IR
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o /dev/null

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1 immarg)
declare void @llvm.memmove.p0.p0.i64(ptr, ptr, i64, i1 immarg)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1 immarg)

; COMMON-LABEL: padding_memset:
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON-NOT: jsr
; COMMON: rts
; COMMON-NEXT: mov.b
; IR-LABEL: define void @padding_memset(
; IR: call void @llvm.memset.p0.i32(ptr align 1 %dst, i8 0, i32 1, i1 false)
define void @padding_memset(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 0, i64 1, i1 false)
	ret void
}

; COMMON-LABEL: memcpy5_align1:
; COMMON-COUNT-9: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON-NOT: jsr
; COMMON: rts
; COMMON-NEXT: mov.b
define void @memcpy5_align1(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i64(ptr align 1 %dst, ptr align 1 %src, i64 5, i1 false)
	ret void
}

; COMMON-LABEL: memcpy7_align2:
; COMMON-COUNT-6: mov.w
; COMMON: mov.b
; COMMON-NOT: jsr
; COMMON: rts
; COMMON-NEXT: mov.b
define void @memcpy7_align2(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i64(ptr align 2 %dst, ptr align 2 %src, i64 7, i1 false)
	ret void
}

; COMMON-LABEL: memcpy12_align4:
; COMMON-COUNT-5: mov.l
; COMMON-NOT: jsr
; COMMON: rts
; COMMON-NEXT: mov.l
define void @memcpy12_align4(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i64(ptr align 4 %dst, ptr align 4 %src, i64 12, i1 false)
	ret void
}

; All loads precede all stores, so either overlap direction is safe.
; COMMON-LABEL: memmove8_align1:
; COMMON-COUNT-16: mov.b
; COMMON-NOT: jsr
; COMMON: rts
define void @memmove8_align1(ptr %dst, ptr %src) {
	call void @llvm.memmove.p0.p0.i64(ptr align 1 %dst, ptr align 1 %src, i64 8, i1 false)
	ret void
}

; COMMON-LABEL: memset7_align2:
; COMMON: mov.b
; COMMON-COUNT-2: mov.w
; COMMON-NOT: jsr
; COMMON: rts
; COMMON-NEXT: mov.w
define void @memset7_align2(ptr %dst, i32 %input) {
	%value = trunc i32 %input to i8
	call void @llvm.memset.p0.i64(ptr align 2 %dst, i8 %value, i64 7, i1 false)
	ret void
}

; COMMON-LABEL: memset12_align4:
; COMMON-COUNT-2: mov.l
; COMMON-NOT: jsr
; COMMON: rts
; COMMON-NEXT: mov.l
define void @memset12_align4(ptr %dst, i32 %input) {
	%value = trunc i32 %input to i8
	call void @llvm.memset.p0.i64(ptr align 4 %dst, i8 %value, i64 12, i1 false)
	ret void
}

; COMMON-LABEL: zero_lengths:
; COMMON-NOT: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON-NOT: jsr
; COMMON: rts
define void @zero_lengths(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i64(ptr align 4 %dst, ptr align 4 %src, i64 0, i1 false)
	call void @llvm.memmove.p0.p0.i64(ptr align 4 %dst, ptr align 4 %src, i64 0, i1 false)
	call void @llvm.memset.p0.i64(ptr align 4 %dst, i8 1, i64 0, i1 false)
	ret void
}

; Constants through 32 use inline operations; larger constants use correctly
; typed i32-size libcalls according to the existing SH thresholds. Keeping one
; operation per function also checks that lowering does not create an
; artificial large frame.
define void @memset_len0(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 0, i1 false)
	ret void
}

define void @memset_len1(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 1, i1 false)
	ret void
}

define void @memset_len2(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 2, i1 false)
	ret void
}

define void @memset_len3(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 3, i1 false)
	ret void
}

define void @memset_len4(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 4, i1 false)
	ret void
}

define void @memset_len5(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 5, i1 false)
	ret void
}

define void @memset_len7(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 7, i1 false)
	ret void
}

define void @memset_len8(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 8, i1 false)
	ret void
}

define void @memset_len12(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 12, i1 false)
	ret void
}

define void @memset_len16(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 16, i1 false)
	ret void
}

define void @memset_len31(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 31, i1 false)
	ret void
}

define void @memset_len32(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 32, i1 false)
	ret void
}

define void @memset_len64(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 64, i1 false)
	ret void
}

define void @memset_len255(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 255, i1 false)
	ret void
}

define void @memset_len256(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 256, i1 false)
	ret void
}

define void @memset_len_uint32_max(ptr %dst) {
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 90, i64 4294967295, i1 false)
	ret void
}

define void @memcpy_len64(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i64(ptr align 4 %dst, ptr align 4 %src, i64 64, i1 false)
	ret void
}

define void @memmove_len31(ptr %dst, ptr %src) {
	call void @llvm.memmove.p0.p0.i64(ptr align 4 %dst, ptr align 4 %src, i64 31, i1 false)
	ret void
}

; COMMON-LABEL: zext_lengths:
; COMMON: jsr
; COMMON: .long	memset
; COMMON: .long	memcpy
; COMMON: .long	memmove
; IR-LABEL: define void @zext_lengths(
; IR: call void @llvm.memset.p0.i32(ptr align 1 %dst, i8 %value, i32 %size, i1 false)
; IR: call void @llvm.memcpy.p0.p0.i32(ptr align 2 %dst, ptr align 2 %src, i32 %size, i1 false)
; IR: call void @llvm.memmove.p0.p0.i32(ptr align 4 %dst, ptr align 4 %src, i32 %size, i1 false)
define void @zext_lengths(ptr %dst, ptr %src, i32 %size, i32 %input) {
	%value = trunc i32 %input to i8
	%wide = zext i32 %size to i64
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 %value, i64 %wide, i1 false)
	call void @llvm.memcpy.p0.p0.i64(ptr align 2 %dst, ptr align 2 %src, i64 %wide, i1 false)
	call void @llvm.memmove.p0.p0.i64(ptr align 4 %dst, ptr align 4 %src, i64 %wide, i1 false)
	ret void
}

define void @masked_length(ptr %dst, i64 %size, i32 %input) {
	%value = trunc i32 %input to i8
	%bounded = and i64 %size, 4294967295
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 %value, i64 %bounded, i1 false)
	ret void
}

; MIR-LABEL: name: volatile_i64_intrinsics
; MIR: volatile load
; MIR: volatile store
; MIR: volatile store
; MIR-NOT: JSR
; IR-LABEL: define void @volatile_i64_intrinsics(
; IR: call void @llvm.memcpy.p0.p0.i32({{.*}}i32 4, i1 true)
; IR: call void @llvm.memmove.p0.p0.i32({{.*}}i32 4, i1 true)
; IR: call void @llvm.memset.p0.i32({{.*}}i32 4, i1 true)
define void @volatile_i64_intrinsics(ptr %dst, ptr %src, i32 %input) {
	%value = trunc i32 %input to i8
	call void @llvm.memcpy.p0.p0.i64(ptr align 1 %dst, ptr align 1 %src, i64 4, i1 true)
	call void @llvm.memmove.p0.p0.i64(ptr align 2 %dst, ptr align 2 %src, i64 4, i1 true)
	call void @llvm.memset.p0.i64(ptr align 1 %dst, i8 %value, i64 4, i1 true)
	ret void
}

; MIR-LABEL: name: metadata_i64
; MIR-COUNT-8: !alias.scope {{![0-9]+}}, !noalias {{![0-9]+}}
; IR-LABEL: define void @metadata_i64(
; IR: call void @llvm.memcpy.p0.p0.i32({{.*}}i32 4, i1 false), !tbaa {{![0-9]+}}, !alias.scope {{![0-9]+}}, !noalias {{![0-9]+}}
define void @metadata_i64(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i64(ptr align 1 %dst, ptr align 1 %src, i64 4, i1 false), !tbaa !0, !alias.scope !3, !noalias !6
	ret void
}

!0 = !{!1, !1, i64 0}
!1 = !{!"bytes", !2, i64 0}
!2 = !{!"SH memory intrinsic TBAA"}
!3 = !{!4}
!4 = distinct !{!4, !5, !"scope"}
!5 = distinct !{!5, i1 false, !"domain"}
!6 = !{!7}
!7 = distinct !{!7, !5, !"other"}
