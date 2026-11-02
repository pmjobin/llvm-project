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

define i32 @masked_equal_zero(i32 %value) {
; ISEL-LABEL: name:            masked_equal_zero
; ISEL-NOT: ANDrr
; ISEL: TSTri_PSEUDO {{%[0-9]+}}, 2, implicit-def $tbit
; ISEL-NEXT: BT %bb.{{[0-9]+}}, implicit $tbit
; ASM-LABEL: masked_equal_zero:
; ASM-NOT: and
; ASM: mov	r4,r0
; ASM-NEXT: tst	#2,r0
; ASM: {{bt|bf}}
	%masked = and i32 %value, 2
	%condition = icmp eq i32 %masked, 0
	br i1 %condition, label %zero, label %nonzero
nonzero:
	ret i32 %value
zero:
	ret i32 11
}

define i32 @masked_not_equal_zero(i32 %value) {
; ISEL-LABEL: name:            masked_not_equal_zero
; ISEL-NOT: ANDrr
; ISEL: TSTri_PSEUDO {{%[0-9]+}}, 128, implicit-def $tbit
; ISEL-NEXT: BF %bb.{{[0-9]+}}, implicit $tbit
; ASM-LABEL: masked_not_equal_zero:
; ASM-NOT: and
; ASM: mov	r4,r0
; ASM-NEXT: tst	#128,r0
; ASM: {{bt|bf}}
	%masked = and i32 %value, 128
	%condition = icmp ne i32 %masked, 0
	br i1 %condition, label %nonzero, label %zero
zero:
	ret i32 11
nonzero:
	ret i32 %value
}

define i32 @register_masked_equal_zero(i32 %lhs, i32 %rhs) {
; ISEL-LABEL: name:            register_masked_equal_zero
; ISEL-NOT: ANDrr
; ISEL: TST {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BT %bb.{{[0-9]+}}, implicit $tbit
; ASM-LABEL: register_masked_equal_zero:
; ASM-NOT: and
; ASM: tst	{{r[0-9]+}},{{r[0-9]+}}
; ASM: {{bt|bf}}
	%masked = and i32 %lhs, %rhs
	%condition = icmp eq i32 %masked, 0
	br i1 %condition, label %zero, label %nonzero
nonzero:
	ret i32 %lhs
zero:
	ret i32 11
}

define i32 @register_masked_not_equal_zero(i32 %lhs, i32 %rhs) {
; ISEL-LABEL: name:            register_masked_not_equal_zero
; ISEL-NOT: ANDrr
; ISEL: TST {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BF %bb.{{[0-9]+}}, implicit $tbit
; ASM-LABEL: register_masked_not_equal_zero:
; ASM-NOT: and
; ASM: tst	{{r[0-9]+}},{{r[0-9]+}}
; ASM: {{bt|bf}}
	%masked = and i32 %lhs, %rhs
	%condition = icmp ne i32 %masked, 0
	br i1 %condition, label %nonzero, label %zero
zero:
	ret i32 11
nonzero:
	ret i32 %lhs
}

define i32 @register_masked_zero_with_other_use(i32 %lhs, i32 %rhs) {
; ISEL-LABEL: name:            register_masked_zero_with_other_use
; ISEL: ANDrr
; ISEL: TST {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit
; ASM-LABEL: register_masked_zero_with_other_use:
; ASM: and
	%masked = and i32 %lhs, %rhs
	%condition = icmp eq i32 %masked, 0
	br i1 %condition, label %zero, label %nonzero
nonzero:
	ret i32 %lhs
zero:
	ret i32 %masked
}

define i32 @large_constant_masked_equal_zero(i32 %value) {
; ISEL-LABEL: name:            large_constant_masked_equal_zero
; ISEL: ANDrr
; ISEL: TST {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BT %bb.{{[0-9]+}}, implicit $tbit
; ASM-LABEL: large_constant_masked_equal_zero:
; ASM-NOT: and
; ASM: tst	{{r[0-9]+}},{{r[0-9]+}}
; ASM: {{bt|bf}}
	%masked = and i32 %value, 305419896
	%condition = icmp eq i32 %masked, 0
	br i1 %condition, label %zero, label %nonzero
nonzero:
	ret i32 %value
zero:
	ret i32 11
}

define i32 @masked_zero_with_other_use(i32 %value) {
; ISEL-LABEL: name:            masked_zero_with_other_use
; ISEL-NOT: TSTri_PSEUDO
; ISEL: ANDrr
; ISEL: TST {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit
; ASM-LABEL: masked_zero_with_other_use:
; ASM: and
	%masked = and i32 %value, 2
	%condition = icmp eq i32 %masked, 0
	br i1 %condition, label %zero, label %nonzero
nonzero:
	ret i32 %value
zero:
	ret i32 %masked
}
