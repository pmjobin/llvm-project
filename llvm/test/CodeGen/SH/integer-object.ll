; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-readelf -h -s -r %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
; RUN: llvm-readelf -h -s -r %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefixes=DISASM,DISASM-BE
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefixes=DISASM,DISASM-LE

define i32 @constant_deadbeef() minsize nounwind {
	ret i32 -559038737
}

define i32 @subtract(i32 %a, i32 %b) minsize nounwind {
	%result = sub i32 %a, %b
	ret i32 %result
}

define i32 @negate(i32 %value) minsize nounwind {
	%result = sub i32 0, %value
	ret i32 %result
}

define i32 @complement(i32 %value) minsize nounwind {
	%result = xor i32 %value, -1
	ret i32 %result
}

define i32 @left_shift_24(i32 %value) minsize nounwind {
	%result = shl i32 %value, 24
	ret i32 %result
}

define i32 @variable_logical_right(i32 %value, i32 %count) minsize nounwind {
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
; COMMON: Size: 50
; COMMON: Name: .note.GNU-stack
; COMMON: Size: 0
; COMMON-NOT: Name: .rodata
; COMMON-NOT: Name: .data
; COMMON: Relocations [
; COMMON-NEXT: ]
; COMMON: Name: constant_deadbeef
; COMMON-NEXT: Value: 0x0
; COMMON-NEXT: Size: 12
; COMMON: Name: subtract
; COMMON-NEXT: Value: 0xC
; COMMON-NEXT: Size: 6
; COMMON: Name: negate
; COMMON-NEXT: Value: 0x12
; COMMON-NEXT: Size: 4
; COMMON: Name: complement
; COMMON-NEXT: Value: 0x16
; COMMON-NEXT: Size: 4
; COMMON: Name: left_shift_24
; COMMON-NEXT: Value: 0x1A
; COMMON-NEXT: Size: 8
; COMMON: Name: variable_logical_right
; COMMON-NEXT: Value: 0x22
; COMMON-NEXT: Size: 16

; BE: Hex dump of section '.text':
; BE-NEXT: 0x00000000 d001000b 00090009 deadbeef 6043000b
; BE-NEXT: 0x00000010 3058000b 604b000b 60476043 4028000b
; BE-NEXT: 0x00000020 40182558 8d036043 40014510 8bfc000b
; BE-NEXT: 0x00000030 0009
; LE: Hex dump of section '.text':
; LE-NEXT: 0x00000000 01d00b00 09000900 efbeadde 43600b00
; LE-NEXT: 0x00000010 58300b00 4b600b00 47604360 28400b00
; LE-NEXT: 0x00000020 18405825 038d4360 01401045 fc8b0b00
; LE-NEXT: 0x00000030 0900

; READELF: Class: ELF32
; READELF-BE: Data: 2's complement, big endian
; READELF-LE: Data: 2's complement, little endian
; READELF: Machine: Hitachi SH
; READELF: Flags: 0x2
; READELF: There are no relocations in this file.
; READELF: 2: 00000000 12 FUNC GLOBAL DEFAULT
; READELF-SAME: constant_deadbeef
; READELF: 7: 00000022 16 FUNC GLOBAL DEFAULT
; READELF-SAME: variable_logical_right

; DISASM-LABEL: <constant_deadbeef>:
; DISASM-BE-NEXT: 0: d0 01 mov.l {{.*}},r0
; DISASM-LE-NEXT: 0: 01 d0 mov.l {{.*}},r0
; DISASM-LABEL: <subtract>:
; DISASM-BE-NEXT: c: 60 43 mov r4,r0
; DISASM-BE-NEXT: e: 00 0b rts
; DISASM-BE-NEXT: 10: 30 58 sub r5,r0
; DISASM-LE-NEXT: c: 43 60 mov r4,r0
; DISASM-LE-NEXT: e: 0b 00 rts
; DISASM-LE-NEXT: 10: 58 30 sub r5,r0
; DISASM-LABEL: <negate>:
; DISASM-BE-NEXT: 12: 00 0b rts
; DISASM-BE-NEXT: 14: 60 4b neg r4,r0
; DISASM-LE-NEXT: 12: 0b 00 rts
; DISASM-LE-NEXT: 14: 4b 60 neg r4,r0
; DISASM-LABEL: <complement>:
; DISASM-BE-NEXT: 16: 00 0b rts
; DISASM-BE-NEXT: 18: 60 47 not r4,r0
; DISASM-LE-NEXT: 16: 0b 00 rts
; DISASM-LE-NEXT: 18: 47 60 not r4,r0
; DISASM-LABEL: <left_shift_24>:
; DISASM-BE: 1c: 40 28 shll16 r0
; DISASM-BE: 20: 40 18 shll8 r0
; DISASM-LE: 1c: 28 40 shll16 r0
; DISASM-LE: 20: 18 40 shll8 r0
; DISASM-LABEL: <variable_logical_right>:
; DISASM-BE: 22: 25 58 tst r5,r5
; DISASM-BE-NEXT: 24: 8d 03 bt/s
; DISASM-BE-NEXT: 26: 60 43 mov r4,r0
; DISASM-BE-NEXT: 28: 40 01 shlr r0
; DISASM-BE-NEXT: 2a: 45 10 dt r5
; DISASM-BE-NEXT: 2c: 8b fc bf
; DISASM-LE: 22: 58 25 tst r5,r5
; DISASM-LE-NEXT: 24: 03 8d bt/s
; DISASM-LE-NEXT: 26: 43 60 mov r4,r0
; DISASM-LE-NEXT: 28: 01 40 shlr r0
; DISASM-LE-NEXT: 2a: 10 45 dt r5
; DISASM-LE-NEXT: 2c: fc 8b bf
