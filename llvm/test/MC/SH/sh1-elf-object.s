! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh1 -filetype=obj -o %t.be.o %s
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh1 -filetype=obj -o %t.le.o %s
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
! RUN: llvm-readelf -h %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
! RUN: llvm-readelf -h %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
! RUN: llvm-objdump -d --mcpu=sh1 %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d --mcpu=sh1 %t.le.o | FileCheck %s --check-prefix=DIS

! COMMON: Class: 32-bit (0x1)
! BE: DataEncoding: BigEndian (0x2)
! LE: DataEncoding: LittleEndian (0x1)
! COMMON: Machine: EM_SH (0x2A)
! COMMON: Flags [ (0x1)
! COMMON-NEXT: 0x1
! BE: Hex dump of section '.text':
! BE-NEXT: 0x00000000 62130c0c 412b0009
! LE: Hex dump of section '.text':
! LE-NEXT: 0x00000000 13620c0c 2b410900

! READELF: Class: ELF32
! READELF-BE: Data: 2's complement, big endian
! READELF-LE: Data: 2's complement, little endian
! READELF: Machine: Hitachi SH
! READELF: Flags: 0x1

! DIS: mov	r1,r2
! DIS-NEXT: mov.b	@(r0,r0),r12
! DIS-NEXT: jmp	@r1
! DIS-NEXT: nop

	mov	r1,r2
	mov.b	@(r0,r0),r12
	jmp	@r1
	nop
