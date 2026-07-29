; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -verify-machineinstrs -filetype=obj %s -o %t-be.o
; RUN: llvm-readobj --file-headers --relocations %t-be.o | FileCheck %s --check-prefixes=READOBJ,BE-READOBJ
; RUN: llvm-readelf -h -s -r %t-be.o | FileCheck %s --check-prefixes=ELF,BE-ELF
; RUN: llvm-objdump -d --show-all-symbols %t-be.o | FileCheck %s --check-prefix=BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -verify-machineinstrs -filetype=obj %s -o %t-le.o
; RUN: llvm-readobj --file-headers --relocations %t-le.o | FileCheck %s --check-prefixes=READOBJ,LE-READOBJ
; RUN: llvm-readelf -h -s -r %t-le.o | FileCheck %s --check-prefixes=ELF,LE-ELF
; RUN: llvm-objdump -d --show-all-symbols %t-le.o | FileCheck %s --check-prefix=LE

define internal i32 @return_fifth_object(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4) nounwind {
entry:
	ret i32 %a4
}

define i32 @direct_fifth_object() nounwind {
entry:
	%result = call i32 @return_fifth_object(i32 1, i32 2, i32 3, i32 4, i32 5)
	ret i32 %result
}

define i32 @indirect_eight_object(ptr %fn) nounwind {
entry:
	%result = call i32 %fn(i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	ret i32 %result
}

; READOBJ: Class: 32-bit
; BE-READOBJ: DataEncoding: BigEndian
; LE-READOBJ: DataEncoding: LittleEndian
; READOBJ: Machine: EM_SH
; READOBJ: Flags [ (0x2)
; READOBJ: 0x2
; READOBJ-NEXT: ]
; READOBJ: Relocations [
; READOBJ-NEXT: ]

; ELF: Class:                             ELF32
; BE-ELF: Data:                              2's complement, big endian
; LE-ELF: Data:                              2's complement, little endian
; ELF: Machine:                           Hitachi SH
; ELF: Flags:                             0x2
; ELF: There are no relocations in this file.
; ELF: 00000000     6 FUNC    LOCAL  DEFAULT     2 return_fifth_object
; ELF: 00000008    28 FUNC    GLOBAL DEFAULT     2 direct_fifth_object
; ELF: 00000024    42 FUNC    GLOBAL DEFAULT     2 indirect_eight_object

; BE-LABEL: <return_fifth_object>:
; BE: 0: 60 f2         mov.l @r15,r0
; BE-LABEL: <direct_fifth_object>:
; BE: a: 7f fc         add #-4,r15
; BE: e: 2f 02         mov.l r0,@r15
; BE: 18: bf f2         bsr {{.*}} <return_fifth_object>
; BE-NEXT: 1a: 00 09         nop
; BE-NEXT: 1c: 7f 04         add #4,r15
; BE-LABEL: <indirect_eight_object>:
; BE: 28: 7f f0         add #-16,r15
; BE: 2c: 1f 13         mov.l r1,@(12,r15)
; BE: 30: 1f 12         mov.l r1,@(8,r15)
; BE: 34: 1f 11         mov.l r1,@(4,r15)
; BE: 38: 2f 12         mov.l r1,@r15
; BE: 42: 40 0b         jsr @r0
; BE-NEXT: 44: 00 09         nop
; BE-NEXT: 46: 7f 10         add #16,r15

; LE-LABEL: <return_fifth_object>:
; LE: 0: f2 60         mov.l @r15,r0
; LE-LABEL: <direct_fifth_object>:
; LE: a: fc 7f         add #-4,r15
; LE: e: 02 2f         mov.l r0,@r15
; LE: 18: f2 bf         bsr {{.*}} <return_fifth_object>
; LE-NEXT: 1a: 09 00         nop
; LE-NEXT: 1c: 04 7f         add #4,r15
; LE-LABEL: <indirect_eight_object>:
; LE: 28: f0 7f         add #-16,r15
; LE: 2c: 13 1f         mov.l r1,@(12,r15)
; LE: 30: 12 1f         mov.l r1,@(8,r15)
; LE: 34: 11 1f         mov.l r1,@(4,r15)
; LE: 38: 12 2f         mov.l r1,@r15
; LE: 42: 0b 40         jsr @r0
; LE-NEXT: 44: 09 00         nop
; LE-NEXT: 46: 10 7f         add #16,r15
