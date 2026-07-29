; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-before=prolog-epilog < %s | FileCheck %s --check-prefix=PREPEI
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=prolog-epilog < %s | FileCheck %s --check-prefix=PEI
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-delay-slot-filler < %s -o %t.mir
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -verify-machineinstrs -run-pass=sh-delay-slot-filler,sh-delay-slot-filler %t.mir -o - | FileCheck %s --check-prefix=DELAY

define internal i32 @pipeline5_callee(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4) noinline nounwind {
	ret i32 %a4
}

define i32 @pipeline5(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4) nounwind {
	%result = call i32 @pipeline5_callee(i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a0)
	ret i32 %result
}

define i32 @pipeline_indirect8(ptr %fn, i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7) nounwind {
	%result = call i32 %fn(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7)
	ret i32 %result
}

; ISEL-LABEL: name:            pipeline5{{$}}
; ISEL: fixedStack:
; ISEL-NEXT: - { id: 0, type: default, offset: 0, size: 4, alignment: 4
; ISEL-LABEL: body:
; ISEL: %[[INCOMING:[0-9]+]]:gpr = MOVL_load_disp %fixed-stack.0, 0 :: (load (s32) from %fixed-stack.0)
; ISEL: ADJCALLSTACKDOWN 4, 0
; ISEL-NEXT: MOVL_store_reg %{{[0-9]+}}, $r15 :: (store (s32) into stack)
; ISEL-NEXT: $r4 = COPY
; ISEL-NEXT: $r5 = COPY
; ISEL-NEXT: $r6 = COPY
; ISEL-NEXT: $r7 = COPY %[[INCOMING]]
; ISEL-NEXT: BSR @pipeline5_callee, csr_sh
; ISEL-NEXT: ADJCALLSTACKUP 4, 0
; ISEL-NEXT: %{{[0-9]+}}:gpr = COPY $r0

; ISEL-LABEL: name:            pipeline_indirect8{{$}}
; ISEL: fixedStack:
; ISEL-DAG: offset: 0, size: 4, alignment: 4
; ISEL-DAG: offset: 4, size: 4, alignment: 4
; ISEL-DAG: offset: 8, size: 4, alignment: 4
; ISEL-DAG: offset: 12, size: 4, alignment: 4
; ISEL-DAG: offset: 16, size: 4, alignment: 4
; ISEL-LABEL: body:
; ISEL: ADJCALLSTACKDOWN 16, 0
; ISEL-DAG: MOVL_store_reg {{.*}}, $r15 :: (store (s32) into stack)
; ISEL-DAG: MOVL_store_disp {{.*}}, $r15, 4 :: (store (s32) into stack + 4)
; ISEL-DAG: MOVL_store_disp {{.*}}, $r15, 8 :: (store (s32) into stack + 8)
; ISEL-DAG: MOVL_store_disp {{.*}}, $r15, 12 :: (store (s32) into stack + 12)
; ISEL: $r4 = COPY
; ISEL: JSR %{{[0-9]+}}, csr_sh
; ISEL-NEXT: ADJCALLSTACKUP 16, 0

; PREPEI-LABEL: name:            pipeline5{{$}}
; PREPEI: stackSize:       0
; PREPEI: maxCallFrameSize: 4294967295
; PREPEI: fixedStack:
; PREPEI-NEXT: - { id: 0, type: default, offset: 0, size: 4, alignment: 4
; PREPEI: stack:
; PREPEI-NOT: type: default
; PREPEI-LABEL: body:
; PREPEI: MOVL_load_disp %fixed-stack.0, 0
; PREPEI: ADJCALLSTACKDOWN 4, 0
; PREPEI: MOVL_store_reg {{.*}}, $r15 :: (store (s32) into stack)
; PREPEI: BSR @pipeline5_callee, csr_sh
; PREPEI: ADJCALLSTACKUP 4, 0

; PEI-LABEL: name:            pipeline5{{$}}
; PEI: noVRegs:         true
; PEI: stackSize:       {{[4-9]|[1-5][0-9]}}
; PEI-LABEL: body:
; PEI-NOT: ADJCALLSTACK
; PEI-NOT: MOVL_{{.*}} %fixed-stack
; PEI: $r15 = frame-setup STS_L_PR $r15
; PEI: MOVL_load_disp $r15, {{[4-9]|[1-5][0-9]}}
; PEI: $r15 = ADDri $r15, -4
; PEI-NEXT: MOVL_store_reg {{.*}}, $r15
; PEI: BSR @pipeline5_callee
; PEI-NEXT: $r15 = ADDri $r15, 4
; PEI: $r15 = frame-destroy LDS_L_PR $r15

; DELAY-LABEL: name:            pipeline5{{$}}
; DELAY: noVRegs:         true
; DELAY-LABEL: body:
; DELAY-NOT: ADJCALLSTACK
; DELAY: $r15 = ADDri $r15, -4
; DELAY: BSR @pipeline5_callee, csr_sh
; DELAY-NEXT: NOP
; DELAY-NEXT: }
; DELAY-NEXT: $r15 = ADDri $r15, 4
