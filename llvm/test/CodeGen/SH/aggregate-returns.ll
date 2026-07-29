; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s

%S1 = type { i8 }
%S2 = type { i16 }
%S3 = type { i8, i8, i8 }
%S4 = type { i32 }
%A5 = type [5 x i8]
%S5 = type { i32, i8 }
%S8 = type { i32, i32 }
%S12 = type { i32, i32, i32 }
%P = type <{ i8, i32 }>

@global_s12 = global %S12 zeroinitializer, align 4

; CHECK-LABEL: return_s1:
; CHECK: extu.b	r4,r0
; CHECK-NEXT: rts
define %S1 @return_s1(%S1 %x) {
	ret %S1 %x
}

; CHECK-LABEL: return_s2:
; CHECK: extu.w	r4,r0
; CHECK-NEXT: rts
define %S2 @return_s2(%S2 %x) {
	ret %S2 %x
}

; CHECK-LABEL: return_s4:
; CHECK: mov	r4,r0
; CHECK-NEXT: rts
define %S4 @return_s4(%S4 %x) {
	ret %S4 %x
}

; Eight-byte aggregates return memory word zero in r0 and memory word one in
; r1 for both target endiannesses.
;
; CHECK-LABEL: return_s8:
; CHECK: mov	r5,r1
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: rts
define %S8 @return_s8(%S8 %x) {
	ret %S8 %x
}

; The C S5 layout has allocation size eight and alignment four, so it is the
; five-data-byte boundary that still returns directly in r0/r1.
;
; CHECK-LABEL: return_s5:
; CHECK: mov	r4,r0
; CHECK: {{.*}},r1
; CHECK: rts
define %S5 @return_s5(%S5 %x) {
	ret %S5 %x
}

; Three- and five-byte, insufficiently aligned aggregates are indirect. The
; hidden result pointer arrives in r2, does not consume r4, and is returned in
; r0.
;
; CHECK-LABEL: return_s3:
; CHECK: mov	r2,r0
; CHECK: mov.b	{{.*}},@{{.*}}
; CHECK: rts
define %S3 @return_s3(%S3 %x) {
	ret %S3 %x
}

; CHECK-LABEL: return_a5:
; CHECK: mov	r2,r0
; CHECK: mov.b
; CHECK: rts
define %A5 @return_a5(%A5 %x) {
	ret %A5 %x
}

define %P @return_packed(%P %x) {
	ret %P %x
}

define %P @call_packed(%P %x) {
	%result = call %P @return_packed(%P %x)
	ret %P %result
}

; CHECK-LABEL: explicit_sret:
; CHECK: mov	r2,r0
; CHECK: mov.l	r6,@(8,r2)
; CHECK: mov.l	r5,@(4,r2)
; CHECK: mov.l	r4,@r2
; CHECK: rts
define void @explicit_sret(ptr sret(%S12) align 4 %result, i32 %a, i32 %b, i32 %c) {
	%x0 = insertvalue %S12 poison, i32 %a, 0
	%x1 = insertvalue %S12 %x0, i32 %b, 1
	%x2 = insertvalue %S12 %x1, i32 %c, 2
	store %S12 %x2, ptr %result, align 4
	ret void
}

define void @call_explicit_sret(ptr %result) {
	call void @explicit_sret(ptr sret(%S12) align 4 %result, i32 1, i32 2, i32 3)
	ret void
}

; Forwarding the caller-owned result pointer requires no temporary aggregate
; or second copy.
;
; CHECK-LABEL: forward_explicit_sret:
; CHECK: add	#-4,r15
; CHECK: mov	r2,r0
; CHECK: mov.l	r0,@r15
; CHECK: jsr	@
; CHECK: mov.l	@r15,r0
; CHECK: add	#4,r15
define void @forward_explicit_sret(ptr sret(%S12) align 4 %result, i32 %a, i32 %b, i32 %c) {
	call void @explicit_sret(ptr sret(%S12) align 4 %result, i32 %a, i32 %b, i32 %c)
	ret void
}

define void @call_global_sret() {
	call void @explicit_sret(ptr sret(%S12) align 4 @global_s12, i32 1, i32 2, i32 3)
	ret void
}

define void @call_sret_indirectly(ptr %fn, ptr %result, i32 %value) {
	call void %fn(ptr sret(%S12) align 4 %result, i32 %value, i32 2, i32 3)
	ret void
}

define void @recursive_sret(ptr sret(%S12) align 4 %result, i32 %count) {
	%done = icmp eq i32 %count, 0
	br i1 %done, label %base, label %recurse

base:
	store %S12 zeroinitializer, ptr %result, align 4
	ret void

recurse:
	%next = sub i32 %count, 1
	call void @recursive_sret(ptr sret(%S12) align 4 %result, i32 %next)
	ret void
}

define %S12 @automatic_sret(i32 %a, i32 %b, i32 %c) {
	%x0 = insertvalue %S12 poison, i32 %a, 0
	%x1 = insertvalue %S12 %x0, i32 %b, 1
	%x2 = insertvalue %S12 %x1, i32 %c, 2
	ret %S12 %x2
}

define %S12 @call_automatic_sret(i32 %a, i32 %b, i32 %c) {
	%result = call %S12 @automatic_sret(i32 %a, i32 %b, i32 %c)
	ret %S12 %result
}

define %S8 @call_direct_indirectly(ptr %fn, %S8 %x) {
	%result = call %S8 %fn(%S8 %x)
	ret %S8 %result
}
