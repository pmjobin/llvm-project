; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=SIZE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL-O2

define i32 @variable_shl(i32 %value, i32 %count) {
; ISEL-LABEL: name:            variable_shl
; ISEL-NOT: MOVri 31
; ISEL: [[COUNT:%[0-9]+]]:gpr = COPY $r5
; ISEL: TST [[COUNT]], [[COUNT]], implicit-def $tbit
; ISEL-NEXT: BT %bb.{{[0-9]+}}, implicit $tbit
; ISEL: [[VALUE_PHI:%[0-9]+]]:gpr = PHI
; ISEL: [[COUNT_PHI:%[0-9]+]]:gpr = PHI
; ISEL: [[NEXT_VALUE:%[0-9]+]]:gpr = SHLL [[VALUE_PHI]], implicit-def $tbit
; ISEL-NEXT: [[NEXT_COUNT:%[0-9]+]]:gpr = DT [[COUNT_PHI]], implicit-def $tbit
; ISEL-NEXT: BF %bb.{{[0-9]+}}, implicit $tbit
; ISEL: {{%[0-9]+}}:gpr = PHI
; ISEL-NOT: SHLrr
; ASM-LABEL: variable_shl:
; ASM: mov.l	[[HELPER:.LCPI[0-9_]+]],{{r[0-9]+}}
; ASM: jsr	@{{r[0-9]+}}
; ASM-NOT: shll
; ASM: [[HELPER]]:
; ASM-NEXT: .long	__ashlsi3
	%result = shl i32 %value, %count
	ret i32 %result
}

define i32 @variable_lshr(i32 %value, i32 %count) {
; ISEL-LABEL: name:            variable_lshr
; ISEL-NOT: MOVri 31
; ISEL: TST {{.*}}, implicit-def $tbit
; ISEL: BT
; ISEL: [[VALUE_PHI:%[0-9]+]]:gpr = PHI
; ISEL: [[NEXT_VALUE:%[0-9]+]]:gpr = SHLR [[VALUE_PHI]], implicit-def $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = DT {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BF {{.*}}, implicit $tbit
; ISEL-NOT: SRLrr
; ASM-LABEL: variable_lshr:
; ASM: mov.l	[[HELPER:.LCPI[0-9_]+]],{{r[0-9]+}}
; ASM: jsr	@{{r[0-9]+}}
; ASM-NOT: shlr
; ASM: [[HELPER]]:
; ASM-NEXT: .long	__lshrsi3
	%result = lshr i32 %value, %count
	ret i32 %result
}

define i32 @variable_ashr(i32 %value, i32 %count) {
; ISEL-LABEL: name:            variable_ashr
; ISEL-NOT: MOVri 31
; ISEL: TST {{.*}}, implicit-def $tbit
; ISEL: BT
; ISEL: [[VALUE_PHI:%[0-9]+]]:gpr = PHI
; ISEL: [[NEXT_VALUE:%[0-9]+]]:gpr = SHAR [[VALUE_PHI]], implicit-def $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = DT {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BF {{.*}}, implicit $tbit
; ISEL-NOT: SRArr
; ASM-LABEL: variable_ashr:
; ASM: mov.l	[[HELPER:.LCPI[0-9_]+]],{{r[0-9]+}}
; ASM: jsr	@{{r[0-9]+}}
; ASM-NOT: shar
; ASM: [[HELPER]]:
; ASM-NEXT: .long	__ashrsi3
	%result = ashr exact i32 %value, %count
	ret i32 %result
}

define i32 @variable_same_value_and_count(i32 %value) {
; ISEL-LABEL: name:            variable_same_value_and_count
; ISEL: [[SOURCE:%[0-9]+]]:gpr = COPY $r4
; ISEL: TST [[SOURCE]], [[SOURCE]], implicit-def $tbit
; ISEL: {{%[0-9]+}}:gpr = PHI [[SOURCE]], %bb.
; ISEL: {{%[0-9]+}}:gpr = PHI {{%[0-9]+}}, %bb.
; ASM-LABEL: variable_same_value_and_count:
; ASM: jsr
; ASM-NEXT: mov	r4,r5
; ASM: .long	__ashlsi3
	%result = shl i32 %value, %value
	ret i32 %result
}

define i32 @variable_sources_live(i32 %value, i32 %count) {
; ISEL-LABEL: name:            variable_sources_live
; ISEL: SHLR
; ISEL: ADDrr
; ISEL: ADDrr
; ASM-LABEL: variable_sources_live:
; ASM: jsr
; ASM: add	r4,r0
; ASM-NEXT: add	r5,r0
; ASM: .long	__lshrsi3
; ISEL-O2-LABEL: name:            variable_sources_live
; ISEL-O2: JSR {{.*}}, csr_sh_shift, {{.*}}implicit $r4, implicit $r5, {{.*}}implicit-def $r0
	%shifted = lshr i32 %value, %count
	%with_value = add i32 %shifted, %value
	%result = add i32 %with_value, %count
	ret i32 %result
}

define i32 @multiple_variable_shifts(i32 %value, i32 %left_count, i32 %right_count) {
; ISEL-LABEL: name:            multiple_variable_shifts
; ISEL: SHLL
; ISEL: SHAR
; ISEL-NOT: SHLrr
; ISEL-NOT: SRArr
; ASM-LABEL: multiple_variable_shifts:
; ASM-COUNT-2: jsr
; ASM: .long	__ashlsi3
; ASM: .long	__ashrsi3
	%left = shl i32 %value, %left_count
	%result = ashr i32 %left, %right_count
	ret i32 %result
}

define i32 @variable_lshr_minsize(i32 %value, i32 %count) minsize {
; SIZE-LABEL: variable_lshr_minsize:
; SIZE-NOT: __lshrsi3
; SIZE: tst
; SIZE: shlr
; SIZE-NEXT: dt
; SIZE: bf
	%result = lshr i32 %value, %count
	ret i32 %result
}

define i32 @variable_lshr_optsize(i32 %value, i32 %count) optsize {
; SIZE-LABEL: variable_lshr_optsize:
; SIZE: jsr
; SIZE-NOT: shlr
; SIZE: .long	__lshrsi3
	%result = lshr i32 %value, %count
	ret i32 %result
}
