; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -target-abi=gnu -filetype=obj < %s -o %t.mixed.be.o
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu -filetype=obj < %s -o %t.mixed.le.o
; RUN: llvm-readobj --file-headers %t.mixed.be.o %t.mixed.le.o | FileCheck %s --check-prefix=MIXED
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -target-abi=gnu -filetype=obj < %s -o %t.rank.be.o
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -target-abi=gnu -filetype=obj < %s -o %t.rank.le.o
; RUN: llvm-readobj --file-headers %t.rank.be.o %t.rank.le.o | FileCheck %s --check-prefix=NOFPU-RANK
; RUN: llvm-extract -func=nofpu_only < %s | llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -target-abi=gnu -filetype=obj -o %t.nofpu.o
; RUN: llvm-readobj --file-headers %t.nofpu.o | FileCheck %s --check-prefix=NOFPU

; MIXED-COUNT-2: Flags [ (0x9)
; NOFPU-RANK-COUNT-2: Flags [ (0x10)
; NOFPU: Flags [ (0x10)

define i32 @nofpu_only(i32 %value) #0 {
	%result = add i32 %value, 1
	ret i32 %result
}

define float @full_fpu(float %left, float %right) {
	%result = fadd float %left, %right
	ret float %result
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "use-soft-float"="true" }

!0 = !{i32 1, !"target-abi", !"gnu"}
!llvm.module.flags = !{!0}
