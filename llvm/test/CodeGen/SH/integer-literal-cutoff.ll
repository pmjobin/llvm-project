; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SIZE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SIZE
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SIZE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SIZE
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SIZE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SIZE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SIZE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=SIZE,O0

declare void @consume(i32)

define i32 @materialize_long_constants() optsize {
entry:
	call void @consume(i32 -1073217536)
	call void @consume(i32 -1076887552)
	ret i32 -1076887552
}

define i32 @materialize_long_constant_no_size() {
entry:
	ret i32 -1076887552
}

; SIZE-LABEL: materialize_long_constants:
; SIZE: mov.l	[[FIRST:.LCPI[0-9_]+]],r{{[0-9]+}}
; SIZE: mov.l	[[CONSUME:.LCPI[0-9_]+]],[[CALL_REG:r[0-9]+]]
; SIZE: jsr	@[[CALL_REG]]
; SIZE: mov.l	[[SECOND:.LCPI[0-9_]+]],r{{[0-9]+}}
; SIZE: jsr	@[[CALL_REG]]
; SIZE: [[FIRST]]:
; SIZE-NEXT: .long	3221749760
; SIZE: [[SECOND]]:
; SIZE-NEXT: .long	3218079744

; At the global -O0 level a function without size attributes retains the
; synthesis plan, so literal selection is never correctness-required.
; O0-LABEL: materialize_long_constant_no_size:
; O0-NOT: mov.l
; O0: mov	#-64,r0
; O0: shll16	r0
