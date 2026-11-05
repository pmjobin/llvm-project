# REQUIRES: sh
# RUN: llvm-mc -triple=shle-unknown-none-elf -mcpu=sh4 -filetype=obj %s -o %t.le.o
# RUN: llvm-mc -triple=sh-unknown-none-elf -mcpu=sh4 -filetype=obj %s -o %t.be.o
# RUN: ld.lld -m shlelf --image-base=0 -Ttext=0x1000 --defsym=external=0x1100 %t.le.o -o %t.le
# RUN: ld.lld -m shelf --image-base=0 -Ttext=0x1000 --defsym=external=0x1100 %t.be.o -o %t.be
# RUN: llvm-readobj -h -r -x .text %t.le | FileCheck %s --check-prefixes=HEADER,LE
# RUN: llvm-readobj -h -r -x .text %t.be | FileCheck %s --check-prefixes=HEADER,BE
# RUN: ld.lld -m shlelf_linux --image-base=0 -Ttext=0x1000 --defsym=external=0x1100 %t.le.o -o %t.linux
# RUN: llvm-objcopy --dump-section .text=%t.text %t.le
# RUN: llvm-objcopy --dump-section .text=%t.linux.text %t.linux
# RUN: cmp %t.text %t.linux.text
# RUN: ld.lld -r %t.le.o -o %t.r.o
# RUN: ld.lld --image-base=0 -Ttext=0x1000 --defsym=external=0x1100 %t.r.o -o %t.r
# RUN: llvm-objcopy --dump-section .text=%t.r.text %t.r
# RUN: cmp %t.text %t.r.text

## S = 0x1100. IND12W uses the explicit addend, including -4 for PC+4.
## The other instruction relocations use scaled in-place addends. Literal
## loads use PC+4, with the PC aligned down to four bytes for mov.l/mova.
## The second mov.l is at P=0x1012, testing the aligned PC independently of
## the section alignment. A nonzero low field on IND12W must be overwritten.
.globl _start
_start:
 bsr external
 nop
 bra external+4
 nop
 bt external+2
 bf external-2
 mov.l external,r0
 mov.w external,r1
 nop
 mov.l external+4,r2
 .long external+4
 .long external-4
 .long external-.
 .long external+4-.
 .short external+4
 .short external-4

# HEADER: Class: 32-bit
# HEADER: OS/ABI: SystemV
# HEADER: Type: Executable
# HEADER: Machine: EM_SH
# HEADER: Entry: 0x1000
# HEADER: Flags [ (0x9)
# HEADER: Relocations [
# HEADER-NEXT: ]
# LE: 0x00001000 7eb00900 7ea00900 7b89788b 3cd07791
# LE-NEXT: 0x00001010 09003cd2 04110000 fc100000 e4000000
# LE-NEXT: 0x00001020 e4000000 0411fc10
# BE: 0x00001000 b07e0009 a07e0009 897b8b78 d03c9177
# BE-NEXT: 0x00001010 0009d23c 00001104 000010fc 000000e4
# BE-NEXT: 0x00001020 000000e4 110410fc
