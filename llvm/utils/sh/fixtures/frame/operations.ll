; Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

target triple = "shle-unknown-linux-gnu"

declare i32 @frame_observe(ptr, i32, i32) nounwind

; Fixed input constraints force real register definitions. A clobber-only empty
; asm would request saves without changing the registers and could not detect a
; missing restore at runtime. The GNU caller checks all r8-r14 and fr12-fr15.
define i32 @frame_gpr(i32 %input) noinline nounwind "frame-pointer"="none" {
  call void asm sideeffect "", "{r8},{r9},{r10},{r11},{r12},{r13},{r14}"(i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0)
  %result = add i32 %input, 1
  ret i32 %result
}

define i32 @frame_fpr(i32 %input) noinline nounwind uwtable "frame-pointer"="none" {
  call void asm sideeffect "", "{fr12},{fr13},{fr14},{fr15}"(float 0.0, float 0.0, float 0.0, float 0.0)
  %result = add i32 %input, 2
  ret i32 %result
}

define i32 @frame_large(i32 %input) noinline nounwind uwtable "frame-pointer"="none" {
  %buffer = alloca [2048 x i8], align 4
  %last = getelementptr [2048 x i8], ptr %buffer, i32 0, i32 2047
  store volatile i8 17, ptr %buffer, align 4
  store volatile i8 29, ptr %last, align 1
  call void asm sideeffect "", "{r8},{r9},{r10},{r11},{r12},{r13},{r14},{fr12},{fr13},{fr14},{fr15}"(i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, float 0.0, float 0.0, float 0.0, float 0.0)
  %observed = call i32 @frame_observe(ptr %buffer, i32 2048, i32 4)
  %first = load volatile i8, ptr %buffer, align 4
  %tail = load volatile i8, ptr %last, align 1
  %a = zext i8 %first to i32
  %b = zext i8 %tail to i32
  %sum = add i32 %a, %b
  %total = add i32 %sum, %observed
  %result = add i32 %total, %input
  ret i32 %result
}

define i32 @frame_dynamic(i32 %count) noinline nounwind uwtable {
  %buffer = alloca i8, i32 %count, align 4
  %last.index = add i32 %count, -1
  %last = getelementptr i8, ptr %buffer, i32 %last.index
  store volatile i8 17, ptr %buffer, align 4
  store volatile i8 29, ptr %last, align 1
  ; r14 is the active frame pointer and is already changed by the prologue.
  call void asm sideeffect "", "{r8},{r9},{r10},{r11},{r12},{r13},{fr12},{fr13},{fr14},{fr15}"(i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, float 0.0, float 0.0, float 0.0, float 0.0)
  %observed = call i32 @frame_observe(ptr %buffer, i32 %count, i32 4)
  %first = load volatile i8, ptr %buffer, align 4
  %tail = load volatile i8, ptr %last, align 1
  %a = zext i8 %first to i32
  %b = zext i8 %tail to i32
  %sum = add i32 %a, %b
  %result = add i32 %sum, %observed
  ret i32 %result
}

define i32 @frame_realigned(i32 %count) noinline nounwind uwtable {
  %fixed = alloca [1024 x i8], align 32
  %dynamic = alloca i8, i32 %count, align 32
  %fixed.last = getelementptr [1024 x i8], ptr %fixed, i32 0, i32 1023
  %last.index = add i32 %count, -1
  %dynamic.last = getelementptr i8, ptr %dynamic, i32 %last.index
  store volatile i8 17, ptr %fixed, align 32
  store volatile i8 29, ptr %fixed.last, align 1
  store volatile i8 17, ptr %dynamic, align 32
  store volatile i8 29, ptr %dynamic.last, align 1
  ; r13/r14 are the active base/frame pointers and are changed by the prologue.
  call void asm sideeffect "", "{r8},{r9},{r10},{r11},{r12},{fr12},{fr13},{fr14},{fr15}"(i32 0, i32 0, i32 0, i32 0, i32 0, float 0.0, float 0.0, float 0.0, float 0.0)
  %fixed.observed = call i32 @frame_observe(ptr %fixed, i32 1024, i32 32)
  %dynamic.observed = call i32 @frame_observe(ptr %dynamic, i32 %count, i32 32)
  %fixed.first.value = load volatile i8, ptr %fixed, align 32
  %fixed.last.value = load volatile i8, ptr %fixed.last, align 1
  %dynamic.first.value = load volatile i8, ptr %dynamic, align 32
  %dynamic.last.value = load volatile i8, ptr %dynamic.last, align 1
  %a = zext i8 %fixed.first.value to i32
  %b = zext i8 %fixed.last.value to i32
  %c = zext i8 %dynamic.first.value to i32
  %d = zext i8 %dynamic.last.value to i32
  %ab = add i32 %a, %b
  %cd = add i32 %c, %d
  %sum = add i32 %ab, %cd
  %observed = add i32 %fixed.observed, %dynamic.observed
  %result = add i32 %sum, %observed
  ret i32 %result
}
