; RUN: split-file %s %t
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=finalize-isel %t/positive.ll -o - | FileCheck %s --check-prefixes=COMMON,LE --implicit-check-not=SWAP_W --implicit-check-not=XORrr
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=finalize-isel %t/positive.ll -o - | FileCheck %s --check-prefixes=COMMON,BE --implicit-check-not=SWAP_W --implicit-check-not=XORrr
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=finalize-isel %t/negative.ll -o - | FileCheck %s --check-prefix=NEG
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=finalize-isel %t/negative.ll -o - | FileCheck %s --check-prefix=NEG
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs %t/negative.ll -o - | FileCheck %s --check-prefix=ATOMIC
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs %t/negative.ll -o - | FileCheck %s --check-prefix=ATOMIC
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %t/positive.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %t/positive.ll -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh1 -O2 -verify-machineinstrs %t/positive.ll -o /dev/null
; RUN: llc -mtriple=sh -mcpu=sh1 -O2 -verify-machineinstrs %t/positive.ll -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh2a -O2 -verify-machineinstrs %t/positive.ll -o /dev/null
; RUN: llc -mtriple=sh -mcpu=sh2a -O2 -verify-machineinstrs %t/positive.ll -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh3 -O2 -verify-machineinstrs %t/positive.ll -o /dev/null
; RUN: llc -mtriple=sh -mcpu=sh3 -O2 -verify-machineinstrs %t/positive.ll -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh4a -O2 -verify-machineinstrs %t/positive.ll -o /dev/null
; RUN: llc -mtriple=sh -mcpu=sh4a -O2 -verify-machineinstrs %t/positive.ll -o /dev/null
;
; No speculative wider memory access or assemble-then-swap is needed. Check
; the top byte's shift for each endian and retain all four byte memory effects.
; COMMON-LABEL: name: read_reverse
; COMMON: body:
; LE-DAG: [[TOP:%[0-9]+]]:gpr = MOVB_load{{.*}} :: (load (s8) from %ir.p)
; BE-DAG: [[TOP:%[0-9]+]]:gpr = MOVB_load{{.*}} :: (load (s8) from %ir.p + 3)
; COMMON-DAG: [[TOP16:%[0-9]+]]:gpr = SHLL16 [[TOP]]
; COMMON-DAG: {{%[0-9]+}}:gpr = SHLL8 [[TOP16]]
; LE-DAG: MOVB_load{{.*}} :: (load (s8) from %ir.p + 1)
; LE-DAG: MOVB_load{{.*}} :: (load (s8) from %ir.p + 2)
; LE-DAG: MOVB_load{{.*}} :: (load (s8) from %ir.p + 3)
; BE-DAG: MOVB_load{{.*}} :: (load (s8) from %ir.p)
; BE-DAG: MOVB_load{{.*}} :: (load (s8) from %ir.p + 1)
; BE-DAG: MOVB_load{{.*}} :: (load (s8) from %ir.p + 2)
; COMMON: RTS
;
; Capture the aliased byte before overwriting it. Loads of other bytes may
; legally move past the byte store because those memory locations are disjoint.
; COMMON-LABEL: name: reverse_then_store
; COMMON: body:
; COMMON: MOVB_load{{.*}} :: (load (s8) from %ir.p)
; COMMON: MOVB_store_reg
; COMMON: RTS
;
; An unknown clobber can modify every byte: all four loads must precede it.
; COMMON-LABEL: name: reverse_then_call
; COMMON: body:
; COMMON-COUNT-4: MOVB_load
; COMMON: JSR
; COMMON: RTS
;
; Volatile accesses keep their existing ordered expansion. A second raw-value
; consumer must not be given a second load, and aligned loads retain their
; existing word instruction and byte-swap lowering.
; NEG-LABEL: name: volatile_reverse
; NEG: body:
; NEG-COUNT-4: MOVB_load
; NEG: SWAP_W
; NEG: RTS
; NEG-LABEL: name: two_value_uses
; NEG: body:
; NEG-COUNT-4: MOVB_load
; NEG-DAG: SWAP_W
; NEG-DAG: MOVL_store_reg
; NEG: RTS
; NEG-LABEL: name: aligned_reverse
; NEG: body:
; NEG: MOVL_load_reg
; NEG: SWAP_W
; NEG: RTS
; ATOMIC-LABEL: atomic_reverse:
; ATOMIC: .long{{[ \t]+}}__atomic_load
;
;--- positive.ll
declare i32 @llvm.bswap.i32(i32)

define i32 @read_reverse(ptr %p) {
  %value = load i32, ptr %p, align 1
  %reverse = call i32 @llvm.bswap.i32(i32 %value)
  ret i32 %reverse
}

define i32 @reverse_then_store(ptr %p, i8 %new) {
  %value = load i32, ptr %p, align 1
  %reverse = call i32 @llvm.bswap.i32(i32 %value)
  store volatile i8 %new, ptr %p, align 1
  ret i32 %reverse
}

declare void @clobber(ptr)
define i32 @reverse_then_call(ptr %p) {
  %value = load i32, ptr %p, align 1
  %reverse = call i32 @llvm.bswap.i32(i32 %value)
  call void @clobber(ptr %p)
  ret i32 %reverse
}

;--- negative.ll
declare i32 @llvm.bswap.i32(i32)

define i32 @volatile_reverse(ptr %p) {
  %value = load volatile i32, ptr %p, align 1
  %reverse = call i32 @llvm.bswap.i32(i32 %value)
  ret i32 %reverse
}

define i32 @two_value_uses(ptr %p, ptr %out) {
  %value = load i32, ptr %p, align 1
  %reverse = call i32 @llvm.bswap.i32(i32 %value)
  store volatile i32 %value, ptr %out, align 4
  ret i32 %reverse
}

define i32 @aligned_reverse(ptr %p) {
  %value = load i32, ptr %p, align 4
  %reverse = call i32 @llvm.bswap.i32(i32 %value)
  ret i32 %reverse
}

define i32 @atomic_reverse(ptr %p) {
  %value = load atomic i32, ptr %p acquire, align 1
  %reverse = call i32 @llvm.bswap.i32(i32 %value)
  ret i32 %reverse
}
