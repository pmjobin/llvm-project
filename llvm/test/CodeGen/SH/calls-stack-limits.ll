; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define internal i32 @take6(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5) noinline nounwind {
	ret i32 %a5
}

define internal i32 @take19(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7, i32 %a8, i32 %a9, i32 %a10, i32 %a11, i32 %a12, i32 %a13, i32 %a14, i32 %a15, i32 %a16, i32 %a17, i32 %a18) noinline nounwind {
	ret i32 %a18
}

define i32 @call6(i32 %a4, i32 %a5) nounwind {
; CHECK-LABEL: call6:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: add	#-8,r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@(4,r15)
; CHECK: bsr	take6
; CHECK-NEXT: nop
; CHECK-NEXT: add	#8,r15
; CHECK-NEXT: lds.l	@r15+,pr
	%result = call i32 @take6(i32 1, i32 2, i32 3, i32 4, i32 %a4, i32 %a5)
	ret i32 %result
}

define i32 @call19() nounwind {
; CHECK-LABEL: call19:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NEXT: add	#-60,r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@(4,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(8,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(12,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(16,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(20,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(24,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(28,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(32,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(36,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(40,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(44,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(48,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(52,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(56,r15)
; CHECK: bsr	take19
; CHECK-NEXT: nop
; CHECK-NEXT: add	#60,r15
; CHECK-NEXT: lds.l	@r15+,pr
	%result = call i32 @take19(i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
	ret i32 %result
}
