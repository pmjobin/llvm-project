; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -filetype=obj %s -o %t.le.o
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -filetype=obj %s -o %t.be.o
; RUN: llvm-readobj --hex-dump=.splat --hex-dump=.packed_splat --hex-dump=.mixed --hex-dump=.negative_zero %t.le.o | FileCheck %s --check-prefix=LE
; RUN: llvm-readobj --hex-dump=.splat --hex-dump=.packed_splat --hex-dump=.mixed --hex-dump=.negative_zero %t.be.o | FileCheck %s --check-prefix=BE
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -asm-verbose=false %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -asm-verbose=false %s -o - | FileCheck %s --check-prefix=ASM

; A floating splat is represented by a vector-typed ConstantFP. It is not one
; scalar APFloat widened to the allocation size of the entire vector.
@splat = constant <2 x float> <float 1.0, float 1.0>, section ".splat", align 4
@packed_splat = constant <{ i8, <2 x float> }> <{ i8 7, <2 x float> <float 1.0, float 1.0> }>, section ".packed_splat", align 4
; Non-data vectors must retain the packed member's offset when emitting their
; lanes. GNU .long autoalignment would corrupt this layout; use .ualong.
@mixed = constant <{ i8, <4 x float> }> <{ i8 7, <4 x float> <float -0.0, float undef, float 0x7FF82468A0000000, float poison> }>, section ".mixed", align 4
@negative_zero = constant <2 x float> <float -0.0, float -0.0>, section ".negative_zero", align 4

; LE: Hex dump of section '.splat':
; LE-NEXT: 0x00000000 0000803f 0000803f
; LE: Hex dump of section '.packed_splat':
; LE-NEXT: 0x00000000 07000080 3f000080 3f
; LE: Hex dump of section '.mixed':
; LE-NEXT: 0x00000000 07000000 80000000 004523c1 7f000000
; LE-NEXT: 0x00000010 00
; LE: Hex dump of section '.negative_zero':
; LE-NEXT: 0x00000000 00000080 00000080
; BE: Hex dump of section '.splat':
; BE-NEXT: 0x00000000 3f800000 3f800000
; BE: Hex dump of section '.packed_splat':
; BE-NEXT: 0x00000000 073f8000 003f8000 00
; BE: Hex dump of section '.mixed':
; BE-NEXT: 0x00000000 07800000 00000000 007fc123 45000000
; BE-NEXT: 0x00000010 00
; BE: Hex dump of section '.negative_zero':
; BE-NEXT: 0x00000000 80000000 80000000
; ASM-LABEL: packed_splat:
; ASM-NEXT: .byte 7
; ASM-NEXT: .ualong 1065353216
; ASM-NEXT: .ualong 1065353216
; ASM-NEXT: .size packed_splat, 9
; ASM-LABEL: mixed:
; ASM-NEXT: .byte 7
; ASM-NEXT: .ualong 2147483648
; ASM-NEXT: .zero 4
; ASM-NEXT: .ualong 2143363909
; ASM-NEXT: .zero 4
; ASM-NEXT: .size mixed, 17
