; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llvm-dwarfdump --eh-frame %t.be.o 2>&1 | FileCheck %s --check-prefix=FRAME
; RUN: llvm-dwarfdump --eh-frame %t.be.o | %python %S/Inputs/sh-unwind-model.py --scenarios=leaf-no-frame,leaf-fixed,call-only,callee-saved-r8,fixed-twelve,several-callee-saved,byval-sret,outgoing-sixty,varargs-12,varargs-8,varargs-4 | FileCheck %s --check-prefix=MODEL
; RUN: llvm-readobj --relocations --unwind %t.be.o 2>&1 | FileCheck %s --check-prefix=OBJECT
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-dwarfdump --eh-frame %t.le.o 2>&1 | FileCheck %s --check-prefix=FRAME
; RUN: llvm-dwarfdump --eh-frame %t.le.o | %python %S/Inputs/sh-unwind-model.py --scenarios=leaf-no-frame,leaf-fixed,call-only,callee-saved-r8,fixed-twelve,several-callee-saved,byval-sret,outgoing-sixty,varargs-12,varargs-8,varargs-4 | FileCheck %s --check-prefix=MODEL
; RUN: llvm-readobj --relocations --unwind %t.le.o 2>&1 | FileCheck %s --check-prefix=OBJECT
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -force-dwarf-frame-section -filetype=obj %s -o %t.debug-frame.o
; RUN: llvm-dwarfdump --debug-frame %t.debug-frame.o | FileCheck %s --check-prefix=DEBUG-FRAME

