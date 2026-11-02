; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s

; CodeGenPrepare queries inline-assembly memory operands with an unknown
; address space. Register-indirect addressing must remain legal, including
; when the address has to be computed from an offset or an arbitrary index.
define i32 @asm_register(ptr %address) {
; CHECK-LABEL: asm_register:
; CHECK: mov.l @r4,r0
	%value = call i32 asm "mov.l $1,$0", "=r,*m"(ptr elementtype(i32) %address)
	ret i32 %value
}

define i32 @asm_offset(ptr %address) {
; CHECK-LABEL: asm_offset:
; CHECK: add #64,r4
; CHECK: mov.l @r4,r0
	%field = getelementptr i8, ptr %address, i32 64
	%value = call i32 asm "mov.l $1,$0", "=r,*m"(ptr elementtype(i32) %field)
	ret i32 %value
}

define i32 @asm_index(ptr %address, i32 %offset) {
; CHECK-LABEL: asm_index:
; CHECK: add r5,r4
; CHECK: mov.l @r4,r0
	%field = getelementptr i8, ptr %address, i32 %offset
	%value = call i32 asm "mov.l $1,$0", "=r,*m"(ptr elementtype(i32) %field)
	ret i32 %value
}
