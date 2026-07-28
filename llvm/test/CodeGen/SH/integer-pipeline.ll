; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=phi-node-elimination < %s | FileCheck %s --check-prefix=PHI
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=virtregrewriter < %s | FileCheck %s --check-prefix=RA
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-delay-slot-filler < %s -o %t.mir
; RUN: FileCheck %s --check-prefix=DELAY < %t.mir
; RUN: llc -mtriple=sh-unknown-elf -verify-machineinstrs -run-pass=sh-delay-slot-filler,sh-delay-slot-filler %t.mir -o - | FileCheck %s --check-prefix=DELAY

define i32 @variable_shift_pipeline(i32 %value, i32 %count) {
	%shifted = shl i32 %value, %count
	%constant = xor i32 %shifted, 305419896
	ret i32 %constant
}

; ISEL-LABEL: name:            variable_shift_pipeline
; ISEL-NOT: SHLrr
; ISEL-NOT: MOVi32
; ISEL: successors: %bb.1
; ISEL: MOVri 31
; ISEL-NEXT: {{%[0-9]+}}:gpr = ANDrr
; ISEL-NEXT: TST {{.*}}, implicit-def $tbit
; ISEL-NEXT: BT %bb.2, implicit $tbit
; ISEL: {{%[0-9]+}}:gpr = PHI
; ISEL-NEXT: {{%[0-9]+}}:gpr = PHI
; ISEL-NEXT: {{%[0-9]+}}:gpr = SHLL {{.*}}, implicit-def $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = DT {{.*}}, implicit-def $tbit
; ISEL-NEXT: BF %bb.1, implicit $tbit
; ISEL: {{%[0-9]+}}:gpr = PHI
; ISEL: SHLL8
; ISEL: XORrr
; ISEL-NOT: SHLrr
; ISEL-NOT: MOVi32

; PHI-LABEL: name:            variable_shift_pipeline
; PHI-NOT: PHI
; PHI: SHLL {{.*}}, implicit-def $tbit
; PHI-NEXT: DT {{.*}}, implicit-def $tbit
; PHI: BF %bb.1, implicit $tbit
; PHI: XORrr

; RA-LABEL: name:            variable_shift_pipeline
; RA: noVRegs:         true
; RA: registers:       []
; RA-NOT: PHI
; RA-NOT: %{{[0-9]+}}
; RA: SHLL {{.*}}, implicit-def $tbit
; RA-NEXT: DT {{.*}}, implicit-def $tbit
; RA: BF %bb.1, implicit $tbit
; RA: XORrr

; DELAY-LABEL: name:            variable_shift_pipeline
; DELAY: TST {{.*}}, implicit-def $tbit
; DELAY: BT %bb.2, implicit $tbit
; DELAY-NOT: NOP
; DELAY: SHLL {{.*}}, implicit-def $tbit
; DELAY-NEXT: DT {{.*}}, implicit-def $tbit
; DELAY: BF %bb.1, implicit $tbit
; DELAY-NOT: NOP
; DELAY: RTS {{.*}} {
; DELAY-NEXT: NOP
; DELAY-NEXT: }
