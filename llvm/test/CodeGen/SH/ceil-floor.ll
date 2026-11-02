; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh3 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh3 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh3 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh3 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s

; Ordinary and size-optimized functions use the public libm C ABI.
; Volatile result stores keep the calls observable and prevent tail calls.

declare float @llvm.ceil.f32(float)
declare double @llvm.ceil.f64(double)
declare float @llvm.floor.f32(float)
declare double @llvm.floor.f64(double)

; CHECK-LABEL: ceil_f32:
; CHECK: jsr
; CHECK: .long	ceilf
define void @ceil_f32(float %x, ptr %out) #0 {
	%r = call float @llvm.ceil.f32(float %x)
	store volatile float %r, ptr %out, align 4
	ret void
}

; CHECK-LABEL: ceil_f64:
; CHECK: jsr
; CHECK: .long	ceil
define void @ceil_f64(double %x, ptr %out) #0 {
	%r = call double @llvm.ceil.f64(double %x)
	store volatile double %r, ptr %out, align 4
	ret void
}

; CHECK-LABEL: floor_f32:
; CHECK: jsr
; CHECK: .long	floorf
define void @floor_f32(float %x, ptr %out) #0 {
	%r = call float @llvm.floor.f32(float %x)
	store volatile float %r, ptr %out, align 4
	ret void
}

; CHECK-LABEL: floor_f64:
; CHECK: jsr
; CHECK: .long	floor
define void @floor_f64(double %x, ptr %out) #0 {
	%r = call double @llvm.floor.f64(double %x)
	store volatile double %r, ptr %out, align 4
	ret void
}

; CHECK-LABEL: ceil_f32_size:
; CHECK: jsr
; CHECK: .long	ceilf
define void @ceil_f32_size(float %x, ptr %out) #1 {
	%r = call float @llvm.ceil.f32(float %x)
	store volatile float %r, ptr %out, align 4
	ret void
}

; CHECK-LABEL: ceil_f64_size:
; CHECK: jsr
; CHECK: .long	ceil
define void @ceil_f64_size(double %x, ptr %out) #1 {
	%r = call double @llvm.ceil.f64(double %x)
	store volatile double %r, ptr %out, align 4
	ret void
}

; CHECK-LABEL: floor_f32_size:
; CHECK: jsr
; CHECK: .long	floorf
define void @floor_f32_size(float %x, ptr %out) #1 {
	%r = call float @llvm.floor.f32(float %x)
	store volatile float %r, ptr %out, align 4
	ret void
}

; CHECK-LABEL: floor_f64_size:
; CHECK: jsr
; CHECK: .long	floor
define void @floor_f64_size(double %x, ptr %out) #1 {
	%r = call double @llvm.floor.f64(double %x)
	store volatile double %r, ptr %out, align 4
	ret void
}

attributes #0 = { noinline }
attributes #1 = { noinline optsize minsize }
