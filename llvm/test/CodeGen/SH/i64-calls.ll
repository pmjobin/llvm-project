; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR

define i64 @return_i64(i64 %value) {
; COMMON-LABEL: return_i64:
; COMMON: mov	r5,r1
; COMMON-NEXT: mov	r4,r0
; COMMON-NEXT: rts
	ret i64 %value
}

define i64 @return_constant_i64() {
; COMMON-LABEL: return_constant_i64:
; BE: mov	#18,r0
; LE: mov	#-102,r0
; COMMON: rts
	ret i64 1311768467463790320
}

define i64 @one_i64(i64 %value) {
; COMMON-LABEL: one_i64:
; COMMON: mov	r5,r1
; COMMON-NEXT: mov	r4,r0
	ret i64 %value
}

define i32 @one_i64_low(i64 %value) {
; COMMON-LABEL: one_i64_low:
; BE: mov	r5,r0
; LE: mov	r4,r0
	%low = trunc i64 %value to i32
	ret i32 %low
}

define i32 @one_i64_high(i64 %value) {
; COMMON-LABEL: one_i64_high:
; BE: mov	r4,r0
; LE: mov	r5,r0
	%shifted = lshr i64 %value, 32
	%high = trunc i64 %shifted to i32
	ret i32 %high
}

define i64 @two_i64(i64 %first, i64 %second) {
; COMMON-LABEL: two_i64:
; COMMON: mov	r7,r1
; COMMON-NEXT: mov	r6,r0
	ret i64 %second
}

define i64 @i32_i64(i32 %a, i64 %value) {
; COMMON-LABEL: i32_i64:
; COMMON: mov	r6,r1
; COMMON-NEXT: mov	r5,r0
	ret i64 %value
}

define i64 @i64_i32(i64 %value, i32 %a) {
; COMMON-LABEL: i64_i32:
; COMMON: mov	r5,r1
; COMMON-NEXT: mov	r4,r0
	ret i64 %value
}

define i64 @two_i32_i64(i32 %a, i32 %b, i64 %value) {
; COMMON-LABEL: two_i32_i64:
; COMMON: mov	r7,r1
; COMMON-NEXT: mov	r6,r0
	ret i64 %value
}

define i64 @three_i32_i64(i32 %a, i32 %b, i32 %c, i64 %value) {
; COMMON-LABEL: three_i32_i64:
; COMMON: mov	r7,r0
; COMMON-NEXT: mov.l	@r15,r1
; COMMON: rts
	ret i64 %value
}

define i32 @three_i32_i64_low(i32 %a, i32 %b, i32 %c, i64 %value) {
; COMMON-LABEL: three_i32_i64_low:
; BE: mov.l	@r15,r0
; LE: mov	r7,r0
	%low = trunc i64 %value to i32
	ret i32 %low
}

define i32 @three_i32_i64_high(i32 %a, i32 %b, i32 %c, i64 %value) {
; COMMON-LABEL: three_i32_i64_high:
; BE: mov	r7,r0
; LE: mov.l	@r15,r0
	%shifted = lshr i64 %value, 32
	%high = trunc i64 %shifted to i32
	ret i32 %high
}

define i64 @four_i32_i64(i32 %a, i32 %b, i32 %c, i32 %d, i64 %value) {
; COMMON-LABEL: four_i32_i64:
; COMMON: mov.l	@(4,r15),r1
; COMMON-NEXT: mov.l	@r15,r0
; COMMON: rts
	ret i64 %value
}

define i32 @four_i32_i64_low(i32 %a, i32 %b, i32 %c, i32 %d, i64 %value) {
; COMMON-LABEL: four_i32_i64_low:
; BE: mov.l	@(4,r15),r0
; LE: mov.l	@r15,r0
	%low = trunc i64 %value to i32
	ret i32 %low
}

define i32 @four_i32_i64_high(i32 %a, i32 %b, i32 %c, i32 %d, i64 %value) {
; COMMON-LABEL: four_i32_i64_high:
; BE: mov.l	@r15,r0
; LE: mov.l	@(4,r15),r0
	%shifted = lshr i64 %value, 32
	%high = trunc i64 %shifted to i32
	ret i32 %high
}

define i64 @mixed_pointer(ptr %pointer, i64 %value, i32 %tail) {
; COMMON-LABEL: mixed_pointer:
; COMMON: mov	r6,r1
; COMMON-NEXT: mov	r5,r0
	ret i64 %value
}

define internal i64 @callee_partial(i32 %a, i32 %b, i32 %c, i64 %value) {
	ret i64 %value
}

