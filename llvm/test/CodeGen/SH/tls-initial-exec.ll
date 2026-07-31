; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -code-model=large -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --sections --symbols --relocations %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --sections --symbols --relocations %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: %python %S/Inputs/check-sh-pic-elf.py llvm-readelf %t.be.o %t.le.o

@tls_ie = external thread_local(initialexec) global i32, align 4

define ptr @ie_address_addend() {
entry:
	ret ptr getelementptr (i8, ptr @tls_ie, i32 -12)
}

; ASM-LABEL: ie_address_addend:
; ASM: mova	[[GOTPC:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[GOTPC]],r12
; ASM-NEXT: add	r0,r12
; ASM: mov.l	[[OFFSET:.LCPI[0-9_]+]],r0
; ASM-NEXT: stc	gbr,[[ADDRESS:r[0-9]+]]
; ASM-NEXT: mov.l	@(r0,r12),r0
; ASM-NEXT: bra
; ASM-NEXT: add	r0,[[ADDRESS]]
; ASM: [[OFFSET]]:
; ASM-NEXT: .long	tls_ie@GOTTPOFF
; ASM: add	#-12,[[ADDRESS]]
; ASM-NOT: __tls_get_addr
; ASM: [[GOTPC]]:
; ASM-NEXT: .long	_GLOBAL_OFFSET_TABLE_

; OBJECT: R_SH_TLS_IE_32 tls_ie
; OBJECT: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; OBJECT-NOT: R_SH_PLT32
; OBJECT: Name: tls_ie
; OBJECT: Type: TLS
