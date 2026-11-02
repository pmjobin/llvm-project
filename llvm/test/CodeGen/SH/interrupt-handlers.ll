; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -relocation-model=static -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=STATIC,COMMON
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -relocation-model=static -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=STATIC,COMMON
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=PIC,COMMON
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=PIC,COMMON
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -relocation-model=static -O2 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -relocation-model=static -O2 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-dwarfdump --verify %t.be.o %t.le.o

@global = global i32 0, align 4
@external_global = external global i32, align 4
@mmio = external global i32, align 4
@dynamic_size = global i32 1, align 4
@vector_table = constant [2 x ptr] [ptr @interrupt_empty, ptr @trapa_empty], align 4
@llvm.used = appending global [1 x ptr] [ptr @vector_table], section "llvm.metadata"

@interrupt_alias = alias void (), ptr @interrupt_empty

declare void @helper(i32, i32, i32, i32, i32, i32, i32, i32)
declare void @sink(ptr)

define void @interrupt_empty() #0 {
	ret void
}

define weak void @interrupt_weak() #0 {
	ret void
}

define void @interrupt_used() #0 {
	call void asm sideeffect "", "~{r0},~{r8},~{mach},~{macl}"()
	ret void
}

define void @interrupt_pr_used() #0 {
	call void asm sideeffect "", "~{pr}"()
	ret void
}

define void @interrupt_pic() #0 {
	%local = load volatile i32, ptr @global, align 4
	%external = load volatile i32, ptr @external_global, align 4
	%sum = add i32 %local, %external
	store volatile i32 %sum, ptr @mmio, align 4
	ret void
}

define void @interrupt_nonleaf() #0 {
	%value = load volatile i32, ptr @global, align 4
	call void @helper(i32 %value, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	ret void
}

define void @interrupt_multiple_calls() #0 {
	call void @helper(i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	call void @helper(i32 8, i32 7, i32 6, i32 5, i32 4, i32 3, i32 2, i32 1)
	ret void
}

define internal void @local_helper() #2 {
	call void @helper(i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	ret void
}

define void @interrupt_local_call() #0 {
	call void @local_helper()
	ret void
}

define void @interrupt_large_frame() #0 {
	%frame = alloca [300 x i8], align 4
	call void @sink(ptr %frame)
	ret void
}

define void @interrupt_dynamic_alloca() #0 {
	%size = load volatile i32, ptr @dynamic_size, align 4
	%frame = alloca i8, i32 %size, align 4
	call void @sink(ptr %frame)
	ret void
}

define void @trapa_empty() #1 {
	ret void
}

define void @trapa_used() #1 {
	call void asm sideeffect "", "~{r0},~{r8}"()
	ret void
}

define void @trapa_nonleaf() #1 {
	%value = load volatile i32, ptr @global, align 4
	call void @helper(i32 %value, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	ret void
}

attributes #0 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "target-abi"="gnu" "target-cpu"="sh2" "target-features"="+sh2,-single-fpu,-double-fpu" }
attributes #1 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="trapa" "target-abi"="gnu" "target-cpu"="sh2" "target-features"="+sh2,-single-fpu,-double-fpu" }
attributes #2 = { noinline nounwind }

; COMMON-LABEL: interrupt_empty:
; COMMON-NEXT: .cfi_startproc
; COMMON-NEXT: ! %bb.0:
; COMMON-NOT: mov.l
; COMMON-NOT: sts.l
; COMMON-NEXT: rte
; COMMON-NEXT: nop

; COMMON-LABEL: interrupt_weak:
; COMMON: rte
; COMMON-NEXT: nop

; COMMON-LABEL: interrupt_used:
; COMMON-NOT: sts.l pr,@-r15
; COMMON: sts.l mach,@-r15
; COMMON: sts.l macl,@-r15
; COMMON: mov.l r0,
; COMMON: mov.l r8,
; COMMON-NOT: mov.l r1,@
; COMMON-NOT: mov.l r2,@
; COMMON-NOT: mov.l r3,@
; COMMON-NOT: mov.l r4,@
; COMMON-NOT: mov.l r5,@
; COMMON-NOT: mov.l r6,@
; COMMON-NOT: mov.l r7,@
; COMMON-NOT: mov.l r9,@
; COMMON-NOT: mov.l r10,@
; COMMON-NOT: mov.l r11,@
; COMMON-NOT: mov.l r12,@
; COMMON-NOT: mov.l r13,@
; COMMON-NOT: mov.l r14,@
; COMMON: lds.l @r15+,macl
; COMMON: lds.l @r15+,mach
; COMMON: rte
; COMMON-NEXT: nop

