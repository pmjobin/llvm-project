; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -relocation-model=static -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -relocation-model=static -O3 -force-new-pm-codegen -verify-machineinstrs < %s | FileCheck %s
;
; The caller supplies all four words in r4-r7. A local aligned copy must be
; initialized before any IR load from the byval parameter. Generic stack PSV
; metadata wrongly claimed that initialization could not alias the IR pointer,
; allowing post-RA scheduling to read uninitialized storage at O3.
%S16 = type { i32, i32, i32, i32 }
@result = external global [256 x [8 x i32]], align 4

define void @overaligned_byval(ptr readonly byval(%S16) align 16 %v, i32 %tail) nounwind "target-abi"="gnu-hard-float" {
; CHECK-LABEL: overaligned_byval:
; CHECK: mov.l r4,@[[BASE:r[0-9]+]]
; CHECK: mov.l @[[BASE]],[[FIRST:r[0-9]+]]
; CHECK: mov.l [[FIRST]],@{{r[0-9]+}}
  %first = load i32, ptr %v, align 16
  store i32 %first, ptr getelementptr (i8, ptr @result, i32 2496), align 4
  %next = getelementptr i8, ptr %v, i32 4
  %second = load i32, ptr %next, align 4
  store i32 %second, ptr getelementptr (i8, ptr @result, i32 2500), align 4
  store i32 %tail, ptr getelementptr (i8, ptr @result, i32 2516), align 4
  ret void
}
