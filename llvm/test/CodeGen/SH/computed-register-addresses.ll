; Arbitrary GPR producers remain legal independently of folded address shapes.
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh1 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh1 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2a -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2a -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -verify-machineinstrs < %s | FileCheck %s

@marker = external global i32

define i8 @xor_i8_load_same(i32 %a, i32 %b, i1 %choose, i8 %value) #0 {
; CHECK-LABEL: xor_i8_load_same:
; CHECK: xor {{r[0-9]+}},[[ADDR_XOR_I8_LOAD_SAME:r[0-9]+]]
; CHECK: mov.b @[[ADDR_XOR_I8_LOAD_SAME]],{{(r|fr|dr)[0-9]+}}
entry:
	%address = xor i32 %a, %b
	%p = inttoptr i32 %address to ptr
	%v = load volatile i8, ptr %p, align 4
	ret i8 %v
}

define i8 @xor_i8_load_cross(i32 %a, i32 %b, i1 %choose, i8 %value) #0 {
; CHECK-LABEL: xor_i8_load_cross:
; CHECK: mov.b @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = xor i32 %a, %b
	br i1 %choose, label %use, label %side
side:
	store volatile i32 %a, ptr @marker
	br label %use
use:
	%p = inttoptr i32 %address to ptr
	%v = load volatile i8, ptr %p, align 4
	ret i8 %v
}

define void @xor_i8_store_same(i32 %a, i32 %b, i1 %choose, i8 %value) #0 {
; CHECK-LABEL: xor_i8_store_same:
; CHECK: xor {{r[0-9]+}},[[ADDR_XOR_I8_STORE_SAME:r[0-9]+]]
; CHECK: mov.b {{(r|fr|dr)[0-9]+}},@[[ADDR_XOR_I8_STORE_SAME]]
entry:
	%address = xor i32 %a, %b
	%p = inttoptr i32 %address to ptr
	store volatile i8 %value, ptr %p, align 4
	ret void
}

define void @xor_i8_store_cross(i32 %a, i32 %b, i1 %choose, i8 %value) #0 {
; CHECK-LABEL: xor_i8_store_cross:
; CHECK: mov.b {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%address = xor i32 %a, %b
	br i1 %choose, label %use, label %side
side:
	store volatile i32 %a, ptr @marker
	br label %use
use:
	%p = inttoptr i32 %address to ptr
	store volatile i8 %value, ptr %p, align 4
	ret void
}

define i16 @xor_i16_load_same(i32 %a, i32 %b, i1 %choose, i16 %value) #0 {
; CHECK-LABEL: xor_i16_load_same:
; CHECK: xor {{r[0-9]+}},[[ADDR_XOR_I16_LOAD_SAME:r[0-9]+]]
; CHECK: mov.w @[[ADDR_XOR_I16_LOAD_SAME]],{{(r|fr|dr)[0-9]+}}
entry:
	%address = xor i32 %a, %b
	%p = inttoptr i32 %address to ptr
	%v = load volatile i16, ptr %p, align 4
	ret i16 %v
}

define i16 @xor_i16_load_cross(i32 %a, i32 %b, i1 %choose, i16 %value) #0 {
; CHECK-LABEL: xor_i16_load_cross:
; CHECK: mov.w @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = xor i32 %a, %b
	br i1 %choose, label %use, label %side
side:
	store volatile i32 %a, ptr @marker
	br label %use
use:
	%p = inttoptr i32 %address to ptr
	%v = load volatile i16, ptr %p, align 4
	ret i16 %v
}

define void @xor_i16_store_same(i32 %a, i32 %b, i1 %choose, i16 %value) #0 {
; CHECK-LABEL: xor_i16_store_same:
; CHECK: xor {{r[0-9]+}},[[ADDR_XOR_I16_STORE_SAME:r[0-9]+]]
; CHECK: mov.w {{(r|fr|dr)[0-9]+}},@[[ADDR_XOR_I16_STORE_SAME]]
entry:
	%address = xor i32 %a, %b
	%p = inttoptr i32 %address to ptr
	store volatile i16 %value, ptr %p, align 4
	ret void
}

