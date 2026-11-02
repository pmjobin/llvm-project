; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -stop-after=early-machinelicm %s -o - | FileCheck %s

; Pre-RA invariant hoisting extends live ranges around loops.  Keep it enabled
; normally, but avoid its register-pressure and spill cost for size functions.

define void @default_size() nounwind {
entry:
	br label %loop

loop:
	%cond = select i1 false, float 0.000000e+00, float 0.000000e+00
	br label %loop
}

define void @opt_size() nounwind optsize {
entry:
	br label %loop

loop:
	%cond = select i1 false, float 0.000000e+00, float 0.000000e+00
	br label %loop
}

define void @min_size() nounwind minsize optsize {
entry:
	br label %loop

loop:
	%cond = select i1 false, float 0.000000e+00, float 0.000000e+00
	br label %loop
}

; CHECK-LABEL: name: default_size
; CHECK: bb.0.entry:
; CHECK: %[[DEFAULT:[0-9]+]]:gpr = MOVri 0
; CHECK: bb.1.loop:
; CHECK-NOT: MOVri
; CHECK: TST %[[DEFAULT]], %[[DEFAULT]], implicit-def $tbit

; CHECK-LABEL: name: opt_size
; CHECK: bb.0.entry:
; CHECK-NOT: MOVri
; CHECK: bb.1.loop:
; CHECK: %[[OPT:[0-9]+]]:gpr = MOVri 0
; CHECK: TST %[[OPT]], %[[OPT]], implicit-def $tbit

; CHECK-LABEL: name: min_size
; CHECK: bb.0.entry:
; CHECK-NOT: MOVri
; CHECK: bb.1.loop:
; CHECK: %[[MIN:[0-9]+]]:gpr = MOVri 0
; CHECK: TST %[[MIN]], %[[MIN]], implicit-def $tbit
