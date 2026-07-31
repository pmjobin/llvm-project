; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=static -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -code-model=large -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --sections --symbols --relocations %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --sections --symbols --relocations %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: %python %S/Inputs/check-sh-pic-elf.py llvm-readelf %t.be.o %t.le.o

@tls_le = internal thread_local(localexec) global [4 x i32] zeroinitializer, align 16

define ptr @le_address_addend() {
entry:
	ret ptr getelementptr (i8, ptr @tls_le, i32 12)
}

; ASM-LABEL: le_address_addend:
; ASM: mov.l	[[OFFSET:.LCPI[0-9_]+]],[[VALUE:r[0-9]+]]
; ASM-NEXT: stc	gbr,[[ADDRESS:r[0-9]+]]
; ASM-NEXT: add	[[VALUE]],[[ADDRESS]]
; ASM: [[OFFSET]]:
; ASM-NEXT: .long	tls_le@TPOFF+12
; ASM-NOT: _GLOBAL_OFFSET_TABLE_
; ASM-NOT: __tls_get_addr

; OBJECT: Name: .tbss
; OBJECT: SHF_TLS
; OBJECT: Relocations [
; OBJECT: R_SH_TLS_LE_32 tls_le
; OBJECT-NOT: R_SH_GOTPC
; OBJECT-NOT: R_SH_PLT32
; OBJECT: Symbols [
; OBJECT: Name: tls_le
; OBJECT: Type: TLS
