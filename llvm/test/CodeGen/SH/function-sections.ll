; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -function-sections -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -function-sections -O2 -verify-machineinstrs -filetype=obj < %s -o %t.o
; RUN: llvm-readobj -S -r %t.o | FileCheck %s --check-prefix=OBJ

define internal i32 @callee(i32 %value) {
	ret i32 %value
}

define dso_local i32 @caller(i32 %value) {
; ASM-LABEL: caller:
; ASM-NOT: bsr	callee
; ASM: mov.l	.LCPI{{[0-9]+}}_0_0,[[CALLEE:r[0-9]+]]
; ASM: jsr	@[[CALLEE]]
; ASM-NEXT: nop
; ASM: .LCPI{{[0-9]+}}_0_0:
; ASM-NEXT: .long	callee
	%result = call i32 @callee(i32 %value)
	ret i32 %result
}

define dso_local i32 @recursive(i32 %value) {
; ASM-LABEL: recursive:
; ASM: bsr
; ASM-NOT: jsr
	%done = icmp eq i32 %value, 0
	br i1 %done, label %exit, label %recurse
recurse:
	%next = sub i32 %value, 1
	%result = call i32 @recursive(i32 %next)
	ret i32 %result
exit:
	ret i32 0
}

; OBJ: Name: .text.caller
; OBJ: Type: SHT_PROGBITS
; OBJ: SHF_EXECINSTR
; OBJ: Name: .rela.text.caller
; OBJ: 0x{{[0-9A-F]+}} R_SH_DIR32
; OBJ-SAME: callee
; OBJ-NOT: R_SH_NONE
