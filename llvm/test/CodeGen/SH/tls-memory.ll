; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --relocations %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --relocations %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: %python %S/Inputs/check-sh-pic-elf.py llvm-readelf %t.be.o %t.le.o

@tls_gd = external thread_local global i32, align 4
@tls_ld = internal thread_local(localdynamic) global i32 1, align 4
@tls_ie = external thread_local(initialexec) global i32, align 4
@tls_le = internal thread_local(localexec) global { i64, [16 x i8] } zeroinitializer, align 8

declare void @escape(ptr)
declare ptr @identity(ptr)
declare void @llvm.memcpy.p0.p0.i32(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i32, i1 immarg)
declare void @llvm.memmove.p0.p0.i32(ptr nocapture writeonly, ptr nocapture readonly, i32, i1 immarg)
declare void @llvm.memset.p0.i32(ptr nocapture writeonly, i8, i32, i1 immarg)

define i32 @load_gd() {
	%value = load i32, ptr @tls_gd, align 4
	ret i32 %value
}

define void @store_ld(i32 %value) {
	store i32 %value, ptr @tls_ld, align 4
	ret void
}

define i32 @volatile_ie(i32 %value) {
	%old = load volatile i32, ptr @tls_ie, align 4
	store volatile i32 %value, ptr @tls_ie, align 4
	ret i32 %old
}

define i64 @load_le_i64() {
	%value = load i64, ptr @tls_le, align 8
	ret i64 %value
}

define ptr @le_field_address() {
	ret ptr getelementptr inbounds ({ i64, [16 x i8] }, ptr @tls_le, i32 0, i32 1, i32 7)
}

define void @escape_gd() {
	call void @escape(ptr @tls_gd)
	ret void
}

define i32 @compare_gd_null() {
	%equal = icmp eq ptr @tls_gd, null
	br i1 %equal, label %yes, label %no
yes:
	ret i32 1
no:
	ret i32 0
}

define i32 @compare_two_ld_materializations() {
	%first = call ptr @identity(ptr @tls_ld)
	%equal = icmp eq ptr %first, @tls_ld
	br i1 %equal, label %yes, label %no
yes:
	ret i32 1
no:
	ret i32 0
}

define void @memory_intrinsics(ptr %source) {
	%bytes = getelementptr inbounds { i64, [16 x i8] }, ptr @tls_le, i32 0, i32 1
	call void @llvm.memcpy.p0.p0.i32(ptr align 8 @tls_le, ptr align 8 %source, i32 8, i1 false)
	call void @llvm.memmove.p0.p0.i32(ptr align 1 %bytes, ptr align 1 %source, i32 8, i1 false)
	call void @llvm.memset.p0.i32(ptr align 1 %bytes, i8 90, i32 16, i1 false)
	ret void
}

; ASM-LABEL: load_gd:
; ASM: jsr	@r1
; ASM-NEXT: add	r12,r4
; ASM: .long	tls_gd@TLSGD
; ASM: mov.l	@r0,r0

; ASM-LABEL: store_ld:
; ASM: jsr	@r1
; ASM: .long	tls_ld@TLSLDM
; ASM: mov.l	{{r[0-9]+}},@{{r[0-9]+}}
; ASM: .long	tls_ld@DTPOFF

; ASM-LABEL: volatile_ie:
; ASM: stc	gbr,
; ASM: .long	tls_ie@GOTTPOFF
; ASM-NOT: __tls_get_addr

; ASM-LABEL: load_le_i64:
; ASM: stc	gbr,
; ASM: .long	tls_le@TPOFF

; ASM-LABEL: escape_gd:
; ASM: .long	tls_gd@TLSGD
; ASM: .long	escape@PLT

; ASM-LABEL: memory_intrinsics:
; ASM: .long	tls_le@TPOFF

; OBJECT-DAG: R_SH_TLS_GD_32 tls_gd
; OBJECT-DAG: R_SH_TLS_LD_32 tls_ld
; OBJECT-DAG: R_SH_TLS_LDO_32 tls_ld
; OBJECT-DAG: R_SH_TLS_IE_32 tls_ie
; OBJECT-DAG: R_SH_TLS_LE_32 tls_le
