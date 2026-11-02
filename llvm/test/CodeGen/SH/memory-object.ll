; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --file-headers --sections --symbols --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj --file-headers --sections --symbols --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-readelf -h -s %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
; RUN: llvm-readelf -h -s %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefixes=DISASM,DISASM-BE
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefixes=DISASM,DISASM-LE

define i32 @load_i32(ptr %p) minsize nounwind {
	%value = load i32, ptr %p, align 4
	ret i32 %value
}

define void @store_i32(ptr %p, i32 %value) minsize nounwind {
	store i32 %value, ptr %p, align 4
	ret void
}

define i32 @load_offset_12(ptr %p) minsize nounwind {
	%address = getelementptr i8, ptr %p, i32 12
	%value = load i32, ptr %address, align 4
	ret i32 %value
}

define void @store_offset_12(ptr %p, i32 %value) minsize nounwind {
	%address = getelementptr i8, ptr %p, i32 12
	store i32 %value, ptr %address, align 4
	ret void
}

define i32 @stack_roundtrip(i32 %value) minsize nounwind {
	%slot = alloca i32, align 4
	store volatile i32 %value, ptr %slot, align 4
	%result = load volatile i32, ptr %slot, align 4
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
; COMMON: SHF_ALLOC (0x2)
; COMMON-NEXT: SHF_EXECINSTR (0x4)
; COMMON: Size: 28
; COMMON: AddressAlignment: 4

; COMMON: Name: load_i32
; COMMON-NEXT: Value: 0x0
; COMMON-NEXT: Size: 4
; COMMON: Name: store_i32
; COMMON-NEXT: Value: 0x4
; COMMON-NEXT: Size: 4
; COMMON: Name: load_offset_12
; COMMON-NEXT: Value: 0x8
; COMMON-NEXT: Size: 4
; COMMON: Name: store_offset_12
; COMMON-NEXT: Value: 0xC
; COMMON-NEXT: Size: 4
; COMMON: Name: stack_roundtrip
; COMMON-NEXT: Value: 0x10
; COMMON-NEXT: Size: 12

; BE: Hex dump of section '.text':
; BE-NEXT: 0x00000000 000b6042 000b2452 000b5043 000b1453
; BE-NEXT: 0x00000010 7ffc2f42 60f27f04 000b0009
; LE: Hex dump of section '.text':
; LE-NEXT: 0x00000000 0b004260 0b005224 0b004350 0b005314
; LE-NEXT: 0x00000010 fc7f422f f260047f 0b000900

; READELF: Class: ELF32
; READELF-BE: Data: 2's complement, big endian
; READELF-LE: Data: 2's complement, little endian
; READELF: Machine: Hitachi SH
; READELF: Flags: 0x2
; READELF: 2: 00000000 4 FUNC GLOBAL DEFAULT
; READELF-SAME: load_i32
; READELF: 3: 00000004 4 FUNC GLOBAL DEFAULT
; READELF-SAME: store_i32
; READELF: 4: 00000008 4 FUNC GLOBAL DEFAULT
; READELF-SAME: load_offset_12
; READELF: 5: 0000000c 4 FUNC GLOBAL DEFAULT
; READELF-SAME: store_offset_12
; READELF: 6: 00000010 12 FUNC GLOBAL DEFAULT
; READELF-SAME: stack_roundtrip

; DISASM-LABEL: <load_i32>:
; DISASM-BE-NEXT: 0: 00 0b rts
; DISASM-BE-NEXT: 2: 60 42 mov.l @r4,r0
; DISASM-LE-NEXT: 0: 0b 00 rts
; DISASM-LE-NEXT: 2: 42 60 mov.l @r4,r0
; DISASM-LABEL: <store_i32>:
; DISASM-BE-NEXT: 4: 00 0b rts
; DISASM-BE-NEXT: 6: 24 52 mov.l r5,@r4
; DISASM-LE-NEXT: 4: 0b 00 rts
; DISASM-LE-NEXT: 6: 52 24 mov.l r5,@r4
; DISASM-LABEL: <load_offset_12>:
; DISASM-BE-NEXT: 8: 00 0b rts
; DISASM-BE-NEXT: a: 50 43 mov.l @(12,r4),r0
; DISASM-LE-NEXT: 8: 0b 00 rts
; DISASM-LE-NEXT: a: 43 50 mov.l @(12,r4),r0
; DISASM-LABEL: <store_offset_12>:
; DISASM-BE-NEXT: c: 00 0b rts
; DISASM-BE-NEXT: e: 14 53 mov.l r5,@(12,r4)
; DISASM-LE-NEXT: c: 0b 00 rts
; DISASM-LE-NEXT: e: 53 14 mov.l r5,@(12,r4)
; DISASM-LABEL: <stack_roundtrip>:
; DISASM-BE-NEXT: 10: 7f fc add #-4,r15
; DISASM-BE-NEXT: 12: 2f 42 mov.l r4,@r15
; DISASM-BE-NEXT: 14: 60 f2 mov.l @r15,r0
; DISASM-BE-NEXT: 16: 7f 04 add #4,r15
; DISASM-BE-NEXT: 18: 00 0b rts
; DISASM-BE-NEXT: 1a: 00 09 nop
; DISASM-LE-NEXT: 10: fc 7f add #-4,r15
; DISASM-LE-NEXT: 12: 42 2f mov.l r4,@r15
; DISASM-LE-NEXT: 14: f2 60 mov.l @r15,r0
; DISASM-LE-NEXT: 16: 04 7f add #4,r15
; DISASM-LE-NEXT: 18: 0b 00 rts
; DISASM-LE-NEXT: 1a: 09 00 nop
