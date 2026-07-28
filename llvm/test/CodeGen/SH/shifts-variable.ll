; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

define i32 @variable_shl(i32 %value, i32 %count) {
; ISEL-LABEL: name:            variable_shl
; ISEL-NOT: SHLrr
; ISEL: [[MASK:%[0-9]+]]:gpr = MOVri 31
; ISEL-NEXT: [[COUNT:%[0-9]+]]:gpr = ANDrr [[MASK]], {{%[0-9]+}}
; ISEL-NEXT: TST [[COUNT]], [[COUNT]], implicit-def $tbit
; ISEL-NEXT: BT %bb.{{[0-9]+}}, implicit $tbit
; ISEL: [[VALUE_PHI:%[0-9]+]]:gpr = PHI
; ISEL: [[COUNT_PHI:%[0-9]+]]:gpr = PHI
; ISEL: [[NEXT_VALUE:%[0-9]+]]:gpr = SHLL [[VALUE_PHI]], implicit-def $tbit
; ISEL-NEXT: [[NEXT_COUNT:%[0-9]+]]:gpr = DT [[COUNT_PHI]], implicit-def $tbit
; ISEL-NEXT: BF %bb.{{[0-9]+}}, implicit $tbit
; ISEL: {{%[0-9]+}}:gpr = PHI
; ISEL-NOT: SHLrr
; ASM-LABEL: variable_shl:
; ASM: mov	#31,[[COUNTREG:r[0-9]+]]
; ASM-NEXT: and	r5,[[COUNTREG]]
; ASM-NEXT: tst	[[COUNTREG]],[[COUNTREG]]
; ASM-NEXT: bt	[[DONE:.LBB[0-9_]+]]
; ASM-NEXT: [[LOOP:.LBB[0-9_]+]]:
; ASM: shll	{{r[0-9]+}}
; ASM-NEXT: dt	{{r[0-9]+}}
; ASM-NEXT: bf	[[LOOP]]
; ASM-NEXT: [[DONE]]:
	%result = shl i32 %value, %count
	ret i32 %result
}

define i32 @variable_lshr(i32 %value, i32 %count) {
; ISEL-LABEL: name:            variable_lshr
; ISEL-NOT: SRLrr
; ISEL: MOVri 31
; ISEL: ANDrr
; ISEL: TST {{.*}}, implicit-def $tbit
; ISEL: BT
; ISEL: [[VALUE_PHI:%[0-9]+]]:gpr = PHI
; ISEL: [[NEXT_VALUE:%[0-9]+]]:gpr = SHLR [[VALUE_PHI]], implicit-def $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = DT {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BF {{.*}}, implicit $tbit
; ISEL-NOT: SRLrr
; ASM-LABEL: variable_lshr:
; ASM: mov	#31,[[COUNTREG:r[0-9]+]]
; ASM-NEXT: and	r5,[[COUNTREG]]
; ASM-NEXT: tst	[[COUNTREG]],[[COUNTREG]]
; ASM-NEXT: bt	[[DONE:.LBB[0-9_]+]]
; ASM-NEXT: [[LOOP:.LBB[0-9_]+]]:
; ASM: shlr	{{r[0-9]+}}
; ASM-NEXT: dt	{{r[0-9]+}}
; ASM-NEXT: bf	[[LOOP]]
; ASM-NEXT: [[DONE]]:
	%result = lshr i32 %value, %count
	ret i32 %result
}

define i32 @variable_ashr(i32 %value, i32 %count) {
; ISEL-LABEL: name:            variable_ashr
; ISEL-NOT: SRArr
; ISEL: MOVri 31
; ISEL: ANDrr
; ISEL: TST {{.*}}, implicit-def $tbit
; ISEL: BT
; ISEL: [[VALUE_PHI:%[0-9]+]]:gpr = PHI
; ISEL: [[NEXT_VALUE:%[0-9]+]]:gpr = SHAR [[VALUE_PHI]], implicit-def $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = DT {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BF {{.*}}, implicit $tbit
; ISEL-NOT: SRArr
; ASM-LABEL: variable_ashr:
; ASM: mov	#31,[[COUNTREG:r[0-9]+]]
; ASM-NEXT: and	r5,[[COUNTREG]]
; ASM-NEXT: tst	[[COUNTREG]],[[COUNTREG]]
; ASM-NEXT: bt	[[DONE:.LBB[0-9_]+]]
; ASM-NEXT: [[LOOP:.LBB[0-9_]+]]:
; ASM: shar	{{r[0-9]+}}
; ASM-NEXT: dt	{{r[0-9]+}}
; ASM-NEXT: bf	[[LOOP]]
; ASM-NEXT: [[DONE]]:
	%result = ashr exact i32 %value, %count
	ret i32 %result
}

define i32 @variable_same_value_and_count(i32 %value) {
; ISEL-LABEL: name:            variable_same_value_and_count
; ISEL: ANDrr {{.*}}, [[SOURCE:%[0-9]+]]
; ISEL: {{%[0-9]+}}:gpr = PHI [[SOURCE]], %bb.
; ISEL: {{%[0-9]+}}:gpr = PHI {{%[0-9]+}}, %bb.
; ASM-LABEL: variable_same_value_and_count:
; ASM: and
; ASM: shll
; ASM: dt
	%result = shl i32 %value, %value
	ret i32 %result
}

define i32 @variable_sources_live(i32 %value, i32 %count) {
; ISEL-LABEL: name:            variable_sources_live
; ISEL: SHLR
; ISEL: ADDrr
; ISEL: ADDrr
; ASM-LABEL: variable_sources_live:
; ASM: shlr
; ASM: add
; ASM: add
	%shifted = lshr i32 %value, %count
	%with_value = add i32 %shifted, %value
	%result = add i32 %with_value, %count
	ret i32 %result
}

define i32 @multiple_variable_shifts(i32 %value, i32 %left_count, i32 %right_count) {
; ISEL-LABEL: name:            multiple_variable_shifts
; ISEL: MOVri 31
; ISEL: SHLL
; ISEL: MOVri 31
; ISEL: SHAR
; ISEL-NOT: SHLrr
; ISEL-NOT: SRArr
; ASM-LABEL: multiple_variable_shifts:
; ASM: mov	#31
; ASM: and
; ASM: shll
; ASM: and
; ASM: shar
	%left = shl i32 %value, %left_count
	%result = ashr i32 %left, %right_count
	ret i32 %result
}