define i64 @direct_partial(i32 %a, i32 %b, i32 %c, i64 %value) {
; COMMON-LABEL: direct_partial:
; COMMON: sts.l	pr,@-r15
; COMMON: add	#-4,r15
; COMMON: mov.l
; COMMON: bsr	callee_partial
; COMMON: add	#4,r15
; COMMON: lds.l	@r15+,pr
; COMMON: rts
	%result = call i64 @callee_partial(i32 %a, i32 %b, i32 %c, i64 %value)
	ret i64 %result
}

define internal i64 @callee_stack(i32 %a, i32 %b, i32 %c, i32 %d, i64 %value) {
	ret i64 %value
}

define i64 @direct_stack(i32 %a, i32 %b, i32 %c, i32 %d, i64 %value) {
; COMMON-LABEL: direct_stack:
; COMMON: sts.l	pr,@-r15
; COMMON: add	#-8,r15
; COMMON: mov.l
; COMMON: mov.l
; COMMON: bsr	callee_stack
; COMMON: add	#8,r15
; COMMON: lds.l	@r15+,pr
; COMMON: rts
	%result = call i64 @callee_stack(i32 %a, i32 %b, i32 %c, i32 %d, i64 %value)
	ret i64 %result
}

define internal void @callee_stack_constant(i32 %a, i32 %b, i32 %c, i32 %d, i64 %value) {
	ret void
}

define void @direct_stack_constant() {
; COMMON-LABEL: direct_stack_constant:
; BE: mov	#2,[[BE_LO:r[0-9]+]]
; BE-NEXT: mov.l	[[BE_LO]],@(4,r15)
; BE: mov	#1,[[BE_HI:r[0-9]+]]
; BE-NEXT: mov.l	[[BE_HI]],@r15
; LE: mov	#1,[[LE_HI:r[0-9]+]]
; LE-NEXT: mov.l	[[LE_HI]],@(4,r15)
; LE: mov	#2,[[LE_LO:r[0-9]+]]
; LE-NEXT: mov.l	[[LE_LO]],@r15
; COMMON: bsr	callee_stack_constant
	call void @callee_stack_constant(i32 0, i32 1, i32 2, i32 3, i64 4294967298)
	ret void
}

define internal i64 @callee_multiple_stack(i32 %a, i32 %b, i32 %c, i32 %d, i64 %first, i64 %second) {
	%result = xor i64 %first, %second
	ret i64 %result
}

define i64 @direct_multiple_stack(i32 %a, i32 %b, i32 %c, i32 %d, i64 %first, i64 %second) {
; COMMON-LABEL: direct_multiple_stack:
; COMMON: sts.l	pr,@-r15
; COMMON: add	#-16,r15
; COMMON: mov.l
; COMMON: mov.l
; COMMON: mov.l
; COMMON: mov.l
; COMMON: bsr	callee_multiple_stack
; COMMON: add	#16,r15
; COMMON: lds.l	@r15+,pr
; COMMON: rts
	%result = call i64 @callee_multiple_stack(i32 %a, i32 %b, i32 %c, i32 %d, i64 %first, i64 %second)
	ret i64 %result
}

define i64 @indirect_i64(ptr %callee, i64 %value) {
; COMMON-LABEL: indirect_i64:
; COMMON: sts.l	pr,@-r15
; COMMON: jsr	@
; COMMON: lds.l	@r15+,pr
; COMMON: rts
	%result = call i64 %callee(i64 %value)
	ret i64 %result
}

; MIR-LABEL: name: three_i32_i64
; MIR: liveins:
; MIR: { reg: '$r7'
; MIR: fixedStack:
; MIR: - { id: 0, type: default, offset: 0, size: 4, alignment: 4, stack-id: default,
; MIR-NEXT: isImmutable: true
; MIR-LABEL: name: four_i32_i64
; MIR: fixedStack:
; MIR-DAG: - { id: 0, type: default, offset: 4, size: 4, alignment: 4, stack-id: default,
; MIR-DAG: - { id: 1, type: default, offset: 0, size: 4, alignment: 4, stack-id: default,
; MIR-LABEL: name: direct_partial
; MIR: ADJCALLSTACKDOWN 4, 0
; MIR: MOVL_store
; MIR: BSR @callee_partial
; MIR: ADJCALLSTACKUP 4, 0
; MIR-LABEL: name: direct_stack
; MIR: ADJCALLSTACKDOWN 8, 0
; MIR-COUNT-2: MOVL_store
; MIR: BSR @callee_stack
; MIR: ADJCALLSTACKUP 8, 0
; MIR-LABEL: name: direct_multiple_stack
; MIR: ADJCALLSTACKDOWN 16, 0
; MIR-COUNT-4: MOVL_store
; MIR: BSR @callee_multiple_stack
; MIR: ADJCALLSTACKUP 16, 0
