; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj -r %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj -r %t.le.o | FileCheck %s --check-prefix=OBJECT

declare ptr @external_identity(ptr)

define ptr @return_blockaddress(i32 %condition) {
entry:
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %target, label %other

target:
	ret ptr blockaddress(@return_blockaddress, %target)

other:
	ret ptr null
}

define i32 @pass_and_dispatch(i32 %condition) {
entry:
	%iszero = icmp eq i32 %condition, 0
	%address = call ptr @external_identity(ptr blockaddress(@pass_and_dispatch, %target))
	br i1 %iszero, label %dispatch, label %other

dispatch:
	indirectbr ptr %address, [label %target]

target:
	ret i32 11

other:
	ret i32 22
}

; ASM-LABEL: return_blockaddress:
; ASM: [[TARGET:.Ltmp[0-9]+]]:
; ASM: add	r12,r0
; ASM: .long	[[TARGET]]@GOTOFF

; ASM-LABEL: pass_and_dispatch:
; ASM: add	r12,{{r[0-9]+}}
; ASM: [[DISPATCH_TARGET:.Ltmp[0-9]+]]:
; ASM: .long	[[DISPATCH_TARGET]]@GOTOFF
; ASM: .long	external_identity@PLT

; OBJECT: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; OBJECT: R_SH_GOTOFF .Ltmp
; OBJECT: R_SH_PLT32 external_identity
; OBJECT-NOT: R_SH_DIR32
