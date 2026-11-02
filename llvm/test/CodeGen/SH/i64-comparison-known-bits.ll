; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o /dev/null

; Widening an i64 comparison to i32 must retain its zero-or-one range.
; In particular, adding the predicate must not mask the materialized value.

define i32 @accumulate_ult(i64 %a, i64 %b, i32 %count) {
; CHECK-LABEL: name: accumulate_ult
; CHECK-NOT: ANDrr
; CHECK: ADDrr
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp ult i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @accumulate_ule(i64 %a, i64 %b, i32 %count) {
; CHECK-LABEL: name: accumulate_ule
; CHECK-NOT: ANDrr
; CHECK: ADDrr
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp ule i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @accumulate_ugt(i64 %a, i64 %b, i32 %count) {
; CHECK-LABEL: name: accumulate_ugt
; CHECK-NOT: ANDrr
; CHECK: ADDrr
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp ugt i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @accumulate_uge(i64 %a, i64 %b, i32 %count) {
; CHECK-LABEL: name: accumulate_uge
; CHECK-NOT: ANDrr
; CHECK: ADDrr
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp uge i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @accumulate_slt(i64 %a, i64 %b, i32 %count) {
; CHECK-LABEL: name: accumulate_slt
; CHECK-NOT: ANDrr
; CHECK: ADDrr
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp slt i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @accumulate_sle(i64 %a, i64 %b, i32 %count) {
; CHECK-LABEL: name: accumulate_sle
; CHECK-NOT: ANDrr
; CHECK: ADDrr
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp sle i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @accumulate_sgt(i64 %a, i64 %b, i32 %count) {
; CHECK-LABEL: name: accumulate_sgt
; CHECK-NOT: ANDrr
; CHECK: ADDrr
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp sgt i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @accumulate_sge(i64 %a, i64 %b, i32 %count) {
; CHECK-LABEL: name: accumulate_sge
; CHECK-NOT: ANDrr
; CHECK: ADDrr
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp sge i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @accumulate_eq(i64 %a, i64 %b, i32 %count) {
; CHECK-LABEL: name: accumulate_eq
; CHECK-NOT: ANDrr
; CHECK: TST
; CHECK-NEXT: {{%[0-9]+}}:gpr = ADDC
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp eq i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @accumulate_ne(i64 %a, i64 %b, i32 %count) {
; CHECK-LABEL: name: accumulate_ne
; CHECK-NOT: ANDrr
; CHECK: TST
; CHECK-NEXT: {{%[0-9]+}}:gpr = SUBC
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp ne i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @size_accumulate(i64 %a, i64 %b, i32 %count) optsize {
; CHECK-LABEL: name: size_accumulate
; CHECK-NOT: ANDrr
; CHECK: ADDrr
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp ult i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @minsize_accumulate(i64 %a, i64 %b, i32 %count) minsize {
; CHECK-LABEL: name: minsize_accumulate
; CHECK-NOT: ANDrr
; CHECK: ADDrr
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp ult i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}

define i32 @optnone_accumulate(i64 %a, i64 %b, i32 %count) noinline optnone {
; CHECK-LABEL: name: optnone_accumulate
; CHECK-NOT: ANDrr
; CHECK: ADDrr
; CHECK-NOT: ANDrr
; CHECK: RTS
	%cmp = icmp ult i64 %a, %b
	%value = zext i1 %cmp to i32
	%result = add i32 %count, %value
	ret i32 %result
}
