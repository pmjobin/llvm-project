; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,LE

%S1 = type { i8 }
%S2 = type { i16 }
%S3 = type { i8, i8, i8 }
%S4 = type { i32 }
%A5 = type [5 x i8]
%S5 = type { i32, i8 }
%S8 = type { i32, i32 }
%S12 = type { i32, i32, i32 }
%P = type <{ i8, i32 }>
%Nested4 = type { %S2, i16 }
%PointerByte = type <{ ptr, i8, [3 x i8] }>

@global_s12 = global %S12 zeroinitializer, align 4

; CHECK-LABEL: return_s1:
; CHECK: rts
; CHECK-NEXT: extu.b	r4,r0
define %S1 @return_s1(%S1 %x) {
	ret %S1 %x
}

; CHECK-LABEL: return_s2:
; CHECK: rts
; CHECK-NEXT: extu.w	r4,r0
define %S2 @return_s2(%S2 %x) {
	ret %S2 %x
}

; CHECK-LABEL: return_s4:
; CHECK: rts
; CHECK-NEXT: mov	r4,r0
define %S4 @return_s4(%S4 %x) {
	ret %S4 %x
}

; LLVM aggregate types do not preserve an explicitly increased source-record
; alignment. Clang emits this direct form only after validating the source
; alignment, so its four physical bytes still return in r0.
;
; CHECK-LABEL: return_nested4:
; CHECK: rts
; CHECK-NEXT: mov	r4,r0
define %Nested4 @return_nested4(%Nested4 %x) {
	ret %Nested4 %x
}

; Eight-byte aggregates return memory word zero in r0 and memory word one in
; r1 for both target endiannesses.
;
; CHECK-LABEL: return_s8:
; CHECK: mov	r5,r1
; CHECK-NEXT: rts
; CHECK-NEXT: mov	r4,r0
define %S8 @return_s8(%S8 %x) {
	ret %S8 %x
}

; More than two legalized IR leaves may still occupy exactly two ABI words.
; The pointer is memory word zero. The byte begins memory word one, and the
; explicit padding must neither become another ABI part nor move the byte.
;
; LE-LABEL: return_pointer_byte:
; LE: mov	r4,r0
; LE: rts
; LE-NEXT: extu.b	r5,r1
; BE-LABEL: return_pointer_byte:
; BE: mov	r5,r1
; BE-NEXT: mov	r4,r0
; BE-NEXT: shll16	r1
; BE-NEXT: rts
; BE-NEXT: shll8	r1
define %PointerByte @return_pointer_byte(ptr %pointer, i8 %byte) {
	%with_pointer = insertvalue %PointerByte poison, ptr %pointer, 0
	%with_byte = insertvalue %PointerByte %with_pointer, i8 %byte, 1
	ret %PointerByte %with_byte
}

; LE-LABEL: call_pointer_byte:
; LE: jsr	@
; LE: extu.b	r1,r0
; BE-LABEL: call_pointer_byte:
; BE: jsr	@
; BE: mov	r1,r0
; BE-NEXT: shlr16	r0
; BE-NEXT: shlr8	r0
define i8 @call_pointer_byte(ptr %pointer, i8 %byte) {
	%result = call %PointerByte @return_pointer_byte(ptr %pointer, i8 %byte)
	%result_byte = extractvalue %PointerByte %result, 1
	ret i8 %result_byte
}

; The C S5 layout has allocation size eight and alignment four, so it is the
; five-data-byte boundary that still returns directly in r0/r1.
;
; CHECK-LABEL: return_s5:
; CHECK: mov	r4,r0
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
; Independent fields can be scheduled in either order, including a return slot.
; CHECK-DAG: mov.l	r6,@(8,r2)
; CHECK-DAG: mov.l	r5,@(4,r2)
; CHECK-DAG: mov.l	r4,@r2
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
