; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --file-headers --sections --symbols --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj --file-headers --sections --symbols --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-readelf -h %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
; RUN: llvm-readelf -h %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DISASM-BE
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DISASM-LE

define i32 @add_i32(i32 %a, i32 %b) {
	%sum = add i32 %a, %b
	ret i32 %sum
}

; BE: Format: elf32-sh
; BE-NEXT: Arch: sh
; LE: Format: elf32-shl
; LE-NEXT: Arch: shle
; COMMON: AddressSize: 32bit
; COMMON: Class: 32-bit (0x1)
; BE: DataEncoding: BigEndian (0x2)
; LE: DataEncoding: LittleEndian (0x1)
; COMMON: Type: Relocatable (0x1)
; COMMON-NEXT: Machine: EM_SH (0x2A)
; COMMON: Flags [ (0x2)
; COMMON-NEXT: 0x2

; COMMON: Name: .text
; COMMON: Type: SHT_PROGBITS (0x1)
; COMMON: SHF_ALLOC (0x2)
; COMMON-NEXT: SHF_EXECINSTR (0x4)
; COMMON: Size: 8

; COMMON: Name: add_i32
; COMMON-NEXT: Value: 0x0
; COMMON-NEXT: Size: 8
; COMMON-NEXT: Binding: Global (0x1)
; COMMON-NEXT: Type: Function (0x2)
; COMMON: Section: .text

; BE: Hex dump of section '.text':
; BE-NEXT: 0x00000000 6043305c 000b0009
; LE: Hex dump of section '.text':
; LE-NEXT: 0x00000000 43605c30 0b000900

; READELF: Class: ELF32
; READELF-BE: Data: 2's complement, big endian
; READELF-LE: Data: 2's complement, little endian
; READELF: Type: REL (Relocatable file)
; READELF-NEXT: Machine: Hitachi SH
; READELF: Flags: 0x2

; DISASM-BE: file format elf32-sh
; DISASM-BE-LABEL: <add_i32>:
; DISASM-BE-NEXT: 0: 60 43 mov r4,r0
; DISASM-BE-NEXT: 2: 30 5c add r5,r0
; DISASM-BE-NEXT: 4: 00 0b rts
; DISASM-BE-NEXT: 6: 00 09 nop

; DISASM-LE: file format elf32-shl
; DISASM-LE-LABEL: <add_i32>:
; DISASM-LE-NEXT: 0: 43 60 mov r4,r0
; DISASM-LE-NEXT: 2: 5c 30 add r5,r0
; DISASM-LE-NEXT: 4: 0b 00 rts
; DISASM-LE-NEXT: 6: 09 00 nop
