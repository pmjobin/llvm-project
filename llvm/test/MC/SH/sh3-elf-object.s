! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh3 -filetype=obj -o %t.be.o %s
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh3 -filetype=obj -o %t.le.o %s
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
! RUN: llvm-readelf -h %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
! RUN: llvm-readelf -h %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
! RUN: llvm-objdump -d --mcpu=sh3 %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d --mcpu=sh3 %t.le.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d %t.be.o 2>&1 | FileCheck %s --check-prefix=EXPLICIT-CPU
! RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh3 -mattr=-mmu -filetype=obj -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=IDENTITY
! RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh2 -mattr=+dynamic-shift -filetype=obj -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=IDENTITY

! COMMON: Class: 32-bit (0x1)
! BE: DataEncoding: BigEndian (0x2)
! LE: DataEncoding: LittleEndian (0x1)
! COMMON: Machine: EM_SH (0x2A)
! COMMON: Flags [ (0x3)
! COMMON-NEXT: 0x1
! COMMON-NEXT: 0x2
! BE: Hex dump of section '.text':
! BE-NEXT: 0x00000000 432c0009
! LE: Hex dump of section '.text':
! LE-NEXT: 0x00000000 2c430900

! READELF: Class: ELF32
! READELF-BE: Data: 2's complement, big endian
! READELF-LE: Data: 2's complement, little endian
! READELF: Machine: Hitachi SH
! READELF: Flags: 0x3

! DIS: shad	r2,r3
! DIS-NEXT: nop
! EXPLICIT-CPU: <unknown>
! IDENTITY: error: SH feature combination has no supported ELF machine identity

shad r2,r3
nop
