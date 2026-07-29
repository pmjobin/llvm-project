; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null

%B1 = type [1 x i8]
%B2 = type [2 x i8]
%B3 = type [3 x i8]
%B4 = type [4 x i8]
%B5 = type [5 x i8]
%B8 = type [8 x i8]
%B12 = type [12 x i8]
%B16 = type [16 x i8]
%B28 = type [28 x i8]
%B60 = type [60 x i8]
%B76 = type [76 x i8]
%P = type <{ i8, i32 }>

declare void @escape(ptr)

; Register words are materialized in a private fixed bridge object. A call made
; by the callee cannot observe the caller's source object through this pointer.
;
; CHECK-LABEL: byval_escape:
; CHECK: add	#-16,r15
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: mov.l	r5,@({{[0-9]+}},r15)
; CHECK: mov.l	r4,@({{[0-9]+}},r15)
; CHECK: mov	r15,r4
; CHECK-NEXT: add	#{{[0-9]+}},r4
; CHECK: jsr	@
; CHECK-NEXT: nop
; CHECK: lds.l	@r15+,pr
; CHECK-NEXT: add	#16,r15
define i32 @byval_escape(ptr byval(%B8) align 4 %x) nounwind {
	call void @escape(ptr %x)
	%first = load i8, ptr %x, align 4
	%result = zext i8 %first to i32
	ret i32 %result
}

; With three scalar arguments, the first byval word arrives in r7 and the
; second at incoming stack offset zero. The bridge spans those adjacent areas.
define i32 @byval_split(i32 %a, i32 %b, i32 %c, ptr byval(%B8) align 4 %x) nounwind {
	call void @escape(ptr %x)
	%second = getelementptr i8, ptr %x, i32 4
	%value = load i32, ptr %second, align 4
	ret i32 %value
}

; Independent register-home slices keep several addressable byval objects
; distinct: r4 is at entry-SP-16 and r5 is at entry-SP-12.
;
; CHECK-LABEL: several_byval:
; CHECK: mov	r15,r4
; CHECK-NEXT: add	#8,r4
; CHECK: jsr	@
; CHECK: mov	r15,r4
; CHECK-NEXT: add	#12,r4
; CHECK: jsr	@
define void @several_byval(ptr byval(%B4) align 4 %left, ptr byval(%B4) align 4 %right) nounwind {
	call void @escape(ptr %left)
	call void @escape(ptr %right)
	ret void
}

; CHECK-LABEL: call_byval_split:
; CHECK: add	#-4,r15
; CHECK: mov.l	@r4,r7
; CHECK: mov.l	@(4,r4),{{.*}}
; CHECK: mov.l	{{.*}},@r15
; CHECK: mov	#1,r4
; CHECK: mov	#2,r5
; CHECK: mov	#3,r6
; CHECK: jsr	@
; CHECK-NEXT: nop
; CHECK-NEXT: add	#4,r15
define i32 @call_byval_split(ptr %source) nounwind {
	%result = call i32 @byval_split(i32 1, i32 2, i32 3, ptr byval(%B8) align 4 %source)
	ret i32 %result
}

declare void @take1(ptr byval(%B1) align 1)
declare void @take2(ptr byval(%B2) align 2)
declare void @take3(ptr byval(%B3) align 1)
declare void @take4(ptr byval(%B4) align 4)
declare void @take5(ptr byval(%B5) align 1)
declare void @take8(ptr byval(%B8) align 4)
declare void @take12(ptr byval(%B12) align 4)
declare void @take16(ptr byval(%B16) align 4)
declare void @take28(ptr byval(%B28) align 4)
declare void @take60(ptr byval(%B60) align 4)
declare void @take76(ptr byval(%B76) align 4)
declare void @take_packed(ptr byval(%P) align 1)

; Every required size and supported alignment reaches the same content-word
; classifier. The 60-byte case uses four registers plus 44 stack bytes; 76
; bytes reaches the exact 60-byte stack-suffix limit.
;
; CHECK-LABEL: byval_sizes:
; CHECK: add	#-60,r15
; CHECK: jsr	@
; CHECK-NEXT: nop
; CHECK: add	#60,r15
define void @byval_sizes(ptr %source) nounwind {
	call void @take1(ptr byval(%B1) align 1 %source)
	call void @take2(ptr byval(%B2) align 2 %source)
	call void @take3(ptr byval(%B3) align 1 %source)
	call void @take4(ptr byval(%B4) align 4 %source)
	call void @take5(ptr byval(%B5) align 1 %source)
	call void @take8(ptr byval(%B8) align 4 %source)
	call void @take12(ptr byval(%B12) align 4 %source)
	call void @take16(ptr byval(%B16) align 4 %source)
	call void @take28(ptr byval(%B28) align 4 %source)
	call void @take60(ptr byval(%B60) align 4 %source)
	call void @take76(ptr byval(%B76) align 4 %source)
	call void @take_packed(ptr byval(%P) align 1 %source)
	ret void
}

; The complete addressable incoming object remains contiguous across its
; register-home prefix and stack suffix. Sixty bytes is the required object
; boundary; 76 is the largest standalone object with a 60-byte stack suffix.
;
; CHECK-LABEL: byval_sixty:
; CHECK: jsr	@
; CHECK: mov.b	@
; CHECK: rts
define i32 @byval_sixty(ptr byval(%B60) align 4 %value) nounwind {
	call void @escape(ptr %value)
	%last = getelementptr i8, ptr %value, i32 59
	%byte = load i8, ptr %last, align 1
	%result = zext i8 %byte to i32
	ret i32 %result
}

; CHECK-LABEL: byval_seventy_six:
; CHECK: jsr	@
; CHECK: mov.b	@
; CHECK: rts
define i32 @byval_seventy_six(ptr byval(%B76) align 4 %value) nounwind {
	call void @escape(ptr %value)
	%last = getelementptr i8, ptr %value, i32 75
	%byte = load i8, ptr %last, align 1
	%result = zext i8 %byte to i32
	ret i32 %result
}

declare void @sret_and_stack(ptr sret(%B28) align 4, i32, i32, i32, i32, i64, ptr byval(%B8) align 4)

; r2 carries sret independently while ordinary values continue at r4.
define void @mixed_sret_byval(ptr %result, ptr %source, i64 %wide) nounwind {
	call void @sret_and_stack(ptr sret(%B28) align 4 %result, i32 1, i32 2, i32 3, i32 4, i64 %wide, ptr byval(%B8) align 4 %source)
	ret void
}
