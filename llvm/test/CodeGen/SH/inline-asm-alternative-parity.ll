; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null

; These templates mirror MatchesAsmPrinterAlternativeSelection in
; SHInlineAsmSizeTest. The unit test proves the exact text selected for sizing;
; this test proves AsmPrinter emits the same SH (variant zero) alternative.

define void @no_alternatives() {
; CHECK-LABEL: no_alternatives:
; CHECK: !APP
; CHECK-NEXT: nop
; CHECK-NEXT: !NO_APP
	call void asm sideeffect "nop", ""()
	ret void
}

define void @selected_sh_alternative() {
; CHECK-LABEL: selected_sh_alternative:
; CHECK: !APP
; CHECK-NEXT: nop
; CHECK-NEXT: !NO_APP
	call void asm sideeffect "$(nop$|add r0,r1$)", ""()
	ret void
}

define void @multiple_alternatives() {
; CHECK-LABEL: multiple_alternatives:
; CHECK: !APP
; CHECK-NEXT: nop
; CHECK-NEXT: !NO_APP
	call void asm sideeffect "$(nop$|add r0,r1$|rts$)", ""()
	ret void
}

define void @empty_unselected_alternative() {
; CHECK-LABEL: empty_unselected_alternative:
; CHECK: !APP
; CHECK-NEXT: nop
; CHECK-NEXT: !NO_APP
	call void asm sideeffect "$(nop$|$)", ""()
	ret void
}

define void @literal_dollar_and_percent() {
; CHECK-LABEL: literal_dollar_and_percent:
; CHECK: !APP
; CHECK-NEXT: .ascii "$X"
; CHECK-NEXT: .ascii "%%"
; CHECK-NEXT: !NO_APP
	call void asm sideeffect ".ascii \22$$X\22\0A.ascii \22%%\22", ""()
	ret void
}

define void @quoted_braces_and_separator() {
; CHECK-LABEL: quoted_braces_and_separator:
; CHECK: !APP
; CHECK-NEXT: .ascii "{a|b}"
; CHECK-NEXT: .ascii "a;b"
; CHECK-NEXT: !NO_APP
	call void asm sideeffect "$(.ascii \22{a|b}\22$|nop$)\0A$(.ascii \22a;b\22$|nop$)", ""()
	ret void
}

define void @unterminated_final_alternative() {
; CHECK-LABEL: unterminated_final_alternative:
; CHECK: !APP
; CHECK-NEXT: nop
; CHECK-NEXT: !NO_APP
	call void asm sideeffect "$(nop$|rts", ""()
	ret void
}

define void @operand_placeholders(i32 %source, i32 %destination) {
; CHECK-LABEL: operand_placeholders:
; CHECK: !APP
; CHECK-NEXT: mov r{{[0-9]+}},r{{[0-9]+}}
; CHECK-NEXT: !NO_APP
	call void asm sideeffect "$(mov $0,$1$|add $0,$1$)", "r,r"(i32 %source, i32 %destination)
	ret void
}
