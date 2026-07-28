; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-readelf -h -s -r %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
; RUN: llvm-readelf -h -s -r %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefixes=DISASM,DISASM-BE
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefixes=DISASM,DISASM-LE

define i32 @constant_deadbeef() minsize {
	ret i32 -559038737
}

define i32 @subtract(i32 %a, i32 %b) minsize {
	%result = sub i32 %a, %b
	ret i32 %result
}

define i32 @negate(i32 %value) minsize {
	%result = sub i32 0, %value
	ret i32 %result
}

define i32 @complement(i32 %value) minsize {
	%result = xor i32 %value, -1
	ret i32 %result
}

define i32 @left_shift_24(i32 %value) minsize {
	%result = shl i32 %value, 24
	ret i32 %result
}

define i32 @variable_logical_right(i32 %value, i32 %count) minsize {
	%result = lshr i32 %value, %count
	ret i32 %result
}

; BE: Format: elf32-sh
; BE-NEXT: Arch: sh
; LE: Format: elf32-shl
; LE-NEXT: Arch: shle
; COMMON: AddressSize: 32bit
; COMMON: Class: 32-bit (0x1)
; BE: DataEncoding: BigEndian (0x2)
; LE: DataEncoding: LittleEndian (0x1)
; COMMON: Machine: EM_SH (0x2A)
; COMMON: Flags [ (0x2)
; COMMON-NEXT: 0x2
; COMMON: Name: .text
; COMMON: Type: SHT_PROGBITS (0x1)
; COMMON: Size: 82
; COMMON: Name: .note.GNU-stack
; COMMON: Size: 0
; COMMON-NOT: Name: .rodata
; COMMON-NOT: Name: .data
; COMMON: Relocations [
; COMMON-NEXT: ]
; COMMON: Name: constant_deadbeef
; COMMON-NEXT: Value: 0x0
; COMMON-NEXT: Size: 32
; COMMON: Name: subtract
; COMMON-NEXT: Value: 0x20
; COMMON-NEXT: Size: 8
; COMMON: Name: negate
; COMMON-NEXT: Value: 0x28
; COMMON-NEXT: Size: 6
; COMMON: Name: complement
; COMMON-NEXT: Value: 0x2E
; COMMON-NEXT: Size: 6
; COMMON: Name: left_shift_24
; COMMON-NEXT: Value: 0x34
; COMMON-NEXT: Size: 10
; COMMON: Name: variable_logical_right
; COMMON-NEXT: Value: 0x3E
; COMMON-NEXT: Size: 20

; BE: Hex dump of section '.text':
; BE-NEXT: 0x00000000 e0de600c 4018e1ad 611c210b 4118e0be
; BE-NEXT: 0x00000010 620c221b 4218e0ef 600c202b 000b0009
; BE-NEXT: 0x00000020 60433058 000b0009 604b000b 00096047
; BE-NEXT: 0x00000030 000b0009 60434028 4018000b 00096043
; BE-NEXT: 0x00000040 e11f2159 21188902 40014110 8bfc000b
; BE-NEXT: 0x00000050 0009
; LE: Hex dump of section '.text':
; LE-NEXT: 0x00000000 dee00c60 1840ade1 1c610b21 1841bee0
; LE-NEXT: 0x00000010 0c621b22 1842efe0 0c602b20 0b000900
; LE-NEXT: 0x00000020 43605830 0b000900 4b600b00 09004760
; LE-NEXT: 0x00000030 0b000900 43602840 18400b00 09004360
; LE-NEXT: 0x00000040 1fe15921 18210289 01401041 fc8b0b00
; LE-NEXT: 0x00000050 0900

; READELF: Class: ELF32
; READELF-BE: Data: 2's complement, big endian
; READELF-LE: Data: 2's complement, little endian
; READELF: Machine: Hitachi SH
; READELF: Flags: 0x2
; READELF: There are no relocations in this file.
; READELF: 2: 00000000 32 FUNC GLOBAL DEFAULT
; READELF-SAME: constant_deadbeef
; READELF: 7: 0000003e 20 FUNC GLOBAL DEFAULT
; READELF-SAME: variable_logical_right

; DISASM-LABEL: <constant_deadbeef>:
; DISASM-BE-NEXT: 0: e0 de mov #-34,r0
; DISASM-LE-NEXT: 0: de e0 mov #-34,r0
; DISASM: shll8
; DISASM: or
; DISASM-LABEL: <subtract>:
; DISASM-BE-NEXT: 20: 60 43 mov r4,r0
; DISASM-BE-NEXT: 22: 30 58 sub r5,r0
; DISASM-LE-NEXT: 20: 43 60 mov r4,r0
; DISASM-LE-NEXT: 22: 58 30 sub r5,r0
; DISASM-LABEL: <negate>:
; DISASM-BE-NEXT: 28: 60 4b neg r4,r0
; DISASM-LE-NEXT: 28: 4b 60 neg r4,r0
; DISASM-LABEL: <complement>:
; DISASM-BE-NEXT: 2e: 60 47 not r4,r0
; DISASM-LE-NEXT: 2e: 47 60 not r4,r0
; DISASM-LABEL: <left_shift_24>:
; DISASM-BE: 36: 40 28 shll16 r0
; DISASM-BE-NEXT: 38: 40 18 shll8 r0
; DISASM-LE: 36: 28 40 shll16 r0
; DISASM-LE-NEXT: 38: 18 40 shll8 r0
; DISASM-LABEL: <variable_logical_right>:
; DISASM-BE: 42: 21 59 and r5,r1
; DISASM-BE-NEXT: 44: 21 18 tst r1,r1
; DISASM-BE-NEXT: 46: 89 02 bt
; DISASM-BE-NEXT: 48: 40 01 shlr r0
; DISASM-BE-NEXT: 4a: 41 10 dt r1
; DISASM-BE-NEXT: 4c: 8b fc bf
; DISASM-LE: 42: 59 21 and r5,r1
; DISASM-LE-NEXT: 44: 18 21 tst r1,r1
; DISASM-LE-NEXT: 46: 02 89 bt
; DISASM-LE-NEXT: 48: 01 40 shlr r0
; DISASM-LE-NEXT: 4a: 10 41 dt r1
; DISASM-LE-NEXT: 4c: fc 8b bf
