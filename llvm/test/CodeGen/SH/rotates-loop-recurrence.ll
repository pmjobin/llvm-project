; The specialized loop-rotation pass is omitted. Keep recurrence and
; poison/overflow witnesses under ordinary rotate lowering.
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -enable-new-pm=0 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=SELECT
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O3 -enable-new-pm=1 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=SELECT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -enable-new-pm=1 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O3 -enable-new-pm=0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM

declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)

; The source also initializes the checksum and remains live after the loop.
; Both stores use the current phase. The output after the loop is not next.
define i32 @left_live(i32 noundef %seed, ptr %p, ptr %q, i32 %n) {
; SELECT-LABEL: name: left_live
; SELECT: PHI
; SELECT-NOT: NATIVE_PSEUDO
; SELECT-NOT: SHORT_PSEUDO
; SELECT: JSR
; ASM-LABEL: left_live:
; ASM: jsr

entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %preheader
preheader:
	br label %loop
loop:
	%i = phi i32 [ 0, %preheader ], [ %next, %loop ]
	%sum = phi i32 [ %seed, %preheader ], [ %added, %loop ]
	%c = and i32 %i, 31
	%r = call noundef i32 @llvm.fshl.i32(i32 %seed, i32 %seed, i32 range(i32 0, 32) %c)
	%address = getelementptr i32, ptr %p, i32 %i
	%x = load i32, ptr %address
	%value = xor i32 %x, %r
	%out = getelementptr i32, ptr %q, i32 %i
	store i32 %value, ptr %out
	%added = add i32 %sum, %value
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %exit, label %loop
exit:
	%result = phi i32 [ %seed, %entry ], [ %added, %loop ]
	%mixed = xor i32 %result, %seed
	ret i32 %mixed
}

define i32 @right_offset(i32 noundef %seed, ptr %q, i32 %n) {
; SELECT-LABEL: name: right_offset
; SELECT-NOT: NATIVE_PSEUDO
; SELECT: JSR
; ASM-LABEL: right_offset:
; ASM: jsr

entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %preheader
preheader:
	br label %loop
loop:
	%i = phi i32 [ 0, %preheader ], [ %next, %loop ]
	%phase = add i32 %i, 31
	%r = call i32 @llvm.fshr.i32(i32 %seed, i32 %seed, i32 %phase)
	%out = getelementptr i32, ptr %q, i32 %i
	store i32 %r, ptr %out
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %exit, label %loop
exit:
	%result = phi i32 [ %seed, %entry ], [ %r, %loop ]
	ret i32 %result
}

; Flagged wrapping transport is not justified by the permutation identity.
define i32 @flagged_count(i32 noundef %seed, ptr %q, i32 %n) {
; SELECT-LABEL: name: flagged_count
; SELECT: {{JSR|NATIVE_PSEUDO}}
; ASM-LABEL: flagged_count:
; ASM: {{jsr|tst}}

entry:
	br label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %loop ]
	%r = call i32 @llvm.fshl.i32(i32 %seed, i32 %seed, i32 %i)
	%out = getelementptr i32, ptr %q, i32 %i
	store i32 %r, ptr %out
	%next = add nuw i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %exit, label %loop
exit:
	ret i32 %seed
}

; An unmasked count with this range is not the masked source contract.
define i32 @unproved_range(i32 noundef %seed, ptr %q, i32 %n) {
; SELECT-LABEL: name: unproved_range
; SELECT: {{JSR|NATIVE_PSEUDO}}
; ASM-LABEL: unproved_range:
; ASM: {{jsr|tst}}

entry:
	br label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %loop ]
	%r = call noundef i32 @llvm.fshl.i32(i32 %seed, i32 %seed, i32 range(i32 0, 32) %i)
	%out = getelementptr i32, ptr %q, i32 %i
	store i32 %r, ptr %out
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %exit, label %loop
exit:
	ret i32 %seed
}
