! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2 -filetype=obj -o %t.be.o %s
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh2 -filetype=obj -o %t.le.o %s
! RUN: llvm-readobj --file-headers --sections --symbols --hex-dump=.text --hex-dump=.data %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
! RUN: llvm-readobj --file-headers --sections --symbols --hex-dump=.text --hex-dump=.data %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
! RUN: llvm-readelf -h -S -s %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
! RUN: llvm-readelf -h -S -s %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
! RUN: llvm-objdump -s %t.be.o | FileCheck %s --check-prefix=CONTENTS-BE
! RUN: llvm-objdump -s %t.le.o | FileCheck %s --check-prefix=CONTENTS-LE
! RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefixes=DISASM,DISASM-BE
! RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefixes=DISASM,DISASM-LE

! BE: Format: elf32-sh
! BE-NEXT: Arch: sh
! LE: Format: elf32-shl
! LE-NEXT: Arch: shle
! COMMON: AddressSize: 32bit
! COMMON: Class: 32-bit (0x1)
! BE: DataEncoding: BigEndian (0x2)
! LE: DataEncoding: LittleEndian (0x1)
! COMMON: OS/ABI: SystemV (0x0)
! COMMON-NEXT: ABIVersion: 0
! COMMON: Type: Relocatable (0x1)
! COMMON-NEXT: Machine: EM_SH (0x2A)
! COMMON: Flags [ (0x2)
! COMMON-NEXT: 0x2

! COMMON: Name: .text
! COMMON: Type: SHT_PROGBITS (0x1)
! COMMON: SHF_ALLOC (0x2)
! COMMON-NEXT: SHF_EXECINSTR (0x4)
! COMMON: Size: 8
! COMMON: AddressAlignment: 2

! COMMON: Name: sh_add
! COMMON: Value: 0x0
! COMMON-NEXT: Size: 8
! COMMON-NEXT: Binding: Global (0x1)
! COMMON-NEXT: Type: Function (0x2)
! COMMON: Section: .text

! BE: Hex dump of section '.text':
! BE-NEXT: 0x00000000 6043305c 000b0009
! BE: Hex dump of section '.data':
! BE-NEXT: 0x00000000 ab123412 345678
! LE: Hex dump of section '.text':
! LE-NEXT: 0x00000000 43605c30 0b000900
! LE: Hex dump of section '.data':
! LE-NEXT: 0x00000000 ab341278 563412

! READELF: Class: ELF32
! READELF-BE: Data: 2's complement, big endian
! READELF-LE: Data: 2's complement, little endian
! READELF: OS/ABI: UNIX - System V
! READELF-NEXT: ABI Version: 0
! READELF: Type: REL (Relocatable file)
! READELF-NEXT: Machine: Hitachi SH
! READELF: Flags: 0x2
! READELF: .text
! READELF-SAME: PROGBITS
! READELF-SAME: AX
! READELF-SAME: 2
! READELF: 1: 00000000 8 FUNC GLOBAL DEFAULT
! READELF-SAME: sh_add

! CONTENTS-BE: Contents of section .text:
! CONTENTS-BE-NEXT: 0000 6043305c 000b0009
! CONTENTS-BE: Contents of section .data:
! CONTENTS-BE-NEXT: 0000 ab123412 345678
! CONTENTS-LE: Contents of section .text:
! CONTENTS-LE-NEXT: 0000 43605c30 0b000900
! CONTENTS-LE: Contents of section .data:
! CONTENTS-LE-NEXT: 0000 ab341278 563412

! DISASM-BE: file format elf32-sh
! DISASM-LE: file format elf32-shl
! DISASM-LABEL: <sh_add>:
! DISASM-NEXT: 0: {{(60 43|43 60)}} mov r4,r0
! DISASM-NEXT: 2: {{(30 5c|5c 30)}} add r5,r0
! DISASM-NEXT: 4: {{(00 0b|0b 00)}} rts
! DISASM-NEXT: 6: {{(00 09|09 00)}} nop

.text
.globl sh_add
.type sh_add,@function
sh_add:
	mov r4,r0
	add r5,r0
	rts
	nop
.size sh_add,.-sh_add

.data
.byte 0xab
.short 0x1234
.long 0x12345678