declare i32 @callee()
declare i32 @five(i32, i32, i32, i32, i32)
declare i32 @seven(i32, i32, i32, i32, i32, i32, i32)
declare i32 @nineteen(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare void @escape(ptr)
declare void @llvm.va_start(ptr)
declare void @llvm.va_end(ptr)

%Pair = type { i32, i32 }

; ASM-LABEL: leaf_no_frame:
; ASM-NEXT: .cfi_startproc
; ASM-NEXT: rts
; ASM-NEXT: nop
define void @leaf_no_frame() nounwind uwtable(sync) {
  ret void
}

; ASM-LABEL: leaf_fixed:
; ASM-NEXT: .cfi_startproc
; ASM-NEXT: add #-4,r15
; ASM-NEXT: .cfi_def_cfa_offset 4
; ASM: add #4,r15
; ASM-NEXT: .cfi_def_cfa_offset 0
; ASM-NEXT: rts
define i32 @leaf_fixed(i32 %value) nounwind uwtable(async) {
  %slot = alloca i32, align 4
  store volatile i32 %value, ptr %slot, align 4
  %result = load volatile i32, ptr %slot, align 4
  ret i32 %result
}

; ASM-LABEL: call_only:
; ASM-NEXT: .cfi_startproc
; ASM-NEXT: sts.l pr,@-r15
; ASM-NEXT: .cfi_def_cfa_offset 4
; ASM-NEXT: .cfi_offset pr, -4
; ASM: lds.l @r15+,pr
; ASM-NEXT: .cfi_restore pr
; ASM-NEXT: .cfi_def_cfa_offset 0
; ASM-NEXT: rts
define i32 @call_only() uwtable {
  %result = call i32 @callee()
  ret i32 %result
}

; ASM-LABEL: save_r8:
; ASM: sts.l pr,@-r15
; ASM: add #-4,r15
; ASM-NEXT: .cfi_def_cfa_offset 8
; ASM-NEXT: mov.l r8,@r15
; ASM-NEXT: .cfi_offset r8, -8
; ASM: mov.l @r15,r8
; ASM-NEXT: .cfi_restore r8
; ASM: lds.l @r15+,pr
; ASM-NEXT: .cfi_restore pr
define i32 @save_r8(i32 %value) uwtable {
  %saved = add i32 %value, 1
  %called = call i32 @callee()
  %result = add i32 %called, %saved
  ret i32 %result
}

; ASM-LABEL: fixed_twelve:
; ASM: sts.l pr,@-r15
; ASM-NEXT: .cfi_def_cfa_offset 4
; ASM: add #-8,r15
; ASM-NEXT: .cfi_def_cfa_offset 12
; ASM: add #8,r15
; ASM-NEXT: .cfi_def_cfa_offset 4
define i32 @fixed_twelve(i32 %value) uwtable {
  %slot = alloca [2 x i32], align 4
  store volatile i32 %value, ptr %slot, align 4
  %called = call i32 @callee()
  %saved = load volatile i32, ptr %slot, align 4
  %result = add i32 %called, %saved
  ret i32 %result
}

; ASM-LABEL: several_callee_saved:
; ASM: .cfi_offset r8,
; ASM: .cfi_offset r9,
; ASM: .cfi_offset r10,
; ASM: .cfi_offset r11,
; ASM: .cfi_offset r12,
; ASM: .cfi_offset r13,
; ASM: .cfi_offset r14,
; ASM: .cfi_restore r14
; ASM: .cfi_restore r8
define i32 @several_callee_saved(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f, i32 %g) uwtable {
  %called = call i32 @callee()
  %used = call i32 @seven(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f, i32 %g)
  %result = add i32 %used, %called
  ret i32 %result
}

; ASM-LABEL: byval_sret:
; ASM: .cfi_offset pr, -{{[0-9]+}}
; ASM: jsr
; ASM-NEXT: nop
; ASM: .cfi_restore pr
define void @byval_sret(ptr sret(%Pair) align 4 %out, ptr byval(%Pair) align 4 %in) uwtable {
  call void @escape(ptr %in)
  %first = load i32, ptr %in, align 4
  %second_ptr = getelementptr inbounds %Pair, ptr %in, i32 0, i32 1
  %second = load i32, ptr %second_ptr, align 4
  store i32 %first, ptr %out, align 4
  %out_second = getelementptr inbounds %Pair, ptr %out, i32 0, i32 1
  store i32 %second, ptr %out_second, align 4
  ret void
}

; ASM-LABEL: outgoing_sixty:
; ASM: add #-60,r15
; ASM-NEXT: .cfi_adjust_cfa_offset 60
; ASM: jsr
; ASM-NEXT: nop
; ASM-NEXT: add #60,r15
; ASM-NEXT: .cfi_adjust_cfa_offset -60
define i32 @outgoing_sixty(i32 %value) uwtable {
  %result = call i32 @nineteen(i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value, i32 %value)
  ret i32 %result
}

; ASM-LABEL: varargs_save12:
; ASM-NEXT: .cfi_startproc
; ASM-NEXT: add #-12,r15
; ASM-NEXT: .cfi_def_cfa_offset 12
; ASM-NEXT: sts.l pr,@-r15
; ASM-NEXT: .cfi_def_cfa_offset 16
; ASM-NEXT: .cfi_offset pr, -16
; ASM: add #-4,r15
; ASM-NEXT: .cfi_def_cfa_offset 20
; ASM: add #-4,r15
; ASM-NEXT: .cfi_adjust_cfa_offset 4
; ASM: jsr
; ASM-NEXT: nop
; ASM-NEXT: add #4,r15
; ASM-NEXT: .cfi_adjust_cfa_offset -4
; ASM: lds.l @r15+,pr
; ASM-NEXT: .cfi_restore pr
; ASM-NEXT: .cfi_def_cfa_offset 12
; ASM-NEXT: add #12,r15
; ASM-NEXT: .cfi_def_cfa_offset 0
define i32 @varargs_save12(i32 %fixed, ...) uwtable {
  %ap = alloca ptr, align 4
  call void @llvm.va_start(ptr %ap)
  %value = va_arg ptr %ap, i32
  %result = call i32 @five(i32 %fixed, i32 %value, i32 3, i32 4, i32 5)
  call void @llvm.va_end(ptr %ap)
  ret i32 %result
}

; ASM-LABEL: varargs_save8:
; ASM-NEXT: .cfi_startproc
; ASM-NEXT: add #-8,r15
; ASM-NEXT: .cfi_def_cfa_offset 8
; ASM-NEXT: sts.l pr,@-r15
; ASM-NEXT: .cfi_def_cfa_offset 12
; ASM-NEXT: .cfi_offset pr, -12
; ASM: lds.l @r15+,pr
; ASM-NEXT: .cfi_restore pr
; ASM-NEXT: .cfi_def_cfa_offset 8
; ASM-NEXT: add #8,r15
; ASM-NEXT: .cfi_def_cfa_offset 0
define i32 @varargs_save8(i32 %a, i32 %b, ...) uwtable {
  %ap = alloca ptr, align 4
  call void @llvm.va_start(ptr %ap)
  %value = va_arg ptr %ap, i32
  %result = call i32 @five(i32 %a, i32 %b, i32 %value, i32 4, i32 5)
  call void @llvm.va_end(ptr %ap)
  ret i32 %result
}

; ASM-LABEL: varargs_save4:
; ASM-NEXT: .cfi_startproc
; ASM-NEXT: add #-4,r15
; ASM-NEXT: .cfi_def_cfa_offset 4
; ASM-NEXT: sts.l pr,@-r15
; ASM-NEXT: .cfi_def_cfa_offset 8
; ASM-NEXT: .cfi_offset pr, -8
; ASM: lds.l @r15+,pr
; ASM-NEXT: .cfi_restore pr
; ASM-NEXT: .cfi_def_cfa_offset 4
; ASM-NEXT: add #4,r15
; ASM-NEXT: .cfi_def_cfa_offset 0
define i32 @varargs_save4(i32 %a, i32 %b, i32 %c, ...) uwtable {
  %ap = alloca ptr, align 4
  call void @llvm.va_start(ptr %ap)
  %value = va_arg ptr %ap, i32
  %result = call i32 @five(i32 %a, i32 %b, i32 %c, i32 %value, i32 5)
  call void @llvm.va_end(ptr %ap)
  ret i32 %result
}

; FRAME: .eh_frame contents:
; FRAME: Version: 1
; FRAME: Augmentation: "zR"
; FRAME: Code alignment factor: 2
; FRAME: Data alignment factor: -4
; FRAME: Return address column: 17
; FRAME: DW_CFA_def_cfa: R15 +0
; FRAME-NOT: warning:
; FRAME: FDE cie=
; FRAME: 0x0: CFA=R15
; FRAME: FDE cie=
; FRAME: CFA=R15+4
; FRAME: FDE cie=
; FRAME: CFA=R15+4: PR=[CFA-4]
; FRAME: FDE cie=
; FRAME: CFA=R15+8: R8=[CFA-8], PR=[CFA-4]
; FRAME: FDE cie=
; FRAME: CFA=R15+64: PR=[CFA-4]
; FRAME: FDE cie=
; FRAME: CFA=R15+24: PR=[CFA-16]
; FRAME: FDE cie=
; FRAME: CFA=R15+20: PR=[CFA-12]
; FRAME: FDE cie=
; FRAME: CFA=R15+16: PR=[CFA-8]

; OBJECT: .rela.eh_frame
; OBJECT-COUNT-11: R_SH_DIR32
; OBJECT-NOT: R_SH_REL32
; OBJECT-NOT: warning:
; OBJECT: return_address_register: 17
; OBJECT: DW_CFA_def_cfa: reg15 +0

; DEBUG-FRAME: .debug_frame contents:
; DEBUG-FRAME: Version: 4
; DEBUG-FRAME: Address size: 4
; DEBUG-FRAME: Code alignment factor: 2
; DEBUG-FRAME: Data alignment factor: -4
; DEBUG-FRAME: Return address column: 17
; DEBUG-FRAME: DW_CFA_def_cfa: R15 +0

; MODEL: scenarios: 11
; MODEL: rows: {{[0-9]+}}
; MODEL: recoveries: {{[0-9]+}}
; MODEL: caller CFA, PR, r8-r14, and SP recovered
