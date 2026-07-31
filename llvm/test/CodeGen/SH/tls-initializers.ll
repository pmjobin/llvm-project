; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -filetype=obj %t/constants.ll -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -filetype=obj %t/constants.ll -o %t.le.o
; RUN: llvm-readobj --sections --symbols --hex-dump=.tdata %t.be.o | FileCheck %s --check-prefixes=OBJECT,BE
; RUN: llvm-readobj --sections --symbols --hex-dump=.tdata %t.le.o | FileCheck %s --check-prefixes=OBJECT,LE
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/pointer.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=POINTER

; OBJECT: Name: .tdata
; OBJECT: Type: SHT_PROGBITS
; OBJECT: SHF_TLS
; OBJECT-DAG: Name: integers
; OBJECT-DAG: Type: TLS
; OBJECT-DAG: Name: floating
; OBJECT-DAG: Type: TLS
; BE: Hex dump of section '.tdata':
; BE: 0x00000000 11223344 55667788 3fc00000 40040000
; LE: Hex dump of section '.tdata':
; LE: 0x00000000 44332211 88776655 0000c03f 00000000
; LE-NEXT: 0x00000010 00000440

; POINTER: LLVM ERROR: SH pointer-valued TLS initializers are not supported

;--- constants.ll
@integers = thread_local(localexec) global { i32, i32 } { i32 287454020, i32 1432778632 }, align 4
@floating = thread_local(localexec) global { float, double } { float 1.500000e+00, double 2.500000e+00 }, align 4

;--- pointer.ll
@ordinary = global i32 0
@pointer_tls = thread_local(localexec) global ptr @ordinary
