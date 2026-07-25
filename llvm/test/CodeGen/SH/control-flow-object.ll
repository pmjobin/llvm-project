; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-readelf -h %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
; RUN: llvm-readelf -h %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DIS-BE
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DIS-LE

define i32 @choose_equal(i32 %a, i32 %b) #0 {
	%equal = icmp eq i32 %a, %b
	br i1 %equal, label %same, label %different
same:
	ret i32 %a
different:
	ret i32 %b
}

define i32 @count_down(i32 %n) #0 {
entry:
	br label %loop
loop:
	%value = phi i32 [ %n, %entry ], [ %next, %loop ]
	%next = add i32 %value, -1
	%continue = icmp ne i32 %next, 0
	br i1 %continue, label %loop, label %exit
exit:
	ret i32 %next
}

attributes #0 = { noinline optnone }

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
; COMMON-NEXT: ]

; COMMON: Name: choose_equal
; COMMON: Value: 0x0
; COMMON: Size: 20
; COMMON: Type: Function (0x2)
; COMMON: Name: count_down
; COMMON: Value: 0x14
; COMMON: Size: 22
; COMMON: Type: Function (0x2)

; BE: Hex dump of section '.text':
; BE-NEXT: 0x00000000 34508b04 a0000009 6043000b 00096053
; BE-NEXT: 0x00000010 000b0009 6043a000 000970ff e1003010
; BE-NEXT: 0x00000020 8bfba000 0009000b 0009
; LE: Hex dump of section '.text':
; LE-NEXT: 0x00000000 5034048b 00a00900 43600b00 09005360
; LE-NEXT: 0x00000010 0b000900 436000a0 0900ff70 00e11030
; LE-NEXT: 0x00000020 fb8b00a0 09000b00 0900

; READELF: Class: ELF32
; READELF-BE: Data: 2's complement, big endian
; READELF-LE: Data: 2's complement, little endian
; READELF: Machine: Hitachi SH
; READELF: Flags: 0x2

; DIS-BE: file format elf32-sh
; DIS-BE-LABEL: <choose_equal>:
; DIS-BE: 0: 34 50 cmp/eq r5,r4
; DIS-BE: 2: 8b 04 bf	0xe <choose_equal+0xe>
; DIS-BE-LABEL: <count_down>:
; DIS-BE: 20: 8b fb bf	0x1a <count_down+0x6>

; DIS-LE: file format elf32-shl
; DIS-LE-LABEL: <choose_equal>:
; DIS-LE: 0: 50 34 cmp/eq r5,r4
; DIS-LE: 2: 04 8b bf	0xe <choose_equal+0xe>
; DIS-LE-LABEL: <count_down>:
; DIS-LE: 20: fb 8b bf	0x1a <count_down+0x6>
