; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ROT
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ROT
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=machine-scheduler < %s | FileCheck %s --check-prefix=SCHED
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=machine-scheduler < %s | FileCheck %s --check-prefix=SCHED
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=virtregrewriter < %s | FileCheck %s --check-prefix=RA
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=virtregrewriter < %s | FileCheck %s --check-prefix=RA
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-delay-slot-filler < %s -o %t.be.mir
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-delay-slot-filler < %s -o %t.le.mir
; RUN: FileCheck %s --check-prefix=DELAY < %t.be.mir
; RUN: FileCheck %s --check-prefix=DELAY < %t.le.mir
; RUN: llc -mtriple=sh-unknown-elf -verify-machineinstrs -run-pass=sh-delay-slot-filler,sh-delay-slot-filler %t.be.mir -o - | FileCheck %s --check-prefix=DELAY
; RUN: llc -mtriple=shle-unknown-elf -verify-machineinstrs -run-pass=sh-delay-slot-filler,sh-delay-slot-filler %t.le.mir -o - | FileCheck %s --check-prefix=DELAY

define i32 @multiply_pipeline(i32 %a, i32 %b) {
	%result = mul i32 %a, %b
	ret i32 %result
}

define i32 @unsigned_divrem_pipeline(i32 %a, i32 %b) {
	%quotient = udiv i32 %a, %b
	%remainder = urem i32 %a, %b
	%result = add i32 %quotient, %remainder
	ret i32 %result
}

define i32 @signed_divrem_pipeline(i32 %a, i32 %b) {
	%quotient = sdiv i32 %a, %b
	%remainder = srem i32 %a, %b
	%result = xor i32 %quotient, %remainder
	ret i32 %result
}

; ISEL-LABEL: name:            multiply_pipeline
; ISEL-NOT: MUL32_PSEUDO
; ISEL: MUL_L {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $macl
; ISEL-NEXT: {{%[0-9]+}}:gpr = STS_MACL implicit $macl
; ISEL-NOT: MUL32_PSEUDO

; ISEL-LABEL: name:            unsigned_divrem_pipeline
; ISEL-NOT: UDIV32_PSEUDO
; ISEL-NOT: UREM32_PSEUDO
; ISEL-NOT: UDIVREM32_PSEUDO
; ISEL: DIV0U implicit-def $mbit, implicit-def $qbit, implicit-def $tbit
; ISEL-COUNT-32: DIV1 {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $qbit, implicit-def $tbit, implicit $mbit, implicit $qbit, implicit $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = ROTCL {{%[0-9]+}}, implicit-def $tbit, implicit $tbit
; ISEL-NEXT: MUL_L {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $macl
; ISEL-NEXT: {{%[0-9]+}}:gpr = STS_MACL implicit $macl
; ISEL-NOT: DIV0U
; ISEL-NOT: DIV1

; ISEL-LABEL: name:            signed_divrem_pipeline
; ISEL-NOT: SDIV32_PSEUDO
; ISEL-NOT: SREM32_PSEUDO
; ISEL-NOT: SDIVREM32_PSEUDO
; ISEL: {{%[0-9]+}}:gpr = SHLL {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = SUBC {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit, implicit $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = XORrr
; ISEL-NEXT: {{%[0-9]+}}:gpr = SUBC {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit, implicit $tbit
; ISEL-NEXT: DIV0S {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $mbit, implicit-def $qbit, implicit-def $tbit
; ISEL-COUNT-32: DIV1 {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $qbit, implicit-def $tbit, implicit $mbit, implicit $qbit, implicit $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = ROTCL {{%[0-9]+}}, implicit-def $tbit, implicit $tbit
; ISEL-NEXT: {{%[0-9]+}}:gpr = ADDC {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit, implicit $tbit
; ISEL-NEXT: MUL_L {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $macl
; ISEL-NEXT: {{%[0-9]+}}:gpr = STS_MACL implicit $macl
; ISEL-NOT: DIV0S
; ISEL-NOT: DIV1

; ROT-LABEL: name:            unsigned_divrem_pipeline
; ROT-COUNT-33: ROTCL
; ROT-LABEL: name:            signed_divrem_pipeline
; ROT-COUNT-33: ROTCL

; SCHED-LABEL: name:            multiply_pipeline
; SCHED: MUL_L
; SCHED-NEXT: {{%[0-9]+}}:gpr = STS_MACL implicit $macl

; SCHED-LABEL: name:            unsigned_divrem_pipeline
; SCHED: DIV0U implicit-def $mbit, implicit-def $qbit, implicit-def $tbit
; SCHED-COUNT-32: DIV1 {{.*}} implicit-def $qbit, implicit-def $tbit, implicit $mbit, implicit $qbit, implicit $tbit
; SCHED: ROTCL
; SCHED-NEXT: MUL_L
; SCHED-NEXT: {{%[0-9]+}}:gpr = STS_MACL implicit $macl

; SCHED-LABEL: name:            signed_divrem_pipeline
; SCHED: SHLL {{.*}} implicit-def $tbit
; SCHED: SUBC {{.*}} implicit-def $tbit, implicit $tbit
; SCHED: XORrr
; SCHED: SUBC {{.*}} implicit-def $tbit, implicit $tbit
; SCHED: DIV0S {{.*}} implicit-def $mbit, implicit-def $qbit, implicit-def $tbit
; SCHED-COUNT-32: DIV1 {{.*}} implicit-def $qbit, implicit-def $tbit, implicit $mbit, implicit $qbit, implicit $tbit
; SCHED: ROTCL
; SCHED-NEXT: {{%[0-9]+}}:gpr = ADDC {{.*}} implicit-def $tbit, implicit $tbit
; SCHED-NEXT: MUL_L
; SCHED-NEXT: {{%[0-9]+}}:gpr = STS_MACL implicit $macl

; RA-LABEL: name:            unsigned_divrem_pipeline
; RA: noVRegs:         true
; RA: registers:       []
; RA-NOT: %{{[0-9]+}}
; RA: DIV0U implicit-def $mbit, implicit-def $qbit, implicit-def $tbit
; RA-COUNT-32: DIV1 {{.*}} implicit-def $qbit, implicit-def $tbit, implicit $mbit, implicit $qbit, implicit $tbit
; RA: MUL_L {{.*}} implicit-def $macl
; RA-NEXT: $r{{[0-9]+}} = STS_MACL implicit $macl

; RA-LABEL: name:            signed_divrem_pipeline
; RA: noVRegs:         true
; RA: registers:       []
; RA-NOT: %{{[0-9]+}}
; RA: DIV0S {{.*}} implicit-def $mbit, implicit-def $qbit, implicit-def $tbit
; RA-COUNT-32: DIV1 {{.*}} implicit-def $qbit, implicit-def $tbit, implicit $mbit, implicit $qbit, implicit $tbit
; RA: MUL_L {{.*}} implicit-def $macl
; RA-NEXT: $r{{[0-9]+}} = STS_MACL implicit $macl

; DELAY-LABEL: name:            unsigned_divrem_pipeline
; DELAY: DIV0U
; DELAY-NOT: NOP
; DELAY-COUNT-32: DIV1
; DELAY-NOT: NOP
; DELAY: RTS {{.*}} {
; DELAY-NEXT: NOP
; DELAY-NEXT: }

; DELAY-LABEL: name:            signed_divrem_pipeline
; DELAY: DIV0S
; DELAY-NOT: NOP
; DELAY-COUNT-32: DIV1
; DELAY-NOT: NOP
; DELAY: RTS {{.*}} {
; DELAY-NEXT: NOP
; DELAY-NEXT: }
