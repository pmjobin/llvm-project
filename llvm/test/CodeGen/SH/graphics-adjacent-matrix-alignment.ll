; RUN: llc -mtriple=shle -mcpu=sh4 -stop-after=sh-prepare-ir %s -o - | FileCheck %s --check-prefix=IR
; RUN: llc -mtriple=shle -mcpu=sh4 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle -mcpu=sh4 -enable-new-pm=1 -stop-after=sh-prepare-ir-module %s -o - | FileCheck %s --check-prefix=IR
; RUN: llc -mtriple=shle -mcpu=sh4 -enable-new-pm=1 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh -mcpu=sh4 -stop-after=sh-prepare-ir %s -o - | FileCheck %s --check-prefix=IR
; RUN: llc -mtriple=sh -mcpu=sh4 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh -mcpu=sh4 -enable-new-pm=1 -stop-after=sh-prepare-ir-module %s -o - | FileCheck %s --check-prefix=IR
; RUN: llc -mtriple=sh -mcpu=sh4 -enable-new-pm=1 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM

; Every access must independently meet the counted intrinsic's alignment.
; Rejected pairs retain their ordinary vector accesses and legalization.
declare <4 x float> @llvm.sh.ftrv(ptr, <4 x float>)

; IR-LABEL: define void @load0_align1(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 1
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 16
; IR: %v1 = load <4 x float>, ptr %s1, align 16
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 16
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: load0_align1:
; ASM-DAG: mov.b
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @load0_align1(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 1
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; IR-LABEL: define void @load0_align2(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 2
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 16
; IR: %v1 = load <4 x float>, ptr %s1, align 16
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 16
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: load0_align2:
; ASM-DAG: mov.w
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @load0_align2(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 2
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; IR-LABEL: define void @load1_align1(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 16
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 16
; IR: %v1 = load <4 x float>, ptr %s1, align 1
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 16
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: load1_align1:
; ASM-DAG: mov.b
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @load1_align1(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 1
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; IR-LABEL: define void @load1_align2(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 16
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 16
; IR: %v1 = load <4 x float>, ptr %s1, align 2
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 16
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: load1_align2:
; ASM-DAG: mov.w
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @load1_align2(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 2
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; IR-LABEL: define void @store0_align1(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 16
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 1
; IR: %v1 = load <4 x float>, ptr %s1, align 16
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 16
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: store0_align1:
; ASM-DAG: mov.b
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @store0_align1(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 1
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; IR-LABEL: define void @store0_align2(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 16
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 2
; IR: %v1 = load <4 x float>, ptr %s1, align 16
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 16
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: store0_align2:
; ASM-DAG: mov.w
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @store0_align2(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 2
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; IR-LABEL: define void @store1_align1(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 16
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 16
; IR: %v1 = load <4 x float>, ptr %s1, align 16
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 1
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: store1_align1:
; ASM-DAG: mov.b
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @store1_align1(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 1
  ret void
}

; IR-LABEL: define void @store1_align2(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 16
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 16
; IR: %v1 = load <4 x float>, ptr %s1, align 16
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 2
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: store1_align2:
; ASM-DAG: mov.w
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @store1_align2(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 2
  ret void
}

; IR-LABEL: define void @source_align1(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 1
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 16
; IR: %v1 = load <4 x float>, ptr %s1, align 1
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 16
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: source_align1:
; ASM-DAG: mov.b
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @source_align1(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 1
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 1
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; IR-LABEL: define void @source_align2(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 2
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 16
; IR: %v1 = load <4 x float>, ptr %s1, align 2
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 16
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: source_align2:
; ASM-DAG: mov.w
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @source_align2(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 2
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 16
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 2
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}

; IR-LABEL: define void @destination_align1(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 16
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 1
; IR: %v1 = load <4 x float>, ptr %s1, align 16
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 1
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: destination_align1:
; ASM-DAG: mov.b
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @destination_align1(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 1
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 1
  ret void
}

; IR-LABEL: define void @destination_align2(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 16
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 2
; IR: %v1 = load <4 x float>, ptr %s1, align 16
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 2
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: destination_align2:
; ASM-DAG: mov.w
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @destination_align2(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 16
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 2
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 2
  ret void
}

; IR-LABEL: define void @both_align1(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 1
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 1
; IR: %v1 = load <4 x float>, ptr %s1, align 1
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 1
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: both_align1:
; ASM-DAG: mov.b
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @both_align1(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 1
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 1
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 1
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 1
  ret void
}

; IR-LABEL: define void @both_align2(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 2
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 2
; IR: %v1 = load <4 x float>, ptr %s1, align 2
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 2
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: both_align2:
; ASM-DAG: mov.w
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @both_align2(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 2
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 2
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 2
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 2
  ret void
}

; IR-LABEL: define void @mixed_alignments(
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: %v0 = load <4 x float>, ptr %in, align 2
; IR: %r0 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r0, ptr %out, align 8
; IR: %v1 = load <4 x float>, ptr %s1, align 16
; IR: %r1 = call <4 x float> @llvm.sh.ftrv
; IR: store <4 x float> %r1, ptr %d1, align 1
; IR-NOT: call void @llvm.sh.ftrv.array
; IR: ret void
; ASM-LABEL: mixed_alignments:
; ASM-DAG: mov.b
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @mixed_alignments(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 2
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 8
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 1
  ret void
}

; IR-LABEL: define void @positive_align4(
; IR-NOT: call <4 x float> @llvm.sh.ftrv
; IR: call void @llvm.sh.ftrv.array(ptr align 4 %out, ptr align 4 %in, i32 2, ptr align 16 %matrix)
; IR-NOT: call <4 x float> @llvm.sh.ftrv
; IR: ret void
; ASM-LABEL: positive_align4:
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @positive_align4(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 4
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 4
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 4
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 4
  ret void
}

; IR-LABEL: define void @positive_align8(
; IR-NOT: call <4 x float> @llvm.sh.ftrv
; IR: call void @llvm.sh.ftrv.array(ptr align 8 %out, ptr align 8 %in, i32 2, ptr align 16 %matrix)
; IR-NOT: call <4 x float> @llvm.sh.ftrv
; IR: ret void
; ASM-LABEL: positive_align8:
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @positive_align8(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 8
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 8
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 8
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 8
  ret void
}

; IR-LABEL: define void @positive_align16(
; IR-NOT: call <4 x float> @llvm.sh.ftrv
; IR: call void @llvm.sh.ftrv.array(ptr align 16 %out, ptr align 16 %in, i32 2, ptr align 16 %matrix)
; IR-NOT: call <4 x float> @llvm.sh.ftrv
; IR: ret void
; ASM-LABEL: positive_align16:
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @positive_align16(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
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

; IR-LABEL: define void @positive_mixed(
; IR-NOT: call <4 x float> @llvm.sh.ftrv
; IR: call void @llvm.sh.ftrv.array(ptr align 8 %out, ptr align 4 %in, i32 2, ptr align 16 %matrix)
; IR-NOT: call <4 x float> @llvm.sh.ftrv
; IR: ret void
; ASM-LABEL: positive_mixed:
; ASM-DAG: ftrv
; ASM-DAG: ftrv
; ASM: .Lfunc_end
define void @positive_mixed(ptr noalias %out, ptr noalias %in, ptr noalias %matrix) {
entry:
  %v0 = load <4 x float>, ptr %in, align 4
  %r0 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v0)
  store <4 x float> %r0, ptr %out, align 8
  %s1 = getelementptr inbounds i8, ptr %in, i32 16
  %v1 = load <4 x float>, ptr %s1, align 16
  %r1 = call <4 x float> @llvm.sh.ftrv(ptr align 16 %matrix, <4 x float> %v1)
  %d1 = getelementptr inbounds i8, ptr %out, i32 16
  store <4 x float> %r1, ptr %d1, align 16
  ret void
}
