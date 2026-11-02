; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,LE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=NOOPT --implicit-check-not=SUBC

; The optimized unsigned encoding uses an incoming low-word borrow. Signed,
; size and unoptimized predicates continue through native type legalization.
; NOOPT-LABEL: name: ult
; NOOPT: CMP_EQ
; NOOPT: PHI
; NOOPT: RTS

define i32 @ult(i64 %lhs, i64 %rhs) {
; CHECK-LABEL: name: ult
; CHECK-DAG: [[R4:%[0-9]+]]:gpr = COPY $r4
; CHECK-DAG: [[R5:%[0-9]+]]:gpr = COPY $r5
; CHECK-DAG: [[R6:%[0-9]+]]:gpr = COPY $r6
; CHECK-DAG: [[R7:%[0-9]+]]:gpr = COPY $r7
; LE: CMP_HI [[R4]], [[R6]], implicit-def $tbit
; BE: CMP_HI [[R5]], [[R7]], implicit-def $tbit
; LE-NEXT: {{%[0-9]+}}:gpr = SUBC [[R5]], [[R7]], implicit-def $tbit, implicit $tbit
; BE-NEXT: {{%[0-9]+}}:gpr = SUBC [[R4]], [[R6]], implicit-def $tbit, implicit $tbit
; CHECK-NEXT: {{%[0-9]+}}:gpr = MOVT implicit $tbit
; CHECK-NOT: PHI
; CHECK: RTS
	%condition = icmp ult i64 %lhs, %rhs
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @ule(i64 %lhs, i64 %rhs) {
; CHECK-LABEL: name: ule
; CHECK-DAG: [[R4:%[0-9]+]]:gpr = COPY $r4
; CHECK-DAG: [[R5:%[0-9]+]]:gpr = COPY $r5
; CHECK-DAG: [[R6:%[0-9]+]]:gpr = COPY $r6
; CHECK-DAG: [[R7:%[0-9]+]]:gpr = COPY $r7
; LE: CMP_HS [[R4]], [[R6]], implicit-def $tbit
; BE: CMP_HS [[R5]], [[R7]], implicit-def $tbit
; LE-NEXT: {{%[0-9]+}}:gpr = SUBC [[R5]], [[R7]], implicit-def $tbit, implicit $tbit
; BE-NEXT: {{%[0-9]+}}:gpr = SUBC [[R4]], [[R6]], implicit-def $tbit, implicit $tbit
; CHECK-NEXT: {{%[0-9]+}}:gpr = MOVT implicit $tbit
; CHECK-NOT: PHI
; CHECK: RTS
	%condition = icmp ule i64 %lhs, %rhs
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @ugt(i64 %lhs, i64 %rhs) {
; CHECK-LABEL: name: ugt
; CHECK-DAG: [[R4:%[0-9]+]]:gpr = COPY $r4
; CHECK-DAG: [[R5:%[0-9]+]]:gpr = COPY $r5
; CHECK-DAG: [[R6:%[0-9]+]]:gpr = COPY $r6
; CHECK-DAG: [[R7:%[0-9]+]]:gpr = COPY $r7
; LE: CMP_HI [[R6]], [[R4]], implicit-def $tbit
; BE: CMP_HI [[R7]], [[R5]], implicit-def $tbit
; LE-NEXT: {{%[0-9]+}}:gpr = SUBC [[R7]], [[R5]], implicit-def $tbit, implicit $tbit
; BE-NEXT: {{%[0-9]+}}:gpr = SUBC [[R6]], [[R4]], implicit-def $tbit, implicit $tbit
; CHECK-NEXT: {{%[0-9]+}}:gpr = MOVT implicit $tbit
; CHECK-NOT: PHI
; CHECK: RTS
	%condition = icmp ugt i64 %lhs, %rhs
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @uge(i64 %lhs, i64 %rhs) {
; CHECK-LABEL: name: uge
; CHECK-DAG: [[R4:%[0-9]+]]:gpr = COPY $r4
; CHECK-DAG: [[R5:%[0-9]+]]:gpr = COPY $r5
; CHECK-DAG: [[R6:%[0-9]+]]:gpr = COPY $r6
; CHECK-DAG: [[R7:%[0-9]+]]:gpr = COPY $r7
; LE: CMP_HS [[R6]], [[R4]], implicit-def $tbit
; BE: CMP_HS [[R7]], [[R5]], implicit-def $tbit
; LE-NEXT: {{%[0-9]+}}:gpr = SUBC [[R7]], [[R5]], implicit-def $tbit, implicit $tbit
; BE-NEXT: {{%[0-9]+}}:gpr = SUBC [[R6]], [[R4]], implicit-def $tbit, implicit $tbit
; CHECK-NEXT: {{%[0-9]+}}:gpr = MOVT implicit $tbit
; CHECK-NOT: PHI
; CHECK: RTS
	%condition = icmp uge i64 %lhs, %rhs
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @size_ult(i64 %lhs, i64 %rhs) optsize {
; CHECK-LABEL: name: size_ult
; CHECK: CMP_EQ
; CHECK-NOT: SUBC
; CHECK: PHI
; CHECK: RTS
	%condition = icmp ult i64 %lhs, %rhs
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @signed_lt(i64 %lhs, i64 %rhs) {
; CHECK-LABEL: name: signed_lt
; CHECK: CMP_HI
; CHECK: CMP_GT
; CHECK: CMP_EQ
; CHECK: PHI
; CHECK: RTS
	%condition = icmp slt i64 %lhs, %rhs
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @minsize_ult(i64 %lhs, i64 %rhs) minsize {
; CHECK-LABEL: name: minsize_ult
; CHECK: CMP_EQ
; CHECK-NOT: SUBC
; CHECK: PHI
; CHECK: RTS
	%condition = icmp ult i64 %lhs, %rhs
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @noopt_ult(i64 %lhs, i64 %rhs) noinline optnone {
; CHECK-LABEL: name: noopt_ult
; CHECK: CMP_EQ
; CHECK-NOT: SUBC
; CHECK: PHI
; CHECK: RTS
	%condition = icmp ult i64 %lhs, %rhs
	%result = zext i1 %condition to i32
	ret i32 %result
}

; A preceding optnone function must not change the captured command-line level.
define i32 @after_noopt_ult(i64 %lhs, i64 %rhs) {
; CHECK-LABEL: name: after_noopt_ult
; CHECK: CMP_HI
; CHECK-NEXT: {{%[0-9]+}}:gpr = SUBC
; CHECK-NEXT: {{%[0-9]+}}:gpr = MOVT
; CHECK-NOT: PHI
; CHECK: RTS
; NOOPT-LABEL: name: after_noopt_ult
; NOOPT: CMP_EQ
; NOOPT: PHI
; NOOPT: RTS
  %condition = icmp ult i64 %lhs, %rhs
  %result = zext i1 %condition to i32
  ret i32 %result
}
