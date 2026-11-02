; RUN: llc -mtriple=shle -mcpu=sh4a -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh -mcpu=sh4a -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh4a -O0 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh4a -force-new-pm-codegen -verify-machineinstrs < %s -o /dev/null
; RUN: opt -passes='default<O3>' -S %s | FileCheck %s --check-prefix=EFFECTS

declare {float, float} @llvm.sh.fsca(i32)
declare float @llvm.sh.fsrra(float)
declare float @llvm.sh.fipr(<4 x float>, <4 x float>)
declare <4 x float> @llvm.sh.ftrv(ptr, <4 x float>)
declare void @llvm.sh.ftrv4(ptr, ptr, ptr)

; CHECK-LABEL: phase:
; CHECK: lds {{.*}},fpul
; CHECK: fsca fpul,dr
; CHECK: fadd
; CHECK-NOT: __
define float @phase(i32 %p) {
  %v = call {float,float} @llvm.sh.fsca(i32 %p)
  %s = extractvalue {float,float} %v, 0
  %c = extractvalue {float,float} %v, 1
  %r = fadd float %s, %c
  ret float %r
}
; CHECK-LABEL: dot:
; CHECK: fipr
; CHECK: fadd
; CHECK: fadd
define float @dot(ptr %p, ptr %q) {
  %a = load <4 x float>, ptr %p, align 4
  %b = load <4 x float>, ptr %q, align 8
  %r = call float @llvm.sh.fipr(<4 x float> %a, <4 x float> %b)
  %x = extractelement <4 x float> %a, i32 0
  %y = extractelement <4 x float> %b, i32 3
  %t = fadd float %r, %x
  %u = fadd float %t, %y
  ret float %u
}
; CHECK-LABEL: batch:
; CHECK: fmov.d xd14,
; CHECK: fmov.d xd12,
; CHECK: fmov.d {{.*}},xd0
; CHECK-COUNT-4: ftrv xmtrx,
; CHECK: fmov.d {{.*}},xd12
; CHECK: fmov.d {{.*}},xd14
define void @batch(ptr %d, ptr %s, ptr %m) {
  call void @llvm.sh.ftrv4(ptr align 16 %d, ptr align 16 %s, ptr align 16 %m)
  ret void
}

; Unused arithmetic still observes/updates FP state and may trap. Ordinary
; memory writes alias the matrix, and neither transform can be discarded.
; EFFECTS-LABEL: define void @effects(
; EFFECTS: call float @llvm.sh.fsrra
; EFFECTS: call float @llvm.sh.fipr
; EFFECTS: call <4 x float> @llvm.sh.ftrv
; EFFECTS: store float
; EFFECTS: call <4 x float> @llvm.sh.ftrv
; EFFECTS: ret void
define void @effects(ptr %m, <4 x float> %v, float %x) {
  %r = call float @llvm.sh.fsrra(float %x)
  %s = call float @llvm.sh.fipr(<4 x float> %v, <4 x float> %v)
  %a = call <4 x float> @llvm.sh.ftrv(ptr %m, <4 x float> %v)
  store float %x, ptr %m
  %b = call <4 x float> @llvm.sh.ftrv(ptr %m, <4 x float> %v)
  ret void
}
