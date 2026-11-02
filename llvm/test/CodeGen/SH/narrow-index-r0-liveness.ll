; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s -o /dev/null
; A displacement load feeding an indexed address must expose its r0 clobber
; before scheduling. A virtual singleton R0Reg result can be placed inside
; another physical r0 live range, where register allocation cannot spill it.
@decode_table = external global [256 x i8]
define i32 @lookup_offset_byte(ptr %input) {
  %p = getelementptr i8, ptr %input, i32 1
  %v = load i8, ptr %p, align 1
  %i = zext i8 %v to i32
  %q = getelementptr i8, ptr @decode_table, i32 %i
  %r = load i8, ptr %q, align 1
  %result = zext i8 %r to i32
  %p2 = getelementptr i8, ptr %input, i32 2
  %v2 = load i8, ptr %p2, align 1
  %i2 = zext i8 %v2 to i32
  %q2 = getelementptr i8, ptr @decode_table, i32 %i2
  %r2 = load i8, ptr %q2, align 1
  %result2 = zext i8 %r2 to i32
  %sum = add i32 %result, %result2
  ret i32 %sum
}
; CHECK-LABEL: name:            lookup_offset_byte
; CHECK: %{{[0-9]+}}:gpr = MOVB_load_frame %{{[0-9]+}}, 1, implicit-def dead $r0
; CHECK: $r0 = COPY
; CHECK: MOVB_load_indexed {{.*}}, implicit $r0
