; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=STATIC
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=STATIC
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=PIC

declare i32 @same_signature(i32)
declare i32 @five_arguments(i32, i32, i32, i32, i32)
declare i32 @six_arguments(i32, i32, i32, i32, i32, i32)
declare void @same_void()

define internal fastcc i32 @fast_five_arguments(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e) noinline {
	ret i32 %e
}

define i32 @direct_tail_call(i32 %value) {
; STATIC-LABEL: direct_tail_call:
; STATIC-NOT: sts.l
; STATIC: mov.l	{{.*}},[[TARGET:r[0-9]+]]
; STATIC-NEXT: jmp	@[[TARGET]]
; STATIC-NEXT: nop
; STATIC-NOT: rts
; PIC-LABEL: direct_tail_call:
; PIC: sts.l	pr,@-r15
; PIC: jsr	@
	%result = tail call i32 @same_signature(i32 %value)
	ret i32 %result
}

define i32 @ordinary_call(i32 %value) {
; STATIC-LABEL: ordinary_call:
; STATIC: sts.l	pr,@-r15
; STATIC: jsr	@
; STATIC: lds.l	@r15+,pr
; STATIC: rts
	%result = call i32 @same_signature(i32 %value)
	ret i32 %result
}

define i32 @different_arguments(i32 %value, i32 %unused) {
; STATIC-LABEL: different_arguments:
; STATIC-NOT: sts.l
; STATIC: mov.l	{{.*}},[[TARGET:r[0-9]+]]
; STATIC-NEXT: jmp	@[[TARGET]]
; STATIC-NEXT: nop
; STATIC-NOT: rts
	%result = tail call i32 @same_signature(i32 %value)
	ret i32 %result
}

define i32 @stack_argument(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e) {
; STATIC-LABEL: stack_argument:
; STATIC-NOT: sts.l
; STATIC: mov.l	{{.*}},[[TARGET:r[0-9]+]]
; STATIC-NEXT: jmp	@[[TARGET]]
; STATIC-NEXT: nop
; STATIC-NOT: rts
	%result = tail call i32 @five_arguments(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e)
	ret i32 %result
}

define i32 @fast_stack_argument(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e) {
; STATIC-LABEL: fast_stack_argument:
; STATIC-NOT: sts.l
; STATIC: mov.l	{{.*}},[[TARGET:r[0-9]+]]
; STATIC-NEXT: jmp	@[[TARGET]]
; STATIC-NEXT: nop
; STATIC-NOT: rts
	%result = tail call fastcc i32 @fast_five_arguments(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e)
	ret i32 %result
}

define i32 @changed_stack_argument(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e) {
; STATIC-LABEL: changed_stack_argument:
; STATIC: sts.l	pr,@-r15
; STATIC: jsr	@
; STATIC: lds.l	@r15+,pr
; STATIC: rts
	%changed = add i32 %e, 1
	%result = tail call i32 @five_arguments(i32 %a, i32 %b, i32 %c, i32 %d, i32 %changed)
	ret i32 %result
}

define i32 @moved_stack_argument(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f) {
; STATIC-LABEL: moved_stack_argument:
; STATIC: sts.l	pr,@-r15
; STATIC: jsr	@
; STATIC: lds.l	@r15+,pr
; STATIC: rts
	%result = tail call i32 @six_arguments(i32 %a, i32 %b, i32 %c, i32 %d, i32 %f, i32 %e)
	ret i32 %result
}

define i32 @framed_stack_argument(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e) {
; STATIC-LABEL: framed_stack_argument:
; STATIC-NOT: sts.l
; STATIC: mov.l	{{.*}},[[TARGET:r[0-9]+]]
; STATIC: mov.l	{{.*}},@r15
; STATIC: add	#4,r15
; STATIC: jmp	@[[TARGET]]
; STATIC-NEXT: nop
	%slot = alloca i32, align 4
	store volatile i32 %a, ptr %slot, align 4
	%result = tail call i32 @five_arguments(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e)
	ret i32 %result
}

define i32 @framed_tail_call(i32 %value) {
; STATIC-LABEL: framed_tail_call:
; STATIC-NOT: sts.l
; STATIC: mov.l	{{.*}},@r15
; STATIC: add	#4,r15
; STATIC: jmp	@
; STATIC-NEXT: nop
	%slot = alloca i32, align 4
	store volatile i32 %value, ptr %slot, align 4
	%result = tail call i32 @same_signature(i32 %value)
	ret i32 %result
}

define void @interrupt_tail_call() #0 {
; STATIC-LABEL: interrupt_tail_call:
; STATIC: sts.l	pr,@-r15
; STATIC: jsr	@
; STATIC: lds.l	@r15+,pr
; STATIC: rte
	tail call void @same_void()
	ret void
}

attributes #0 = { "sh-interrupt-handler"="interrupt" }
