; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-readelf -h -s %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
; RUN: llvm-readelf -h -s %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefixes=DISASM,DISASM-BE
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefixes=DISASM,DISASM-LE

define i32 @load_s8(ptr %p) minsize nounwind {
	%value = load i8, ptr %p, align 1
	%result = sext i8 %value to i32
	ret i32 %result
}

define i32 @load_u16(ptr %p) minsize nounwind {
	%value = load i16, ptr %p, align 2
	%result = zext i16 %value to i32
	ret i32 %result
}

define void @store_i8(ptr %p, i32 %value) minsize nounwind {
	%narrow = trunc i32 %value to i8
	store i8 %narrow, ptr %p, align 1
	ret void
}

define void @store_i16(ptr %p, i32 %value) minsize nounwind {
	%narrow = trunc i32 %value to i16
	store i16 %narrow, ptr %p, align 2
	ret void
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
; COMMON: Size: 26
; COMMON: Relocations [
; COMMON-NEXT: ]

; COMMON: Name: load_s8
; COMMON-NEXT: Value: 0x0
; COMMON-NEXT: Size: 6
; COMMON: Name: load_u16
; COMMON-NEXT: Value: 0x6
; COMMON-NEXT: Size: 8
; COMMON: Name: store_i8
; COMMON-NEXT: Value: 0xE
; COMMON-NEXT: Size: 6
; COMMON: Name: store_i16
; COMMON-NEXT: Value: 0x14
; COMMON-NEXT: Size: 6

; BE: Hex dump of section '.text':
; BE-NEXT: 0x00000000 6040000b 00096041 600d000b 00092450
; BE-NEXT: 0x00000010 000b0009 2451000b 0009
; LE: Hex dump of section '.text':
; LE-NEXT: 0x00000000 40600b00 09004160 0d600b00 09005024
; LE-NEXT: 0x00000010 0b000900 51240b00 0900

; READELF: Class: ELF32
; READELF-BE: Data: 2's complement, big endian
; READELF-LE: Data: 2's complement, little endian
; READELF: Machine: Hitachi SH
; READELF: Flags: 0x2
; READELF: 2: 00000000 6 FUNC GLOBAL DEFAULT
; READELF-SAME: load_s8
; READELF: 3: 00000006 8 FUNC GLOBAL DEFAULT
; READELF-SAME: load_u16
; READELF: 4: 0000000e 6 FUNC GLOBAL DEFAULT
; READELF-SAME: store_i8
; READELF: 5: 00000014 6 FUNC GLOBAL DEFAULT
; READELF-SAME: store_i16

; DISASM-LABEL: <load_s8>:
; DISASM-BE-NEXT: 0: 60 40 mov.b @r4,r0
; DISASM-LE-NEXT: 0: 40 60 mov.b @r4,r0
; DISASM-LABEL: <load_u16>:
; DISASM-BE-NEXT: 6: 60 41 mov.w @r4,r0
; DISASM-BE-NEXT: 8: 60 0d extu.w r0,r0
; DISASM-LE-NEXT: 6: 41 60 mov.w @r4,r0
; DISASM-LE-NEXT: 8: 0d 60 extu.w r0,r0
; DISASM-LABEL: <store_i8>:
; DISASM-BE-NEXT: e: 24 50 mov.b r5,@r4
; DISASM-LE-NEXT: e: 50 24 mov.b r5,@r4
; DISASM-LABEL: <store_i16>:
; DISASM-BE-NEXT: 14: 24 51 mov.w r5,@r4
; DISASM-LE-NEXT: 14: 51 24 mov.w r5,@r4
