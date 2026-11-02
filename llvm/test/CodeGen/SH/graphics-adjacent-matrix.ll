; RUN: llc -mtriple=shle -mcpu=sh4 -stop-after=sh-prepare-ir %s -o - | FileCheck %s
; RUN: llc -mtriple=sh -mcpu=sh4 -stop-after=sh-prepare-ir %s -o - | FileCheck %s
; RUN: llc -mtriple=shle -mcpu=sh4 -enable-new-pm=1 -stop-after=sh-prepare-ir-module %s -o - | FileCheck %s
; RUN: llc -mtriple=sh -mcpu=sh4 -enable-new-pm=1 -stop-after=sh-prepare-ir-module %s -o - | FileCheck %s

; Fuse only a local, consecutive, disjoint pair through the existing bounded
; owner. Keeping both FTRVs inside that owner avoids restoring XD12/XD14 and
; then incorrectly treating the matrix as still resident.
declare <4 x float> @llvm.sh.ftrv(ptr, <4 x float>)
declare void @llvm.sh.ftrv.array(ptr, ptr, i32, ptr)
declare i32 @llvm.sh.get.fpscr()
declare void @llvm.sh.set.fpscr(i32)
declare void @unknown()

; CHECK-LABEL: define void @positive(
; CHECK-NOT: call <4 x float> @llvm.sh.ftrv
; CHECK: call void @llvm.sh.ftrv.array(ptr align 16 %out, ptr align 16 %in, i32 2, ptr align 16 %matrix)
; CHECK-NOT: call <4 x float> @llvm.sh.ftrv
; CHECK: ret void
define void @positive(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; CHECK-LABEL: define void @alias(
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: ret void
define void @alias(ptr %out, ptr %in, ptr %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; CHECK-LABEL: define void @mutation(
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: ret void
define void @mutation(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  store float 2.0, ptr %matrix, align 4
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; CHECK-LABEL: define void @call_barrier(
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: ret void
define void @call_barrier(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  call void @unknown()
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; CHECK-LABEL: define void @asm_barrier(
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: ret void
define void @asm_barrier(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  call void asm sideeffect "", "~{memory},~{xd12}"()
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; CHECK-LABEL: define void @fpscr_read(
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: ret void
define void @fpscr_read(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %state = call i32 @llvm.sh.get.fpscr()
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; CHECK-LABEL: define void @fpscr_write(
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: ret void
define void @fpscr_write(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  call void @llvm.sh.set.fpscr(i32 0)
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; CHECK-LABEL: define void @different_matrix(
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: ret void
define void @different_matrix(ptr noalias %out, ptr noalias %in, ptr noalias %matrix, ptr noalias %other) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %other, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; CHECK-LABEL: define void @nonconsecutive(
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: ret void
define void @nonconsecutive(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 32
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; CHECK-LABEL: define void @volatile_source(
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: ret void
define void @volatile_source(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load volatile <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; CHECK-LABEL: define void @zero_barrier(
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK: call void @llvm.sh.ftrv.array({{.*}}i32 0,
; CHECK: call <4 x float> @llvm.sh.ftrv
; CHECK-NOT: call void @llvm.sh.ftrv.array
; CHECK: ret void
define void @zero_barrier(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  call void @llvm.sh.ftrv.array(ptr %out, ptr %in, i32 0, ptr %matrix)
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; CHECK-LABEL: define void @conditional_zero(
; CHECK: br i1 %nonzero, label %body, label %exit
; CHECK: body:
; CHECK-NOT: call <4 x float> @llvm.sh.ftrv
; CHECK: call void @llvm.sh.ftrv.array(ptr align 16 %out, ptr align 16 %in, i32 2, ptr align 16 %matrix)
; CHECK-NOT: call <4 x float> @llvm.sh.ftrv
; CHECK: ret void
define void @conditional_zero(ptr noalias %out, ptr noalias %in, ptr noalias %matrix, i1 %nonzero) {
entry:
  br i1 %nonzero, label %body, label %exit
body:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  br label %exit
exit:
  ret void
}
