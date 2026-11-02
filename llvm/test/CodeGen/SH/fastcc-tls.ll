; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-readobj -r %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj -r %t.le.o | FileCheck %s --check-prefix=OBJECT

@tls_gd = external thread_local global i32
@tls_le = internal thread_local(localexec) global i32 0

define internal fastcc i32 @fast_tls_leaf(i32 %value) noinline nounwind {
	ret i32 %value
}

define internal fastcc i32 @fast_tls_helper(i32 %value) noinline nounwind uwtable {
; ASM-LABEL: fast_tls_helper:
; ASM: sts.l	pr,@-r15
; ASM: mov.l	r12,@{{.*}}
; ASM: .long	tls_gd@TLSGD
; ASM: .long	__tls_get_addr@PLT
; ASM: jsr	@{{r[0-9]+}}
; ASM: mov.l	@{{.*}},r12
; ASM: lds.l	@r15+,pr
; ASM: .long	tls_le@TPOFF
	%frame = alloca [112 x i8], align 4
	%last = getelementptr [112 x i8], ptr %frame, i32 0, i32 111
	store volatile i8 1, ptr %frame, align 4
	store volatile i8 2, ptr %last, align 1
	%gd = load volatile i32, ptr @tls_gd, align 4
	%le = load volatile i32, ptr @tls_le, align 4
	%sum0 = add i32 %value, %gd
	%sum1 = add i32 %sum0, %le
	%result = call fastcc i32 @fast_tls_leaf(i32 %sum1)
	ret i32 %result
}

define i32 @call_fast_tls(i32 %value) nounwind uwtable {
; ASM-LABEL: call_fast_tls:
; ASM: jsr	@{{r[0-9]+}}
	%result = call fastcc i32 @fast_tls_helper(i32 %value)
	ret i32 %result
}

; OBJECT-DAG: R_SH_TLS_GD_32 tls_gd
; OBJECT-DAG: R_SH_PLT32 __tls_get_addr
; OBJECT-DAG: R_SH_TLS_LE_32 tls_le
