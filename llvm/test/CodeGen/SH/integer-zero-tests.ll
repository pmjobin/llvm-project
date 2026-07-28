; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

define i32 @equal_zero(i32 %value) {
; ISEL-LABEL: name:            equal_zero
; ISEL: TST {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BT %bb.{{[0-9]+}}, implicit $tbit
; ASM-LABEL: equal_zero:
; ASM: tst	[[REG:r[0-9]+]],[[REG]]
; ASM-NOT: cmp/eq
; ASM: {{bt|bf}}
	%condition = icmp eq i32 %value, 0
	br i1 %condition, label %zero, label %nonzero
nonzero:
	ret i32 %value
zero:
	ret i32 11
}

define i32 @not_equal_zero(i32 %value) {
; ISEL-LABEL: name:            not_equal_zero
; ISEL: TST {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BF %bb.{{[0-9]+}}, implicit $tbit
; ASM-LABEL: not_equal_zero:
; ASM: tst	[[REG:r[0-9]+]],[[REG]]
; ASM-NOT: cmp/eq
; ASM: {{bt|bf}}
	%condition = icmp ne i32 %value, 0
	br i1 %condition, label %nonzero, label %zero
zero:
	ret i32 11
nonzero:
	ret i32 %value
}
