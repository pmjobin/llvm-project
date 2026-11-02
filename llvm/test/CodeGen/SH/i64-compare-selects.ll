; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM

define i64 @umin_i64(i64 %lhs, i64 %rhs) {
; CHECK-LABEL: name: umin_i64
; CHECK: CMP_HI
; CHECK-NEXT: {{%[0-9]+}}:gpr = SUBC
; CHECK-NEXT: [[PRED:%[0-9]+]]:gpr = MOVT
; CHECK: TST [[PRED]], [[PRED]]
; CHECK-COUNT-2: PHI
	; CHECK-NOT: PHI
	; CHECK: RTS
	%condition = icmp ult i64 %lhs, %rhs
	%result = select i1 %condition, i64 %lhs, i64 %rhs
	ret i64 %result
}

define i64 @select_eq_i64(i64 %lhs, i64 %rhs, i64 %true, i64 %false) {
; CHECK-LABEL: name: select_eq_i64
; CHECK-COUNT-2: XORrr
; CHECK: [[BITS:%[0-9]+]]:gpr = ORrr
; CHECK: TST [[BITS]], [[BITS]]
; CHECK-COUNT-2: PHI
	; CHECK-NOT: PHI
	; CHECK: RTS
	%condition = icmp eq i64 %lhs, %rhs
	%result = select i1 %condition, i64 %true, i64 %false
	ret i64 %result
}

declare i64 @llvm.umin.i64(i64, i64)
declare i64 @llvm.umax.i64(i64, i64)

define i64 @minmax_i64_loop(ptr %aux, i32 %n, i1 %empty, i64 %new.high, i64 %high.ext, i64 %value) {
; CHECK-LABEL: name: minmax_i64_loop
; CHECK: RTS
; ASM-LABEL: minmax_i64_loop:
; ASM: add	#-28,r15
; ASM-NOT: add	#-36,r15
; ASM: add	#28,r15
entry:
	br i1 %empty, label %exit, label %loop

loop:
	%i = phi i32 [ 0, %entry ], [ %n, %loop ]
	%low = phi i64 [ 0, %entry ], [ %new.low, %loop ]
	%aux.ptr = getelementptr [4 x i8], ptr %aux, i32 %i
	%high.shift = shl i64 %high.ext, 32
	%low.word = load i32, ptr %aux.ptr, align 4
	%low.ext = zext i32 %low.word to i64
	%wide = or i64 %high.shift, %low.ext
	%new.low = call i64 @llvm.umin.i64(i64 %wide, i64 %low)
	%next.high = call i64 @llvm.umax.i64(i64 %value, i64 %new.high)
	br i1 %empty, label %exit, label %loop

exit:
	%final.high = phi i64 [ 0, %entry ], [ %next.high, %loop ]
	%final.low = phi i64 [ 0, %entry ], [ %low, %loop ]
	%result = add i64 %final.high, 1
	ret i64 %result
}