; COMMON-LABEL: interrupt_pr_used:
; COMMON: sts.l pr,@-r15
; COMMON-NOT: sts.l mach,@-r15
; COMMON-NOT: sts.l macl,@-r15
; COMMON: lds.l @r15+,pr
; COMMON: rte
; COMMON-NEXT: nop

; STATIC-LABEL: interrupt_pic:
; STATIC-NOT: mov.l r12,
; STATIC: rte
; STATIC-NEXT: nop

; PIC-LABEL: interrupt_pic:
; PIC: mov.l r12,
; PIC: mova [[GOT:.LCPI[0-9]+_[0-9]+_[0-9]+]],r0
; PIC-NEXT: mov.l [[GOT]],r12
; PIC-NEXT: add r0,r12
; PIC: mov.l {{.*}},r12
; PIC: rte
; PIC-NEXT: nop

; COMMON-LABEL: interrupt_nonleaf:
; COMMON: sts.l pr,@-r15
; COMMON-NEXT: .cfi_def_cfa_offset 4
; COMMON-NEXT: .cfi_offset pr, -4
; COMMON: sts.l mach,@-r15
; COMMON: sts.l macl,@-r15
; COMMON: mov.l r0,
; COMMON: mov.l r1,
; COMMON: mov.l r2,
; COMMON: mov.l r3,
; COMMON: mov.l r4,
; COMMON: mov.l r5,
; COMMON: mov.l r6,
; COMMON: mov.l r7,
; COMMON: add #-16,r15
; COMMON: jsr
; COMMON: add #16,r15
; COMMON: lds.l @r15+,macl
; COMMON-NEXT: .cfi_restore macl
; COMMON: lds.l @r15+,mach
; COMMON: lds.l @r15+,pr
; COMMON: rte
; COMMON-NEXT: nop

; COMMON-LABEL: interrupt_multiple_calls:
; COMMON-COUNT-1: sts.l pr,@-r15
; COMMON-COUNT-1: sts.l mach,@-r15
; COMMON-COUNT-1: sts.l macl,@-r15
; COMMON-COUNT-2: jsr
; COMMON: rte
; COMMON-NEXT: nop

; COMMON-LABEL: local_helper:
; COMMON: jsr
; COMMON: rts
; COMMON-NEXT: {{(nop|mov\.l @r15\+,r12)}}

; COMMON-LABEL: interrupt_local_call:
; COMMON: sts.l pr,@-r15
; COMMON: sts.l mach,@-r15
; COMMON: sts.l macl,@-r15
; COMMON: jsr
; COMMON: lds.l @r15+,pr
; COMMON: rte
; COMMON-NEXT: nop

; COMMON-LABEL: interrupt_large_frame:
; COMMON: sts.l pr,@-r15
; COMMON: jsr
; COMMON: lds.l @r15+,pr
; COMMON: rte
; COMMON-NEXT: nop

; COMMON-LABEL: interrupt_dynamic_alloca:
; COMMON: mov.l r14,
; COMMON: jsr
; COMMON: mov.l {{.*}},r14
; COMMON: rte
; COMMON-NEXT: nop

; COMMON-LABEL: trapa_empty:
; COMMON: rte
; COMMON-NEXT: nop

; COMMON-LABEL: trapa_used:
; COMMON-NOT: mov.l r0,
; COMMON: mov.l r8,
; COMMON: mov.l {{.*}},r8
; COMMON-NOT: mov.l {{.*}},r0
; COMMON: rte
; COMMON-NEXT: nop

; COMMON-LABEL: trapa_nonleaf:
; COMMON: sts.l pr,@-r15
; COMMON-NOT: sts.l mach,@-r15
; COMMON-NOT: sts.l macl,@-r15
; COMMON: jsr
; COMMON: lds.l @r15+,pr
; COMMON: rte
; COMMON-NEXT: nop

; STATIC: vector_table:
; STATIC-NEXT: .long interrupt_empty
; STATIC-NEXT: .long trapa_empty
