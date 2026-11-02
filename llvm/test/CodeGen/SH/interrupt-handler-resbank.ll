; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2a -mattr=-single-fpu,-double-fpu -relocation-model=static -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,STATIC
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2a -mattr=-single-fpu,-double-fpu -relocation-model=pic -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,PIC

@global = external global i32

declare void @helper(i32, i32, i32, i32, i32, i32, i32, i32)
declare void @sink(ptr)

define void @empty() #0 {
	ret void
}

define void @used() #0 {
	call void asm sideeffect "", "~{r0},~{r8},~{r14},~{pr},~{mach},~{macl}"()
	ret void
}

define void @nonleaf() #0 {
	call void @helper(i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	ret void
}

define void @frame() #0 {
	%storage = alloca [32 x i8], align 4
	call void @sink(ptr %storage)
	ret void
}

define void @pic() #0 {
	%value = load volatile i32, ptr @global
	call void @helper(i32 %value, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	ret void
}

attributes #0 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-resbank" "target-abi"="gnu" }

; COMMON-LABEL: empty:
; COMMON-NEXT: ! %bb.0:
; COMMON-NEXT: resbank
; COMMON-NEXT: rte
; COMMON-NEXT: nop

; COMMON-LABEL: used:
; COMMON-NOT: mov.l
; COMMON-NOT: sts.l
; COMMON: resbank
; COMMON-NEXT: rte
; COMMON-NEXT: nop

; COMMON-LABEL: nonleaf:
; COMMON-NOT: Folded Spill
; COMMON-NOT: sts.l
; COMMON: add #-16,r15
; COMMON: mov.l r{{[0-9]+}},@({{[0-9]+}},r15)
; COMMON: jsr
; COMMON: add #16,r15
; COMMON: resbank
; COMMON-NEXT: rte
; COMMON-NEXT: nop

; COMMON-LABEL: frame:
; COMMON: add #-32,r15
; COMMON: jsr
; COMMON: add #32,r15
; COMMON: resbank
; COMMON-NEXT: rte
; COMMON-NEXT: nop

; STATIC-LABEL: pic:
; STATIC-NOT: mov.l r12,
; STATIC: resbank

; PIC-LABEL: pic:
; PIC-NOT: mov.l r12,{{.*}}Folded Spill
; PIC: mova
; PIC: resbank
; PIC-NEXT: rte
; PIC-NEXT: nop
