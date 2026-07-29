; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj --file-headers --sections --symbols --relocations --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-readelf -h -s -r %t.be.o | FileCheck %s --check-prefixes=READELF,READELF-BE
; RUN: llvm-readelf -h -s -r %t.le.o | FileCheck %s --check-prefixes=READELF,READELF-LE
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefixes=DISASM,DISASM-BE
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefixes=DISASM,DISASM-LE

define i64 @object_add_i64(i64 %a, i64 %b) nounwind {
	%result = add i64 %a, %b
	ret i64 %result
}

define i64 @object_load_i64(ptr %address) nounwind {
	%result = load i64, ptr %address, align 4
	ret i64 %result
}

define void @object_store_i64(ptr %address, i64 %value) nounwind {
	store i64 %value, ptr %address, align 4
	ret void
}

define i64 @object_lshr_i64(i64 %value, i64 %count) nounwind {
	%result = lshr i64 %value, %count
	ret i64 %result
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
; COMMON: Size: 82
; COMMON-NOT: Name: .rodata
; COMMON-NOT: Name: .data
; COMMON: Relocations [
; COMMON-NEXT: ]
; COMMON: Name: object_add_i64
; COMMON-NEXT: Value: 0x0
; COMMON-NEXT: Size: 14
; COMMON: Name: object_load_i64
; COMMON-NEXT: Value: 0x10
; COMMON-NEXT: Size: 8
; COMMON: Name: object_store_i64
; COMMON-NEXT: Value: 0x18
; COMMON-NEXT: Size: 8
; COMMON: Name: object_lshr_i64
; COMMON-NEXT: Value: 0x20
; COMMON-NEXT: Size: 50
; COMMON-NOT: Name: __adddi3
; COMMON-NOT: Name: __lshrdi3

; READELF: Class: ELF32
; READELF-BE: Data: 2's complement, big endian
; READELF-LE: Data: 2's complement, little endian
; READELF: Machine: Hitachi SH
; READELF: Flags: 0x2
; READELF: There are no relocations in this file.

; DISASM-LABEL: <object_add_i64>:
; DISASM-BE-NEXT: 0: 00 08 clrt
; DISASM-LE-NEXT: 0: 08 00 clrt
; DISASM: addc
; DISASM: addc
; DISASM-LABEL: <object_load_i64>:
; DISASM: mov.l	@r4,r0
; DISASM-NEXT: {{.*}}mov.l	@(4,r4),r1
; DISASM-LABEL: <object_store_i64>:
; DISASM: mov.l	r6,@(4,r4)
; DISASM-NEXT: {{.*}}mov.l	r5,@r4
; DISASM-LABEL: <object_lshr_i64>:
; DISASM: shlr
; DISASM-BE-NEXT: 3a: 42 25 rotcr	r2
; DISASM-LE-NEXT: 3a: 25 42 rotcr	r2
; DISASM: dt
; DISASM: bf
