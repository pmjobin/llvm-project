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
; COMMON: Size: 18
; COMMON: Relocations [
; COMMON-NEXT: ]

; COMMON: Name: load_s8
; COMMON-NEXT: Value: 0x0
; COMMON-NEXT: Size: 4
; COMMON: Name: load_u16
; COMMON-NEXT: Value: 0x4
; COMMON-NEXT: Size: 6
; COMMON: Name: store_i8
; COMMON-NEXT: Value: 0xA
; COMMON-NEXT: Size: 4
; COMMON: Name: store_i16
; COMMON-NEXT: Value: 0xE
; COMMON-NEXT: Size: 4

; BE: Hex dump of section '.text':
; BE-NEXT: 0x00000000 000b6040 6041000b 600d000b 2450000b
; BE-NEXT: 0x00000010 2451
; LE: Hex dump of section '.text':
; LE-NEXT: 0x00000000 0b004060 41600b00 0d600b00 50240b00
; LE-NEXT: 0x00000010 5124

; READELF: Class: ELF32
; READELF-BE: Data: 2's complement, big endian
; READELF-LE: Data: 2's complement, little endian
; READELF: Machine: Hitachi SH
; READELF: Flags: 0x2
; READELF: 2: 00000000 4 FUNC GLOBAL DEFAULT
; READELF-SAME: load_s8
; READELF: 3: 00000004 6 FUNC GLOBAL DEFAULT
; READELF-SAME: load_u16
; READELF: 4: 0000000a 4 FUNC GLOBAL DEFAULT
; READELF-SAME: store_i8
; READELF: 5: 0000000e 4 FUNC GLOBAL DEFAULT
; READELF-SAME: store_i16

; DISASM-LABEL: <load_s8>:
; DISASM-BE-NEXT: 0: 00 0b rts
; DISASM-BE-NEXT: 2: 60 40 mov.b @r4,r0
; DISASM-LE-NEXT: 0: 0b 00 rts
; DISASM-LE-NEXT: 2: 40 60 mov.b @r4,r0
; DISASM-LABEL: <load_u16>:
; DISASM-BE-NEXT: 4: 60 41 mov.w @r4,r0
; DISASM-BE-NEXT: 6: 00 0b rts
; DISASM-BE-NEXT: 8: 60 0d extu.w r0,r0
; DISASM-LE-NEXT: 4: 41 60 mov.w @r4,r0
; DISASM-LE-NEXT: 6: 0b 00 rts
; DISASM-LE-NEXT: 8: 0d 60 extu.w r0,r0
; DISASM-LABEL: <store_i8>:
; DISASM-BE-NEXT: a: 00 0b rts
; DISASM-BE-NEXT: c: 24 50 mov.b r5,@r4
; DISASM-LE-NEXT: a: 0b 00 rts
; DISASM-LE-NEXT: c: 50 24 mov.b r5,@r4
; DISASM-LABEL: <store_i16>:
; DISASM-BE-NEXT: e: 00 0b rts
; DISASM-BE-NEXT: 10: 24 51 mov.w r5,@r4
; DISASM-LE-NEXT: e: 0b 00 rts
; DISASM-LE-NEXT: 10: 51 24 mov.w r5,@r4
