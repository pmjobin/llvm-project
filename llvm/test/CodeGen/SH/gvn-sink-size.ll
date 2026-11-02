; RUN: opt -mtriple=shle-unknown-none-elf -passes='default<O0>' -S %s -o - | FileCheck %s --check-prefix=NO-SINK
; RUN: sed 's/ optsize//' %s | opt -mtriple=shle-unknown-none-elf -passes='default<O2>' -S -o - | FileCheck %s --check-prefix=NO-SINK
; RUN: opt -mtriple=shle-unknown-none-elf -passes='default<O2>' -S %s -o - | FileCheck %s --check-prefix=SINK

target triple = "shle-unknown-none-elf"

declare double @llvm.fmuladd.f64(double, double, double) #0

; Repeated GVN sinking exposes another common suffix in the switch arms.
; One round leaves 17 multiply-adds; four rounds reach 11.
;
; NO-SINK-LABEL: define void @common_polynomial_suffix(
; NO-SINK: switch i32
; NO-SINK-COUNT-21: tail call double @llvm.fmuladd.f64
;
; SINK-LABEL: define void @common_polynomial_suffix(
; SINK: switch i32
; SINK-COUNT-11: tail call double @llvm.fmuladd.f64

define void @common_polynomial_suffix(ptr %f, ptr %x, i32 %len) #1 {
entry:
	br label %for.cond

for.cond:
	%index = phi i32 [ 0, %entry ], [ %inc, %cond.end ]
	%done = icmp eq i32 %index, %len
	br i1 %done, label %for.end, label %for.body

for.body:
	%input.ptr = getelementptr inbounds nuw [8 x i8], ptr %x, i32 %index
	%input = load double, ptr %input.ptr, align 4
	%add = fadd double %input, 2.000000e+00
	%negative = fcmp olt double %add, 0.000000e+00
	%sub = fadd double %add, -1.000000e+00
	%cond = select i1 %negative, double %sub, double %add
	%selector = fptosi double %cond to i32
	%selector.fp = sitofp i32 %selector to double
	%value = fsub double %add, %selector.fp
	switch i32 %selector, label %cond.end [
		i32 0, label %case.0
		i32 1, label %case.1
		i32 2, label %case.2
		i32 3, label %case.3
	]

case.0:
	%0 = tail call double @llvm.fmuladd.f64(double %value, double 3.000000e+00, double -1.050000e+01)
	%1 = tail call double @llvm.fmuladd.f64(double %0, double %value, double 1.250000e+01)
	%2 = tail call double @llvm.fmuladd.f64(double %1, double %value, double -5.000000e+00)
	%mul.0 = fmul double %value, %2
	%3 = tail call double @llvm.fmuladd.f64(double %mul.0, double %value, double 0.000000e+00)
	%4 = tail call double @llvm.fmuladd.f64(double %3, double %value, double 0.000000e+00)
	%result.0 = fmul double %value, %4
	br label %cond.end

case.1:
	%5 = tail call double @llvm.fmuladd.f64(double %value, double -9.000000e+00, double 3.150000e+01)
	%6 = tail call double @llvm.fmuladd.f64(double %5, double %value, double -3.750000e+01)
	%7 = tail call double @llvm.fmuladd.f64(double %6, double %value, double 1.500000e+01)
	%mul.1 = fmul double %value, %7
	%8 = tail call double @llvm.fmuladd.f64(double %mul.1, double %value, double 5.000000e-01)
	%9 = tail call double @llvm.fmuladd.f64(double %8, double %value, double 5.000000e-01)
	%result.1 = fmul double %value, %9
	br label %cond.end

case.2:
	%10 = tail call double @llvm.fmuladd.f64(double %value, double 9.000000e+00, double -3.150000e+01)
	%11 = tail call double @llvm.fmuladd.f64(double %10, double %value, double 3.750000e+01)
	%12 = tail call double @llvm.fmuladd.f64(double %11, double %value, double -1.500000e+01)
	%mul.2 = fmul double %value, %12
	%13 = tail call double @llvm.fmuladd.f64(double %mul.2, double %value, double -1.000000e+00)
	%14 = tail call double @llvm.fmuladd.f64(double %13, double %value, double 0.000000e+00)
	%result.2 = tail call double @llvm.fmuladd.f64(double %14, double %value, double 1.000000e+00)
	br label %cond.end

case.3:
	%15 = tail call double @llvm.fmuladd.f64(double %value, double -3.000000e+00, double 1.050000e+01)
	%16 = tail call double @llvm.fmuladd.f64(double %15, double %value, double -1.250000e+01)
	%17 = tail call double @llvm.fmuladd.f64(double %16, double %value, double 5.000000e+00)
	%mul.3 = fmul double %value, %17
	%18 = tail call double @llvm.fmuladd.f64(double %mul.3, double %value, double 5.000000e-01)
	%19 = tail call double @llvm.fmuladd.f64(double %18, double %value, double -5.000000e-01)
	%result.3 = fmul double %value, %19
	br label %cond.end

cond.end:
	%result = phi double [ %result.0, %case.0 ], [ %result.1, %case.1 ], [ %result.2, %case.2 ], [ %result.3, %case.3 ], [ 0.000000e+00, %for.body ]
	%output.ptr = getelementptr inbounds nuw [8 x i8], ptr %f, i32 %index
	store double %result, ptr %output.ptr, align 4
	%inc = add i32 %index, 1
	br label %for.cond

for.end:
	ret void
}

attributes #0 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }
attributes #1 = { noinline optsize }
