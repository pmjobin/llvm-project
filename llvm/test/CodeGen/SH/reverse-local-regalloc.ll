; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 < %s | FileCheck %s --check-prefix=REVERSE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -greedy-reverse-local-assignment=false < %s | FileCheck %s --check-prefix=FORWARD

; Rematerializing immediate arguments lets both local allocation orders give
; equivalent sibling blocks consistent registers and share their common tails.
;
; REVERSE-COUNT-10: jsr
; REVERSE-NOT: jsr
; FORWARD-COUNT-10: jsr
; FORWARD-NOT: jsr

target triple = "shle-unknown-none-elf"

declare double @llvm.fmuladd.f64(double, double, double)

define void @reverse_local_assignment(i32 %len, float %add) optsize {
entry:
	br label %for.cond

for.cond:
	switch i32 %len, label %cond.end [
		i32 4, label %cond.true.a
		i32 5, label %cond.true.b
		i32 2, label %cond.true.c
	]

cond.true.a:
	%conv.a = fpext float %add to double
	%a0 = tail call double @llvm.fmuladd.f64(double 0.0, double %conv.a, double 0.0)
	%a1 = tail call double @llvm.fmuladd.f64(double %a0, double %conv.a, double 4.0e-1)
	br label %cond.end

cond.true.b:
	%conv.b = fpext float %add to double
	%b0 = tail call double @llvm.fmuladd.f64(double 0.0, double %conv.b, double 0.0)
	%b1 = tail call double @llvm.fmuladd.f64(double %b0, double %conv.b, double 4.0e-1)
	br label %cond.end

cond.true.c:
	%conv.c = fpext float %add to double
	%c0 = tail call double @llvm.fmuladd.f64(double 0.0, double %conv.c, double 0.0)
	%c1 = tail call double @llvm.fmuladd.f64(double %c0, double %conv.c, double 0.0)
	br label %cond.end

cond.end:
	%value = phi double [ 0.0, %for.cond ], [ %a1, %cond.true.a ], [ %b1, %cond.true.b ], [ %c1, %cond.true.c ]
	%result = fptrunc double %value to float
	store float %result, ptr null, align 4
	br label %for.cond
}
