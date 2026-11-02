; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -filetype=obj %s -o %t.le.o
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -filetype=obj %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -enable-new-pm -filetype=obj %s -o %t.new.o
; RUN: llvm-readobj --hex-dump=.v2 --hex-dump=.v4 --hex-dump=.packed --hex-dump=.tdata %t.le.o | FileCheck %s --check-prefix=LE
; RUN: llvm-readobj --hex-dump=.v2 --hex-dump=.v4 --hex-dump=.packed --hex-dump=.tdata %t.be.o | FileCheck %s --check-prefix=BE
; RUN: cmp %t.le.o %t.new.o
;
; Ordinary constant promotion can put a native vector inside a global matrix.
; Check exact lane bytes, nested aggregate layout, and unaligned emission.
@pair = constant <2 x float> <float 1.0, float 2.0>, section ".v2", align 8
@matrix = constant { [1 x <4 x float>] } { [1 x <4 x float>] [<4 x float> <float 1.0, float -0.0, float 2.0, float 3.0>] }, section ".v4", align 16
@packed = constant <{ i8, <2 x float> }> <{ i8 7, <2 x float> <float 1.0, float 2.0> }>, section ".packed", align 1
@tls = thread_local global <2 x float> <float 1.0, float 2.0>, align 8

; LE: Hex dump of section '.v2':
; LE-NEXT: 0x00000000 0000803f 00000040
; LE: Hex dump of section '.v4':
; LE-NEXT: 0x00000000 0000803f 00000080 00000040 00004040
; LE: Hex dump of section '.packed':
; LE-NEXT: 0x00000000 07000080 3f000000 40
; LE: Hex dump of section '.tdata':
; LE-NEXT: 0x00000000 0000803f 00000040
; BE: Hex dump of section '.v2':
; BE-NEXT: 0x00000000 3f800000 40000000
; BE: Hex dump of section '.v4':
; BE-NEXT: 0x00000000 3f800000 80000000 40000000 40400000
; BE: Hex dump of section '.packed':
; BE-NEXT: 0x00000000 073f8000 00400000 00
; BE: Hex dump of section '.tdata':
; BE-NEXT: 0x00000000 3f800000 40000000
