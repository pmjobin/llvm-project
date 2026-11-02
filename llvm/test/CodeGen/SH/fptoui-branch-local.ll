; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -O3 -mtriple=sh-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs %s -o /dev/null
; RUN: llc -O3 -mtriple=sh-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs %s -o /dev/null
; RUN: llc -O0 -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs -stop-before=finalize-isel %s -o - | FileCheck %s --check-prefix=BASE
; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu -verify-machineinstrs -stop-before=finalize-isel %s -o - | FileCheck %s --check-prefix=BASE

; Branch before correction, merge the FP operand, then signed-convert once.
; Merge a zero/high-bit integer mask without preserving the comparison T bit.
; The source stays live for the following defined store.
; CHECK-LABEL: name: single
; CHECK: FCMP_GT_FR
; CHECK-NEXT: BT %bb.[[DONE:[0-9]+]]
; CHECK: FSUB_FR
; CHECK: ROTR
; CHECK: bb.[[DONE]]
; CHECK: PHI
; CHECK-NEXT: {{.*}}PHI
; CHECK: F32_TO_I32_PSEUDO
; CHECK: XORrr
; BASE-LABEL: name: single
; BASE: SELECT_F32_PSEUDO
define i32 @single(float %x, ptr %reuse) #0 {
	%r = fptoui float %x to i32
	store volatile float %x, ptr %reuse, align 4
	ret i32 %r
}

; CHECK-LABEL: name: wide
; CHECK: FCMP_GT_DR
; CHECK-NEXT: BT %bb.[[DONE:[0-9]+]]
; CHECK: FSUB_DR
; CHECK: ROTR
; CHECK: bb.[[DONE]]
; CHECK: PHI
; CHECK-NEXT: {{.*}}PHI
; CHECK: F64_TO_I32_PSEUDO
; CHECK: XORrr
; BASE-LABEL: name: wide
; BASE: SELECT_F64_PSEUDO
define i32 @wide(double %x, ptr %reuse) #0 {
	%r = fptoui double %x to i32
	store volatile double %x, ptr %reuse, align 4
	ret i32 %r
}

; The split must transfer the loop backedge and repair both outgoing IR PHIs.
; CHECK-LABEL: name: loop
; CHECK: FCMP_GT_DR
; CHECK: FSUB_DR
; CHECK: PHI
define i32 @loop(ptr %input, i32 %n) #0 {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %body
body:
	%i = phi i32 [ 0, %entry ], [ %next, %body ]
	%sum = phi i32 [ 0, %entry ], [ %add, %body ]
	%p = getelementptr double, ptr %input, i32 %i
	%x = load double, ptr %p, align 4
	%r = fptoui double %x to i32
	%add = add i32 %sum, %r
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %exit, label %body
exit:
	%result = phi i32 [ 0, %entry ], [ %add, %body ]
	ret i32 %result
}

; A stable frozen FP value remains shared by conversion and transport.
; CHECK-LABEL: name: frozen_source
; CHECK: FCMP_GT_DR
; CHECK: FSUB_DR
; CHECK: F64_TO_I32_PSEUDO
define i32 @frozen_source(double %x, ptr %reuse) #0 {
	%stable = freeze double %x
	%r = fptoui double %stable to i32
	store volatile double %stable, ptr %reuse, align 4
	ret i32 %r
}

; Poison is not a numeric oracle. Freeze still requires one stable result for
; both uses; the conversion has one merged integer result before transport.
; CHECK-LABEL: name: frozen_result
; CHECK: F32_TO_I32_PSEUDO
; CHECK-NOT: F32_TO_I32_PSEUDO
define i32 @frozen_result(float %x, ptr %first, ptr %second) #0 {
	%r = fptoui float %x to i32
	%stable = freeze i32 %r
	store volatile i32 %stable, ptr %first, align 4
	store volatile i32 %stable, ptr %second, align 4
	ret i32 %stable
}

attributes #0 = { "no-trapping-math"="true" }
