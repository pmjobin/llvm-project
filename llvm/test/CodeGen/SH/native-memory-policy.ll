; RUN: llc -mtriple=shle -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh3 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh -mcpu=sh3 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh4 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh -mcpu=sh4 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh4 -enable-new-pm=1 -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle -mcpu=sh4 -O0 -verify-machineinstrs -print-after=sh-prepare-ir -filter-print-funcs=copy56_align1 %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=IR

; The IR pass retains the intrinsic. The single expansion decision counts
; legal stores, including differing source/destination alignment and optsize.
; IR: call void @llvm.memcpy.p0.p0.i32(ptr align 1 %d, ptr align 1 %s, i32 56, i1 false)

declare void @llvm.memcpy.p0.p0.i32(ptr, ptr, i32, i1 immarg)
declare void @llvm.memmove.p0.p0.i32(ptr, ptr, i32, i1 immarg)
declare void @llvm.memset.p0.i32(ptr, i8, i32, i1 immarg)

; CHECK-LABEL: copy56_align1:
; CHECK: .long memcpy
define void @copy56_align1(ptr %d, ptr %s) {
  call void @llvm.memcpy.p0.p0.i32(ptr align 1 %d, ptr align 1 %s, i32 56, i1 false)
  ret void
}

; CHECK-LABEL: copy8_align1:
; CHECK-NOT: .long memcpy
; CHECK: .size copy8_align1
define void @copy8_align1(ptr %d, ptr %s) {
  call void @llvm.memcpy.p0.p0.i32(ptr align 1 %d, ptr align 1 %s, i32 8, i1 false)
  ret void
}

; CHECK-LABEL: copy9_align1:
; CHECK: .long memcpy
define void @copy9_align1(ptr %d, ptr %s) {
  call void @llvm.memcpy.p0.p0.i32(ptr align 1 %d, ptr align 1 %s, i32 9, i1 false)
  ret void
}

; CHECK-LABEL: copy32_align4:
; CHECK-NOT: .long memcpy
; CHECK: .size copy32_align4
define void @copy32_align4(ptr %d, ptr %s) {
  call void @llvm.memcpy.p0.p0.i32(ptr align 4 %d, ptr align 4 %s, i32 32, i1 false)
  ret void
}

; CHECK-LABEL: copy32_mixed:
; CHECK: .long memcpy
define void @copy32_mixed(ptr %d, ptr %s) {
  call void @llvm.memcpy.p0.p0.i32(ptr align 4 %d, ptr align 1 %s, i32 32, i1 false)
  ret void
}

; CHECK-LABEL: copy16_size:
; CHECK-NOT: .long memcpy
; CHECK: .size copy16_size
define void @copy16_size(ptr %d, ptr %s) optsize {
  call void @llvm.memcpy.p0.p0.i32(ptr align 4 %d, ptr align 4 %s, i32 16, i1 false)
  ret void
}

; CHECK-LABEL: copy17_size:
; CHECK: .long memcpy
define void @copy17_size(ptr %d, ptr %s) optsize {
  call void @llvm.memcpy.p0.p0.i32(ptr align 4 %d, ptr align 4 %s, i32 17, i1 false)
  ret void
}

; CHECK-LABEL: move17_align4:
; CHECK-NOT: .long memmove
; CHECK: .size move17_align4
define void @move17_align4(ptr %d, ptr %s) {
  call void @llvm.memmove.p0.p0.i32(ptr align 4 %d, ptr align 4 %s, i32 17, i1 false)
  ret void
}

; CHECK-LABEL: set9_align1:
; CHECK: .long memset
define void @set9_align1(ptr %d) {
  call void @llvm.memset.p0.i32(ptr align 1 %d, i8 165, i32 9, i1 false)
  ret void
}

; CHECK-LABEL: zero:
; CHECK-NOT: .long mem
; CHECK: rts
; CHECK: .size zero
define void @zero(ptr %d, ptr %s) {
  call void @llvm.memcpy.p0.p0.i32(ptr align 1 %d, ptr align 1 %s, i32 0, i1 false)
  call void @llvm.memmove.p0.p0.i32(ptr align 1 %d, ptr align 1 %s, i32 0, i1 false)
  call void @llvm.memset.p0.i32(ptr align 1 %d, i8 165, i32 0, i1 false)
  ret void
}

; Even optsize keeps each admitted volatile access explicit. These exceed the
; ordinary four-store limit; the overlap-safe native expansion handles moves.
; CHECK-LABEL: volatile_size:
; CHECK-NOT: jsr
; CHECK-NOT: .long mem
; CHECK: .size volatile_size
define void @volatile_size(ptr %d, ptr %s, i8 %value) optsize {
  call void @llvm.memcpy.p0.p0.i32(ptr align 1 %d, ptr align 1 %s, i32 60, i1 true)
  call void @llvm.memmove.p0.p0.i32(ptr align 1 %d, ptr align 1 %s, i32 16, i1 true)
  call void @llvm.memset.p0.i32(ptr align 1 %d, i8 %value, i32 60, i1 true)
  ret void
}
