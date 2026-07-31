; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: %python %S/Inputs/check-sh-pic-elf.py llvm-readelf %t.be.o %t.le.o

%Result = type { i64, i32 }
%Aggregate = type { i32, i64, [8 x i8] }

@tls_gd = external thread_local global i32, align 4
@tls_ld = internal thread_local(localdynamic) global %Aggregate zeroinitializer, align 8
@tls_ie = external thread_local(initialexec) global i32, align 4
@tls_le = internal thread_local(localexec) global i64 17, align 8

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_end.p0(ptr)
declare i32 @external_variadic(i32, ...)
declare i32 @external_call(i32)
declare void @take_pointer(ptr)

define i32 @tls_inside_variadic(i32 %fixed, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%argument = va_arg ptr %ap, i32
	%tls = load i32, ptr @tls_ie, align 4
	%sum = add i32 %fixed, %argument
	%result = add i32 %sum, %tls
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

define i32 @pass_tls_to_variadic() {
	%result = call i32 (i32, ...) @external_variadic(i32 1, ptr @tls_gd, i32 2)
	ret i32 %result
}

define void @pass_tls_as_ordinary_argument() {
	call void @take_pointer(ptr @tls_ld)
	ret void
}

define void @tls_before_and_after_call(i32 %value) {
	store i32 %value, ptr @tls_ld, align 8
	%called = call i32 @external_call(i32 %value)
	store i32 %called, ptr @tls_ld, align 8
	ret void
}

define void @sret_from_tls(ptr sret(%Result) align 4 %out, i32 %tag) {
	%value = load i64, ptr @tls_le, align 8
	%first = insertvalue %Result poison, i64 %value, 0
	%result = insertvalue %Result %first, i32 %tag, 1
	store %Result %result, ptr %out, align 4
	ret void
}

define i64 @aggregate_field() {
	%field = getelementptr inbounds %Aggregate, ptr @tls_ld, i32 0, i32 1
	%value = load i64, ptr %field, align 8
	ret i64 %value
}

; ASM-LABEL: tls_inside_variadic:
; ASM-DAG: mov.l	r5,
; ASM-DAG: mov.l	r6,
; ASM-DAG: mov.l	r7,
; ASM: stc	gbr,
; ASM: .long	tls_ie@GOTTPOFF

; ASM-LABEL: pass_tls_to_variadic:
; ASM: .long	tls_gd@TLSGD
; ASM: .long	external_variadic@PLT

; ASM-LABEL: pass_tls_as_ordinary_argument:
; ASM: .long	tls_ld@TLSLDM
; ASM: .long	tls_ld@DTPOFF
; ASM: .long	take_pointer@PLT

; ASM-LABEL: tls_before_and_after_call:
; ASM: .long	tls_ld@TLSLDM
; ASM: .long	tls_ld@DTPOFF
; ASM: .long	external_call@PLT

; ASM-LABEL: sret_from_tls:
; ASM: stc	gbr,
; ASM: .long	tls_le@TPOFF

; ASM-LABEL: aggregate_field:
; ASM: .long	tls_ld@TLSLDM
; ASM: .long	tls_ld@DTPOFF+8
