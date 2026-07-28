; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

define i32 @variable_shift_diamond(i32 %value, i32 %count, i32 %flag) {
; ISEL-LABEL: name:            variable_shift_diamond
; ISEL: TST
; ISEL: BT
; ISEL: [[LOOP_VALUE:%[0-9]+]]:gpr = PHI
; ISEL: [[NEXT_VALUE:%[0-9]+]]:gpr = SHLL [[LOOP_VALUE]], implicit-def $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = DT {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BF
; ISEL: [[SHIFTED:%[0-9]+]]:gpr = PHI {{%[0-9]+}}, %bb.{{[0-9]+}}, [[NEXT_VALUE]], %bb.{{[0-9]+}}
; ISEL: BRA
; ISEL: {{%[0-9]+}}:gpr = PHI {{.*}}[[SHIFTED]], %bb.{{[0-9]+}}
; ISEL-NOT: SHLrr
; ASM-LABEL: variable_shift_diamond:
; ASM: shll
; ASM-NEXT: dt
; ASM-NEXT: bf
; ASM: rts
	%condition = icmp eq i32 %flag, 0
	br i1 %condition, label %shift, label %other

other:
	%other_value = add i32 %value, 1
	br label %merge

shift:
	%shifted = shl i32 %value, %count
	br label %merge

merge:
	%result = phi i32 [ %shifted, %shift ], [ %other_value, %other ]
	ret i32 %result
}

define i32 @variable_shift_source_loop(i32 %value, i32 %count, i32 %iterations) {
; ISEL-LABEL: name:            variable_shift_source_loop
; ISEL: {{%[0-9]+}}:gpr = PHI
; ISEL: CMP_GE
; ISEL: BF
; ISEL: TST
; ISEL: BT
; ISEL: {{%[0-9]+}}:gpr = PHI
; ISEL: SHAR {{.*}}, implicit-def $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = DT {{.*}}, implicit-def $tbit
; ISEL-NEXT: BF
; ISEL-NOT: SRArr
; ASM-LABEL: variable_shift_source_loop:
; ASM: cmp/ge
; ASM: bf
; ASM: shar
; ASM-NEXT: dt
; ASM-NEXT: bf
	entry:
	br label %loop

loop:
	%accumulator = phi i32 [ %value, %entry ], [ %shifted, %body ]
	%iteration = phi i32 [ %iterations, %entry ], [ %next_iteration, %body ]
	%continue = icmp sgt i32 %iteration, 0
	br i1 %continue, label %body, label %exit

body:
	%shifted = ashr i32 %accumulator, %count
	%next_iteration = add i32 %iteration, -1
	br label %loop

exit:
	ret i32 %accumulator
}

define internal i32 @shift_identity(i32 %value) noinline {
	ret i32 %value
}

define i32 @variable_shifts_around_call(i32 %value, i32 %count) {
; ISEL-LABEL: name:            variable_shifts_around_call
; ISEL: SHLL
; ISEL: DT
; ISEL: BF
; ISEL: BSR @shift_identity
; ISEL: SHLR
; ISEL: DT
; ISEL: BF
; ISEL: ADDrr
; ISEL: ADDrr
; ASM-LABEL: variable_shifts_around_call:
; ASM: shll
; ASM: bsr	shift_identity
; ASM: shlr
; ASM: add
; ASM: add
	%before = shl i32 %value, %count
	%called = call i32 @shift_identity(i32 %before)
	%after = lshr i32 %called, %count
	%with_value = add i32 %after, %value
	%result = add i32 %with_value, %count
	ret i32 %result
}

define i32 @variable_shift_feeds_existing_phi(i32 %value, i32 %count, i32 %flag) {
; ISEL-LABEL: name:            variable_shift_feeds_existing_phi
; ISEL: SHLR
; ISEL: DT
; ISEL: BF
; ISEL: {{%[0-9]+}}:gpr = PHI {{%[0-9]+}}, %bb.{{[0-9]+}},
; ASM-LABEL: variable_shift_feeds_existing_phi:
; ASM: shlr
; ASM: rts
	%condition = icmp ne i32 %flag, 0
	br i1 %condition, label %left, label %right

left:
	%shifted = lshr i32 %value, %count
	br label %merge

right:
	%complement = xor i32 %value, -1
	br label %merge

merge:
	%result = phi i32 [ %shifted, %left ], [ %complement, %right ]
	ret i32 %result
}

define i32 @variable_shift_weighted_successors(i32 %value, i32 %count, i32 %flag) {
; ISEL-LABEL: name:            variable_shift_weighted_successors
; ISEL: TST
; ISEL: BT
; ISEL: SHLL
; ISEL: DT
; ISEL: BF
; ISEL: successors: %bb.{{[0-9]+}}(0x73333333), %bb.{{[0-9]+}}(0x0ccccccd)
; ASM-LABEL: variable_shift_weighted_successors:
; ASM: shll
; ASM: tst
	%shifted = shl i32 %value, %count
	%condition = icmp eq i32 %flag, 0
	br i1 %condition, label %zero, label %nonzero, !prof !0

zero:
	ret i32 %shifted

nonzero:
	ret i32 %value
}

!0 = !{!"branch_weights", i32 9, i32 1}
