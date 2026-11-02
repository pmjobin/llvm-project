; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static %t/callbr.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static %t/invoke.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static %t/personality.ll -o /dev/null
; RUN: not llc -mtriple=sh-unknown-elf -relocation-model=static %t/unknown-personality.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PERSONALITY
; RUN: not llc -mtriple=sh-unknown-elf -relocation-model=static %t/funclet.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=FUNCLET
; RUN: not llc -mtriple=sh-unknown-elf -relocation-model=static %t/sjlj.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SJLJ
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static %t/blockaddress-select.ll -o /dev/null
; RUN: not llc -mtriple=sh-unknown-elf -relocation-model=static %t/blockaddress-arithmetic.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARITHMETIC
; RUN: not llc -mtriple=sh-unknown-elf -relocation-model=static %t/nonzero-address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE

; PERSONALITY: error: {{.*}}SH only supports GNU Itanium landing-pad personalities
; FUNCLET: error: {{.*}}SH only supports Itanium landingpad EH pads
; SJLJ: error: {{.*}}SH language exception handling is not supported: EH intrinsic
; ARITHMETIC: error: {{.*}}SH block-address arithmetic is not supported
; ADDRESS-SPACE: error: {{.*}}SH nonzero address spaces are not supported

;--- callbr.ll
define void @supported_callbr() {
entry:
	callbr void asm sideeffect "", ""() to label %normal []
normal:
	ret void
}

;--- invoke.ll
declare void @may_throw()
declare i32 @__gxx_personality_v0(...)

define void @unsupported_invoke() personality ptr @__gxx_personality_v0 {
entry:
	invoke void @may_throw() to label %normal unwind label %landing
normal:
	ret void
landing:
	%exception = landingpad { ptr, i32 } cleanup
	ret void
}

;--- personality.ll
declare i32 @__gxx_personality_v0(...)

define void @unsupported_personality() personality ptr @__gxx_personality_v0 {
	ret void
}

;--- unknown-personality.ll
declare i32 @unknown_personality(...)

define void @unsupported_personality() personality ptr @unknown_personality {
	ret void
}

;--- funclet.ll
declare i32 @__gxx_personality_v0(...)
declare void @may_throw()

define void @unsupported_funclet() personality ptr @__gxx_personality_v0 {
entry:
	invoke void @may_throw() to label %exit unwind label %catch.dispatch
catch.dispatch:
	%switch = catchswitch within none [label %handler] unwind to caller
handler:
	%pad = catchpad within %switch [ptr null, i32 0, ptr null]
	catchret from %pad to label %exit
exit:
	ret void
}

;--- sjlj.ll
declare i32 @llvm.eh.sjlj.setjmp(ptr)

define i32 @unsupported_sjlj(ptr %context) {
	%result = call i32 @llvm.eh.sjlj.setjmp(ptr %context)
	ret i32 %result
}

;--- blockaddress-select.ll
define i32 @unsupported_blockaddress_select(i32 %condition) {
entry:
	%address = select i1 true, ptr blockaddress(@unsupported_blockaddress_select, %left), ptr blockaddress(@unsupported_blockaddress_select, %right)
	indirectbr ptr %address, [label %left, label %right]
left:
	ret i32 1
right:
	ret i32 2
}

;--- blockaddress-arithmetic.ll
define i32 @unsupported_blockaddress_arithmetic() {
entry:
	%address = ptrtoint ptr blockaddress(@unsupported_blockaddress_arithmetic, %target) to i32
	%adjusted = add i32 %address, 2
	%pointer = inttoptr i32 %adjusted to ptr
	indirectbr ptr %pointer, [label %target]
target:
	ret i32 1
}

;--- nonzero-address-space.ll
define ptr addrspace(1) @unsupported_blockaddress_address_space(i32 %condition) {
entry:
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %return, label %target
return:
	%address = addrspacecast ptr blockaddress(@unsupported_blockaddress_address_space, %target) to ptr addrspace(1)
	ret ptr addrspace(1) %address
target:
	ret ptr addrspace(1) null
}
