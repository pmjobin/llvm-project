; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4al-dsp -target-abi=gnu -O2 -verify-machineinstrs -o %t.sh4al.s %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4a -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -target-abi=gnu -O2 -verify-machineinstrs -o %t.sh4a.s %s
; RUN: FileCheck %s --input-file=%t.sh4al.s --check-prefix=ASM --implicit-check-not=clrdmxy --implicit-check-not=ldrc --implicit-check-not=movs. --implicit-check-not=movx. --implicit-check-not=movy. --implicit-check-not=padd --implicit-check-not=pmuls --implicit-check-not=psub
; RUN: FileCheck %s --input-file=%t.sh4a.s --check-prefix=ASM --implicit-check-not=clrdmxy --implicit-check-not=ldrc --implicit-check-not=movs. --implicit-check-not=movx. --implicit-check-not=movy. --implicit-check-not=padd --implicit-check-not=pmuls --implicit-check-not=psub
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4al-dsp -target-abi=gnu -O2 -verify-machineinstrs -filetype=obj -o %t.be.o %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4al-dsp -target-abi=gnu -O2 -verify-machineinstrs -filetype=obj -o %t.le.o %s
; RUN: llvm-readobj --file-headers %t.be.o %t.le.o | FileCheck %s --check-prefix=OBJECT

declare i32 @llvm.sadd.sat.i32(i32, i32)
declare i32 @llvm.sh.tas.b(ptr)

define i32 @integer(i32 %left, i32 %right) {
	%sum = add i32 %left, %right
	%shift = shl i32 %sum, 7
	ret i32 %shift
}

define i64 @wide_integer(i64 %left, i64 %right) {
	%product = mul i64 %left, %right
	%result = add i64 %product, %left
	ret i64 %result
}

define i32 @saturating(i32 %left, i32 %right) {
	%result = call i32 @llvm.sadd.sat.i32(i32 %left, i32 %right)
	ret i32 %result
}

define i32 @test_and_set(ptr %address) {
	%result = call i32 @llvm.sh.tas.b(ptr %address)
	ret i32 %result
}

define void @sequential_fence() {
	fence seq_cst
	ret void
}

; ASM-LABEL: integer:
; ASM: shld
; ASM-LABEL: saturating:
; ASM-NOT: padd
; ASM-LABEL: test_and_set:
; ASM: tas.b	@r4
; ASM-LABEL: sequential_fence:
; ASM: synco

; OBJECT-COUNT-2: Flags [ (0x6)
