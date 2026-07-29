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
; COMMON: Size: 36
; COMMON: Relocations [
; COMMON-NEXT: ]

; COMMON: Name: add_one
; COMMON-NEXT: Value: 0x0
; COMMON-NEXT: Size: 8
; COMMON: Name: call_add_one
; COMMON-NEXT: Value: 0x8
; COMMON-NEXT: Size: 12
; COMMON: Name: call_pointer
; COMMON-NEXT: Value: 0x14
; COMMON-NEXT: Size: 16

; BE: Hex dump of section '.text':
; BE-NEXT: 0x00000000 60437001 000b0009 4f22bff9 00094f26
; BE-NEXT: 0x00000010 000b0009 4f226043 6453400b 00094f26
; BE-NEXT: 0x00000020 000b0009
; LE: Hex dump of section '.text':
; LE-NEXT: 0x00000000 43600170 0b000900 224ff9bf 0900264f
; LE-NEXT: 0x00000010 0b000900 224f4360 53640b40 0900264f
; LE-NEXT: 0x00000020 0b000900

; READELF: Class: ELF32
; READELF-BE: Data: 2's complement, big endian
; READELF-LE: Data: 2's complement, little endian
; READELF: Machine: Hitachi SH
; READELF: Flags: 0x2

; DIS-BE-LABEL: <call_add_one>:
; DIS-BE-NEXT: 8: 4f 22 sts.l pr,@-r15
; DIS-BE-NEXT: a: bf f9 bsr 0x0 <add_one>
; DIS-BE-NEXT: c: 00 09 nop
; DIS-BE-NEXT: e: 4f 26 lds.l @r15+,pr
; DIS-LE-LABEL: <call_add_one>:
; DIS-LE-NEXT: 8: 22 4f sts.l pr,@-r15
; DIS-LE-NEXT: a: f9 bf bsr 0x0 <add_one>
; DIS-LE-NEXT: c: 09 00 nop
; DIS-LE-NEXT: e: 26 4f lds.l @r15+,pr

; DIS-BE-LABEL: <call_pointer>:
; DIS-BE: 1a: 40 0b jsr @r0
; DIS-BE-NEXT: 1c: 00 09 nop
; DIS-LE-LABEL: <call_pointer>:
; DIS-LE: 1a: 0b 40 jsr @r0
; DIS-LE-NEXT: 1c: 09 00 nop
