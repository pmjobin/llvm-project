! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj --hex-dump=.text --relocations %t.be.o | FileCheck %s --check-prefix=OBJ-BE
! RUN: llvm-readobj --hex-dump=.text --relocations %t.le.o | FileCheck %s --check-prefix=OBJ-LE
! RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DIS

! ASM: cmp/eq	r5,r4
! BE-SAME: encoding: [0x34,0x50]
! LE-SAME: encoding: [0x50,0x34]
cmp/eq r5,r4

! ASM: cmp/hs	r5,r4
! BE-SAME: encoding: [0x34,0x52]
! LE-SAME: encoding: [0x52,0x34]
cmp/hs r5,r4

! ASM: cmp/ge	r5,r4
! BE-SAME: encoding: [0x34,0x53]
! LE-SAME: encoding: [0x53,0x34]
cmp/ge r5,r4

! ASM: cmp/hi	r5,r4
! BE-SAME: encoding: [0x34,0x56]
! LE-SAME: encoding: [0x56,0x34]
cmp/hi r5,r4

! ASM: cmp/gt	r5,r4
! BE-SAME: encoding: [0x34,0x57]
! LE-SAME: encoding: [0x57,0x34]
cmp/gt r5,r4

! At .Lback, S = 10.
.Lback:
nop

! P = 12, S = 16: (16 - (12 + 4)) / 2 = 0.
bt .Lbt_zero
nop
.Lbt_zero:
nop

! P = 18, S = 10: (10 - (18 + 4)) / 2 = -6.
bf .Lback

! P = 20, S = 26: (26 - (20 + 4)) / 2 = 1.
bra .Lforward
nop
nop
.Lforward:
nop

! P = 28, S = 10: (10 - (28 + 4)) / 2 = -11.
bt .Lback

! P = 30, S = 34: (34 - (30 + 4)) / 2 = 0.
bf .Lend
nop
.Lend:
nop

! P = 36, S = 40: (40 - (36 + 4)) / 2 = 0.
bra .Lbra_zero
nop
.Lbra_zero:
nop

! At .Lbra_back, S = 42.
.Lbra_back:
nop
! P = 44, S = 42: (42 - (44 + 4)) / 2 = -3.
bra .Lbra_back
nop

! OBJ-BE: Relocations [
! OBJ-BE-NEXT: ]
! OBJ-BE: Hex dump of section '.text':
! OBJ-BE-NEXT: 0x00000000 34503452 34533456 34570009 89000009
! OBJ-BE-NEXT: 0x00000010 00098bfa a0010009 00090009 89f58b00
! OBJ-BE-NEXT: 0x00000020 00090009 a0000009 00090009 affd0009

! OBJ-LE: Relocations [
! OBJ-LE-NEXT: ]
! OBJ-LE: Hex dump of section '.text':
! OBJ-LE-NEXT: 0x00000000 50345234 53345634 57340900 00890900
! OBJ-LE-NEXT: 0x00000010 0900fa8b 01a00900 09000900 f589008b
! OBJ-LE-NEXT: 0x00000020 09000900 00a00900 09000900 fdaf0900

! DIS: 0: {{.*}} cmp/eq r5,r4
! DIS: 2: {{.*}} cmp/hs r5,r4
! DIS: 4: {{.*}} cmp/ge r5,r4
! DIS: 6: {{.*}} cmp/hi r5,r4
! DIS: 8: {{.*}} cmp/gt r5,r4
! DIS: c: {{.*}} bt	0x10
! DIS: 12: {{.*}} bf	0xa
! DIS: 14: {{.*}} bra	0x1a
! DIS: 1c: {{.*}} bt	0xa
! DIS: 1e: {{.*}} bf	0x22
! DIS: 24: {{.*}} bra	0x28
! DIS: 2c: {{.*}} bra	0x2a
