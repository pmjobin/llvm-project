; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s

; Range assertions introduced when the computed address crosses a basic-block
; boundary are not machine instructions and must not prevent selecting the
; canonical register-indirect memory forms.

; CHECK-LABEL: store_computed_i8:
; CHECK: mov.b	{{r[0-9]+}},@{{r[0-9]+}}
define void @store_computed_i8(i32 %address, i8 %value, i1 %condition) {
entry:
	%scaled = lshr i32 %address, 3
	%computed = or i32 %scaled, 536870912
	br i1 %condition, label %access, label %exit

access:
	%pointer = inttoptr i32 %computed to ptr
	store i8 %value, ptr %pointer, align 1
	br label %exit

exit:
	ret void
}

; CHECK-LABEL: store_computed_i16:
; CHECK: mov.w	{{r[0-9]+}},@{{r[0-9]+}}
define void @store_computed_i16(i32 %address, i16 %value, i1 %condition) {
entry:
	%scaled = lshr i32 %address, 3
	%computed = or i32 %scaled, 536870912
	br i1 %condition, label %access, label %exit

access:
	%pointer = inttoptr i32 %computed to ptr
	store i16 %value, ptr %pointer, align 2
	br label %exit

exit:
	ret void
}

; CHECK-LABEL: store_computed_i32:
; CHECK: mov.l	{{r[0-9]+}},@{{r[0-9]+}}
define void @store_computed_i32(i32 %address, i32 %value, i1 %condition) {
entry:
	%scaled = lshr i32 %address, 3
	%computed = or i32 %scaled, 536870912
	br i1 %condition, label %access, label %exit

access:
	%pointer = inttoptr i32 %computed to ptr
	store i32 %value, ptr %pointer, align 4
	br label %exit

exit:
	ret void
}

; CHECK-LABEL: load_computed_i8:
; CHECK: mov.b	@{{r[0-9]+}},{{r[0-9]+}}
define i8 @load_computed_i8(i32 %address, i1 %condition) {
entry:
	%scaled = lshr i32 %address, 3
	%computed = or i32 %scaled, 536870912
	br i1 %condition, label %access, label %exit

access:
	%pointer = inttoptr i32 %computed to ptr
	%value = load i8, ptr %pointer, align 1
	br label %exit

exit:
	%result = phi i8 [ %value, %access ], [ 0, %entry ]
	ret i8 %result
}

; CHECK-LABEL: load_computed_i16:
; CHECK: mov.w	@{{r[0-9]+}},{{r[0-9]+}}
define i16 @load_computed_i16(i32 %address, i1 %condition) {
entry:
	%scaled = lshr i32 %address, 3
	%computed = or i32 %scaled, 536870912
	br i1 %condition, label %access, label %exit

access:
	%pointer = inttoptr i32 %computed to ptr
	%value = load i16, ptr %pointer, align 2
	br label %exit

exit:
	%result = phi i16 [ %value, %access ], [ 0, %entry ]
	ret i16 %result
}

; CHECK-LABEL: load_computed_i32:
; CHECK: mov.l	@{{r[0-9]+}},{{r[0-9]+}}
define i32 @load_computed_i32(i32 %address, i1 %condition) {
entry:
	%scaled = lshr i32 %address, 3
	%computed = or i32 %scaled, 536870912
	br i1 %condition, label %access, label %exit

access:
	%pointer = inttoptr i32 %computed to ptr
	%value = load i32, ptr %pointer, align 4
	br label %exit

exit:
	%result = phi i32 [ %value, %access ], [ 0, %entry ]
	ret i32 %result
}
