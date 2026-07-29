; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj -S -s -r --hex-dump=.data --hex-dump=.rodata %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj -S -s -r --hex-dump=.data --hex-dump=.rodata %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-readelf -r %t.be.o | FileCheck %s --check-prefix=REL
; RUN: llvm-readelf -r %t.le.o | FileCheck %s --check-prefix=REL

@integer = global i32 305419896, align 4
@wide = global i64 1311768467463790320, align 4
@bytes = global [4 x i8] c"ABC\00", align 1
@zero = global { i8, i8, i16, i32 } zeroinitializer, align 4
@readonly = constant i32 7, align 4
@ptr_to_global = global ptr @integer, align 4
@ptr_to_element = global ptr getelementptr ([4 x i8], ptr @bytes, i32 0, i32 3), align 4
@ptr_negative = global ptr getelementptr (i8, ptr @integer, i32 -4), align 4
@function_ptr = global ptr @defined_function, align 4
@external_function_ptr = global ptr @external_function, align 4
@external_global_ptr = global ptr @external_global, align 4

declare void @external_function()
@external_global = external global i32

define void @defined_function() {
	ret void
}

; COMMON: Name: .data
; COMMON: Type: SHT_PROGBITS
; COMMON: SHF_WRITE
; COMMON: Name: .bss
; COMMON: Type: SHT_NOBITS
; COMMON: Name: .rodata
; COMMON: Type: SHT_PROGBITS
; COMMON-NOT: .got
; COMMON-NOT: .plt
; COMMON-NOT: .tdata

; COMMON: Section {{.*}} .rela.data {
; COMMON: R_SH_DIR32 integer
; COMMON: R_SH_DIR32 bytes
; COMMON: R_SH_DIR32 integer
; COMMON: R_SH_DIR32 defined_function
; COMMON: R_SH_DIR32 external_function
; COMMON: R_SH_DIR32 external_global
; COMMON: }
; COMMON-NOT: R_SH_NONE

; COMMON: Name: integer
; COMMON: Size: 4
; COMMON: Section: .data
; COMMON: Name: wide
; COMMON: Size: 8
; COMMON: Section: .data
; COMMON: Name: zero
; COMMON: Size: 8
; COMMON: Section: .bss
; COMMON: Name: readonly
; COMMON: Size: 4
; COMMON: Section: .rodata
; COMMON: Name: external_function
; COMMON: Section: Undefined
; COMMON: Name: external_global
; COMMON: Section: Undefined

; BE: Hex dump of section '.data':
; BE-NEXT: 0x00000000 12345678 12345678 9abcdef0 41424300
; LE: Hex dump of section '.data':
; LE-NEXT: 0x00000000 78563412 f0debc9a 78563412 41424300
; BE: Hex dump of section '.rodata':
; BE-NEXT: 0x00000000 00000007
; LE: Hex dump of section '.rodata':
; LE-NEXT: 0x00000000 07000000

; REL: Relocation section '.rela.data'
; REL: R_SH_DIR32
; REL-NOT: R_SH_NONE
