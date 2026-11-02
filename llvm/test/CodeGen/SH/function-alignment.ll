; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %t/normal.ll -o %t.normal.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %t/normal.ll -o %t.normal.le.o
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %t/minsize.ll -o %t.minsize.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %t/minsize.ll -o %t.minsize.le.o
; RUN: llvm-readobj --symbols --hex-dump=.text %t.normal.be.o | FileCheck %s --check-prefixes=NORMAL,BE-NORMAL
; RUN: llvm-readobj --symbols --hex-dump=.text %t.normal.le.o | FileCheck %s --check-prefixes=NORMAL,LE-NORMAL
; RUN: llvm-readobj --symbols --hex-dump=.text %t.minsize.be.o | FileCheck %s --check-prefixes=MINSIZE,BE-MINSIZE
; RUN: llvm-readobj --symbols --hex-dump=.text %t.minsize.le.o | FileCheck %s --check-prefixes=MINSIZE,LE-MINSIZE
; RUN: llvm-objdump -d %t.normal.be.o | FileCheck %s --check-prefix=DISASM-BE
; RUN: llvm-objdump -d %t.normal.le.o | FileCheck %s --check-prefix=DISASM-LE

; NORMAL: Name: normal_first
; NORMAL-NEXT: Value: 0x0
; NORMAL: Name: normal_second
; NORMAL-NEXT: Value: 0x8
; MINSIZE: Name: minsize_first
; MINSIZE-NEXT: Value: 0x0
; MINSIZE: Name: minsize_second
; MINSIZE-NEXT: Value: 0x6

; NORMAL: Hex dump of section '.text':
; BE-NORMAL-NEXT: 0x00000000 6043000b 70010009 000b6043
; LE-NORMAL-NEXT: 0x00000000 43600b00 01700900 0b004360
; MINSIZE: Hex dump of section '.text':
; BE-MINSIZE-NEXT: 0x00000000 6043000b 7001000b 6043
; LE-MINSIZE-NEXT: 0x00000000 43600b00 01700b00 4360

; DISASM-BE-LABEL: <normal_first>:
; DISASM-BE-NEXT: 0: 60 43 mov r4,r0
; DISASM-BE-NEXT: 2: 00 0b rts
; DISASM-BE-NEXT: 4: 70 01 add #1,r0
; DISASM-BE-NEXT: 6: 00 09 nop
; DISASM-LE-LABEL: <normal_first>:
; DISASM-LE-NEXT: 0: 43 60 mov r4,r0
; DISASM-LE-NEXT: 2: 0b 00 rts
; DISASM-LE-NEXT: 4: 01 70 add #1,r0
; DISASM-LE-NEXT: 6: 09 00 nop

;--- normal.ll
define i32 @normal_first(i32 %value) {
	%result = add i32 %value, 1
	ret i32 %result
}

define i32 @normal_second(i32 %value) {
  ret i32 %value
}

;--- minsize.ll
define i32 @minsize_first(i32 %value) minsize {
	%result = add i32 %value, 1
	ret i32 %result
}

define i32 @minsize_second(i32 %value) minsize {
  ret i32 %value
}
