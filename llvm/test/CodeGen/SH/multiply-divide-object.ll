; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --file-headers --sections --symbols --relocations %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj --file-headers --sections --symbols --relocations %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-readelf -h -s -r %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
; RUN: llvm-readelf -h -s -r %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefixes=DISASM,DISASM-BE
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefixes=DISASM,DISASM-LE

define i32 @object_multiply(i32 %a, i32 %b) {
	%result = mul i32 %a, %b
	ret i32 %result
}

define i32 @object_udiv(i32 %a, i32 %b) {
	%result = udiv i32 %a, %b
	ret i32 %result
}

define i32 @object_srem(i32 %a, i32 %b) {
	%result = srem i32 %a, %b
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
; COMMON-NOT: Name: .rodata
; COMMON-NOT: Name: .data
; COMMON: Relocations [
; COMMON-NEXT: ]
; COMMON: Name: object_multiply
; COMMON-NEXT: Value: 0x0
; COMMON-NEXT: Size: 8
; COMMON: Name: object_udiv
; COMMON: Name: object_srem
; COMMON-NOT: Name: __mulsi3
; COMMON-NOT: Name: __udivsi3
; COMMON-NOT: Name: __divsi3
; COMMON-NOT: Name: __umodsi3
; COMMON-NOT: Name: __modsi3

; READELF: Class: ELF32
; READELF-BE: Data: 2's complement, big endian
; READELF-LE: Data: 2's complement, little endian
; READELF: Machine: Hitachi SH
; READELF: Flags: 0x2
; READELF: There are no relocations in this file.

; DISASM-LABEL: <object_multiply>:
; DISASM-BE-NEXT: 0: 05 47 mul.l r4,r5
; DISASM-BE-NEXT: 2: 00 1a sts macl,r0
; DISASM-LE-NEXT: 0: 47 05 mul.l r4,r5
; DISASM-LE-NEXT: 2: 1a 00 sts macl,r0
; DISASM-LABEL: <object_udiv>:
; DISASM: div0u
; DISASM-COUNT-32: div1
; DISASM: rotcl
; DISASM-LABEL: <object_srem>:
; DISASM: div0s
; DISASM-COUNT-32: div1
; DISASM: addc
; DISASM-NEXT: {{.*}}mul.l
; DISASM-NEXT: {{.*}}sts macl
; DISASM: rts
