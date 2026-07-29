; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -stop-after=virtregrewriter < %s | FileCheck %s --check-prefix=RA
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -stop-after=sh-literal-pool-range-check < %s | FileCheck %s --check-prefix=FINAL
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -enable-new-pm=1 -filetype=null < %s

@global = global i32 1, align 4
declare i32 @external_fn(i32)

define i32 @pipeline(i32 %value) {
	%loaded = load i32, ptr @global, align 4
	%called = call i32 @external_fn(i32 %value)
	%result = add i32 %loaded, %called
	ret i32 %result
}

; ISEL-LABEL: name:            pipeline
; ISEL: constants:
; ISEL: value:           global
; ISEL: isTargetSpecific: true
; ISEL: value:           external_fn
; ISEL: isTargetSpecific: true
; ISEL-NOT: global-address
; ISEL: {{%[0-9]+}}:gpr = MOVL_load_pc %const.0 :: (dereferenceable invariant load (s32) from constant-pool)
; ISEL: {{%[0-9]+}}:gpr = MOVL_load_pc %const.1 :: (dereferenceable invariant load (s32) from constant-pool)
; ISEL: JSR {{.*}}, csr_sh, implicit-def dead $pr

; RA-LABEL: name:            pipeline
; RA: noVRegs:         true
; RA: registers:       []
; RA-NOT: %{{[0-9]+}}
; RA: MOVL_load_pc %const.0
; RA: MOVL_load_pc %const.1
; RA: JSR {{.*}}, csr_sh

; FINAL-LABEL: name:            pipeline
; FINAL: noVRegs:         true
; FINAL: MOVL_load_pc %const.0
; FINAL: JSR {{.*}} {
; FINAL-NEXT: NOP
; FINAL-NEXT: }
; FINAL: RTS {{.*}} {
; FINAL-NEXT: NOP
; FINAL-NEXT: }
