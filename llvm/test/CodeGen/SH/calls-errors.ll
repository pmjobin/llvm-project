; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/byval.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/sret.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SRET
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false %t/tail.ll -o - | FileCheck %s --check-prefix=TAIL
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/musttail.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MUSTTAIL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t/out-of-range.ll -o %t/range.o
; RUN: llvm-readobj -r %t/range.o | FileCheck %s --check-prefix=RANGE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t/out-of-range.ll -o %t/range-le.o
; RUN: llvm-readobj -r %t/range-le.o | FileCheck %s --check-prefix=RANGE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/large-frame.ll -o /dev/null
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/calling-convention.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CC

; ARGUMENT: error: {{.*}}SH byval requires a fixed aggregate containing only integers, address-space-zero pointers, f32, and f64
; SRET: error: {{.*}}SH sret requires a fixed aggregate containing only integers, address-space-zero pointers, f32, and f64
; TAIL-LABEL: tail_call:
; TAIL: sts.l	pr,@-r15
; TAIL: jsr	@
; TAIL-NEXT: mov.l	@r15,r4
; TAIL: lds.l	@r15+,pr
; TAIL: rts
; MUSTTAIL: error: {{.*}}SH musttail calls are not supported
; RANGE: R_SH_DIR32 far_aligned_callee
; RANGE-NOT: R_SH_IND12W
; CC: error: {{.*}}SH only supports the C and fast calling conventions

;--- byval.ll
define void @byval_argument(ptr %fn, ptr %value) {
	call void %fn(ptr byval(i32) %value)
	ret void
}

;--- sret.ll
define void @sret_argument(ptr %fn, ptr %value) {
	call void %fn(ptr sret(i32) %value)
	ret void
}

;--- tail.ll
define i32 @tail_call(ptr %fn, i32 %value) {
	%result = tail call i32 %fn(i32 %value)
	ret i32 %result
}

;--- musttail.ll
define i32 @musttail_call(ptr %fn, i32 %value) {
	%result = musttail call i32 %fn(ptr %fn, i32 %value)
	ret i32 %result
}

;--- out-of-range.ll
define dso_local i32 @out_of_range_call(i32 %value) {
	%result = call i32 @far_aligned_callee(i32 %value)
	ret i32 %result
}

define internal i32 @far_aligned_callee(i32 %value) align 8192 {
	ret i32 %value
}

;--- large-frame.ll
define internal i32 @identity(i32 %value) {
	ret i32 %value
}

define i32 @large_call_frame(i32 %value) {
	%slots = alloca [16384 x i32], align 4
	%last = getelementptr [16384 x i32], ptr %slots, i32 0, i32 16383
	store volatile i32 %value, ptr %slots, align 4
	store volatile i32 %value, ptr %last, align 4
	%result = call i32 @identity(i32 %value)
	ret i32 %result
}

;--- calling-convention.ll
define i32 @unsupported_cc(ptr %fn) {
	%result = call coldcc i32 %fn()
	ret i32 %result
}
