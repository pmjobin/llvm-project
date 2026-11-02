; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 %t/kind.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=KIND
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 %t/signature.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SIGNATURE
; RUN: not llc -mtriple=sh2-unknown-linux-muslfdpic -mcpu=sh2 %t/fdpic.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=FDPIC
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 %t/direct-call.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=DIRECT
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 %t/alias-call.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=DIRECT
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 %t/alwaysinline.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INLINE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 %t/naked.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NAKED
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 %t/eh.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=EH
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 %t/stack-protector.ll -o - | FileCheck %s --check-prefix=STACK
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 %t/stack-realignment.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=REALIGN
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 %t/sanitizer.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SANITIZER

; KIND: error: {{.*}}SH handler attribute has an invalid value
; SIGNATURE: error: {{.*}}SH hardware handlers require the C calling convention and void () type
; FDPIC: error: {{.*}}SH FDPIC hardware handlers are unsupported because entry has no r12 descriptor context
; DIRECT: error: {{.*}}SH hardware handlers cannot be called as ordinary functions
; INLINE: error: {{.*}}SH hardware handlers cannot use the alwaysinline attribute
; NAKED: error: {{.*}}SH hardware handlers cannot use the naked attribute
; EH: error: {{.*}}SH hardware handlers do not support exception handling
; STACK-LABEL: handler_with_ssp:
; STACK: rte
; STACK-NEXT: nop
; REALIGN: error: {{.*}}SH core hardware handlers do not support stack realignment
; SANITIZER: error: {{.*}}SH hardware handler sanitizer instrumentation is unsupported

;--- kind.ll
define void @invalid_kind() #0 {
	ret void
}
attributes #0 = { "sh-interrupt-handler"="invalid" "target-abi"="gnu" }

;--- signature.ll
define i32 @invalid_signature(i32 %arg) #0 {
	ret i32 %arg
}
attributes #0 = { "sh-interrupt-handler"="interrupt" "target-abi"="gnu" }

;--- fdpic.ll
target triple = "sh2-unknown-linux-muslfdpic"
define void @invalid_fdpic() #0 {
	ret void
}
attributes #0 = { "sh-interrupt-handler"="interrupt" "target-abi"="gnu-fdpic" }
!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-fdpic"}

;--- direct-call.ll
define void @handler() #0 {
	ret void
}
define void @invalid_direct_call() {
	call void @handler()
	ret void
}
attributes #0 = { "sh-interrupt-handler"="interrupt" "target-abi"="gnu" }

;--- alias-call.ll
@handler_alias = alias void (), ptr @handler
define void @handler() #0 {
	ret void
}
define void @invalid_alias_call() {
	call void @handler_alias()
	ret void
}
attributes #0 = { "sh-interrupt-handler"="trapa" "target-abi"="gnu" }

;--- alwaysinline.ll
define void @invalid_inline() #0 {
	ret void
}
attributes #0 = { alwaysinline "sh-interrupt-handler"="interrupt" "target-abi"="gnu" }

;--- naked.ll
define void @invalid_naked() #0 {
	ret void
}
attributes #0 = { naked "sh-interrupt-handler"="interrupt" "target-abi"="gnu" }

;--- eh.ll
declare i32 @personality(...)
define void @invalid_eh() #0 personality ptr @personality {
	ret void
}
attributes #0 = { "sh-interrupt-handler"="interrupt" "target-abi"="gnu" }

;--- stack-protector.ll
define void @handler_with_ssp() #0 {
	ret void
}
attributes #0 = { sspstrong "sh-interrupt-handler"="interrupt" "target-abi"="gnu" }

;--- stack-realignment.ll
declare void @use(ptr)
define void @invalid_stack_realignment() #0 {
	%object = alloca i32, align 8
	call void @use(ptr %object)
	ret void
}
attributes #0 = { "sh-interrupt-handler"="interrupt" "target-abi"="gnu" }

;--- sanitizer.ll
define void @invalid_sanitizer() #0 {
	ret void
}
attributes #0 = { sanitize_address "sh-interrupt-handler"="interrupt" "target-abi"="gnu" }
