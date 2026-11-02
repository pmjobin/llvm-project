; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-readelf -h -s %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
; RUN: llvm-readelf -h -s %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DIS-BE
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DIS-LE

define internal i32 @add_one(i32 %x) minsize nounwind {
	%result = add i32 %x, 1
	ret i32 %result
}

define dso_local i32 @call_add_one(i32 %x) minsize nounwind {
	%result = call i32 @add_one(i32 %x)
	ret i32 %result
}

define i32 @call_pointer(ptr %fn, i32 %value) minsize nounwind {
	%result = call i32 %fn(i32 %value)
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
; COMMON: Size: 42
; COMMON: Relocations [
; COMMON: 0x18 R_SH_DIR32 add_one 0x0

; COMMON: Name: add_one
; COMMON-NEXT: Value: 0x0
; COMMON-NEXT: Size: 6
; COMMON: Name: call_add_one
; COMMON-NEXT: Value: 0x8
; COMMON-NEXT: Size: 20
; COMMON: Name: call_pointer
; COMMON-NEXT: Value: 0x1C
; COMMON-NEXT: Size: 14

; BE: Hex dump of section '.text':
; BE-NEXT: 0x00000000 6043000b 70010009 4f22d103 410b0009
; BE-NEXT: 0x00000010 4f26000b 00090009 00000000 4f226043
; BE-NEXT: 0x00000020 400b6453 4f26000b 0009
; LE: Hex dump of section '.text':
; LE-NEXT: 0x00000000 43600b00 01700900 224f03d1 0b410900
; LE-NEXT: 0x00000010 264f0b00 09000900 00000000 224f4360
; LE-NEXT: 0x00000020 0b405364 264f0b00 0900

; READELF: Class: ELF32
; READELF-BE: Data: 2's complement, big endian
; READELF-LE: Data: 2's complement, little endian
; READELF: Machine: Hitachi SH
; READELF: Flags: 0x2

; DIS-BE-LABEL: <call_add_one>:
; DIS-BE-NEXT: 8: 4f 22 sts.l pr,@-r15
; DIS-BE-NEXT: a: d1 03 mov.l @(12,pc),r1
; DIS-BE-NEXT: c: 41 0b jsr @r1
; DIS-BE-NEXT: e: 00 09 nop
; DIS-BE-NEXT: 10: 4f 26 lds.l @r15+,pr
; DIS-LE-LABEL: <call_add_one>:
; DIS-LE-NEXT: 8: 22 4f sts.l pr,@-r15
; DIS-LE-NEXT: a: 03 d1 mov.l @(12,pc),r1
; DIS-LE-NEXT: c: 0b 41 jsr @r1
; DIS-LE-NEXT: e: 09 00 nop
; DIS-LE-NEXT: 10: 26 4f lds.l @r15+,pr

; DIS-BE-LABEL: <call_pointer>:
; DIS-BE: 20: 40 0b jsr @r0
; DIS-BE-NEXT: 22: 64 53 mov r5,r4
; DIS-LE-LABEL: <call_pointer>:
; DIS-LE: 20: 0b 40 jsr @r0
; DIS-LE-NEXT: 22: 53 64 mov r5,r4
