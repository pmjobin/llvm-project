; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define i32 @subtract(i32 %a, i32 %b) nounwind {
; CHECK-LABEL: subtract:
; CHECK: rts
; CHECK-NEXT: sub	r5,{{r[0-9]+}}
	%result = sub i32 %a, %b
	ret i32 %result
}

define i32 @subtract_flags(i32 %a, i32 %b) nounwind {
; CHECK-LABEL: subtract_flags:
; CHECK: rts
; CHECK-NEXT: sub	r5,{{r[0-9]+}}
	%result = sub nsw i32 %a, %b
	ret i32 %result
}

define i32 @negate(i32 %value) nounwind {
; CHECK-LABEL: negate:
; CHECK-NEXT: rts
; CHECK-NEXT: neg	r4,r0
	%result = sub i32 0, %value
	ret i32 %result
}

define i32 @bit_and(i32 %a, i32 %b) nounwind {
; CHECK-LABEL: bit_and:
; CHECK: rts
; CHECK-NEXT: and	r5,{{r[0-9]+}}
	%result = and i32 %a, %b
	ret i32 %result
}

define i32 @bit_or(i32 %a, i32 %b) nounwind {
; CHECK-LABEL: bit_or:
; CHECK: rts
; CHECK-NEXT: or	r5,{{r[0-9]+}}
	%result = or i32 %a, %b
	ret i32 %result
}

define i32 @bit_xor(i32 %a, i32 %b) nounwind {
; CHECK-LABEL: bit_xor:
; CHECK: rts
; CHECK-NEXT: xor	r5,{{r[0-9]+}}
	%result = xor i32 %a, %b
	ret i32 %result
}

define i32 @complement(i32 %value) nounwind {
; CHECK-LABEL: complement:
; CHECK-NEXT: rts
; CHECK-NEXT: not	r4,r0
	%result = xor i32 %value, -1
	ret i32 %result
}

define i32 @values_live(i32 %a, i32 %b, i32 %c) nounwind {
; CHECK-LABEL: values_live:
; CHECK-DAG: sub
; CHECK-DAG: and
; CHECK: or
; CHECK: rts
; CHECK-NEXT: xor
	%sub = sub i32 %a, %b
	%and = and i32 %a, %c
	%or = or i32 %sub, %and
	%result = xor i32 %or, %b
	ret i32 %result
}
