; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=phi-node-elimination < %s | FileCheck %s --check-prefix=PHI
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=virtregrewriter < %s | FileCheck %s --check-prefix=RA

declare i32 @callee(i32)

define i32 @switch_after_unsigned_division(i32 %x, i32 %divisor) {
entry:
	%quotient = udiv i32 %x, %divisor
	switch i32 %x, label %default [
		i32 0, label %c0
		i32 1, label %c1
		i32 2, label %c2
		i32 3, label %c3
		i32 4, label %c4
		i32 5, label %c5
	]
c0:
	%r0 = add i32 %quotient, 11
	ret i32 %r0
c1:
	%r1 = add i32 %quotient, 23
	ret i32 %r1
c2:
	%r2 = add i32 %quotient, 37
	ret i32 %r2
c3:
	%r3 = add i32 %quotient, 53
	ret i32 %r3
c4:
	%r4 = add i32 %quotient, 71
	ret i32 %r4
c5:
	%r5 = add i32 %quotient, 89
	ret i32 %r5
default:
	ret i32 %quotient
}

define i32 @switch_after_i64_carry(i32 %x, i64 %a, i64 %b) {
entry:
	%wide = add i64 %a, %b
	%low = trunc i64 %wide to i32
	%high.shifted = lshr i64 %wide, 32
	%high = trunc i64 %high.shifted to i32
	%folded = xor i32 %low, %high
	switch i32 %x, label %default [
		i32 10, label %c0
		i32 11, label %c1
		i32 12, label %c2
		i32 13, label %c3
		i32 14, label %c4
		i32 15, label %c5
	]
c0:
	%r0 = add i32 %folded, 13
	ret i32 %r0
c1:
	%r1 = add i32 %folded, 29
	ret i32 %r1
c2:
	%r2 = add i32 %folded, 43
	ret i32 %r2
c3:
	%r3 = add i32 %folded, 61
	ret i32 %r3
c4:
	%r4 = add i32 %folded, 79
	ret i32 %r4
c5:
	%r5 = add i32 %folded, 101
	ret i32 %r5
default:
	ret i32 %folded
}

define i32 @switch_before_and_after_calls(i32 %x) {
entry:
	%before = call i32 @callee(i32 %x)
	switch i32 %x, label %default [
		i32 0, label %c0
		i32 1, label %c1
		i32 2, label %c2
		i32 3, label %c3
		i32 4, label %c4
		i32 5, label %c5
	]
c0:
	br label %merge
c1:
	br label %merge
c2:
	br label %merge
c3:
	br label %merge
c4:
	br label %merge
c5:
	br label %merge
default:
	br label %merge
merge:
	%case.value = phi i32 [ 17, %c0 ], [ 31, %c1 ], [ 47, %c2 ], [ 67, %c3 ], [ 89, %c4 ], [ 113, %c5 ], [ -1, %default ]
	%after = call i32 @callee(i32 %case.value)
	%result = add i32 %before, %after
	ret i32 %result
}

define i32 @loop_contained_switch(i32 %x, i32 %count) {
entry:
	br label %loop
loop:
	%index = phi i32 [ 0, %entry ], [ %next.index, %latch ]
	%accumulator = phi i32 [ 0, %entry ], [ %next.accumulator, %latch ]
	switch i32 %x, label %default [
		i32 0, label %c0
		i32 1, label %c1
		i32 2, label %c2
		i32 3, label %c3
		i32 4, label %c4
		i32 5, label %c5
	]
c0:
	%a0 = add i32 %accumulator, 11
	br label %latch
c1:
	%a1 = add i32 %accumulator, 23
	br label %latch
c2:
	%a2 = add i32 %accumulator, 37
	br label %latch
c3:
	%a3 = add i32 %accumulator, 53
	br label %latch
c4:
	%a4 = add i32 %accumulator, 71
	br label %latch
c5:
	%a5 = add i32 %accumulator, 89
	br label %latch
default:
	%ad = add i32 %accumulator, 1
	br label %latch
latch:
	%next.accumulator = phi i32 [ %a0, %c0 ], [ %a1, %c1 ], [ %a2, %c2 ], [ %a3, %c3 ], [ %a4, %c4 ], [ %a5, %c5 ], [ %ad, %default ]
	%next.index = add nuw i32 %index, 1
	%more = icmp ult i32 %next.index, %count
	br i1 %more, label %loop, label %exit
exit:
	ret i32 %next.accumulator
}

define i32 @switch_under_register_pressure(i32 %x, i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f, i32 %g) {
entry:
	%ab = add i32 %a, %b
	%cd = add i32 %c, %d
	%ef = add i32 %e, %f
	%abc = add i32 %ab, %c
	%def = add i32 %cd, %ef
	%saved = add i32 %abc, %def
	switch i32 %x, label %default [
		i32 0, label %c0
		i32 1, label %c1
		i32 2, label %c2
		i32 3, label %c3
		i32 4, label %c4
		i32 5, label %c5
	]
c0:
	%r0 = add i32 %saved, %a
	ret i32 %r0
c1:
	%r1 = add i32 %saved, %b
	ret i32 %r1
c2:
	%r2 = add i32 %saved, %c
	ret i32 %r2
c3:
	%r3 = add i32 %saved, %d
	ret i32 %r3
c4:
	%r4 = add i32 %saved, %e
	ret i32 %r4
c5:
	%r5 = add i32 %saved, %f
	ret i32 %r5
default:
	%rd = add i32 %saved, %g
	ret i32 %rd
}

; ASM-LABEL: switch_after_unsigned_division:
; ASM: div1
; ASM: jmp	@
; ASM-NEXT: nop
; ASM-LABEL: switch_after_i64_carry:
; ASM: addc
; ASM: jmp	@
; ASM-NEXT: nop
; ASM-LABEL: switch_before_and_after_calls:
; ASM: jsr	@
; ASM: jmp	@
; ASM-NEXT: nop
; ASM: jsr	@
; ASM-LABEL: loop_contained_switch:
; ASM: {{bt|bf}}
; ASM: jmp	@
; ASM-NEXT: nop
; ASM-LABEL: switch_under_register_pressure:
; ASM: jmp	@
; ASM-NEXT: nop

; PHI-LABEL: name:            switch_before_and_after_calls
; PHI: noPhis:          true
; PHI-NOT: PHI
; PHI: JMP {{.*}}, %jump-table.0
; PHI-LABEL: name:            loop_contained_switch
; PHI: noPhis:          true
; PHI-NOT: PHI
; PHI: JMP {{.*}}, %jump-table.0

; RA-LABEL: name:            switch_under_register_pressure
; RA: noVRegs:         true
; RA: registers:       []
; RA-NOT: %{{[0-9]+}}
; RA: MOVL_load_reg
; RA-NEXT: JMP {{.*}}, %jump-table.0