define void @xor_i16_store_cross(i32 %a, i32 %b, i1 %choose, i16 %value) #0 {
; CHECK-LABEL: xor_i16_store_cross:
; CHECK: mov.w {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%address = xor i32 %a, %b
	br i1 %choose, label %use, label %side
side:
	store volatile i32 %a, ptr @marker
	br label %use
use:
	%p = inttoptr i32 %address to ptr
	store volatile i16 %value, ptr %p, align 4
	ret void
}

define i32 @xor_i32_load_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: xor_i32_load_same:
; CHECK: xor {{r[0-9]+}},[[ADDR_XOR_I32_LOAD_SAME:r[0-9]+]]
; CHECK: mov.l @[[ADDR_XOR_I32_LOAD_SAME]],{{(r|fr|dr)[0-9]+}}
entry:
	%address = xor i32 %a, %b
	%p = inttoptr i32 %address to ptr
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define i32 @xor_i32_load_cross(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: xor_i32_load_cross:
; CHECK: mov.l @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = xor i32 %a, %b
	br i1 %choose, label %use, label %side
side:
	store volatile i32 %a, ptr @marker
	br label %use
use:
	%p = inttoptr i32 %address to ptr
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define void @xor_i32_store_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: xor_i32_store_same:
; CHECK: xor {{r[0-9]+}},[[ADDR_XOR_I32_STORE_SAME:r[0-9]+]]
; CHECK: mov.l {{(r|fr|dr)[0-9]+}},@[[ADDR_XOR_I32_STORE_SAME]]
entry:
	%address = xor i32 %a, %b
	%p = inttoptr i32 %address to ptr
	store volatile i32 %value, ptr %p, align 4
	ret void
}

define void @xor_i32_store_cross(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: xor_i32_store_cross:
; CHECK: mov.l {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%address = xor i32 %a, %b
	br i1 %choose, label %use, label %side
side:
	store volatile i32 %a, ptr @marker
	br label %use
use:
	%p = inttoptr i32 %address to ptr
	store volatile i32 %value, ptr %p, align 4
	ret void
}

define float @xor_float_load_same(i32 %a, i32 %b, i1 %choose, float %value) #0 {
; CHECK-LABEL: xor_float_load_same:
; CHECK: xor {{r[0-9]+}},[[ADDR_XOR_FLOAT_LOAD_SAME:r[0-9]+]]
; CHECK: {{(fmov|mov.l)}} @[[ADDR_XOR_FLOAT_LOAD_SAME]],{{(r|fr|dr)[0-9]+}}
entry:
	%address = xor i32 %a, %b
	%p = inttoptr i32 %address to ptr
	%v = load volatile float, ptr %p, align 4
	ret float %v
}

define float @xor_float_load_cross(i32 %a, i32 %b, i1 %choose, float %value) #0 {
; CHECK-LABEL: xor_float_load_cross:
; CHECK: {{(fmov|mov.l)}} @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = xor i32 %a, %b
	br i1 %choose, label %use, label %side
side:
	store volatile i32 %a, ptr @marker
	br label %use
use:
	%p = inttoptr i32 %address to ptr
	%v = load volatile float, ptr %p, align 4
	ret float %v
}

define void @xor_float_store_same(i32 %a, i32 %b, i1 %choose, float %value) #0 {
; CHECK-LABEL: xor_float_store_same:
; CHECK: xor {{r[0-9]+}},[[ADDR_XOR_FLOAT_STORE_SAME:r[0-9]+]]
; CHECK: {{(fmov|mov.l)}} {{(r|fr|dr)[0-9]+}},@[[ADDR_XOR_FLOAT_STORE_SAME]]
entry:
	%address = xor i32 %a, %b
	%p = inttoptr i32 %address to ptr
	store volatile float %value, ptr %p, align 4
	ret void
}

