; RUN: llc -enable-new-pm -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefixes=ASM,FIXUP
; RUN: llc -enable-new-pm -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefixes=ASM,FIXUP
; RUN: llc -enable-new-pm -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefixes=ASM,FIXUP
; RUN: llc -enable-new-pm -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefixes=ASM,FIXUP
; RUN: llc -enable-new-pm=0 -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefixes=ASM,FIXUP
; RUN: llc -enable-new-pm=0 -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefixes=ASM,FIXUP
; RUN: llc -enable-new-pm -disable-cfi-fixup -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=ASM --implicit-check-not=.cfi_remember_state --implicit-check-not=.cfi_restore_state
; RUN: llc -enable-new-pm -disable-cfi-fixup -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=ASM --implicit-check-not=.cfi_remember_state --implicit-check-not=.cfi_restore_state
; RUN: llc -enable-new-pm -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t.be.o0.o
; RUN: llvm-dwarfdump --eh-frame %t.be.o0.o | FileCheck %s --check-prefix=FRAME
; RUN: llc -enable-new-pm -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t.le.o0.o
; RUN: llvm-dwarfdump --eh-frame %t.le.o0.o | FileCheck %s --check-prefix=FRAME
; RUN: llc -enable-new-pm -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t.be.o2.o
; RUN: llvm-dwarfdump --eh-frame %t.be.o2.o | FileCheck %s --check-prefix=FRAME
; RUN: llc -enable-new-pm -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t.le.o2.o
; RUN: llvm-dwarfdump --eh-frame %t.le.o2.o | FileCheck %s --check-prefix=FRAME

; The landing pad executes with the prologue's frame, although it is laid out
; after an epilogue. Its unwinding call must recover the caller's saved PR and
; r14, not inherit the preceding return's CFA=SP state. Literal island splitting
; adds another block on that path, which must retain the same unwind state.
; Exercise the complete production pipeline, including O0 and the disable flag.

declare i32 @__gxx_personality_v0(...)
declare void @may_throw()
declare void @rethrow(ptr) noreturn

; ASM-LABEL: cleanup_after_return:
; ASM: sts.l pr,@-r15
; ASM-NEXT: .cfi_def_cfa_offset 4
; ASM-NEXT: .cfi_offset pr, -4
; ASM-NEXT: add #-4,r15
; ASM-NEXT: .cfi_def_cfa_offset 8
; ASM-NEXT: mov.l r14,@r15
; ASM-NEXT: .cfi_offset r14, -8
; ASM-NEXT: mov r15,r14
; ASM-NEXT: .cfi_def_cfa r14, 8
; FIXUP-NEXT: .cfi_remember_state
; ASM: mov r14,r15
; ASM-NEXT: .cfi_def_cfa r15, 8
; ASM-NEXT: mov.l @r15,r14
; ASM-NEXT: .cfi_restore r14
; ASM-NEXT: add #4,r15
; ASM-NEXT: .cfi_def_cfa_offset 4
; ASM-NEXT: lds.l @r15+,pr
; ASM-NEXT: .cfi_restore pr
; ASM-NEXT: .cfi_def_cfa_offset 0
; ASM-NEXT: rts
; ASM-NEXT: nop
; ASM-NEXT: .LBB{{[0-9]+_[0-9]+}}:
; FIXUP-NEXT: .cfi_restore_state
; FIXUP-NEXT: .cfi_remember_state
; ASM-NEXT: .Ltmp{{[0-9]+}}:
; ASM-NEXT: mov.l {{.*}},r0
; ASM-NEXT: bra [[TAIL:.LBB[0-9]+_[0-9]+]]
; ASM-NEXT: nop
; ASM: [[TAIL]]:
; FIXUP-NEXT: .cfi_restore_state
; ASM-NEXT: jsr @r0
; ASM-NEXT: nop
; ASM: .cfi_endproc
define void @cleanup_after_return() uwtable personality ptr @__gxx_personality_v0 {
entry:
  invoke void @may_throw() to label %normal unwind label %lpad

normal:
  ret void

lpad:
  %lp = landingpad { ptr, i32 } cleanup
  %exception = extractvalue { ptr, i32 } %lp, 0
  call void @rethrow(ptr %exception)
  unreachable
}

; Check the object unwind rows, including the CFA register and both saved
; registers at the landing pad and after the literal island. EPI differs at O0
; because the normal invoke destination still has an explicit branch.
; FRAME: FDE cie=
; FRAME: 0x0: CFA=R15{{$}}
; FRAME-NEXT: 0x2: CFA=R15+4: PR=[CFA-4]
; FRAME-NEXT: 0x4: CFA=R15+8: PR=[CFA-4]
; FRAME-NEXT: 0x6: CFA=R15+8: R14=[CFA-8], PR=[CFA-4]
; FRAME-NEXT: 0x8: CFA=R14+8: R14=[CFA-8], PR=[CFA-4]
; FRAME-NEXT: 0x[[#%x,EPI:]]: CFA=R15+8: R14=[CFA-8], PR=[CFA-4]
; FRAME-NEXT: 0x[[#EPI+2]]: CFA=R15+8: PR=[CFA-4]
; FRAME-NEXT: 0x[[#EPI+4]]: CFA=R15+4: PR=[CFA-4]
; FRAME-NEXT: 0x[[#EPI+6]]: CFA=R15{{$}}
; FRAME-NEXT: 0x[[#EPI+10]]: CFA=R14+8: R14=[CFA-8], PR=[CFA-4]
; FRAME-NEXT: 0x[[#EPI+24]]: CFA=R14+8: R14=[CFA-8], PR=[CFA-4]
