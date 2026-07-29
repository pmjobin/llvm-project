! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s --check-prefix=PRINT
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DISASM
! RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DISASM

! CHECK: mov.l	@(0,pc),r4
! BE-SAME: encoding: [0xd4,0x00]
! LE-SAME: encoding: [0x00,0xd4]
! PRINT: mov.l	@(0,pc),r4
mov.l @(0,pc),r4

! CHECK: mov.l	@(12,pc),r4
! BE-SAME: encoding: [0xd4,0x03]
! LE-SAME: encoding: [0x03,0xd4]
! PRINT: mov.l	@(12,pc),r4
mov.l @(12,pc),r4

! CHECK: mov.l	@(1020,pc),r4
! BE-SAME: encoding: [0xd4,0xff]
! LE-SAME: encoding: [0xff,0xd4]
! PRINT: mov.l	@(1020,pc),r4
mov.l @(1020,pc),r4

! Put the two symbolic loads at P mod 4 equal to zero and two. Both aligned
! PC bases are twelve, so both reach the same target with displacement four.
.p2align 2
! CHECK: mov.l	.Lsame_target,r5
! BE-SAME: encoding: [0xd5'A',0x00]
! LE-SAME: encoding: [A,0xd5]
! CHECK: fixup A - offset: 0, value: .Lsame_target, kind: fixup_SH_PCREL8_4
! PRINT: mov.l	.Lsame_target,r5
mov.l .Lsame_target,r5
! CHECK: mov.l	.Lsame_target,r6
! BE-SAME: encoding: [0xd6'A',0x00]
! LE-SAME: encoding: [A,0xd6]
! CHECK: fixup A - offset: 0, value: .Lsame_target, kind: fixup_SH_PCREL8_4
! PRINT: mov.l	.Lsame_target,r6
mov.l .Lsame_target,r6
nop
nop
.Lsame_target:
.long 0

! DISASM: mov.l	4,r4
! DISASM: mov.l	16,r4
! DISASM: mov.l	1028,r4
! DISASM: mov.l	16,r5
! DISASM: mov.l	16,r6
