; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector.ll -o - | FileCheck %s --check-prefix=VECTOR
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i128.ll -o - | FileCheck %s --check-prefix=I128
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i40.ll -o - | FileCheck %s --check-prefix=SCALAR40
; RUN: not llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-i40.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=I40
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/scalable.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SCALABLE

; VECTOR-LABEL: unaligned_vector:
; VECTOR: mov.l	@r4,r0
; I128-LABEL: unaligned_i128:
; I128: mov.l	@(12,r4),r0
; I40: error: {{.*}}SH integer memory operations require a power-of-two bit width
; SCALAR40-LABEL: unaligned_i40:
; SCALAR40: mov.b
; SCALABLE: error: {{.*}}SH scalable vector memory operations are not supported

;--- vector.ll
define i32 @unaligned_vector(ptr %p) {
	%value = load <2 x i16>, ptr %p, align 4
	%result = bitcast <2 x i16> %value to i32
	ret i32 %result
}

;--- i128.ll
define i32 @unaligned_i128(ptr %p) {
	%value = load i128, ptr %p, align 4
	%result = trunc i128 %value to i32
	ret i32 %result
}

;--- i40.ll
define i32 @unaligned_i40(ptr %p) {
	%value = load i40, ptr %p, align 1
	%result = trunc i40 %value to i32
	ret i32 %result
}

;--- vector-i40.ll
define void @unaligned_vector_i40(ptr %p) {
	%v0 = insertelement <2 x i40> poison, i40 1, i32 0
	%value = insertelement <2 x i40> %v0, i40 2, i32 1
	store <2 x i40> %value, ptr %p, align 1
	ret void
}

;--- scalable.ll
define void @scalable_memory(ptr %p) {
	%value = load <vscale x 2 x i32>, ptr %p, align 4
	store <vscale x 2 x i32> %value, ptr %p, align 4
	ret void
}
