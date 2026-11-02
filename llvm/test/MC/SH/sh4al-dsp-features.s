! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4a -mattr=+dsp,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -filetype=obj %s -o %t.composed.o
! RUN: llvm-readobj --file-headers %t.composed.o | FileCheck %s --check-prefix=SH4AL
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4a -mattr=+dsp -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=BAD-BASE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh2 -mattr=+dsp -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=BAD-BASE
! RUN: echo nop | llvm-mc -triple=shle-unknown-elf -mcpu=sh4al-dsp -mattr=-dsp,-sh4al-dsp -filetype=obj -o %t.base.o
! RUN: llvm-readobj --file-headers %t.base.o | FileCheck %s --check-prefix=SH4A-NOFPU
! RUN: echo nop | llvm-mc -triple=shle-unknown-elf -mcpu=sh4al-dsp -mattr=+sh4al-dsp,-dsp -filetype=obj -o %t.cleared.o
! RUN: llvm-readobj --file-headers %t.cleared.o | FileCheck %s --check-prefix=SH4A-NOFPU
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4al-dsp -mattr=-dsp,-sh4al-dsp -filetype=null %s 2>&1 | FileCheck %s --check-prefix=NO-DSP

! SH4AL: Flags [ (0x6)
! SH4A-NOFPU: Flags [ (0x11)
! BAD-BASE: error: SH DSP features require the no-FPU SH-4A architectural base
! NO-DSP: error: instruction requires an unavailable feature

	padd x0,y0,a0
