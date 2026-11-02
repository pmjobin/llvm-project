; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=ASM,NOOPT
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=ASM,OPT
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=ASM,NOOPT
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=ASM,OPT
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=ASM,NOOPT
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=ASM,OPT
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=ASM,NOOPT
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=ASM,OPT
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=MIR,LE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=MIR,BE

; Native unaligned expansion creates its byte ZEXTLOADs after the ordinary
; extension combines. Only the logical highest i32 byte can keep MOVB's sign
; bits: shifting by24 discards all of them. The same rule is valid at Os.
; Lower pieces, i16 results, volatile memory, O0 and optnone retain their
; contracts. A value used outside the shift must remain zero-extended.

; ASM-LABEL: unaligned_word:
; NOOPT-COUNT-4: extu.b
; OPT-COUNT-3: extu.b
; ASM-NOT: extu.b
; ASM: .Lfunc_end{{[0-9]+}}:
; MIR-LABEL: name: unaligned_word
; LE: [[HIGH:%[0-9]+]]:gpr = MOVB_load_{{(reg|frame)}} {{.*}} :: (load (s8) from %ir.p + 3)
; BE: [[HIGH:%[0-9]+]]:gpr = MOVB_load_{{(reg|frame)}} {{.*}} :: (load (s8) from %ir.p)
; MIR: [[SHIFT16:%[0-9]+]]:gpr = SHLL16 [[HIGH]]
; MIR: {{%[0-9]+}}:gpr = SHLL8 [[SHIFT16]]
define i32 @unaligned_word(ptr %p) {
  %v = load i32, ptr %p, align 1
  ret i32 %v
}

; ASM-LABEL: unaligned_word_os:
; NOOPT-COUNT-4: extu.b
; OPT-COUNT-3: extu.b
; ASM-NOT: extu.b
; ASM: .Lfunc_end{{[0-9]+}}:
; MIR-LABEL: name: unaligned_word_os
; LE: [[HIGH:%[0-9]+]]:gpr = MOVB_load_{{(reg|frame)}} {{.*}} :: (load (s8) from %ir.p + 3)
; BE: [[HIGH:%[0-9]+]]:gpr = MOVB_load_{{(reg|frame)}} {{.*}} :: (load (s8) from %ir.p)
; MIR: [[SHIFT16:%[0-9]+]]:gpr = SHLL16 [[HIGH]]
; MIR: {{%[0-9]+}}:gpr = SHLL8 [[SHIFT16]]
define i32 @unaligned_word_os(ptr %p) optsize {
  %v = load i32, ptr %p, align 1
  ret i32 %v
}

; ASM-LABEL: unaligned_halfword:
; ASM-COUNT-2: extu.b
; ASM-NOT: extu.b
; ASM: .Lfunc_end{{[0-9]+}}:
define i32 @unaligned_halfword(ptr %p) {
  %v = load i16, ptr %p, align 1
  %z = zext i16 %v to i32
  ret i32 %z
}

; ASM-LABEL: highest_byte:
; ASM-NOT: extu.b
; ASM: .Lfunc_end{{[0-9]+}}:
define i32 @highest_byte(ptr %p) {
  %v = load i8, ptr %p, align 1
  %z = zext i8 %v to i32
  %s = shl i32 %z, 24
  ret i32 %s
}

; ASM-LABEL: lower_byte:
; ASM-COUNT-1: extu.b
; ASM-NOT: extu.b
; ASM: .Lfunc_end{{[0-9]+}}:
define i32 @lower_byte(ptr %p) {
  %v = load i8, ptr %p, align 1
  %z = zext i8 %v to i32
  %s = shl i32 %z, 16
  ret i32 %s
}

; ASM-LABEL: multiple_use:
; ASM-COUNT-1: extu.b
; ASM-NOT: extu.b
; ASM: .Lfunc_end{{[0-9]+}}:
define i32 @multiple_use(ptr %p, ptr %out) {
  %v = load i8, ptr %p, align 1
  %z = zext i8 %v to i32
  store i32 %z, ptr %out, align 4
  %s = shl i32 %z, 24
  ret i32 %s
}

; ASM-LABEL: volatile_word:
; ASM-COUNT-4: extu.b
; ASM-NOT: extu.b
; ASM: .Lfunc_end{{[0-9]+}}:
define i32 @volatile_word(ptr %p) {
  %v = load volatile i32, ptr %p, align 1
  ret i32 %v
}

; ASM-LABEL: volatile_byte:
; ASM-NOT: extu.b
; ASM: .Lfunc_end{{[0-9]+}}:
define i32 @volatile_byte(ptr %p) {
  %v = load volatile i8, ptr %p, align 1
  %z = zext i8 %v to i32
  %s = shl i32 %z, 24
  ret i32 %s
}

; ASM-LABEL: atomic_byte:
; ASM-NOT: extu.b
; ASM: .long __atomic_load_1
; ASM: .Lfunc_end{{[0-9]+}}:
define i32 @atomic_byte(ptr %p) {
  %v = load atomic i8, ptr %p unordered, align 1
  %z = zext i8 %v to i32
  %s = shl i32 %z, 24
  ret i32 %s
}

; ASM-LABEL: aligned_halfword_word:
; ASM-NOT: extu.b
; ASM-COUNT-2: extu.w
; ASM: .Lfunc_end{{[0-9]+}}:
define i32 @aligned_halfword_word(ptr %p) {
  %v = load i32, ptr %p, align 2
  ret i32 %v
}

; ASM-LABEL: none_word:
; ASM-COUNT-4: extu.b
; ASM-NOT: extu.b
; ASM: .Lfunc_end{{[0-9]+}}:
define i32 @none_word(ptr %p) noinline optnone {
  %v = load i32, ptr %p, align 1
  ret i32 %v
}

; ASM-LABEL: after_optnone_word:
; NOOPT-COUNT-4: extu.b
; OPT-COUNT-3: extu.b
; ASM-NOT: extu.b
; ASM: .Lfunc_end{{[0-9]+}}:
; MIR-LABEL: name: after_optnone_word
; LE: [[HIGH:%[0-9]+]]:gpr = MOVB_load_{{(reg|frame)}} {{.*}} :: (load (s8) from %ir.p + 3)
; BE: [[HIGH:%[0-9]+]]:gpr = MOVB_load_{{(reg|frame)}} {{.*}} :: (load (s8) from %ir.p)
; MIR: [[SHIFT16:%[0-9]+]]:gpr = SHLL16 [[HIGH]]
; MIR: {{%[0-9]+}}:gpr = SHLL8 [[SHIFT16]]
define i32 @after_optnone_word(ptr %p) {
  %v = load i32, ptr %p, align 1
  ret i32 %v
}
