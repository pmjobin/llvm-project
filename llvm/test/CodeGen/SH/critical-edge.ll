; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -phi-elim-split-all-critical-edges -no-phi-elim-live-out-early-exit -stop-after=phi-node-elimination < %s | FileCheck %s --check-prefix=PHI
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -phi-elim-split-all-critical-edges -no-phi-elim-live-out-early-exit -stop-after=phi-node-elimination < %s | FileCheck %s --check-prefix=PHI

define i32 @critical_phi(i32 %a, i32 %b) #0 {
entry:
	%equal = icmp eq i32 %a, %b
	br i1 %equal, label %merge, label %other
other:
	br label %merge
merge:
	%result = phi i32 [ %a, %entry ], [ %b, %other ]
	ret i32 %result
}

attributes #0 = { noinline optnone }

; ISEL-LABEL: name:            critical_phi
; ISEL: bb.0.entry:
; ISEL: CMP_EQ {{.*}}, implicit-def $tbit
; ISEL: PHI

; PHI-LABEL: name:            critical_phi
; PHI: bb.0.entry:
; PHI: successors: %bb.3
; PHI: BF %bb.1, implicit $tbit
; PHI: bb.3:
; PHI: %[[RESULT:[0-9]+]]:gpr = COPY
; PHI: BRA %bb.2
; PHI: bb.1.other:
; PHI: %[[RESULT]]:gpr = COPY
; PHI: bb.2.merge:
; PHI-NOT: PHI
