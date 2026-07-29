; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM --implicit-check-not='jmp	@' --implicit-check-not=.LJTI
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM --implicit-check-not='jmp	@' --implicit-check-not=.LJTI
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL --implicit-check-not=jumpTable --implicit-check-not=SH_JT_DISPATCH
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=null < %s

define i32 @two_cases(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 3, label %three
		i32 4, label %four
	]
three:
	ret i32 17
four:
	ret i32 29
default:
	ret i32 -1
}

define i32 @three_cases(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 10, label %ten
		i32 11, label %eleven
		i32 12, label %twelve
	]
ten:
	ret i32 31
eleven:
	ret i32 47
twelve:
	ret i32 61
default:
	ret i32 -1
}

define i32 @sparse_cases(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 -2000000000, label %a
		i32 -1000000, label %b
		i32 -7, label %c
		i32 1000, label %d
		i32 1000000, label %e
		i32 2000000000, label %f
	]
a:
	ret i32 11
b:
	ret i32 23
c:
	ret i32 37
d:
	ret i32 53
e:
	ret i32 71
f:
	ret i32 89
default:
	ret i32 -1
}

define i32 @sparse_i64(i64 %x) {
entry:
	switch i64 %x, label %default [
		i64 -5000000000, label %a
		i64 0, label %b
		i64 10000000000, label %c
	]
a:
	ret i32 13
b:
	ret i32 41
c:
	ret i32 79
default:
	ret i32 -1
}

; ASM-LABEL: two_cases:
; ASM: cmp/eq
; ASM: {{bt|bf}}
; ASM: cmp/eq
; ASM: {{bt|bf}}

; ASM-LABEL: three_cases:
; ASM: cmp/eq
; ASM: {{bt|bf}}
; ASM: cmp/eq
; ASM: {{bt|bf}}

; ASM-LABEL: sparse_cases:
; ASM: cmp/
; ASM: {{bt|bf}}
; ASM: cmp/
; ASM: {{bt|bf}}

; ASM-LABEL: sparse_i64:
; ASM: cmp/
; ASM: {{bt|bf}}
; ASM: cmp/
; ASM: {{bt|bf}}

; ISEL-LABEL: name:            two_cases
; ISEL: CMP_EQ
; ISEL: {{BT|BF}}
; ISEL-LABEL: name:            three_cases
; ISEL: CMP_EQ
; ISEL: {{BT|BF}}
; ISEL-LABEL: name:            sparse_cases
; ISEL: CMP_
; ISEL: {{BT|BF}}
; ISEL-LABEL: name:            sparse_i64
; ISEL: BR_CC64_PSEUDO
