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
; BE-READOBJ: {{0x[0-9A-F]+}} R_SH_DIR32 return_fifth_object 0x0
; LE-READOBJ: {{0x[0-9A-F]+}} R_SH_DIR32 return_fifth_object 0x0

; ELF: Class:                             ELF32
; BE-ELF: Data:                              2's complement, big endian
; LE-ELF: Data:                              2's complement, little endian
; ELF: Machine:                           Hitachi SH
; ELF: Flags:                             0x2
; BE-ELF: {{[0-9a-f]+}} {{.*}} R_SH_DIR32 {{.*}} return_fifth_object
; LE-ELF: {{[0-9a-f]+}} {{.*}} R_SH_DIR32 {{.*}} return_fifth_object
; ELF: 00000000     4 FUNC    LOCAL  DEFAULT     2 return_fifth_object
; BE-ELF: {{[0-9a-f]+}}    32 FUNC    GLOBAL DEFAULT     2 direct_fifth_object
; BE-ELF: {{[0-9a-f]+}}    40 FUNC    GLOBAL DEFAULT     2 indirect_eight_object
; LE-ELF: {{[0-9a-f]+}}    32 FUNC    GLOBAL DEFAULT     2 direct_fifth_object
; LE-ELF: {{[0-9a-f]+}}    40 FUNC    GLOBAL DEFAULT     2 indirect_eight_object

; BE-LABEL: <return_fifth_object>:
; BE: {{[0-9a-f]+}}: 00 0b         rts
; BE-NEXT: {{[0-9a-f]+}}: 60 f2         mov.l @r15,r0
; BE-LABEL: <direct_fifth_object>:
; BE: {{[0-9a-f]+}}: 7f fc         add #-4,r15
; BE: {{[0-9a-f]+}}: e1 05         mov #5,r1
; BE: {{[0-9a-f]+}}: 40 0b         jsr @r0
; BE-NEXT: {{[0-9a-f]+}}: 2f 12         mov.l r1,@r15
; BE-NEXT: {{[0-9a-f]+}}: 7f 04         add #4,r15
; BE-LABEL: <indirect_eight_object>:
; BE: {{[0-9a-f]+}}: 7f f0         add #-16,r15
; BE: {{[0-9a-f]+}}: 1f 13         mov.l r1,@(12,r15)
; BE: {{[0-9a-f]+}}: 1f 12         mov.l r1,@(8,r15)
; BE: {{[0-9a-f]+}}: 1f 11         mov.l r1,@(4,r15)
; BE: {{[0-9a-f]+}}: 40 0b         jsr @r0
; BE-NEXT: {{[0-9a-f]+}}: 2f 12         mov.l r1,@r15
; BE-NEXT: {{[0-9a-f]+}}: 7f 10         add #16,r15

; LE-LABEL: <return_fifth_object>:
; LE: {{[0-9a-f]+}}: 0b 00         rts
; LE-NEXT: {{[0-9a-f]+}}: f2 60         mov.l @r15,r0
; LE-LABEL: <direct_fifth_object>:
; LE: {{[0-9a-f]+}}: fc 7f         add #-4,r15
; LE: {{[0-9a-f]+}}: 05 e1         mov #5,r1
; LE: {{[0-9a-f]+}}: 0b 40         jsr @r0
; LE-NEXT: {{[0-9a-f]+}}: 12 2f         mov.l r1,@r15
; LE-NEXT: {{[0-9a-f]+}}: 04 7f         add #4,r15
; LE-LABEL: <indirect_eight_object>:
; LE: {{[0-9a-f]+}}: f0 7f         add #-16,r15
; LE: {{[0-9a-f]+}}: 13 1f         mov.l r1,@(12,r15)
; LE: {{[0-9a-f]+}}: 12 1f         mov.l r1,@(8,r15)
; LE: {{[0-9a-f]+}}: 11 1f         mov.l r1,@(4,r15)
; LE: {{[0-9a-f]+}}: 0b 40         jsr @r0
; LE-NEXT: {{[0-9a-f]+}}: 12 2f         mov.l r1,@r15
; LE-NEXT: {{[0-9a-f]+}}: 10 7f         add #16,r15
