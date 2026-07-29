; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,LE

%S1 = type { i8 }
%S2 = type { i16 }
%S3 = type { i8, i8, i8 }
%S5 = type { i32, i8 }
%S8 = type { i32, i32 }
%S12 = type { i32, i32, i32 }
%M = type { i8, i16, i32 }
%N = type { %S5, i16 }
%P = type <{ i8, i32 }>
%U4 = type { [4 x i8] }
%U8 = type { [2 x i32] }
%A7 = type [7 x i8]

; CHECK-LABEL: use_s1:
; CHECK: extu.b	r4,r0
; CHECK-NEXT: rts
define i32 @use_s1(%S1 %x) {
	%a = extractvalue %S1 %x, 0
	%result = zext i8 %a to i32
	ret i32 %result
}

; The first aggregate word uses the last argument register, its second word
; uses stack word zero, and the following scalar uses stack word one.
;
; CHECK-LABEL: use_split:
; BE: mov	r7,r0
; BE: mov.l	@r15,
; BE: mov.l	@(4,r15),
; LE: mov.l	r7,@r15
; LE: mov.b	@(4,r15),
; LE: mov.l	@(8,r15),
; CHECK: rts
define i32 @use_split(i32 %a, i32 %b, i32 %c, %S5 %x, i32 %z) {
	%x0 = extractvalue %S5 %x, 0
	%x1 = extractvalue %S5 %x, 1
	%e = zext i8 %x1 to i32
	%q = add i32 %x0, %e
	%result = add i32 %q, %z
	ret i32 %result
}

; CHECK-LABEL: call_split:
; CHECK: add	#-8,r15
; CHECK: mov.l	{{.*}},@(4,r15)
; CHECK: mov.l	{{.*}},@r15
; CHECK: mov	#1,r4
; CHECK: mov	#2,r5
; CHECK: mov	#3,r6
; CHECK: jsr	@
; CHECK-NEXT: nop
; CHECK: add	#8,r15
define i32 @call_split(%S5 %x) {
	%result = call i32 @use_split(i32 1, i32 2, i32 3, %S5 %x, i32 4)
	ret i32 %result
}

; A standalone subword stack argument is right-adjusted in its ABI slot for
; big endian and begins at slot offset zero for little endian.
;
; BE-LABEL: use_stack_s2:
; BE: mov	r15,r0
; BE-NEXT: add	#2,r0
; BE-NEXT: mov.w	@r0,r0
; LE-LABEL: use_stack_s2:
; LE: mov	r15,r0
; LE-NEXT: mov.w	@r0,r0
define i32 @use_stack_s2(i32 %a, i32 %b, i32 %c, i32 %d, %S2 %x) {
	%x0 = extractvalue %S2 %x, 0
	%result = zext i16 %x0 to i32
	ret i32 %result
}

; BE-LABEL: call_stack_s2:
; BE: add	#-4,r15
; BE: add	#2,
; BE: mov.w	{{.*}},@
; LE-LABEL: call_stack_s2:
; LE: add	#-4,r15
; LE: mov.w	{{.*}},@{{.*}}
define i32 @call_stack_s2(%S2 %x) {
	%result = call i32 @use_stack_s2(i32 1, i32 2, i32 3, i32 4, %S2 %x)
	ret i32 %result
}

define i32 @mixed_signature(i32 %a, i64 %wide, %S5 %x, ptr %p) {
	%lo = trunc i64 %wide to i32
	%x0 = extractvalue %S5 %x, 0
	%pi = ptrtoint ptr %p to i32
	%q = add i32 %a, %lo
	%r = add i32 %q, %x0
	%result = add i32 %r, %pi
	ret i32 %result
}

define i32 @call_mixed_signature(i32 %a, i64 %wide, %S5 %x, ptr %p) {
	%result = call i32 @mixed_signature(i32 %a, i64 %wide, %S5 %x, ptr %p)
	ret i32 %result
}

define i32 @nested_and_array(%N %n, %A7 %a, %P %p, %U4 %u4, %U8 %u8) {
	%n0 = extractvalue %N %n, 0, 0
	%a6 = extractvalue %A7 %a, 6
	%p1 = extractvalue %P %p, 1
	%u = extractvalue %U8 %u8, 0, 1
	%e = zext i8 %a6 to i32
	%q = add i32 %n0, %e
	%r = add i32 %q, %p1
	%result = add i32 %r, %u
	ret i32 %result
}

define i32 @several_aggregates(%S3 %a, %S8 %b, %S12 %c, %M %m) {
	%a0 = extractvalue %S3 %a, 0
	%b1 = extractvalue %S8 %b, 1
	%c2 = extractvalue %S12 %c, 2
	%m2 = extractvalue %M %m, 2
	%e = zext i8 %a0 to i32
	%q = add i32 %e, %b1
	%r = add i32 %q, %c2
	%result = add i32 %r, %m2
	ret i32 %result
}
