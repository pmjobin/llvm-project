; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null

; CHECK-NOT: BR_CC64_PSEUDO

define i32 @eq64(i64 %a, i64 %b) {
; CHECK-LABEL: name: eq64
; CHECK: CMP_EQ
; CHECK-NEXT: BT %[[EQ_LOW:bb\.[0-9]+]]
; CHECK-NEXT: BRA %[[EQ_HIGH_UNEQUAL:bb\.[0-9]+]]
; CHECK: [[EQ_LOW]] {{.*}}:
; CHECK: CMP_EQ
; CHECK-NEXT: BF %[[EQ_HIGH_UNEQUAL]]
; CHECK-NEXT: BRA %[[EQ_LOW_EQUAL:bb\.[0-9]+]]
; CHECK: [[EQ_HIGH_UNEQUAL]] {{.*}}:
; CHECK: BRA %bb.[[EQ_FALSE:[0-9]+]]
; CHECK: [[EQ_LOW_EQUAL]] {{.*}}:
; CHECK: BRA %bb.[[EQ_TRUE:[0-9]+]]
; CHECK: bb.[[EQ_TRUE]].true:
; CHECK: bb.[[EQ_FALSE]].false:
	%condition = icmp eq i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @ne64(i64 %a, i64 %b) {
; CHECK-LABEL: name: ne64
; CHECK: CMP_EQ
; CHECK-NEXT: BT %[[NE_LOW:bb\.[0-9]+]]
; CHECK-NEXT: BRA %[[NE_HIGH_UNEQUAL:bb\.[0-9]+]]
; CHECK: [[NE_LOW]] {{.*}}:
; CHECK: CMP_EQ
; CHECK-NEXT: BT %[[NE_LOW_EQUAL:bb\.[0-9]+]]
; CHECK-NEXT: BRA %[[NE_HIGH_UNEQUAL]]
; CHECK: [[NE_LOW_EQUAL]] {{.*}}:
; CHECK: BRA %bb.[[NE_FALSE:[0-9]+]]
; CHECK: [[NE_HIGH_UNEQUAL]] {{.*}}:
; CHECK: BRA %bb.[[NE_TRUE:[0-9]+]]
; CHECK: bb.[[NE_TRUE]].true:
; CHECK: bb.[[NE_FALSE]].false:
	%condition = icmp ne i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @sge64(i64 %a, i64 %b) {
; CHECK-LABEL: name: sge64
; CHECK: CMP_EQ
; CHECK: CMP_GE
; CHECK: CMP_HS
	%condition = icmp sge i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @slt64(i64 %a, i64 %b) {
; CHECK-LABEL: name: slt64
; CHECK: CMP_EQ
; CHECK: CMP_GE
; CHECK-NEXT: {{BT|BF}}
; CHECK: CMP_HS
; CHECK-NEXT: {{BT|BF}}
	%condition = icmp slt i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @sgt64(i64 %a, i64 %b) {
; CHECK-LABEL: name: sgt64
; CHECK: CMP_EQ
; CHECK: CMP_GT
; CHECK: CMP_HI
	%condition = icmp sgt i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @sle64(i64 %a, i64 %b) {
; CHECK-LABEL: name: sle64
; CHECK: CMP_EQ
; CHECK: CMP_GT
; CHECK-NEXT: {{BT|BF}}
; CHECK: CMP_HI
; CHECK-NEXT: {{BT|BF}}
	%condition = icmp sle i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @uge64(i64 %a, i64 %b) {
; CHECK-LABEL: name: uge64
; CHECK: CMP_EQ
; CHECK-COUNT-2: CMP_HS
	%condition = icmp uge i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @ult64(i64 %a, i64 %b) {
; CHECK-LABEL: name: ult64
; CHECK: CMP_EQ
; CHECK-COUNT-2: CMP_HS
	%condition = icmp ult i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @ugt64(i64 %a, i64 %b) {
; CHECK-LABEL: name: ugt64
; CHECK: CMP_EQ
; CHECK-COUNT-2: CMP_HI
	%condition = icmp ugt i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @ule64(i64 %a, i64 %b) {
; CHECK-LABEL: name: ule64
; CHECK: CMP_EQ
; CHECK-COUNT-2: CMP_HI
	%condition = icmp ule i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @weighted_eq64(i64 %a, i64 %b) {
; CHECK-LABEL: name: weighted_eq64
; CHECK: successors: %bb.{{[0-9]+}}(0x73333333), %bb.{{[0-9]+}}(0x0ccccccd)
; CHECK: successors: %bb.{{[0-9]+}}(0x0ccccccd), %bb.{{[0-9]+}}(0x73333333)
	%condition = icmp eq i64 %a, %b
	br i1 %condition, label %true, label %false, !prof !0
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @weighted_ne64(i64 %a, i64 %b) {
; CHECK-LABEL: name: weighted_ne64
; CHECK: successors: %bb.{{[0-9]+}}(0x0ccccccd), %bb.{{[0-9]+}}(0x73333333)
; CHECK: successors: %bb.{{[0-9]+}}(0x0ccccccd), %bb.{{[0-9]+}}(0x73333333)
	%condition = icmp ne i64 %a, %b
	br i1 %condition, label %true, label %false, !prof !0
true:
	ret i32 1
false:
	ret i32 0
}

!0 = !{!"branch_weights", i32 9, i32 1}
