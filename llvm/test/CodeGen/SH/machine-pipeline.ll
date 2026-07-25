; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=virtregrewriter < %s | FileCheck %s --check-prefix=RA
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-delay-slot-filler < %s -o %t.mir
; RUN: llc -mtriple=sh-unknown-elf -verify-machineinstrs -run-pass=sh-delay-slot-filler,sh-delay-slot-filler %t.mir -o - | FileCheck %s --check-prefix=DELAY

define i32 @add_i32(i32 %a, i32 %b) {
	%sum = add i32 %a, %b
	ret i32 %sum
}

; ISEL-LABEL: name:            add_i32
; ISEL: registers:
; ISEL-DAG: { id: 0, class: gpr,
; ISEL-DAG: { id: 1, class: gpr,
; ISEL-DAG: { id: 2, class: gpr,
; ISEL: liveins:
; ISEL-NEXT: - { reg: '$r4', virtual-reg: '%0' }
; ISEL-NEXT: - { reg: '$r5', virtual-reg: '%1' }
; ISEL: stackSize:       0
; ISEL: fixedStack:      []
; ISEL-NEXT: stack:           []
; ISEL-LABEL: body:
; ISEL: liveins: $r4, $r5
; ISEL-DAG: %{{[0-9]+}}:gpr = COPY $r4
; ISEL-DAG: %{{[0-9]+}}:gpr = COPY $r5
; ISEL: %{{[0-9]+}}:gpr = ADDrr %{{[0-9]+}}, %{{[0-9]+}}
; ISEL-NEXT: $r0 = COPY %{{[0-9]+}}
; ISEL-NEXT: RTS implicit $pr, implicit $r0

; RA-LABEL: name:            add_i32
; RA: noVRegs:         true
; RA: registers:       []
; RA: liveins:
; RA-NEXT: - { reg: '$r4', virtual-reg: '' }
; RA-NEXT: - { reg: '$r5', virtual-reg: '' }
; RA: stackSize:       0
; RA: fixedStack:      []
; RA-NEXT: stack:           []
; RA-LABEL: body:
; RA: liveins: $r4, $r5
; RA: $r0 = COPY $r4
; RA-NEXT: $r0 = ADDrr killed $r0, killed $r5
; RA-NEXT: RTS implicit $pr, implicit $r0

; DELAY-LABEL: name:            add_i32
; DELAY: noVRegs:         true
; DELAY: registers:       []
; DELAY: stackSize:       0
; DELAY: fixedStack:      []
; DELAY-NEXT: stack:           []
; DELAY-LABEL: body:
; DELAY: $r0 = MOVrr $r4
; DELAY-NEXT: $r0 = ADDrr killed $r0, killed $r5
; DELAY-NEXT: RTS implicit $pr, implicit $r0 {
; DELAY-NEXT: NOP
; DELAY-NEXT: }
; DELAY-NOT: NOP
