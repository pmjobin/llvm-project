; RUN: split-file %s %t
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 %t/volatile.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 %t/vector.ll -o /dev/null 2>&1 | FileCheck %s
; CHECK: error: {{.*}}SH integer memory operations require a power-of-two bit width
;--- volatile.ll
define i32 @volatile_i24(ptr %p) {
  %v = load volatile i24, ptr %p, align 1
  %r = zext i24 %v to i32
  ret i32 %r
}
;--- vector.ll
define void @vector_i24(ptr %p, ptr %q) {
  %v = load <2 x i24>, ptr %p, align 1
  store <2 x i24> %v, ptr %q, align 1
  ret void
}
