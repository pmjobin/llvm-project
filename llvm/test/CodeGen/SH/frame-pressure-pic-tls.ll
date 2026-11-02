; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs -filetype=obj %s -o /dev/null

@default_global = external global i32
@hidden_global = external hidden global i32
@tls_gd = external thread_local global i32
@tls_le = internal thread_local(localexec) global i32 0

declare i32 @external_callee(i32, i32, i32, i32, i32)

; CHECK-LABEL: pic_large:
; CHECK: sts.l	pr,@-r15
; CHECK: add	#-128,r15
; CHECK: mov	#124,r0
; CHECK-NEXT: mov.l	r12,@(r0,r15)
; CHECK: mov	r15,r12
; CHECK-NEXT: add	#124,r12
; CHECK-NEXT: mov.l	@r12,r12
; CHECK: add	#127,r15
; CHECK: add	#1,r15
; CHECK: lds.l	@r15+,pr
; CHECK: .long	_GLOBAL_OFFSET_TABLE_
; CHECK: .long	default_global@GOT
; CHECK: .long	hidden_global@GOTOFF
; CHECK: .long	external_callee@PLT
define i32 @pic_large(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e) nounwind uwtable {
	%local = alloca [112 x i8], align 4
	%last = getelementptr [112 x i8], ptr %local, i32 0, i32 111
	store volatile i8 7, ptr %local, align 4
	store volatile i8 9, ptr %last, align 1
	%x = load volatile i32, ptr @default_global, align 4
	%y = load volatile i32, ptr @hidden_global, align 4
	%call = call i32 @external_callee(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e)
	%sum0 = add i32 %x, %y
	%sum1 = add i32 %sum0, %call
	ret i32 %sum1
}

; CHECK-LABEL: tls_large:
; CHECK: sts.l	pr,@-r15
; CHECK: add	#-128,r15
; CHECK-NEXT: .cfi_def_cfa_offset 132
; CHECK-NEXT: add	#-16,r15
; CHECK-NEXT: .cfi_def_cfa_offset 148
; CHECK: mov	r15,r0
; CHECK-NEXT: add	#80,r0
; CHECK-NEXT: mov.l	r12,@(60,r0)
; CHECK: .long	tls_gd@TLSGD
; CHECK: .long	__tls_get_addr@PLT
; CHECK: mov	r15,r12
; CHECK-NEXT: add	#80,r12
; CHECK-NEXT: mov.l	@(60,r12),r12
; CHECK: add	#127,r15
; CHECK: add	#17,r15
; CHECK: lds.l	@r15+,pr
; CHECK: .long	_GLOBAL_OFFSET_TABLE_
; CHECK: .long	tls_le@TPOFF
; CHECK: .long	external_callee@PLT
define i32 @tls_large(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e) nounwind uwtable {
	%local = alloca [112 x i8], align 4
	%last = getelementptr [112 x i8], ptr %local, i32 0, i32 111
	store volatile i8 7, ptr %local, align 4
	store volatile i8 9, ptr %last, align 1
	%x = load volatile i32, ptr @tls_gd, align 4
	%y = load volatile i32, ptr @tls_le, align 4
	%call = call i32 @external_callee(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e)
	%sum0 = add i32 %x, %y
	%sum1 = add i32 %sum0, %call
	ret i32 %sum1
}
