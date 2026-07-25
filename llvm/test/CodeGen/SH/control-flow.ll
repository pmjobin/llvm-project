; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define i32 @choose_equal(i32 %a, i32 %b) #0 {
; CHECK-LABEL: choose_equal:
; CHECK: cmp/eq	r5,r4
; CHECK-NEXT: bf	[[DIFFERENT:.LBB[0-9_]+]]
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{mov(\.l)?}}
; CHECK: [[DIFFERENT]]:
	%equal = icmp eq i32 %a, %b
	br i1 %equal, label %same, label %different
same:
	ret i32 %a
different:
	ret i32 %b
}

define i32 @max_signed(i32 %a, i32 %b) #0 {
; CHECK-LABEL: max_signed:
; CHECK: cmp/gt	r5,{{r[04]}}
; CHECK-NEXT: bf	[[TAKE_B:.LBB[0-9_]+]]
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: [[TAKE_B]]:
; CHECK: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK-NOT: PHI
	%greater = icmp sgt i32 %a, %b
	br i1 %greater, label %take_a, label %take_b
take_a:
	br label %merge
take_b:
	br label %merge
merge:
	%result = phi i32 [ %a, %take_a ], [ %b, %take_b ]
	ret i32 %result
}

define i32 @count_down(i32 %n) #0 {
; CHECK-LABEL: count_down:
; CHECK: add	#-1,{{r[0-9]+}}
; CHECK: mov	#0,[[ZERO:r[0-9]+]]
; CHECK: cmp/eq	[[ZERO]],{{r[0-9]+}}
; CHECK: bf	[[LOOP:.LBB[0-9_]+]]
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{(mov(\.l)?|rts)}}
entry:
	br label %loop
loop:
	%value = phi i32 [ %n, %entry ], [ %next, %loop ]
	%next = add i32 %value, -1
	%continue = icmp ne i32 %next, 0
	br i1 %continue, label %loop, label %exit
exit:
	ret i32 %next
}

define i32 @stack_paths(i32 %a, i32 %b) #0 {
; CHECK-LABEL: stack_paths:
; CHECK: add	#-{{[0-9]+}},r15
; CHECK: mov.l	{{.*}}r15
; CHECK: cmp/gt	r5,r4
; CHECK: mov.l	{{.*}}r15
; CHECK: add	#{{[0-9]+}},r15
; CHECK: rts
; CHECK-NEXT: nop
; CHECK: mov.l	{{.*}}r15
; CHECK: add	#{{[0-9]+}},r15
; CHECK: rts
; CHECK-NEXT: nop
entry:
	%slot = alloca i32, align 4
	store volatile i32 %a, ptr %slot, align 4
	%greater = icmp sgt i32 %a, %b
	br i1 %greater, label %left, label %right
left:
	%old = load volatile i32, ptr %slot, align 4
	%next = add i32 %old, 1
	store volatile i32 %next, ptr %slot, align 4
	ret i32 %next
right:
	store volatile i32 %b, ptr %slot, align 4
	%result = load volatile i32, ptr %slot, align 4
	ret i32 %result
}

attributes #0 = { noinline optnone }
