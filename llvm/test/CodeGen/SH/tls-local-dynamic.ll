; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -code-model=large -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --sections --symbols --relocations %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --sections --symbols --relocations %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: %python %S/Inputs/check-sh-pic-elf.py llvm-readelf %t.be.o %t.le.o

@tls_ld_a = internal thread_local(localdynamic) global i32 1, align 4
@tls_ld_b = hidden thread_local(localdynamic) global [2 x i32] zeroinitializer, align 8

define ptr @ld_address_a() {
entry:
	ret ptr @tls_ld_a
}

define ptr @ld_address_b_addend() {
entry:
	ret ptr getelementptr (i8, ptr @tls_ld_b, i32 4)
}

; ASM-LABEL: ld_address_a:
; ASM: mova	[[GOTPC:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[GOTPC]],r12
; ASM-NEXT: add	r0,r12
; ASM: mov.l	[[OFFSET:.LCPI[0-9_]+]],
; ASM: mov.l	[[DESC:.LCPI[0-9_]+]],r4
; ASM-NEXT: mova	[[RESOLVER:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[RESOLVER]],r1
; ASM-NEXT: add	r0,r1
; ASM-NEXT: jsr	@r1
; ASM-NEXT: add	r12,r4
; ASM: [[DESC]]:
; ASM-NEXT: .long	tls_ld_a@TLSLDM
; ASM: [[RESOLVER]]:
; ASM-NEXT: .long	__tls_get_addr@PLT
; ASM: add	{{r[0-9]+}},r0
; ASM: [[GOTPC]]:
; ASM-NEXT: .long	_GLOBAL_OFFSET_TABLE_
; ASM: [[OFFSET]]:
; ASM-NEXT: .long	tls_ld_a@DTPOFF

; ASM-LABEL: ld_address_b_addend:
; ASM: .long	tls_ld_b@TLSLDM
; ASM: .long	__tls_get_addr@PLT
; ASM: .long	tls_ld_b@DTPOFF+4

; OBJECT: R_SH_TLS_LD_32 tls_ld_a
; OBJECT-NEXT: R_SH_PLT32 __tls_get_addr
; OBJECT: R_SH_TLS_LDO_32 tls_ld_a
; OBJECT: R_SH_TLS_LD_32 tls_ld_b
; OBJECT-NEXT: R_SH_PLT32 __tls_get_addr
; OBJECT: R_SH_TLS_LDO_32 tls_ld_b
; OBJECT: Name: tls_ld_a
; OBJECT: Type: TLS
; OBJECT: Name: tls_ld_b
; OBJECT: Type: TLS
