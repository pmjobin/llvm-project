; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj -r %t.be.o | FileCheck %s --check-prefix=OBJ
; RUN: llvm-readobj -r %t.le.o | FileCheck %s --check-prefix=OBJ

@global = global i32 1, align 4

define i32 @literal_after_relaxed_branch(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: literal_after_relaxed_branch:
; CHECK: cmp/eq
; CHECK: bf	[[DIVIDE:.LBB[0-9_]+]]
; CHECK-NEXT: bra	[[FAR:.LBB[0-9_]+]]
; CHECK-NEXT: nop
; CHECK: [[DIVIDE]]:
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: div0s
; CHECK-COUNT-32: div1
; CHECK: bra	[[FAR]]
; CHECK-NEXT: nop
; CHECK: [[FAR]]:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,
; CHECK: rts
; CHECK-NEXT: nop
; CHECK: .p2align	2
; CHECK: .LCPI{{[0-9]+}}_0_0:
; CHECK-NEXT: .long	global
	entry:
	%equal = icmp eq i32 %a, %b
	br i1 %equal, label %far, label %divide

divide:
	%unsigned = udiv i32 %a, %b
	%signed = sdiv i32 %unsigned, %c
	br label %far

far:
	%selected = phi i32 [ %b, %entry ], [ %signed, %divide ]
	%loaded = load volatile i32, ptr @global, align 4
	%result = add i32 %selected, %loaded
	ret i32 %result
}

; OBJ: Section {{.*}} .rela.text {
; OBJ-NEXT: 0x{{[0-9A-F]+}} R_SH_DIR32 global
; OBJ-NEXT: }
; OBJ-NOT: R_SH_NONE
