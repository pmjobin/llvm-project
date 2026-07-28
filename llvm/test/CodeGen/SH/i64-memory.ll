; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -enable-new-pm=1 -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=null %s
; RUN: llc -enable-new-pm=1 -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=null %s

define i64 @load64(ptr %address) {
; COMMON-LABEL: load64:
; BE: mov.l	@r4,r0
; BE-NEXT: mov.l	@(4,r4),r1
; LE: mov.l	@r4,r0
; LE-NEXT: mov.l	@(4,r4),r1
; COMMON-NEXT: rts
	%value = load i64, ptr %address, align 4
	ret i64 %value
}

define i64 @load64_volatile(ptr %address) {
; COMMON-LABEL: load64_volatile:
; COMMON: mov.l	@r4,
; COMMON-NEXT: mov.l	@(4,r4),
; COMMON-NEXT: rts
	%value = load volatile i64, ptr %address, align 4
	ret i64 %value
}

define void @store64(ptr %address, i64 %value) {
; COMMON-LABEL: store64:
; COMMON: mov.l	r6,@(4,r4)
; COMMON-NEXT: mov.l	r5,@r4
; COMMON-NEXT: rts
	store i64 %value, ptr %address, align 4
	ret void
}

define void @store64_volatile(ptr %address, i64 %value) {
; COMMON-LABEL: store64_volatile:
; COMMON: mov.l	r5,@r4
; COMMON-NEXT: mov.l	r6,@(4,r4)
; COMMON-NEXT: rts
	store volatile i64 %value, ptr %address, align 4
	ret void
}

define i64 @fixed_i64_align4(i64 %value) {
; COMMON-LABEL: fixed_i64_align4:
; COMMON: add	#-8,r15
; COMMON: mov.l
; COMMON: mov.l
; COMMON: add	#8,r15
	%slot = alloca i64, align 4
	store volatile i64 %value, ptr %slot, align 4
	%result = load volatile i64, ptr %slot, align 4
	ret i64 %result
}

define i64 @fixed_i64_align8(i64 %value) {
; COMMON-LABEL: fixed_i64_align8:
; COMMON: add	#-8,r15
; COMMON: mov.l
; COMMON: mov.l
; COMMON: add	#8,r15
	%slot = alloca i64, align 8
	store volatile i64 %value, ptr %slot, align 8
	%result = load volatile i64, ptr %slot, align 8
	ret i64 %result
}

; MIR-LABEL: name: fixed_i64_align4
; MIR: stack:
; MIR: { id: 0, name: slot, type: default, offset: 0, size: 8, alignment: 4
; MIR-COUNT-2: (volatile store (s32) into %ir.slot
; MIR-COUNT-2: (volatile {{.*}}load (s32) from %ir.slot
; MIR-LABEL: name: fixed_i64_align8
; MIR-NOT: align 8
; MIR: stack:
; MIR: { id: 0, name: slot, type: default, offset: 0, size: 8, alignment: 4
; MIR-COUNT-2: (volatile store (s32) into %ir.slot
; MIR-COUNT-2: (volatile {{.*}}load (s32) from %ir.slot
