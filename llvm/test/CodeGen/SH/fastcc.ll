; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR

%S2 = type { i16 }
%Nested4 = type { %S2, i16 }
%S8 = type { i32, i32 }
%S12 = type { i32, i32, i32 }
%Packed2 = type <{ i8, i8 }>

@fast_metadata_slot = internal global i32 0

declare void @llvm.memcpy.p0.p0.i32(ptr, ptr, i32, i1 immarg)
declare void @llvm.memset.p0.i32(ptr, i8, i32, i1 immarg)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1 immarg)

define private fastcc i32 @fast_zero() #0 {
	ret i32 11
}

; CHECK-LABEL: call_fast_zero:
; CHECK: jsr	@{{r[0-9]+}}
define i32 @call_fast_zero() {
	%result = call fastcc i32 @fast_zero()
	ret i32 %result
}

define internal fastcc i32 @fast_scalar(i32 %value, ptr %address) #0 {
	%loaded = load i32, ptr %address, align 4
	%result = add i32 %value, %loaded
	ret i32 %result
}

; CHECK-LABEL: call_fast_scalar:
; CHECK: jsr	@{{r[0-9]+}}
define i32 @call_fast_scalar(i32 %value, ptr %address) {
	%result = call fastcc i32 @fast_scalar(i32 %value, ptr %address)
	ret i32 %result
}

; The fifth argument uses the first stack word under the same physical
; convention as ccc.
; CHECK-LABEL: fast_stack:
; CHECK: mov.l	@r15,r0
define internal fastcc i32 @fast_stack(i32 %a, i32 %b, i32 %c, i32 %d, i32 %stack) #0 {
	ret i32 %stack
}

; CHECK-LABEL: call_fast_stack:
; CHECK: add	#-4,r15
; CHECK: mov.l	{{.*}},@r15
; CHECK: jsr	@{{r[0-9]+}}
; CHECK: add	#4,r15
; MIR: JSR {{.*}}, csr_sh
define i32 @call_fast_stack(i32 %value) {
	%result = call fastcc i32 @fast_stack(i32 1, i32 2, i32 3, i32 4, i32 %value)
	ret i32 %result
}

define internal fastcc i32 @fast_promoted(i32 signext %signed_value, i32 zeroext %unsigned_value) #0 {
	%result = add i32 %signed_value, %unsigned_value
	ret i32 %result
}

define i32 @call_fast_promoted(i32 %signed_value, i32 %unsigned_value) {
	%result = call fastcc i32 @fast_promoted(i32 signext %signed_value, i32 zeroext %unsigned_value)
	ret i32 %result
}

; CHECK-LABEL: fast_recursive:
; CHECK: jsr	@{{r[0-9]+}}
define internal fastcc i32 @fast_recursive(i32 %count) #0 {
	%done = icmp eq i32 %count, 0
	br i1 %done, label %base, label %recurse

base:
	ret i32 1

recurse:
	%next = sub i32 %count, 1
	%value = call fastcc i32 @fast_recursive(i32 %next)
	%result = add i32 %value, %count
	ret i32 %result
}

define i32 @call_fast_recursive(i32 %count) {
	%result = call fastcc i32 @fast_recursive(i32 %count)
	ret i32 %result
}

define internal fastcc i32 @fast_mutual_a(i32 %count) #0 {
	%done = icmp eq i32 %count, 0
	br i1 %done, label %base, label %recurse

base:
	ret i32 3

recurse:
	%next = sub i32 %count, 1
	%result = call fastcc i32 @fast_mutual_b(i32 %next)
	ret i32 %result
}

define internal fastcc i32 @fast_mutual_b(i32 %count) #0 {
	%done = icmp eq i32 %count, 0
	br i1 %done, label %base, label %recurse

base:
	ret i32 5

recurse:
	%next = sub i32 %count, 1
	%result = call fastcc i32 @fast_mutual_a(i32 %next)
	ret i32 %result
}

define i32 @call_fast_mutual(i32 %count) {
	%result = call fastcc i32 @fast_mutual_a(i32 %count)
	ret i32 %result
}

; The comparison is materialized before the fast call because csr_sh does not
; preserve TBit.
; CHECK-LABEL: comparison_across_fast_call:
; CHECK: movt
; CHECK: jsr	@{{r[0-9]+}}
define i32 @comparison_across_fast_call(i32 %a, i32 %b, ptr %address) {
	%condition = icmp ult i32 %a, %b
	%boolean = zext i1 %condition to i32
	%called = call fastcc i32 @fast_scalar(i32 %a, ptr %address)
	%result = add i32 %called, %boolean
	ret i32 %result
}

define internal fastcc i32 @fast_interactions(ptr %source, ptr %destination, i32 %a, i32 %b) #0 {
	%unaligned16 = load i16, ptr %source, align 1
	%wide = zext i16 %unaligned16 to i32
	store i32 %wide, ptr %destination, align 1
	%condition = icmp ult i32 %a, %b
	%selected = select i1 %condition, i32 %a, i32 %b
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 %destination, ptr align 1 %source, i32 4, i1 false)
	call void @llvm.memset.p0.i32(ptr align 1 %destination, i8 0, i32 4, i1 false)
	call void @llvm.memset.p0.i64(ptr align 1 %destination, i8 0, i64 4, i1 false)
	%result = add i32 %wide, %selected
	ret i32 %result
}

define i32 @call_fast_interactions(ptr %source, ptr %destination, i32 %a, i32 %b) {
	%result = call fastcc i32 @fast_interactions(ptr %source, ptr %destination, i32 %a, i32 %b)
	ret i32 %result
}

; CHECK-LABEL: fast_frame_pressure:
; CHECK: add	#-{{[1-9][0-9]+}},r15
; CHECK: jsr	@{{r[0-9]+}}
define i32 @fast_frame_pressure(i32 %value) {
	%frame = alloca [244 x i8], align 4
	%last = getelementptr [244 x i8], ptr %frame, i32 0, i32 243
	store volatile i8 1, ptr %frame, align 4
	store volatile i8 2, ptr %last, align 1
	%result = call fastcc i32 @fast_stack(i32 1, i32 2, i32 3, i32 4, i32 %value)
	ret i32 %result
}

define internal fastcc ptr @fast_returned(ptr returned %value) #0 {
	ret ptr %value
}

define ptr @call_fast_returned(ptr %value) {
	%result = call fastcc ptr @fast_returned(ptr returned %value)
	ret ptr %result
}

; ASan stack descriptions retain a function address as integer metadata after
; local functions have been promoted to fastcc.  The function is still only
; invoked directly.
; CHECK-LABEL: fast_metadata:
; CHECK: mov.l
define internal fastcc void @fast_metadata() #0 {
	store i32 ptrtoint (ptr @fast_metadata to i32), ptr @fast_metadata_slot, align 4
	ret void
}

define void @call_fast_metadata() {
	call fastcc void @fast_metadata()
	ret void
}

attributes #0 = { noinline nounwind }
