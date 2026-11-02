; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs %s -o /dev/null

target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define void @fmac_scavenger(ptr %data, float %loop_value, ptr %load_pointer, ptr %store_pointer, float %arg0, float %arg1, float %arg2, float %arg3, float %arg4) {
entry:
	br label %loop

loop:
	%value = phi float [ 0.000000e+00, %entry ], [ %loop_value, %loop ]
	%pointer = phi ptr [ %data, %entry ], [ null, %loop ]
	%loaded0 = load float, ptr %load_pointer, align 4
	%loaded1 = load float, ptr %data, align 4
	%offset16 = getelementptr i8, ptr %pointer, i32 16
	%loaded2 = load float, ptr %offset16, align 4
	store float 0.000000e+00, ptr %pointer, align 4
	%add0 = fadd float %value, 1.000000e+00
	%add1 = fadd float %add0, %arg3
	%add2 = fadd float %loaded0, %arg2
	%sub0 = fsub float %add1, %add2
	store float %sub0, ptr %offset16, align 4
	%sub1 = fsub float %arg1, %loaded2
	%sub2 = fsub float 0.000000e+00, %loaded1
	%add3 = fadd float %sub2, %sub1
	%fma0 = tail call float @llvm.fmuladd.f32(float %add3, float 0x3FE1517A80000000, float 1.000000e+00)
	%offset20 = getelementptr i8, ptr %pointer, i32 20
	store float %fma0, ptr %offset20, align 4
	%offset12 = getelementptr i8, ptr %pointer, i32 12
	store float +qnan, ptr %offset12, align 4
	%sub3 = fsub float %loaded0, %arg0
	%add4 = fadd float %arg4, %sub3
	%mul = fmul float %arg4, 0.000000e+00
	%fma1 = tail call float @llvm.fmuladd.f32(float %add4, float +qnan, float %mul)
	%offset4 = getelementptr i8, ptr %pointer, i32 4
	store float %fma1, ptr %offset4, align 4
	store float 1.000000e+00, ptr %store_pointer, align 4
	br label %loop
}

declare float @llvm.fmuladd.f32(float, float, float)
