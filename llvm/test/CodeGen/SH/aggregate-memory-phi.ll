; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=greedy %s -o - | FileCheck %s --check-prefix=RA

%S8 = type { i32, i32 }
%M = type { i8, i16, i32 }
%P = type <{ i8, i32 }>
%A7 = type [7 x i8]

@global_s8 = global %S8 zeroinitializer, align 4

declare %S8 @produce(i32)

; CHECK-LABEL: copy_s8:
; CHECK: mov.l	@(4,r5),
; CHECK: mov.l	@r5,
; CHECK: mov.l	{{.*}},@r4
; CHECK: mov.l	{{.*}},@(4,r4)
define void @copy_s8(ptr %dst, ptr %src) {
	%value = load %S8, ptr %src, align 4
	store %S8 %value, ptr %dst, align 4
	ret void
}

; Packed fields are scalarized to byte operations; no misaligned mov.l is
; emitted for the i32 field at offset one.
;
; CHECK-LABEL: copy_packed:
; CHECK-COUNT-5: mov.b	@
; CHECK-NOT: mov.l	@
; CHECK: rts
define void @copy_packed(ptr %dst, ptr %src) {
	%value = load %P, ptr %src, align 1
	store %P %value, ptr %dst, align 1
	ret void
}

define void @copy_array(ptr %dst, ptr %src) {
	%value = load %A7, ptr %src, align 1
	store %A7 %value, ptr %dst, align 1
	ret void
}

define void @copy_m_volatile(ptr %dst, ptr %src) {
	%value = load volatile %M, ptr %src, align 4
	store volatile %M %value, ptr %dst, align 4
	ret void
}

define i32 @aggregate_diamond(i32 %choice, %S8 %left, %S8 %right) {
	%condition = icmp ne i32 %choice, 0
	br i1 %condition, label %take_left, label %take_right

take_left:
	br label %join

take_right:
	br label %join

join:
	%value = phi %S8 [ %left, %take_left ], [ %right, %take_right ]
	%result = extractvalue %S8 %value, 1
	ret i32 %result
}

; RA-LABEL: name:            aggregate_diamond
; RA: noPhis:          true
; RA: noVRegs:         true
; RA: registers:       []
; RA-NOT: PHI

define %S8 @aggregate_loop(i32 %count, %S8 %initial) {
	br label %loop

loop:
	%index = phi i32 [ %count, %0 ], [ %next, %loop ]
	%value = phi %S8 [ %initial, %0 ], [ %updated, %loop ]
	%field = extractvalue %S8 %value, 0
	%incremented = add i32 %field, 1
	%updated = insertvalue %S8 %value, i32 %incremented, 0
	%next = sub i32 %index, 1
	%again = icmp ne i32 %next, 0
	br i1 %again, label %loop, label %done

done:
	ret %S8 %updated
}

define %S8 @aggregate_phi_around_calls(i32 %choice) {
	%condition = icmp ne i32 %choice, 0
	br i1 %condition, label %left, label %right

left:
	%left_value = call %S8 @produce(i32 1)
	br label %join

right:
	%right_value = call %S8 @produce(i32 2)
	br label %join

join:
	%result = phi %S8 [ %left_value, %left ], [ %right_value, %right ]
	ret %S8 %result
}

define void @aggregate_phi_store(i32 %choice, %S8 %left, %S8 %right, ptr %dst) {
	%condition = icmp ne i32 %choice, 0
	br i1 %condition, label %take_left, label %take_right

take_left:
	br label %join

take_right:
	br label %join

join:
	%value = phi %S8 [ %left, %take_left ], [ %right, %take_right ]
	store %S8 %value, ptr %dst, align 4
	ret void
}

define i32 @global_roundtrip(%S8 %value) {
	store %S8 %value, ptr @global_s8, align 4
	%loaded = load %S8, ptr @global_s8, align 4
	%result = extractvalue %S8 %loaded, 1
	ret i32 %result
}
