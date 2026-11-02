; RUN: llc -O3 -mtriple=shle -mcpu=sh2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O3 -mtriple=sh -mcpu=sh2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O3 -mtriple=shle -mcpu=sh3 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O3 -mtriple=sh -mcpu=sh3 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O3 -mtriple=shle -mcpu=sh4 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O3 -mtriple=sh -mcpu=sh4 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O3 -mtriple=shle -mcpu=sh2 -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -O3 -mtriple=sh -mcpu=sh3 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -O3 -mtriple=sh2-linux-muslfdpic -mcpu=sh4 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -O3 -mtriple=sh -mcpu=sh4 -target-abi=gnu -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -O3 -mtriple=shle -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -O0 -mtriple=shle -mcpu=sh2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=O0

; The frame base remains a normal LEA_FI. Only the single-use scaled index is
; constrained to R0. Calls and volatile accesses retain their dependencies.
declare void @fill(ptr)
declare i32 @consume(ptr, i32, i32, i32, i32, i32)

define i32 @load_word(i32 %i) {
; CHECK-LABEL: name: load_word
; CHECK: LEA_FI %stack.0.a, 0
; CHECK: $r0 = COPY
; CHECK-NEXT: {{%[0-9]+}}:gpr = MOVL_load_indexed {{%[0-9]+}}, implicit $r0 :: (volatile load (s32)
; O0-LABEL: name: load_word
; O0-NOT: MOVL_load_indexed
; O0: MOVL_load_reg
entry:
	%a = alloca [32 x i32], align 4
	call void @fill(ptr %a)
	%p = getelementptr [32 x i32], ptr %a, i32 0, i32 %i
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define i32 @load_half(i32 %i) {
; CHECK-LABEL: name: load_half
; The existing anyext-plus-EXTUW legalization remains on its fallback path.
; CHECK: MOVW_load_reg {{.*}} :: (volatile load (s16)
; CHECK: EXTUW
entry:
	%a = alloca [32 x i16], align 4
	call void @fill(ptr %a)
	%p = getelementptr [32 x i16], ptr %a, i32 0, i32 %i
	%v = load volatile i16, ptr %p, align 2
	%r = zext i16 %v to i32
	ret i32 %r
}

define i32 @load_signed_half(i32 %i) {
; CHECK-LABEL: name: load_signed_half
; The signed extension also preserves the existing legalization fallback.
; CHECK: MOVW_load_reg {{.*}} :: (volatile load (s16)
; CHECK: EXTSW
entry:
	%a = alloca [32 x i16], align 4
	call void @fill(ptr %a)
	%p = getelementptr [32 x i16], ptr %a, i32 0, i32 %i
	%v = load volatile i16, ptr %p, align 2
	%r = sext i16 %v to i32
	ret i32 %r
}

define i32 @load_byte(i32 %i) {
; CHECK-LABEL: name: load_byte
; CHECK: MOVB_load_indexed {{%[0-9]+}}, implicit $r0 :: (volatile load (s8)
entry:
	%a = alloca [32 x i8], align 4
	call void @fill(ptr %a)
	%p = getelementptr [32 x i8], ptr %a, i32 0, i32 %i
	%v = load volatile i8, ptr %p, align 1
	%r = sext i8 %v to i32
	ret i32 %r
}

define i32 @store_word(i32 %i, i32 %v, ptr %callee) {
; CHECK-LABEL: name: store_word
; CHECK: MOVL_store_indexed {{%[0-9]+}}, {{%[0-9]+}}, implicit $r0 :: (volatile store (s32)
; CHECK: JSR
entry:
	%a = alloca [2048 x i32], align 4
	%p = getelementptr [2048 x i32], ptr %a, i32 0, i32 %i
	store volatile i32 %v, ptr %p, align 4
	%r = call i32 %callee(ptr %a, i32 %i, i32 %v, i32 3, i32 4, i32 5)
	ret i32 %r
}

define i32 @store_half(i32 %i, i16 %v) {
; CHECK-LABEL: name: store_half
; CHECK: MOVW_store_indexed {{%[0-9]+}}, {{%[0-9]+}}, implicit $r0 :: (volatile store (s16)
entry:
	%a = alloca [32 x i16], align 4
	%p = getelementptr [32 x i16], ptr %a, i32 0, i32 %i
	store volatile i16 %v, ptr %p, align 2
	%r = call i32 @consume(ptr %a, i32 1, i32 2, i32 3, i32 4, i32 5)
	ret i32 %r
}

define void @store_byte(i32 %i, i8 %v) {
; CHECK-LABEL: name: store_byte
; CHECK: MOVB_store_indexed {{%[0-9]+}}, {{(killed )?%[0-9]+}}, implicit $r0 :: (volatile store (s8)
entry:
	%a = alloca [32 x i8], align 4
	%p = getelementptr [32 x i8], ptr %a, i32 0, i32 %i
	store volatile i8 %v, ptr %p, align 1
	ret void
}

define i32 @size_frame(i32 %i) optsize {
; CHECK-LABEL: name: size_frame
; CHECK: ADDrr
; CHECK: MOVL_load_reg
; CHECK-NOT: MOVL_load_indexed
entry:
	%a = alloca [32 x i32], align 4
	call void @fill(ptr %a)
	%p = getelementptr [32 x i32], ptr %a, i32 0, i32 %i
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define i32 @minsize_frame(i32 %i) minsize {
; CHECK-LABEL: name: minsize_frame
; CHECK: ADDrr
; CHECK: MOVL_load_reg
; CHECK-NOT: MOVL_load_indexed
entry:
	%a = alloca [32 x i32], align 4
	call void @fill(ptr %a)
	%p = getelementptr [32 x i32], ptr %a, i32 0, i32 %i
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}

define i32 @optnone_frame(i32 %i) noinline optnone {
; CHECK-LABEL: name: optnone_frame
; CHECK: ADDrr
; CHECK: MOVL_load_reg
; CHECK-NOT: MOVL_load_indexed
entry:
	%a = alloca [32 x i32], align 4
	call void @fill(ptr %a)
	%p = getelementptr [32 x i32], ptr %a, i32 0, i32 %i
	%v = load volatile i32, ptr %p, align 4
	ret i32 %v
}
