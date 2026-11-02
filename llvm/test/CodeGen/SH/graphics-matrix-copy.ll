; RUN: llc -mtriple=shle -mcpu=sh4 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh -mcpu=sh4 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh4 -enable-new-pm=1 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh2 -verify-machineinstrs %s -o /dev/null

declare void @llvm.memcpy.p0.p0.i32(ptr, ptr, i32, i1 immarg)
declare void @llvm.memmove.p0.p0.i32(ptr, ptr, i32, i1 immarg)

; Snapshot semantics require all eight source pairs before any destination
; store. Callee-save stack stores can precede these source reads.
; CHECK-LABEL: move:
; CHECK-COUNT-8: fmov.d @{{r[0-9]+}}+,dr
; CHECK-COUNT-8: fmov.d dr{{[0-9]+}},@-
; CHECK-NOT: .long memmove
; CHECK: .size move
define void @move(ptr %d, ptr %s) {
  call void @llvm.memmove.p0.p0.i32(ptr align 8 %d, ptr align 8 %s, i32 64, i1 false)
  ret void
}

; CHECK-LABEL: copy:
; CHECK: fmov.d
; CHECK-NOT: .long memcpy
; CHECK: .size copy
define void @copy(ptr %d, ptr %s) {
  call void @llvm.memcpy.p0.p0.i32(ptr align 8 %d, ptr align 8 %s, i32 64, i1 false)
  ret void
}

; CHECK-LABEL: no_fp:
; CHECK-NOT: fmov
; CHECK: .size no_fp
define void @no_fp(ptr %d, ptr %s) noimplicitfloat {
  call void @llvm.memmove.p0.p0.i32(ptr align 8 %d, ptr align 8 %s, i32 64, i1 false)
  ret void
}

; CHECK-LABEL: strict_copy:
; CHECK-NOT: fmov
; CHECK: .size strict_copy
define void @strict_copy(ptr %d, ptr %s) strictfp {
  call void @llvm.memmove.p0.p0.i32(ptr align 8 %d, ptr align 8 %s, i32 64, i1 false)
  ret void
}

; CHECK-LABEL: volatile_words:
; CHECK-NOT: fmov
; CHECK: .size volatile_words
define void @volatile_words(ptr %d, ptr %s) {
  call void @llvm.memcpy.p0.p0.i32(ptr align 8 %d, ptr align 8 %s, i32 60, i1 true)
  ret void
}
