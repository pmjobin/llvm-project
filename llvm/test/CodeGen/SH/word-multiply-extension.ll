; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s

; A full i32 product consumes bits 0..15 of each word operand. A byte load
; sign-extends on SH, so the multiply must use the byte extension's result,
; not the original load. Capture the use-def edge, not just an EXTUB spelling.
; stripWordExtension may discard an extension only when its source is i16;
; the upper half is ignored, but all bits of the low word remain observable.
define i32 @unsigned_byte_constant(ptr %p) optsize {
; CHECK-LABEL: name: unsigned_byte_constant
; CHECK: [[LOAD:%[0-9]+]]:gpr = MOVB_load_reg
; CHECK: [[BYTE:%[0-9]+]]:gpr = EXTUB {{(killed )?}}[[LOAD]]
; CHECK: MULU_W {{(killed )?}}[[BYTE]],
  %x = load i8, ptr %p, align 1
  %wide = zext i8 %x to i32
  %result = mul i32 %wide, 3
  ret i32 %result
}

define i32 @unsigned_bytes_dynamic(ptr %p, ptr %q) optsize {
; CHECK-LABEL: name: unsigned_bytes_dynamic
; CHECK-DAG: [[LA:%[0-9]+]]:gpr = MOVB_load_reg {{.*}} :: (load (s8) from %ir.p)
; CHECK-DAG: [[A:%[0-9]+]]:gpr = EXTUB {{(killed )?}}[[LA]]
; CHECK-DAG: [[LB:%[0-9]+]]:gpr = MOVB_load_reg {{.*}} :: (load (s8) from %ir.q)
; CHECK-DAG: [[B:%[0-9]+]]:gpr = EXTUB {{(killed )?}}[[LB]]
; CHECK: MULU_W {{(killed )?}}[[A]], {{(killed )?}}[[B]]
  %x = load i8, ptr %p, align 1
  %y = load i8, ptr %q, align 1
  %a = zext i8 %x to i32
  %b = zext i8 %y to i32
  %result = mul i32 %a, %b
  ret i32 %result
}

define i32 @unsigned_byte_mask(i32 %x) optsize {
; CHECK-LABEL: name: unsigned_byte_mask
; CHECK: [[BYTE:%[0-9]+]]:gpr = EXTUB
; CHECK: MULU_W {{(killed )?}}[[BYTE]],
  %byte = and i32 %x, 255
  %result = mul i32 %byte, 5
  ret i32 %result
}

define i32 @unsigned_byte_cast(i32 %x, i32 %y) optsize {
; CHECK-LABEL: name: unsigned_byte_cast
; CHECK: [[WORD:%[0-9]+]]:gpr = COPY $r5
; CHECK: [[BYTE:%[0-9]+]]:gpr = EXTUB
; CHECK: MULU_W {{(killed )?}}[[BYTE]], {{(killed )?}}[[WORD]]
  %byte = trunc i32 %x to i8
  %a = zext i8 %byte to i32
  %b = and i32 %y, 65535
  %result = mul i32 %a, %b
  ret i32 %result
}

; Keep signed byte semantics. Its sign-extending load already has the right
; low word. A register truncation still requires a sign extension from i8.
define i32 @signed_byte_load(ptr %p) optsize {
; CHECK-LABEL: name: signed_byte_load
; CHECK-NOT: EXTUB
; CHECK: [[BYTE:%[0-9]+]]:gpr = MOVB_load_reg
; CHECK-NOT: EXTUB
; CHECK: MULS_W {{(killed )?}}[[BYTE]],
  %byte = load i8, ptr %p, align 1
  %wide = sext i8 %byte to i32
  %result = mul i32 %wide, 3
  ret i32 %result
}

define i32 @signed_byte_cast(i32 %x) optsize {
; CHECK-LABEL: name: signed_byte_cast
; CHECK: [[BYTE:%[0-9]+]]:gpr = EXTSB
; CHECK: MULS_W {{(killed )?}}[[BYTE]],
  %byte = trunc i32 %x to i8
  %wide = sext i8 %byte to i32
  %result = mul i32 %wide, 7
  ret i32 %result
}

; A word load's sign extension changes only bits 16..31, which a word
; multiply ignores. Do not lose the safe narrowing of a full word product.
define i32 @unsigned_word_load(ptr %p) optsize {
; CHECK-LABEL: name: unsigned_word_load
; CHECK: [[WORD:%[0-9]+]]:gpr = MOVW_load_reg
; CHECK: MULU_W {{(killed )?}}[[WORD]],
  %word = load i16, ptr %p, align 2
  %wide = zext i16 %word to i32
  %result = mul i32 %wide, 257
  ret i32 %result
}

define i32 @unsigned_words_dynamic(ptr %p, ptr %q) optsize {
; CHECK-LABEL: name: unsigned_words_dynamic
; CHECK-DAG: [[A:%[0-9]+]]:gpr = MOVW_load_reg {{.*}} :: (load (s16) from %ir.p)
; CHECK-DAG: [[B:%[0-9]+]]:gpr = MOVW_load_reg {{.*}} :: (load (s16) from %ir.q)
; CHECK: MULU_W {{(killed )?}}[[A]], {{(killed )?}}[[B]]
  %x = load i16, ptr %p, align 2
  %y = load i16, ptr %q, align 2
  %a = zext i16 %x to i32
  %b = zext i16 %y to i32
  %result = mul i32 %a, %b
  ret i32 %result
}

define i32 @signed_word_load(ptr %p) optsize {
; CHECK-LABEL: name: signed_word_load
; CHECK: [[WORD:%[0-9]+]]:gpr = MOVW_load_reg
; CHECK: MULS_W {{(killed )?}}[[WORD]],
  %word = load i16, ptr %p, align 2
  %wide = sext i16 %word to i32
  %result = mul i32 %wide, 257
  ret i32 %result
}

define i32 @mixed_bytes(ptr %p, ptr %q) optsize {
; CHECK-LABEL: name: mixed_bytes
; CHECK: [[SIGNED:%[0-9]+]]:gpr = MOVB_load_reg
; CHECK: [[LOAD:%[0-9]+]]:gpr = MOVB_load_reg
; CHECK: [[UNSIGNED:%[0-9]+]]:gpr = EXTUB {{(killed )?}}[[LOAD]]
; CHECK: MULS_W {{(killed )?}}[[SIGNED]], {{(killed )?}}[[UNSIGNED]]
  %x = load i8, ptr %p, align 1
  %y = load i8, ptr %q, align 1
  %a = sext i8 %x to i32
  %b = zext i8 %y to i32
  %result = mul i32 %a, %b
  ret i32 %result
}

define i32 @full_width(i32 %x, i32 %y) optsize {
; CHECK-LABEL: name: full_width
; CHECK-NOT: MULU_W
; CHECK-NOT: MULS_W
; CHECK: MUL_L
  %result = mul i32 %x, %y
  ret i32 %result
}
