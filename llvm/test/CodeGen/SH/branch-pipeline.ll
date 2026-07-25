; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=phi-node-elimination < %s | FileCheck %s --check-prefix=PHI
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=virtregrewriter < %s | FileCheck %s --check-prefix=RA
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-delay-slot-filler < %s -o %t.mir
; RUN: FileCheck %s --check-prefix=DELAY < %t.mir
; RUN: llc -mtriple=sh-unknown-elf -verify-machineinstrs -run-pass=sh-delay-slot-filler,sh-delay-slot-filler %t.mir -o - | FileCheck %s --check-prefix=DELAY

define i32 @max_signed_pipeline(i32 %a, i32 %b) #0 {
entry:
	%greater = icmp sgt i32 %a, %b
	br i1 %greater, label %take_a, label %take_b
take_a:
	br label %merge
take_b:
	br label %merge
merge:
	%result = phi i32 [ %a, %take_a ], [ %b, %take_b ]
	ret i32 %result
}

define i32 @loop_pipeline(i32 %n) #0 {
entry:
	br label %loop
loop:
	%value = phi i32 [ %n, %entry ], [ %next, %loop ]
	%next = add i32 %value, -1
	%continue = icmp ne i32 %next, 0
	br i1 %continue, label %loop, label %exit
exit:
	ret i32 %next
}

attributes #0 = { noinline optnone }

; ISEL-LABEL: name:            max_signed_pipeline
; ISEL: CMP_GT %{{[0-9]+}}, %{{[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BF %bb.2, implicit $tbit
; ISEL-NEXT: BRA %bb.1
; ISEL: %{{[0-9]+}}:gpr = PHI
; ISEL-LABEL: name:            loop_pipeline
; ISEL: %{{[0-9]+}}:gpr = PHI
; ISEL: CMP_EQ {{.*}}, implicit-def $tbit
; ISEL: BF %bb.1, implicit $tbit
; ISEL-NEXT: BRA %bb.2

; PHI-LABEL: name:            max_signed_pipeline
; PHI: bb.1.take_a:
; PHI: %[[MERGE:[0-9]+]]:gpr = COPY
; PHI: bb.2.take_b:
; PHI: %[[MERGE]]:gpr = COPY
; PHI-NOT: PHI
; PHI-LABEL: name:            loop_pipeline
; PHI: bb.0.entry:
; PHI: %{{[0-9]+}}:gpr = COPY $r4
; PHI-NEXT: %[[LOOP:[0-9]+]]:gpr = COPY
; PHI: bb.1.loop:
; PHI: %[[LOOP]]:gpr = COPY
; PHI-NOT: PHI

; RA-LABEL: name:            max_signed_pipeline
; RA: noVRegs:         true
; RA-NOT: PHI
; RA: CMP_GT {{.*}}, implicit-def $tbit
; RA-NEXT: BF %bb.2, implicit $tbit
; RA-LABEL: name:            loop_pipeline
; RA: noVRegs:         true
; RA-NOT: PHI
; RA: CMP_EQ {{.*}}, implicit-def $tbit
; RA: BF %bb.1, implicit $tbit

; DELAY-LABEL: name:            max_signed_pipeline
; DELAY: CMP_GT {{.*}}, implicit-def $tbit
; DELAY-NEXT: BF %bb.2, implicit $tbit
; DELAY-NEXT: BRA %bb.1 {
; DELAY-NEXT: NOP
; DELAY-NEXT: }
; DELAY: BRA %bb.3 {
; DELAY-NEXT: NOP
; DELAY-NEXT: }
; DELAY-LABEL: name:            loop_pipeline
; DELAY: CMP_EQ {{.*}}, implicit-def $tbit
; DELAY: BF %bb.1, implicit $tbit
; DELAY-NEXT: BRA %bb.2 {
; DELAY-NEXT: NOP
; DELAY-NEXT: }
; DELAY: RTS {{.*}} {
; DELAY-NEXT: NOP
; DELAY-NEXT: }
