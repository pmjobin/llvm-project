; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -code-model=large -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -function-sections -O0 -verify-machineinstrs -filetype=obj < %s -o %t.sections.o
; RUN: llvm-readobj --sections --symbols --relocations %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --sections --symbols --relocations %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --relocations %t.sections.o | FileCheck %s --check-prefix=SECTIONS
; RUN: %python %S/Inputs/check-sh-pic-elf.py llvm-readelf %t.be.o %t.le.o

@tls_gd = external thread_local global i32, align 4

define ptr @gd_address() {
entry:
	ret ptr @tls_gd
}

define i32 @gd_live_value(i32 %value) {
entry:
	%loaded = load i32, ptr @tls_gd, align 4
	%result = add i32 %loaded, %value
	ret i32 %result
}

; ASM-LABEL: gd_address:
; ASM: mova	[[GOTPC:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[GOTPC]],r12
; ASM-NEXT: add	r0,r12
; ASM: mov.l	[[DESC:.LCPI[0-9_]+]],r4
; ASM-NEXT: mova	[[RESOLVER:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[RESOLVER]],r1
; ASM-NEXT: add	r0,r1
; ASM-NEXT: jsr	@r1
; ASM-NEXT: add	r12,r4
; ASM: [[DESC]]:
; ASM-NEXT: .long	tls_gd@TLSGD
; ASM: [[RESOLVER]]:
; ASM-NEXT: .long	__tls_get_addr@PLT
; ASM: [[GOTPC]]:
; ASM-NEXT: .long	_GLOBAL_OFFSET_TABLE_

; ASM-LABEL: gd_live_value:
; ASM: jsr	@r1
; ASM-NEXT: add	r12,r4
; ASM: mov.l	@r0,
; ASM: add

; OBJECT: Name: .rela.text
; OBJECT: R_SH_TLS_GD_32 tls_gd
; OBJECT-NEXT: R_SH_PLT32 __tls_get_addr
; OBJECT: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; OBJECT: Name: tls_gd
; OBJECT: Type: TLS

; SECTIONS: Section {{.*}} .rela.text.gd_address {
; SECTIONS: R_SH_TLS_GD_32 tls_gd
; SECTIONS: Section {{.*}} .rela.text.gd_live_value {
; SECTIONS: R_SH_TLS_GD_32 tls_gd
