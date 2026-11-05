# REQUIRES: sh
# RUN: split-file %s %t
# RUN: llvm-mc -triple=shle-unknown-none-elf -mcpu=sh4 -filetype=obj %t/main.s -o %t/main.o
# RUN: llvm-mc -triple=shle-unknown-none-elf -mcpu=sh4 -filetype=obj %t/lib.s -o %t/lib.o
# RUN: llvm-ar rc %t/lib.a %t/lib.o
# RUN: ld.lld -static --gc-sections -T %t/script %t/main.o %t/lib.a -o %t/out
# RUN: llvm-readobj -h -l -S -s -r -x .text -x .data %t/out | FileCheck %s
# RUN: llvm-nm %t/out | FileCheck %s --check-prefix=NM
# RUN: ld.lld -r %t/main.o %t/lib.a -o %t/partial.o
# RUN: ld.lld --gc-sections -T %t/script %t/partial.o -o %t/partial
# RUN: llvm-objcopy -O binary %t/out %t/out.bin
# RUN: llvm-objcopy -O binary %t/partial %t/partial.bin
# RUN: cmp %t/out.bin %t/partial.bin
# RUN: not ld.lld -T %t/script %t/main.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNDEF

# CHECK: Type: Executable
# CHECK: Machine: EM_SH
# CHECK: Entry: 0x1000
# CHECK: Flags [ (0x9)
# CHECK: Name: .rodata
# CHECK: Name: .data
# CHECK: Name: .bss
# CHECK: Type: SHT_NOBITS
# CHECK: Type: PT_LOAD
# CHECK: VirtualAddress: 0x1000
# CHECK: Relocations [
# CHECK-NEXT: ]
# CHECK: 0x00001000 04b00900 00200000 00210000 0b000900
# CHECK: 0x00002000 00210000 03100000
# NM: 00001000 T _start
# NM: 00002100 B bss
# NM: 00002000 D data
# NM: 0000100c T function
# NM-NOT: unused
# UNDEF: undefined symbol: function

#--- main.s
.section .text.start,"ax",@progbits
.globl _start
_start:
 bsr function
 nop
 .long data
 .long bss
.section .rodata,"a",@progbits
.long 0x12345678
.section .data,"aw",@progbits
.globl data
data:
 .long bss
 .long _start+3
.section .bss,"aw",@nobits
.globl bss
bss:
 .space 16

#--- lib.s
.section .text.function,"ax",@progbits
.globl function
function:
 rts
 nop
.section .text.unused,"ax",@progbits
.globl unused
unused:
 nop

#--- script
ENTRY(_start)
MEMORY { ROM (rx) : ORIGIN = 0x1000, LENGTH = 0x1000
         RAM (rw) : ORIGIN = 0x2000, LENGTH = 0x1000 }
SECTIONS {
 .text : { *(.text.start) *(.text.*) } > ROM
 .rodata : { KEEP(*(.rodata)) } > ROM
 .data : { *(.data) } > RAM
 .bss 0x2100 (NOLOAD) : { *(.bss) } > RAM
}
