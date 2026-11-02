; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s
; A landing pad may be entered before the outgoing stack area is popped.
; Use a stable local-frame base; ordinary functions still omit it.
declare i32 @__gxx_personality_v0(...)
declare void @may_throw(i32, i32, i32, i32, i32, i32)
declare void @use(ptr)
; CHECK-LABEL: catch_with_stack_args:
; CHECK: mov r15,r14
; CHECK: .cfi_def_cfa r14,
; CHECK: jsr
define void @catch_with_stack_args() personality ptr @__gxx_personality_v0 {
entry:
  %local = alloca [12 x i8], align 4
  call void @use(ptr %local)
  invoke void @may_throw(i32 1,i32 2,i32 3,i32 4,i32 5,i32 6) to label %done unwind label %catch
catch:
  %lp = landingpad {ptr,i32} catch ptr null
  call void @use(ptr %local)
  ret void
done:
  ret void
}
; CHECK-LABEL: ordinary:
; CHECK-NOT: mov r15,r14
; CHECK: rts
define i32 @ordinary(i32 %x) {
  %r = add i32 %x,1
  ret i32 %r
}