define void @xor_float_store_cross(i32 %a, i32 %b, i1 %choose, float %value) #0 {
; CHECK-LABEL: xor_float_store_cross:
; CHECK: {{(fmov|mov.l)}} {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%address = xor i32 %a, %b
	br i1 %choose, label %use, label %side
side:
	store volatile i32 %a, ptr @marker
	br label %use
use:
	%p = inttoptr i32 %address to ptr
	store volatile float %value, ptr %p, align 4
	ret void
}

define double @xor_double_load_same(i32 %a, i32 %b, i1 %choose, double %value) #0 {
; CHECK-LABEL: xor_double_load_same:
; CHECK: xor {{r[0-9]+}},[[ADDR_XOR_DOUBLE_LOAD_SAME:r[0-9]+]]
; CHECK: {{(fmov(\.[sd])?|mov.l)}} @[[ADDR_XOR_DOUBLE_LOAD_SAME]],{{(r|fr|dr)[0-9]+}}
entry:
	%address = xor i32 %a, %b
	%p = inttoptr i32 %address to ptr
	%v = load volatile double, ptr %p, align 4
	ret double %v
}

define double @xor_double_load_cross(i32 %a, i32 %b, i1 %choose, double %value) #0 {
; CHECK-LABEL: xor_double_load_cross:
; CHECK: {{(fmov(\.[sd])?|mov.l)}} @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = xor i32 %a, %b
	br i1 %choose, label %use, label %side
side:
	store volatile i32 %a, ptr @marker
	br label %use
use:
	%p = inttoptr i32 %address to ptr
	%v = load volatile double, ptr %p, align 4
	ret double %v
}

define void @xor_double_store_same(i32 %a, i32 %b, i1 %choose, double %value) #0 {
; CHECK-LABEL: xor_double_store_same:
; CHECK: xor {{r[0-9]+}},[[ADDR_XOR_DOUBLE_STORE_SAME:r[0-9]+]]
; CHECK: {{(fmov(\.[sd])?|mov.l)}} {{(r|fr|dr)[0-9]+}},@[[ADDR_XOR_DOUBLE_STORE_SAME]]
entry:
	%address = xor i32 %a, %b
	%p = inttoptr i32 %address to ptr
	store volatile double %value, ptr %p, align 4
	ret void
}

define void @xor_double_store_cross(i32 %a, i32 %b, i1 %choose, double %value) #0 {
; CHECK-LABEL: xor_double_store_cross:
; CHECK: {{(fmov(\.[sd])?|mov.l)}} {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%address = xor i32 %a, %b
	br i1 %choose, label %use, label %side
side:
	store volatile i32 %a, ptr @marker
	br label %use
use:
	%p = inttoptr i32 %address to ptr
	store volatile double %value, ptr %p, align 4
	ret void
}

define i32 @shl_i32_load_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: shl_i32_load_same:
; CHECK: mov.l @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = shl i32 %a, 3
	%p = inttoptr i32 %address to ptr
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define void @shl_i32_store_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: shl_i32_store_same:
; CHECK: mov.l {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%address = shl i32 %a, 3
	%p = inttoptr i32 %address to ptr
	store volatile i32 %value, ptr %p, align 4
	ret void
}

define i32 @srl_i32_load_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: srl_i32_load_same:
; CHECK: mov.l @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = lshr i32 %a, 3
	%p = inttoptr i32 %address to ptr
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define void @srl_i32_store_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: srl_i32_store_same:
; CHECK: mov.l {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%address = lshr i32 %a, 3
	%p = inttoptr i32 %address to ptr
	store volatile i32 %value, ptr %p, align 4
	ret void
}

