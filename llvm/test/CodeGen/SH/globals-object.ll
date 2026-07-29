; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj -S -s -r --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj -S -s -r --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-objdump -dr %t.be.o | FileCheck %s --check-prefix=DISASM
; RUN: llvm-objdump -dr %t.le.o | FileCheck %s --check-prefix=DISASM

@global = global i32 42, align 4
declare i32 @external_fn(i32)

define i32 @load_global() {
	%value = load i32, ptr @global, align 4
	ret i32 %value
}

define i32 @call_external(i32 %value) {
	%result = call i32 @external_fn(i32 %value)
	ret i32 %result
}

; COMMON: Name: .text
; COMMON: Type: SHT_PROGBITS
; COMMON: SHF_EXECINSTR
; COMMON: AddressAlignment: 4
; COMMON: Section {{.*}} .rel.text {
; COMMON: R_SH_DIR32 global
; COMMON: R_SH_DIR32 external_fn
; COMMON: }
; COMMON-NOT: R_SH_NONE

; COMMON: Name: load_global
; COMMON: Size: 12
; COMMON: Type: Function
; COMMON: Name: call_external
; COMMON: Type: Function
; COMMON: Name: external_fn
; COMMON: Section: Undefined

; BE: Hex dump of section '.text':
; BE-NEXT: 0x00000000 d0016002 000b0009 00000000 {{[0-9a-f]+}}
; LE: Hex dump of section '.text':
; LE-NEXT: 0x00000000 01d00260 0b000900 00000000 {{[0-9a-f]+}}

; DISASM-LABEL: <load_global>:
; DISASM: mov.l
; DISASM-NEXT: mov.l	@r0,r0
; DISASM: R_SH_DIR32	global
; DISASM-LABEL: <call_external>:
; DISASM: mov.l
; DISASM: jsr
; DISASM-NEXT: {{.*}}nop
; DISASM: R_SH_DIR32	external_fn
