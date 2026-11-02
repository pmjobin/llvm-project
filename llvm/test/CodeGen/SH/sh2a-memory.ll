; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2a < %s | FileCheck %s --check-prefixes=CHECK,SH2A
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2a < %s | FileCheck %s --check-prefixes=CHECK,SH2A
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -mtune=sh2a < %s | FileCheck %s --check-prefixes=CHECK,SH2

define signext i8 @load_i8(ptr %base) {
; CHECK-LABEL: load_i8:
; SH2A: mov.b @(2048,r4),r0
; SH2-NOT: mov.b @(2048,r4),r0
	%ptr = getelementptr i8, ptr %base, i32 2048
	%value = load i8, ptr %ptr, align 1
	ret i8 %value
}

define zeroext i8 @load_u8(ptr %base) {
; CHECK-LABEL: load_u8:
; SH2A: movu.b @(4095,r4),r0
; SH2-NOT: movu.b
	%ptr = getelementptr i8, ptr %base, i32 4095
	%value = load i8, ptr %ptr, align 1
	ret i8 %value
}

define signext i16 @load_i16(ptr %base) {
; CHECK-LABEL: load_i16:
; SH2A: mov.w @(8190,r4),r0
; SH2-NOT: mov.w @(8190,r4),r0
	%ptr = getelementptr i8, ptr %base, i32 8190
	%value = load i16, ptr %ptr, align 2
	ret i16 %value
}

define zeroext i16 @load_u16(ptr %base) {
; CHECK-LABEL: load_u16:
; SH2A: movu.w @(4096,r4),r0
; SH2-NOT: movu.w
	%ptr = getelementptr i8, ptr %base, i32 4096
	%value = load i16, ptr %ptr, align 2
	ret i16 %value
}

define i32 @load_i32(ptr %base) {
; CHECK-LABEL: load_i32:
; SH2A: mov.l @(16380,r4),r0
; SH2-NOT: mov.l @(16380,r4),r0
	%ptr = getelementptr i8, ptr %base, i32 16380
	%value = load i32, ptr %ptr, align 4
	ret i32 %value
}

define void @store_i8(ptr %base, i8 %value) {
; CHECK-LABEL: store_i8:
; SH2A: mov.b r5,@(4095,r4)
; SH2-NOT: mov.b r5,@(4095,r4)
	%ptr = getelementptr i8, ptr %base, i32 4095
	store i8 %value, ptr %ptr, align 1
	ret void
}

define void @store_i16(ptr %base, i16 %value) {
; CHECK-LABEL: store_i16:
; SH2A: mov.w r5,@(8190,r4)
; SH2-NOT: mov.w r5,@(8190,r4)
	%ptr = getelementptr i8, ptr %base, i32 8190
	store i16 %value, ptr %ptr, align 2
	ret void
}

define void @store_i32(ptr %base, i32 %value) {
; CHECK-LABEL: store_i32:
; SH2A: mov.l r5,@(16380,r4)
; SH2-NOT: mov.l r5,@(16380,r4)
	%ptr = getelementptr i8, ptr %base, i32 16380
	store i32 %value, ptr %ptr, align 4
	ret void
}

define float @load_f32(ptr %base) {
; CHECK-LABEL: load_f32:
; SH2A: fmov.s @(16380,r4),fr0
; SH2-NOT: fmov
	%ptr = getelementptr i8, ptr %base, i32 16380
	%value = load float, ptr %ptr, align 4
	ret float %value
}

define double @load_f64(ptr %base) {
; CHECK-LABEL: load_f64:
; SH2A: fmov.d @(32760,r4),dr0
; SH2-NOT: fmov
	%ptr = getelementptr i8, ptr %base, i32 32760
	%value = load double, ptr %ptr, align 8
	ret double %value
}

define void @store_f32(ptr %base, float %value) {
; CHECK-LABEL: store_f32:
; SH2A: fmov.s fr{{[45]}},@(16380,r4)
; SH2-NOT: fmov
	%ptr = getelementptr i8, ptr %base, i32 16380
	store float %value, ptr %ptr, align 4
	ret void
}

define void @store_f64(ptr %base, double %value) {
; CHECK-LABEL: store_f64:
; SH2A: fmov.d dr{{[46]}},@(32760,r4)
; SH2-NOT: fmov
	%ptr = getelementptr i8, ptr %base, i32 32760
	store double %value, ptr %ptr, align 8
	ret void
}
