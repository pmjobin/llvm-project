; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O1 < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O1 < %s | FileCheck %s

declare void @escape(ptr)
declare i32 @callee0()
declare i32 @callee5(i32, i32, i32, i32, i32)
declare i32 @callee8(i32, i32, i32, i32, i32, i32, i32, i32)

; A frame-pointer-based CFA is independent of temporary outgoing-stack space.
; CHECK-LABEL: outgoing_zero:
; CHECK: .cfi_def_cfa r14,
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: jsr
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: .cfi_def_cfa r15,
define i32 @outgoing_zero(i32 %value) #0 {
	%local = alloca [3 x i32], align 4
	store volatile i32 %value, ptr %local, align 4
	call void @escape(ptr %local)
	%result = call i32 @callee0()
	%loaded = load volatile i32, ptr %local, align 4
	%sum = add i32 %result, %loaded
	ret i32 %sum
}

; CHECK-LABEL: outgoing_first:
; CHECK: .cfi_def_cfa r14,
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: add #-4,r15
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: jsr
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: add #4,r15
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: .cfi_def_cfa r15,
define i32 @outgoing_first(i32 %value) #0 {
	%local = alloca [3 x i32], align 4
	store volatile i32 %value, ptr %local, align 4
	call void @escape(ptr %local)
	%result = call i32 @callee5(i32 %value, i32 2, i32 3, i32 4, i32 5)
	%loaded = load volatile i32, ptr %local, align 4
	%sum = add i32 %result, %loaded
	ret i32 %sum
}

; CHECK-LABEL: outgoing_multiple:
; CHECK: .cfi_def_cfa r14,
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: add #-16,r15
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: jsr
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: add #16,r15
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: .cfi_def_cfa r15,
define i32 @outgoing_multiple(i32 %value) #0 {
	%local = alloca [3 x i32], align 4
	store volatile i32 %value, ptr %local, align 4
	call void @escape(ptr %local)
	%result = call i32 @callee8(i32 %value, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	%loaded = load volatile i32, ptr %local, align 4
	%sum = add i32 %result, %loaded
	ret i32 %sum
}

; Dynamic stack allocation requires r14 to anchor the CFA. Outgoing arguments
; still must not change that r14-relative rule.
; CHECK-LABEL: outgoing_dynamic:
; CHECK: .cfi_def_cfa r14,
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: add #-4,r15
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: jsr
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: add #4,r15
; CHECK-NOT: .cfi_adjust_cfa_offset
; CHECK: .cfi_def_cfa r15,
define i32 @outgoing_dynamic(i32 %count, i32 %value) #1 {
	%local = alloca i32, i32 %count, align 4
	store volatile i32 %value, ptr %local, align 4
	call void @escape(ptr %local)
	%result = call i32 @callee5(i32 %value, i32 2, i32 3, i32 4, i32 5)
	%loaded = load volatile i32, ptr %local, align 4
	%sum = add i32 %result, %loaded
	ret i32 %sum
}

; An r15-based CFA must continue to track temporary outgoing-stack space.
; CHECK-LABEL: outgoing_frameless:
; CHECK-NOT: .cfi_def_cfa r14,
; CHECK: .cfi_offset r8,
; CHECK: add #-4,r15
; CHECK-NEXT: .cfi_adjust_cfa_offset 4
; CHECK: jsr
; CHECK: add #4,r15
; CHECK-NEXT: .cfi_adjust_cfa_offset -4
; CHECK-NOT: .cfi_def_cfa r14,
define i32 @outgoing_frameless(i32 %value) #2 {
	%result = call i32 @callee5(i32 %value, i32 2, i32 3, i32 4, i32 5)
	%sum = add i32 %result, %value
	ret i32 %sum
}

attributes #0 = { noinline nounwind uwtable(sync) "frame-pointer"="all" }
attributes #1 = { noinline nounwind uwtable(sync) }
attributes #2 = { noinline nounwind uwtable(sync) "frame-pointer"="none" }
