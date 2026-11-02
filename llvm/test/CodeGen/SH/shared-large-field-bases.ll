; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

; Nearby fields beyond the longword displacement range should share an
; adjusted base. Materialize one large offset, then use short displacements.
define void @shared_fields(ptr %base, i1 %condition) optsize nounwind {
; CHECK-LABEL: shared_fields:
; CHECK: mov.l [[OFFSET:.LCPI[^,]+]],[[SCRATCH:r[0-9]+]]
; CHECK-NEXT: add [[SCRATCH]],[[BASE:r[0-9]+]]
; CHECK-NEXT: mov #0,[[ZERO:r[0-9]+]]
; CHECK-DAG: mov.l [[ZERO]],@(16,[[BASE]])
; CHECK-DAG: mov.l [[ZERO]],@(12,[[BASE]])
; CHECK-DAG: mov.l [[ZERO]],@(8,[[BASE]])
; CHECK-DAG: mov.l [[ZERO]],@(4,[[BASE]])
; CHECK-DAG: mov.l [[ZERO]],@[[BASE]]
; CHECK: [[OFFSET]]:
; CHECK-NEXT: .long 64000
entry:
	%p0 = getelementptr i32, ptr %base, i32 16000
	%p1 = getelementptr i32, ptr %base, i32 16001
	%p2 = getelementptr i32, ptr %base, i32 16002
	%p3 = getelementptr i32, ptr %base, i32 16003
	%p4 = getelementptr i32, ptr %base, i32 16004
	br i1 %condition, label %stores, label %exit
stores:
	store i32 0, ptr %p0, align 4
	store i32 0, ptr %p1, align 4
	store i32 0, ptr %p2, align 4
	store i32 0, ptr %p3, align 4
	store i32 0, ptr %p4, align 4
	br label %exit
exit:
	ret void
}
