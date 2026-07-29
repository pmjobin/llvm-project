; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-readobj --relocations %t.be.o | FileCheck %s --check-prefix=RELOC
; RUN: llvm-readobj --relocations %t.le.o | FileCheck %s --check-prefix=RELOC
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=OBJECT

define internal void @branch_pad() noinline nounwind {
	ret void
}

define i64 @i64_branch_range(i32 %selector, i64 %value, i64 %count) nounwind {
; CHECK-LABEL: i64_branch_range:
; CHECK: tst
; CHECK: bf	[[NEAR:.LBB[0-9_]+]]
; CHECK-NEXT: bra	[[FAR:.LBB[0-9_]+]]
; CHECK-NEXT: nop
; CHECK: [[NEAR]]:
; CHECK: shll
; CHECK-NEXT: rotcl
; CHECK-NEXT: dt
; CHECK: bf
; CHECK-COUNT-65: bsr	branch_pad
; CHECK: [[FAR]]:
; OBJECT-LABEL: <i64_branch_range>:
; OBJECT-NOT: __
; OBJECT: shll
; OBJECT-NEXT: rotcl
; OBJECT-NEXT: dt
; RELOC: Relocations [
; RELOC-NEXT: ]
entry:
	%condition = icmp eq i32 %selector, 0
	br i1 %condition, label %far, label %work

work:
	%shifted = shl i64 %value, %count
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	call void @branch_pad()
	br label %far

far:
	%result = phi i64 [ %value, %entry ], [ %shifted, %work ]
	ret i64 %result
}