define i32 @mul_i32_load_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: mul_i32_load_same:
; CHECK: mov.l @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = mul i32 %a, %b
	%p = inttoptr i32 %address to ptr
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define void @mul_i32_store_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: mul_i32_store_same:
; CHECK: mov.l {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%address = mul i32 %a, %b
	%p = inttoptr i32 %address to ptr
	store volatile i32 %value, ptr %p, align 4
	ret void
}

define i32 @or_i32_load_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: or_i32_load_same:
; CHECK: mov.l @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = or i32 %a, %b
	%p = inttoptr i32 %address to ptr
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define void @or_i32_store_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: or_i32_store_same:
; CHECK: mov.l {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%address = or i32 %a, %b
	%p = inttoptr i32 %address to ptr
	store volatile i32 %value, ptr %p, align 4
	ret void
}

define i32 @add_i32_load_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: add_i32_load_same:
; CHECK: mov.l @{{(r[0-9]+|\(r0,r[0-9]+\))}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = add i32 %a, %b
	%p = inttoptr i32 %address to ptr
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define void @add_i32_store_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: add_i32_store_same:
; CHECK: mov.l {{(r|fr|dr)[0-9]+}},@{{(r[0-9]+|\(r0,r[0-9]+\))}}
entry:
	%address = add i32 %a, %b
	%p = inttoptr i32 %address to ptr
	store volatile i32 %value, ptr %p, align 4
	ret void
}

define i32 @and_i32_load_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: and_i32_load_same:
; CHECK: mov.l @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = and i32 %a, %b
	%p = inttoptr i32 %address to ptr
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define void @and_i32_store_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: and_i32_store_same:
; CHECK: mov.l {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%address = and i32 %a, %b
	%p = inttoptr i32 %address to ptr
	store volatile i32 %value, ptr %p, align 4
	ret void
}

define i32 @select_i32_load_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: select_i32_load_same:
; CHECK: mov.l @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%address = select i1 %choose, i32 %a, i32 %b
	%p = inttoptr i32 %address to ptr
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define void @select_i32_store_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: select_i32_store_same:
; CHECK: mov.l {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%address = select i1 %choose, i32 %a, i32 %b
	%p = inttoptr i32 %address to ptr
	store volatile i32 %value, ptr %p, align 4
	ret void
}

define i32 @phi_i32_load_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: phi_i32_load_same:
; CHECK: mov.l @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	br i1 %choose, label %left, label %right
left:
	%x = xor i32 %a, %b
	br label %merged
right:
	%y = shl i32 %a, 3
	br label %merged
merged:
	%address = phi i32 [ %x, %left ], [ %y, %right ]
	%p = inttoptr i32 %address to ptr
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define void @phi_i32_store_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: phi_i32_store_same:
; CHECK: mov.l {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	br i1 %choose, label %left, label %right
left:
	%x = xor i32 %a, %b
	br label %merged
right:
	%y = shl i32 %a, 3
	br label %merged
merged:
	%address = phi i32 [ %x, %left ], [ %y, %right ]
	%p = inttoptr i32 %address to ptr
	store volatile i32 %value, ptr %p, align 4
	ret void
}

define i32 @nested_i32_load_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: nested_i32_load_same:
; CHECK: mov.l @{{r[0-9]+}},{{(r|fr|dr)[0-9]+}}
entry:
	%shifted = shl i32 %a, 3
	%address = xor i32 %shifted, %b
	%p = inttoptr i32 %address to ptr
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define void @nested_i32_store_same(i32 %a, i32 %b, i1 %choose, i32 %value) #0 {
; CHECK-LABEL: nested_i32_store_same:
; CHECK: mov.l {{(r|fr|dr)[0-9]+}},@{{r[0-9]+}}
entry:
	%shifted = shl i32 %a, 3
	%address = xor i32 %shifted, %b
	%p = inttoptr i32 %address to ptr
	store volatile i32 %value, ptr %p, align 4
	ret void
}

attributes #0 = { noinline nounwind optsize }
