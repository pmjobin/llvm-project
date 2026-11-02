; REQUIRES: sh-registered-target
; RUN: opt -mtriple=shle-unknown-none-elf -mcpu=sh2 -S -passes='require<profile-summary>,function(codegenprepare)' %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs %s -o /dev/null

; These byte GEPs address longwords. Without the legacy CGP access-type
; extension, upstream keeps separate roots. Preserve exact addresses and widths;
; missed sharing is a recorded optimization limitation, not a legality failure.
define void @wide_accesses(ptr %base, i1 %condition) optsize {
; CHECK-LABEL: define void @wide_accesses(
; CHECK-DAG: [[BASE:%[^ ]+]] = getelementptr i8, ptr %base, i32 64000
; CHECK-DAG: [[NEXT:%[^ ]+]] = getelementptr i8, ptr %base, i32 64016
; CHECK-DAG: [[LAST:%[^ ]+]] = getelementptr i8, ptr %base, i32 64060
; CHECK: store i32 0, ptr [[BASE]]
; CHECK: store i32 0, ptr [[NEXT]]
; CHECK: store i32 0, ptr [[LAST]]
entry:
	%first = getelementptr i8, ptr %base, i32 64000
	%next = getelementptr i8, ptr %base, i32 64016
	%last = getelementptr i8, ptr %base, i32 64060
	br i1 %condition, label %stores, label %exit
stores:
	store i32 0, ptr %first, align 4
	store i32 0, ptr %next, align 4
	store i32 0, ptr %last, align 4
	br label %exit
exit:
	ret void
}

; Every use must accept the displacement. The byte load at offset 16
; prevents the second field from sharing the first field's base.
define i8 @mixed_accesses(ptr %base, i1 %condition) optsize {
; CHECK-LABEL: define i8 @mixed_accesses(
; CHECK-DAG: [[SECOND:%[^ ]+]] = getelementptr i8, ptr %base, i32 64016
; CHECK-DAG: [[FIRST:%[^ ]+]] = getelementptr i8, ptr %base, i32 64000
; CHECK: store i32 0, ptr [[FIRST]]
; CHECK: store i32 0, ptr [[SECOND]]
; CHECK: load volatile i8, ptr [[SECOND]]
entry:
	%first = getelementptr i8, ptr %base, i32 64000
	%second = getelementptr i8, ptr %base, i32 64016
	br i1 %condition, label %accesses, label %exit
accesses:
	store i32 0, ptr %first, align 4
	store i32 0, ptr %second, align 4
	%byte = load volatile i8, ptr %second, align 1
	br label %exit
exit:
	%result = phi i8 [ %byte, %accesses ], [ 0, %entry ]
	ret i8 %result
}

; A scalar FPU offset of 128 cannot be materialized in R0 with one immediate
; move. Upstream currently hoists separate bases; preserve both exact loads.
define float @unfoldable_offsets(ptr %base, i1 %condition) optsize "target-cpu"="sh4" {
; CHECK-LABEL: define float @unfoldable_offsets(
; CHECK: entry:
; CHECK-DAG: [[A:%[^ ]+]] = getelementptr i8, ptr %base, i32 1024
; CHECK-DAG: [[B:%[^ ]+]] = getelementptr i8, ptr %base, i32 1152
; CHECK: br i1 %condition, label %accesses, label %exit
; CHECK: accesses:
; CHECK: load float, ptr [[A]]
; CHECK: load float, ptr [[B]]
entry:
	br i1 %condition, label %accesses, label %exit
accesses:
	%first = getelementptr i8, ptr %base, i32 1024
	%second = getelementptr i8, ptr %base, i32 1152
	%a = load float, ptr %first, align 4
	%b = load float, ptr %second, align 4
	%sum = fadd float %a, %b
	br label %exit
exit:
	%result = phi float [ %sum, %accesses ], [ 0.0, %entry ]
	ret float %result
}

; The legacy shared-root optimization is absent. Both field addresses remain
; exact and the ordinary backend must produce legal scalar FPU accesses.
define float @indexed_scalar_offsets(ptr %base, i1 %condition) optsize "target-cpu"="sh4" {
; CHECK-LABEL: define float @indexed_scalar_offsets(
; CHECK: entry:
; CHECK-DAG: [[ROOT:%[^ ]+]] = getelementptr i8, ptr %base, i32 1024
; CHECK-DAG: [[OFFSET:%[^ ]+]] = getelementptr i8, ptr %base, i32 1040
; CHECK: br i1 %condition, label %accesses, label %exit
; CHECK: accesses:
; CHECK: load float, ptr [[ROOT]]
; CHECK: load float, ptr [[OFFSET]]
entry:
	br i1 %condition, label %accesses, label %exit
accesses:
	%first = getelementptr i8, ptr %base, i32 1024
	%second = getelementptr i8, ptr %base, i32 1040
	%a = load float, ptr %first, align 4
	%b = load float, ptr %second, align 4
	%sum = fadd float %a, %b
	br label %exit
exit:
	%result = phi float [ %sum, %accesses ], [ 0.0, %entry ]
	ret float %result
}
