; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/variadic-function.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARIADIC-FUNCTION
; RUN: not llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 %t/variadic-function.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARIADIC-FUNCTION
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/variadic-call.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARIADIC-CALL
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/musttail.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MUSTTAIL
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/mismatch.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MISMATCH
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/narrow-formal.ll -o - | FileCheck %s --check-prefix=NARROW
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i1-return.ll -o - | FileCheck %s --check-prefix=I1-RETURN
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/malformed-sret.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SRET
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/malformed-byval.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=BYVAL
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/indirect.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INDIRECT
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/address-compared.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/address-stored.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/linkonce.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=LINKONCE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/available.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=AVAILABLE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/hidden.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=LINKAGE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/external-definition.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=LINKAGE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/external-declaration.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=LINKAGE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/unsupported-cc.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=OTHER-CC

; VARIADIC-FUNCTION: Calling convention does not support varargs or perfect forwarding!
; VARIADIC-CALL: Calling convention does not support varargs or perfect forwarding!
; MUSTTAIL: error: {{.*}}SH musttail calls are not supported
; MISMATCH: error: {{.*}}SH call site and callee calling conventions must match
; NARROW-LABEL: bad:
; NARROW: rts
; NARROW-NEXT: extu.w	r4,r0
; I1-RETURN-LABEL: bad:
; I1-RETURN: tst	r4,r4
; I1-RETURN: rts
; I1-RETURN-NEXT: movt	r0
; SRET: error: {{.*}}SH sret requires a fixed aggregate containing only integers, address-space-zero pointers, f32, and f64
; BYVAL: error: {{.*}}SH byval requires a fixed aggregate containing only integers, address-space-zero pointers, f32, and f64
; INDIRECT: error: {{.*}}SH indirect fast calls are not supported
; ADDRESS: error: {{.*}}SH fast calling convention functions may only be used by direct calls
; LINKAGE: error: {{.*}}SH fast calling convention requires internal or private linkage
; LINKONCE: error: {{.*}}SH fast calling convention requires internal or private linkage
; AVAILABLE: error: {{.*}}SH fast calling convention requires internal or private linkage
; OTHER-CC: error: {{.*}}SH only supports the C and fast calling conventions

;--- variadic-function.ll
define internal fastcc i32 @bad(i32 %fixed, ...) {
	ret i32 %fixed
}

define i32 @use(i32 %value) {
	%result = call fastcc i32 (i32, ...) @bad(i32 %value)
	ret i32 %result
}

;--- variadic-call.ll
declare fastcc i32 @external_variadic(i32, ...)

define i32 @bad(i32 %value) {
	%result = call fastcc i32 (i32, ...) @external_variadic(i32 %value)
	ret i32 %result
}

;--- musttail.ll
define internal fastcc i32 @callee(i32 %value) {
	ret i32 %value
}

define internal fastcc i32 @caller(i32 %value) {
	%result = musttail call fastcc i32 @callee(i32 %value)
	ret i32 %result
}

define i32 @use(i32 %value) {
	%result = call fastcc i32 @caller(i32 %value)
	ret i32 %result
}

;--- mismatch.ll
define internal i32 @callee(i32 %value) {
	ret i32 %value
}

define i32 @bad(i32 %value) {
	%result = call fastcc i32 @callee(i32 %value)
	ret i32 %result
}

;--- narrow-formal.ll
define internal fastcc i32 @bad(i16 %value) {
	%result = zext i16 %value to i32
	ret i32 %result
}

;--- i1-return.ll
define internal fastcc i1 @bad(i32 %value) {
	%result = icmp eq i32 %value, 0
	ret i1 %result
}

;--- malformed-sret.ll
define internal fastcc void @bad(ptr sret(i32) %result) {
	ret void
}

;--- malformed-byval.ll
define internal fastcc void @bad(ptr byval(i32) %value) {
	ret void
}

;--- indirect.ll
define i32 @bad(ptr %function, i32 %value) {
	%result = call fastcc i32 %function(i32 %value)
	ret i32 %result
}

;--- address-compared.ll
define internal fastcc i32 @target(i32 %value) {
	ret i32 %value
}

define i1 @bad(ptr %other) {
	%result = icmp eq ptr %other, @target
	ret i1 %result
}

;--- address-stored.ll
@slot = internal global ptr null

define internal fastcc i32 @target(i32 %value) {
	ret i32 %value
}

define void @bad() {
	store ptr @target, ptr @slot, align 4
	ret void
}

;--- linkonce.ll
define linkonce_odr fastcc i32 @bad(i32 %value) {
	ret i32 %value
}

;--- available.ll
define available_externally fastcc i32 @bad(i32 %value) {
	ret i32 %value
}

;--- hidden.ll
define hidden fastcc i32 @bad(i32 %value) {
	ret i32 %value
}

;--- external-definition.ll
define fastcc i32 @bad(i32 %value) {
	ret i32 %value
}

;--- external-declaration.ll
declare fastcc i32 @external_fast(i32)

define i32 @bad(i32 %value) {
	%result = call fastcc i32 @external_fast(i32 %value)
	ret i32 %result
}

;--- unsupported-cc.ll
define internal coldcc i32 @bad(i32 %value) {
	ret i32 %value
}
