; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/symbol-repeat.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SYMBOL
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/operand-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=OPERAND
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/overflow.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=OVERFLOW
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/too-large.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TOO-LARGE
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/unterminated.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNTERMINATED
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/section.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SECTION
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/alignment.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ALIGNMENT
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=asm %t/expression-depth.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=EXPRESSION-DEPTH
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/unary-depth.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNARY-DEPTH
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/shift-overflow.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SHIFT-OVERFLOW
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/shift-count.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SHIFT-COUNT
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/nested-alternative.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NESTED-ALTERNATIVE

; SYMBOL: error: SH inline assembly size cannot be determined: .rept has an symbol-dependent expression
; OPERAND: error: SH inline assembly size cannot be determined: .space has an operand-dependent expression
; OVERFLOW-COUNT-1: error: SH inline assembly size cannot be determined: inline assembly size multiplication overflows
; TOO-LARGE-COUNT-1: error: SH inline assembly size exceeds the representable range
; UNTERMINATED: error: SH inline assembly size cannot be determined: unterminated inline assembly repetition
; SECTION: error: SH inline assembly size cannot be determined: unsupported size-affecting directive '.section'
; ALIGNMENT: error: SH inline assembly size cannot be determined: .balign alignment is not a power of two below 2**32
; EXPRESSION-DEPTH-COUNT-1: error: SH inline assembly size cannot be determined: .space has a malformed expression: constant expression nesting is too deep
; UNARY-DEPTH-COUNT-1: error: SH inline assembly size cannot be determined: .space has a malformed expression: constant expression nesting is too deep
; SHIFT-OVERFLOW-COUNT-1: error: SH inline assembly size cannot be determined: .space has a malformed expression: constant expression left shift overflows
; SHIFT-COUNT-COUNT-1: error: SH inline assembly size cannot be determined: .space has a malformed expression: constant expression invalid shift count
; NESTED-ALTERNATIVE-COUNT-1: error: SH inline assembly size cannot be determined: nested inline assembly alternatives

;--- symbol-repeat.ll
define void @symbol_repeat() {
	call void asm sideeffect ".rept count\0A\09nop\0A\09.endr", ""()
	ret void
}

;--- operand-space.ll
define void @operand_space() {
	call void asm sideeffect ".space $0", "i"(i32 4)
	ret void
}

;--- overflow.ll
define void @overflow() {
	call void asm sideeffect ".rept 9223372036854775807\0A\09.quad 0,0,0\0A\09.endr", ""()
	ret void
}

;--- too-large.ll
define void @too_large() {
	call void asm sideeffect ".space 4294967296", ""()
	ret void
}

;--- unterminated.ll
define void @unterminated() {
	call void asm sideeffect ".rept 2\0A\09nop", ""()
	ret void
}

;--- section.ll
define void @section_change() {
	call void asm sideeffect ".section .other\0A\09nop", ""()
	ret void
}

;--- alignment.ll
define void @malformed_alignment() {
	call void asm sideeffect ".balign 3", ""()
	ret void
}

;--- expression-depth.ll
define void @expression_depth() {
	call void asm sideeffect ".space (((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((1)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))", ""()
	ret void
}

;--- unary-depth.ll
define void @unary_depth() {
	call void asm sideeffect ".space +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++1", ""()
	ret void
}

;--- shift-overflow.ll
define void @shift_overflow() {
	call void asm sideeffect ".space 3 << 62", ""()
	ret void
}

;--- shift-count.ll
define void @shift_count() {
	call void asm sideeffect ".space 1 << 64", ""()
	ret void
}

;--- nested-alternative.ll
define void @nested_alternative() {
	call void asm sideeffect "$(nop$(rts$)$)", ""()
	ret void
}
