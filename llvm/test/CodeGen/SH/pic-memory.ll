; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj -r %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj -r %t.le.o | FileCheck %s --check-prefix=OBJECT

%Aggregate = type { i32, ptr }

@external_i8 = external global i8
@external_i16 = external global i16
@external_i32 = external global i32
@external_i64 = external global i64
@external_pointer = external global ptr
@external_aggregate = external global %Aggregate

define i32 @load_i8() {
entry:
	%value = load i8, ptr @external_i8, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define void @store_i16(i32 %value) {
entry:
	%narrow = trunc i32 %value to i16
	store i16 %narrow, ptr @external_i16, align 2
	ret void
}

define i32 @load_volatile_i32() {
entry:
	%value = load volatile i32, ptr @external_i32, align 4
	ret i32 %value
}

define i64 @load_i64() {
entry:
	%value = load i64, ptr @external_i64, align 4
	ret i64 %value
}

define ptr @load_pointer() {
entry:
	%value = load ptr, ptr @external_pointer, align 4
	ret ptr %value
}

define void @copy_aggregate(ptr %destination) {
entry:
	%value = load %Aggregate, ptr @external_aggregate, align 4
	store %Aggregate %value, ptr %destination, align 4
	ret void
}

; ASM-LABEL: load_i8:
; ASM: mov.l	@{{r[0-9]+}},{{r[0-9]+}}
; ASM: mov.b	@
; ASM: .long	external_i8@GOT

; ASM-LABEL: store_i16:
; ASM: mov.l	@{{r[0-9]+}},{{r[0-9]+}}
; ASM: mov.w
; ASM: .long	external_i16@GOT

; ASM-LABEL: load_volatile_i32:
; ASM: .long	external_i32@GOT

; ASM-LABEL: load_i64:
; ASM: .long	external_i64@GOT

; ASM-LABEL: load_pointer:
; ASM: .long	external_pointer@GOT

; ASM-LABEL: copy_aggregate:
; ASM: .long	external_aggregate@GOT

; OBJECT-DAG: R_SH_GOT32 external_i8
; OBJECT-DAG: R_SH_GOT32 external_i16
; OBJECT-DAG: R_SH_GOT32 external_i32
; OBJECT-DAG: R_SH_GOT32 external_i64
; OBJECT-DAG: R_SH_GOT32 external_pointer
; OBJECT-DAG: R_SH_GOT32 external_aggregate
; OBJECT-NOT: R_SH_DIR32
