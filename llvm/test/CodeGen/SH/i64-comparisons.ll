; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,LE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null

; Native type legalization uses XOR/OR for equality and selects the low-word
; predicate when the high words agree. Check operand significance and branch
; destinations, independently of the retired target-specific CFG.

define i32 @eq64(i64 %a, i64 %b) {
; CHECK-LABEL: name: eq64
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK-DAG: [[XLO:%[0-9]+]]:gpr = XORrr [[ALO]], [[BLO]]
; CHECK-DAG: [[XHI:%[0-9]+]]:gpr = XORrr [[AHI]], [[BHI]]
; CHECK: [[BITS:%[0-9]+]]:gpr = ORrr [[XLO]], [[XHI]]
; CHECK-NEXT: TST [[BITS]], [[BITS]], implicit-def $tbit
; CHECK-NEXT: BF %[[FALSE:bb\.[0-9]+]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE:bb\.[0-9]+]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp eq i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @ne64(i64 %a, i64 %b) {
; CHECK-LABEL: name: ne64
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK-DAG: [[XLO:%[0-9]+]]:gpr = XORrr [[ALO]], [[BLO]]
; CHECK-DAG: [[XHI:%[0-9]+]]:gpr = XORrr [[AHI]], [[BHI]]
; CHECK: [[BITS:%[0-9]+]]:gpr = ORrr [[XLO]], [[XHI]]
; CHECK-NEXT: TST [[BITS]], [[BITS]], implicit-def $tbit
; CHECK-NEXT: BT %[[FALSE:bb\.[0-9]+]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE:bb\.[0-9]+]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp ne i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @sge64(i64 %a, i64 %b) {
; CHECK-LABEL: name: sge64
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK: CMP_HI [[ALO]], [[BLO]], implicit-def $tbit
; CHECK-NEXT: [[LOW:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_GT [[AHI]], [[BHI]], implicit-def $tbit
; CHECK-NEXT: [[HIGH:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_EQ [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[EQUAL:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK-NEXT: TST [[EQUAL]], [[EQUAL]], implicit-def $tbit
; CHECK-NEXT: BF %[[JOIN:bb\.[0-9]+]], implicit $tbit
; CHECK: [[HIGH_PATH:bb\.[0-9]+]] {{.*}}:
; CHECK: [[JOIN]] {{.*}}:
; CHECK: [[PRED:%[0-9]+]]:gpr = PHI [[LOW]], %bb.0, [[HIGH]], %[[HIGH_PATH]]
; CHECK-NEXT: TST [[PRED]], [[PRED]], implicit-def $tbit
; CHECK-NEXT: BF %[[FALSE:bb\.[0-9]+]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE:bb\.[0-9]+]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp sge i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @slt64(i64 %a, i64 %b) {
; CHECK-LABEL: name: slt64
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK: CMP_HS [[BLO]], [[ALO]], implicit-def $tbit
; CHECK-NEXT: [[LOW:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_GE [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[HIGH:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_EQ [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[EQUAL:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK-NEXT: TST [[EQUAL]], [[EQUAL]], implicit-def $tbit
; CHECK-NEXT: BF %[[JOIN:bb\.[0-9]+]], implicit $tbit
; CHECK: [[HIGH_PATH:bb\.[0-9]+]] {{.*}}:
; CHECK: [[JOIN]] {{.*}}:
; CHECK: [[PRED:%[0-9]+]]:gpr = PHI [[LOW]], %bb.0, [[HIGH]], %[[HIGH_PATH]]
; CHECK-NEXT: TST [[PRED]], [[PRED]], implicit-def $tbit
; CHECK-NEXT: BF %[[FALSE:bb\.[0-9]+]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE:bb\.[0-9]+]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp slt i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @sgt64(i64 %a, i64 %b) {
; CHECK-LABEL: name: sgt64
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK: CMP_HS [[ALO]], [[BLO]], implicit-def $tbit
; CHECK-NEXT: [[LOW:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_GE [[AHI]], [[BHI]], implicit-def $tbit
; CHECK-NEXT: [[HIGH:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_EQ [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[EQUAL:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK-NEXT: TST [[EQUAL]], [[EQUAL]], implicit-def $tbit
; CHECK-NEXT: BF %[[JOIN:bb\.[0-9]+]], implicit $tbit
; CHECK: [[HIGH_PATH:bb\.[0-9]+]] {{.*}}:
; CHECK: [[JOIN]] {{.*}}:
; CHECK: [[PRED:%[0-9]+]]:gpr = PHI [[LOW]], %bb.0, [[HIGH]], %[[HIGH_PATH]]
; CHECK-NEXT: TST [[PRED]], [[PRED]], implicit-def $tbit
; CHECK-NEXT: BF %[[FALSE:bb\.[0-9]+]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE:bb\.[0-9]+]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp sgt i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @sle64(i64 %a, i64 %b) {
; CHECK-LABEL: name: sle64
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK: CMP_HI [[BLO]], [[ALO]], implicit-def $tbit
; CHECK-NEXT: [[LOW:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_GT [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[HIGH:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_EQ [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[EQUAL:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK-NEXT: TST [[EQUAL]], [[EQUAL]], implicit-def $tbit
; CHECK-NEXT: BF %[[JOIN:bb\.[0-9]+]], implicit $tbit
; CHECK: [[HIGH_PATH:bb\.[0-9]+]] {{.*}}:
; CHECK: [[JOIN]] {{.*}}:
; CHECK: [[PRED:%[0-9]+]]:gpr = PHI [[LOW]], %bb.0, [[HIGH]], %[[HIGH_PATH]]
; CHECK-NEXT: TST [[PRED]], [[PRED]], implicit-def $tbit
; CHECK-NEXT: BF %[[FALSE:bb\.[0-9]+]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE:bb\.[0-9]+]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp sle i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @uge64(i64 %a, i64 %b) {
; CHECK-LABEL: name: uge64
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK: CMP_HI [[ALO]], [[BLO]], implicit-def $tbit
; CHECK-NEXT: [[LOW:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_HI [[AHI]], [[BHI]], implicit-def $tbit
; CHECK-NEXT: [[HIGH:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_EQ [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[EQUAL:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK-NEXT: TST [[EQUAL]], [[EQUAL]], implicit-def $tbit
; CHECK-NEXT: BF %[[JOIN:bb\.[0-9]+]], implicit $tbit
; CHECK: [[HIGH_PATH:bb\.[0-9]+]] {{.*}}:
; CHECK: [[JOIN]] {{.*}}:
; CHECK: [[PRED:%[0-9]+]]:gpr = PHI [[LOW]], %bb.0, [[HIGH]], %[[HIGH_PATH]]
; CHECK-NEXT: TST [[PRED]], [[PRED]], implicit-def $tbit
; CHECK-NEXT: BF %[[FALSE:bb\.[0-9]+]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE:bb\.[0-9]+]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp uge i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @ult64(i64 %a, i64 %b) {
; CHECK-LABEL: name: ult64
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK: CMP_HS [[BLO]], [[ALO]], implicit-def $tbit
; CHECK-NEXT: [[LOW:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_HS [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[HIGH:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_EQ [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[EQUAL:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK-NEXT: TST [[EQUAL]], [[EQUAL]], implicit-def $tbit
; CHECK-NEXT: BF %[[JOIN:bb\.[0-9]+]], implicit $tbit
; CHECK: [[HIGH_PATH:bb\.[0-9]+]] {{.*}}:
; CHECK: [[JOIN]] {{.*}}:
; CHECK: [[PRED:%[0-9]+]]:gpr = PHI [[LOW]], %bb.0, [[HIGH]], %[[HIGH_PATH]]
; CHECK-NEXT: TST [[PRED]], [[PRED]], implicit-def $tbit
; CHECK-NEXT: BF %[[FALSE:bb\.[0-9]+]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE:bb\.[0-9]+]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp ult i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @ugt64(i64 %a, i64 %b) {
; CHECK-LABEL: name: ugt64
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK: CMP_HS [[ALO]], [[BLO]], implicit-def $tbit
; CHECK-NEXT: [[LOW:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_HS [[AHI]], [[BHI]], implicit-def $tbit
; CHECK-NEXT: [[HIGH:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_EQ [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[EQUAL:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK-NEXT: TST [[EQUAL]], [[EQUAL]], implicit-def $tbit
; CHECK-NEXT: BF %[[JOIN:bb\.[0-9]+]], implicit $tbit
; CHECK: [[HIGH_PATH:bb\.[0-9]+]] {{.*}}:
; CHECK: [[JOIN]] {{.*}}:
; CHECK: [[PRED:%[0-9]+]]:gpr = PHI [[LOW]], %bb.0, [[HIGH]], %[[HIGH_PATH]]
; CHECK-NEXT: TST [[PRED]], [[PRED]], implicit-def $tbit
; CHECK-NEXT: BF %[[FALSE:bb\.[0-9]+]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE:bb\.[0-9]+]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp ugt i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @ule64(i64 %a, i64 %b) {
; CHECK-LABEL: name: ule64
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK: CMP_HI [[BLO]], [[ALO]], implicit-def $tbit
; CHECK-NEXT: [[LOW:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_HI [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[HIGH:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK: CMP_EQ [[BHI]], [[AHI]], implicit-def $tbit
; CHECK-NEXT: [[EQUAL:%[0-9]+]]:gpr = MOVT implicit $tbit
; CHECK-NEXT: TST [[EQUAL]], [[EQUAL]], implicit-def $tbit
; CHECK-NEXT: BF %[[JOIN:bb\.[0-9]+]], implicit $tbit
; CHECK: [[HIGH_PATH:bb\.[0-9]+]] {{.*}}:
; CHECK: [[JOIN]] {{.*}}:
; CHECK: [[PRED:%[0-9]+]]:gpr = PHI [[LOW]], %bb.0, [[HIGH]], %[[HIGH_PATH]]
; CHECK-NEXT: TST [[PRED]], [[PRED]], implicit-def $tbit
; CHECK-NEXT: BF %[[FALSE:bb\.[0-9]+]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE:bb\.[0-9]+]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp ule i64 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @weighted_eq64(i64 %a, i64 %b) {
; CHECK-LABEL: name: weighted_eq64
; CHECK: successors: %[[TRUE:bb\.[0-9]+]](0x73333333), %[[FALSE:bb\.[0-9]+]](0x0ccccccd)
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK-DAG: [[XLO:%[0-9]+]]:gpr = XORrr [[ALO]], [[BLO]]
; CHECK-DAG: [[XHI:%[0-9]+]]:gpr = XORrr [[AHI]], [[BHI]]
; CHECK: [[BITS:%[0-9]+]]:gpr = ORrr [[XLO]], [[XHI]]
; CHECK-NEXT: TST [[BITS]], [[BITS]], implicit-def $tbit
; CHECK-NEXT: BF %[[FALSE]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp eq i64 %a, %b
	br i1 %condition, label %true, label %false, !prof !0
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @weighted_ne64(i64 %a, i64 %b) {
; CHECK-LABEL: name: weighted_ne64
; CHECK: successors: %[[TRUE:bb\.[0-9]+]](0x73333333), %[[FALSE:bb\.[0-9]+]](0x0ccccccd)
; LE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r4
; LE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r5
; LE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r6
; LE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r7
; BE-DAG: [[AHI:%[0-9]+]]:gpr = COPY $r4
; BE-DAG: [[ALO:%[0-9]+]]:gpr = COPY $r5
; BE-DAG: [[BHI:%[0-9]+]]:gpr = COPY $r6
; BE-DAG: [[BLO:%[0-9]+]]:gpr = COPY $r7
; CHECK-DAG: [[XLO:%[0-9]+]]:gpr = XORrr [[ALO]], [[BLO]]
; CHECK-DAG: [[XHI:%[0-9]+]]:gpr = XORrr [[AHI]], [[BHI]]
; CHECK: [[BITS:%[0-9]+]]:gpr = ORrr [[XLO]], [[XHI]]
; CHECK-NEXT: TST [[BITS]], [[BITS]], implicit-def $tbit
; CHECK-NEXT: BT %[[FALSE]], implicit $tbit
; CHECK-NEXT: BRA %[[TRUE]]
; CHECK: [[TRUE]].true:
; CHECK: MOVri 1
; CHECK: [[FALSE]].false:
; CHECK: MOVri 0
	%condition = icmp ne i64 %a, %b
	br i1 %condition, label %true, label %false, !prof !0
true:
	ret i32 1
false:
	ret i32 0
}

!0 = !{!"branch_weights", i32 9, i32 1}
