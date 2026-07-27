; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=virtregrewriter < %s | FileCheck %s --check-prefix=RA
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=prolog-epilog < %s | FileCheck %s --check-prefix=FRAME
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-delay-slot-filler < %s -o %t.mir
; RUN: FileCheck %s --check-prefix=DELAY < %t.mir
; RUN: llc -mtriple=sh-unknown-elf -verify-machineinstrs -run-pass=sh-delay-slot-filler,sh-delay-slot-filler %t.mir -o - | FileCheck %s --check-prefix=DELAY

define internal i32 @add_one(i32 %x) {
	%result = add i32 %x, 1
	ret i32 %result
}

define i32 @direct_pipeline(i32 %x, i32 %y) {
	%saved = add i32 %x, %y
	%called = call i32 @add_one(i32 %x)
	%result = add i32 %called, %saved
	ret i32 %result
}

define i32 @indirect_pipeline(ptr %fn, i32 %value) {
	%result = call i32 %fn(i32 %value)
	ret i32 %result
}

; ISEL-LABEL: name:            direct_pipeline
; ISEL: hasCalls:        true
; ISEL-LABEL: body:
; ISEL: ADJCALLSTACKDOWN 0, 0
; ISEL-NEXT: $r4 = COPY %{{[0-9]+}}
; ISEL-NEXT: BSR @add_one, csr_sh, implicit-def dead $pr, implicit $r4, implicit-def $r15, implicit-def $r0
; ISEL-NEXT: ADJCALLSTACKUP 0, 0
; ISEL-NEXT: %{{[0-9]+}}:gpr = COPY $r0

; ISEL-LABEL: name:            indirect_pipeline
; ISEL-LABEL: body:
; ISEL: %[[VALUE:[0-9]+]]:gpr = COPY $r5
; ISEL: %[[CALLEE:[0-9]+]]:gpr = COPY $r4
; ISEL: ADJCALLSTACKDOWN 0, 0
; ISEL-NEXT: $r4 = COPY %[[VALUE]]
; ISEL-NEXT: JSR %[[CALLEE]], csr_sh, implicit-def dead $pr, implicit $r4, implicit-def $r15, implicit-def $r0
; ISEL-NEXT: ADJCALLSTACKUP 0, 0
; ISEL-NEXT: %{{[0-9]+}}:gpr = COPY $r0

; RA-LABEL: name:            direct_pipeline
; RA: noVRegs:         true
; RA: registers:       []
; RA: BSR @add_one, csr_sh, implicit-def dead $pr, implicit $r4, implicit-def $r15, implicit-def $r0
; RA-LABEL: name:            indirect_pipeline
; RA: noVRegs:         true
; RA: registers:       []
; RA: JSR {{.*}}, csr_sh, implicit-def dead $pr, implicit $r4, implicit-def $r15, implicit-def $r0

; FRAME-LABEL: name:            direct_pipeline
; FRAME: stackSize:       8
; FRAME: fixedStack:
; FRAME-NEXT: - { id: 0, type: spill-slot, offset: -4, size: 4, alignment: 4
; FRAME: stack:
; FRAME: callee-saved-register: '$r8'
; FRAME-LABEL: body:
; FRAME: $r15 = frame-setup STS_L_PR $r15, implicit $pr :: (store (s32) into %fixed-stack.0)
; FRAME-NEXT: $r15 = frame-setup ADDri $r15, -4
; FRAME: BSR @add_one, csr_sh, implicit-def dead $pr
; FRAME: $r15 = frame-destroy ADDri $r15, 4
; FRAME-NEXT: $r15 = frame-destroy LDS_L_PR $r15, implicit-def $pr :: (load (s32) from %fixed-stack.0)

; FRAME-LABEL: name:            indirect_pipeline
; FRAME: stackSize:       4
; FRAME: fixedStack:
; FRAME-NEXT: - { id: 0, type: spill-slot, offset: -4, size: 4, alignment: 4
; FRAME: stack:           []
; FRAME-LABEL: body:
; FRAME: $r15 = frame-setup STS_L_PR $r15, implicit $pr :: (store (s32) into %fixed-stack.0)
; FRAME-NOT: ADDri $r15
; FRAME: JSR
; FRAME: $r15 = frame-destroy LDS_L_PR $r15, implicit-def $pr :: (load (s32) from %fixed-stack.0)

; DELAY-LABEL: name:            direct_pipeline
; DELAY: noVRegs:         true
; DELAY-LABEL: body:
; DELAY: BSR @add_one, csr_sh, implicit-def dead $pr, implicit $r4, implicit-def $r15, implicit-def $r0 {
; DELAY-NEXT: NOP
; DELAY-NEXT: }
; DELAY: RTS implicit $pr, implicit $r0 {
; DELAY-NEXT: NOP

; DELAY-LABEL: name:            indirect_pipeline
; DELAY-LABEL: body:
; DELAY: JSR {{.*}}, csr_sh, implicit-def dead $pr, implicit $r4, implicit-def $r15, implicit-def $r0 {
; DELAY-NEXT: NOP
; DELAY-NEXT: }
