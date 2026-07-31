! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s --check-prefix=PRINT
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DISASM

! CHECK: mova	@(0,pc),r0
! BE-SAME: encoding: [0xc7,0x00]
! LE-SAME: encoding: [0x00,0xc7]
! PRINT: mova	@(0,pc),r0
mova @(0,pc),r0

! CHECK: mova	@(12,pc),r0
! BE-SAME: encoding: [0xc7,0x03]
! LE-SAME: encoding: [0x03,0xc7]
! PRINT: mova	@(12,pc),r0
mova @(12,pc),r0

! CHECK: mova	@(1020,pc),r0
! BE-SAME: encoding: [0xc7,0xff]
! LE-SAME: encoding: [0xff,0xc7]
! PRINT: mova	@(1020,pc),r0
mova @(1020,pc),r0

.p2align 2
! CHECK: mova	.Lsame_target,r0
! BE-SAME: encoding: [0xc7'A',0x00]
! LE-SAME: encoding: [A,0xc7]
! CHECK: fixup A - offset: 0, value: .Lsame_target, kind: fixup_SH_PCREL8_4
! PRINT: mova	.Lsame_target,r0
mova .Lsame_target,r0
! CHECK: mova	.Lsame_target,r0
! BE-SAME: encoding: [0xc7'A',0x00]
! LE-SAME: encoding: [A,0xc7]
! CHECK: fixup A - offset: 0, value: .Lsame_target, kind: fixup_SH_PCREL8_4
! PRINT: mova	.Lsame_target,r0
mova .Lsame_target,r0
nop
nop
.Lsame_target:
.long 0

! DISASM: mova	4,r0
! DISASM: mova	16,r0
! DISASM: mova	1028,r0
! DISASM: mova	16,r0
! DISASM: mova	16,r0
