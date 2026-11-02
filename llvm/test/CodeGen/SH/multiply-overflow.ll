; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llvm-as %s -o %t.bc
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null

declare { i32, i1 } @llvm.umul.with.overflow.i32(i32, i32)
declare { i32, i1 } @llvm.smul.with.overflow.i32(i32, i32)

define i32 @unsigned_multiply_overflow(i32 %a, i32 %b, ptr %overflow) {
; CHECK-LABEL: unsigned_multiply_overflow:
; CHECK: dmulu.l
; CHECK-NEXT: sts	macl,
; CHECK-NEXT: sts	mach,
; CHECK: movt
; CHECK: rts
	%pair = call { i32, i1 } @llvm.umul.with.overflow.i32(i32 %a, i32 %b)
	%result = extractvalue { i32, i1 } %pair, 0
	%flag = extractvalue { i32, i1 } %pair, 1
	store i1 %flag, ptr %overflow, align 1
	ret i32 %result
}

define i32 @signed_multiply_overflow(i32 %a, i32 %b, ptr %overflow) {
; CHECK-LABEL: signed_multiply_overflow:
; CHECK: dmuls.l
; CHECK-NEXT: sts	macl,
; CHECK-NEXT: sts	mach,
; CHECK: movt
; CHECK: rts
	%pair = call { i32, i1 } @llvm.smul.with.overflow.i32(i32 %a, i32 %b)
	%result = extractvalue { i32, i1 } %pair, 0
	%flag = extractvalue { i32, i1 } %pair, 1
	store i1 %flag, ptr %overflow, align 1
	ret i32 %result
}
