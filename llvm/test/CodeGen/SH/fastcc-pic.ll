; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O0 -relocation-model=pic -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-readobj -r %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj -r %t.le.o | FileCheck %s --check-prefix=OBJECT

@default_data = external global i32
@hidden_data = external hidden global i32

declare i32 @external_callee(i32)

define internal fastcc i32 @fast_pic_helper(i32 %value) noinline nounwind uwtable {
; ASM-LABEL: fast_pic_helper:
; ASM: sts.l	pr,@-r15
; ASM: mov.l	r12,@{{.*}}
; ASM: .long	_GLOBAL_OFFSET_TABLE_
; ASM: .long	default_data@GOT
; ASM: .long	hidden_data@GOTOFF
; ASM: .long	external_callee@PLT
	%default = load volatile i32, ptr @default_data, align 4
	%hidden = load volatile i32, ptr @hidden_data, align 4
	%sum0 = add i32 %value, %default
	%sum1 = add i32 %sum0, %hidden
	%called = call i32 @external_callee(i32 %sum1)
	ret i32 %called
}

define i32 @call_fast_pic(i32 %value) nounwind uwtable {
; ASM-LABEL: call_fast_pic:
; ASM: jsr	@{{r[0-9]+}}
	%result = call fastcc i32 @fast_pic_helper(i32 %value)
	ret i32 %result
}

; OBJECT: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; OBJECT-DAG: R_SH_GOT32 default_data
; OBJECT-DAG: R_SH_GOTOFF hidden_data
; OBJECT-DAG: R_SH_PLT32 external_callee
