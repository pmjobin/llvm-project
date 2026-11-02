; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=CHECK
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=CHECK
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --symbols --relocations --hex-dump=.rodata %t.be.o | FileCheck %s --check-prefixes=OBJ,OBJ-BE
; RUN: llvm-readobj --symbols --relocations --hex-dump=.rodata %t.le.o | FileCheck %s --check-prefixes=OBJ,OBJ-LE

%P16 = type <{ i8, i16 }>
%P32 = type <{ i8, i32 }>
%P64 = type <{ i8, i64 }>
%Nested = type <{ i8, %P16, i32 }>
%Pointers = type <{ i8, ptr, i8, ptr, i8, ptr, i8 }>
%Float = type <{ i8, float, i8, double }>
%Aligned = type <{ i8, [3 x i8], i32 }>

@external_object = external global i32
@anchor = internal constant [7 x i8] c"anchor\00", align 1
@p16 = constant %P16 <{ i8 -95, i16 4660 }>, align 1
@p32 = constant %P32 <{ i8 -94, i32 305419896 }>, align 1
@p64 = constant %P64 <{ i8 -93, i64 81985529216486895 }>, align 1
@nested = constant %Nested <{ i8 -92, %P16 <{ i8 -91, i16 22136 }>, i32 270544960 }>, align 4
@pointers = constant %Pointers <{ i8 -90, ptr @external_object, i8 -89, ptr @external_function, i8 -88, ptr getelementptr ([7 x i8], ptr @anchor, i32 0, i32 3), i8 -87 }>, align 1
@float = constant %Float <{ i8 -86, float -0.000000e+00, i8 -85, double -1.000000e+00 }>, align 1
@aligned = constant %Aligned <{ i8 -84, [3 x i8] zeroinitializer, i32 2882400001 }>, align 4
@float_special = constant <{ i8, float, i8, float }> <{ i8 -83, float 0x7FF0000000000000, i8 -82, float 0x7FF82468A0000000 }>, align 1
@double_special = constant <{ i8, double, i8, double }> <{ i8 -81, double 0x7FF0000000000000, i8 -80, double 0x7FF80123456789AB }>, align 1

declare void @external_function()

; CHECK-LABEL: p16:
; CHECK-NEXT: .byte 161
; CHECK-NEXT: .uaword 4660
; CHECK-NEXT: .size p16, 3

; CHECK-LABEL: p32:
; CHECK-NEXT: .byte 162
; CHECK-NEXT: .ualong 305419896
; CHECK-NEXT: .size p32, 5

; CHECK-LABEL: p64:
; CHECK-NEXT: .byte 163
; CHECK-NEXT: .uaquad 81985529216486895
; CHECK-NEXT: .size p64, 9

; CHECK-LABEL: nested:
; CHECK-NEXT: .byte 164
; CHECK-NEXT: .byte 165
; CHECK-NEXT: .short 22136
; CHECK-NEXT: .long 270544960
; CHECK-NEXT: .size nested, 8

; CHECK-LABEL: pointers:
; CHECK-NEXT: .byte 166
; CHECK-NEXT: .ualong external_object
; CHECK-NEXT: .byte 167
; CHECK-NEXT: .ualong external_function
; CHECK-NEXT: .byte 168
; CHECK-NEXT: .ualong anchor+3
; CHECK-NEXT: .byte 169
; CHECK-NEXT: .size pointers, 16

; CHECK-LABEL: float:
; CHECK-NEXT: .byte 170
; CHECK-NEXT: .ualong 2147483648
; CHECK-NEXT: .byte 171
; CHECK-NEXT: .uaquad -4616189618054758400
; CHECK-NEXT: .size float, 14

; CHECK-LABEL: aligned:
; CHECK-NEXT: .byte 172
; CHECK-NEXT: .zero 3
; CHECK-NEXT: .long 2882400001
; CHECK-NEXT: .size aligned, 8

; CHECK-LABEL: float_special:
; CHECK-NEXT: .byte 173
; CHECK-NEXT: .ualong 2139095040
; CHECK-NEXT: .byte 174
; CHECK-NEXT: .ualong 2143363909
; CHECK-NEXT: .size float_special, 10

; CHECK-LABEL: double_special:
; CHECK-NEXT: .byte 175
; CHECK-NEXT: .uaquad 9218868437227405312
; CHECK-NEXT: .byte 176
; CHECK-NEXT: .uaquad 9221121488040987051
; CHECK-NEXT: .size double_special, 18

; OBJ:      Relocations [
; OBJ:      R_SH_DIR32 external_object
; OBJ-NEXT: R_SH_DIR32 external_function
; OBJ-NEXT: R_SH_DIR32 anchor
; OBJ:      Name: p16
; OBJ-NEXT: Value: 0x7
; OBJ-NEXT: Size: 3
; OBJ:      Name: p32
; OBJ-NEXT: Value: 0xA
; OBJ-NEXT: Size: 5
; OBJ:      Name: p64
; OBJ-NEXT: Value: 0xF
; OBJ-NEXT: Size: 9
; OBJ:      Name: nested
; OBJ-NEXT: Value: 0x18
; OBJ-NEXT: Size: 8
; OBJ:      Name: pointers
; OBJ-NEXT: Value: 0x20
; OBJ-NEXT: Size: 16
; OBJ:      Name: float
; OBJ-NEXT: Value: 0x30
; OBJ-NEXT: Size: 14
; OBJ:      Name: aligned
; OBJ-NEXT: Value: 0x40
; OBJ-NEXT: Size: 8

; OBJ-BE: Hex dump of section '.rodata':
; OBJ-BE: 0x00000000 616e6368 6f7200a1 1234a212 345678a3
; OBJ-BE: 0x00000010 01234567 89abcdef a4a55678 10203040
; OBJ-BE: 0x00000020 a6000000 00a70000 0000a800 000003a9
; OBJ-BE: 0x00000030 aa800000 00abbff0 00000000 00000000
; OBJ-BE: 0x00000040 ac000000 abcdef01 ad7f8000 00ae7fc1
; OBJ-BE: 0x00000050 2345af7f f0000000 000000b0 7ff80123
; OBJ-BE: 0x00000060 456789ab

; OBJ-LE: Hex dump of section '.rodata':
; OBJ-LE: 0x00000000 616e6368 6f7200a1 3412a278 563412a3
; OBJ-LE: 0x00000010 efcdab89 67452301 a4a57856 40302010
; OBJ-LE: 0x00000020 a6000000 00a70000 0000a803 000000a9
; OBJ-LE: 0x00000030 aa000000 80ab0000 00000000 f0bf0000
; OBJ-LE: 0x00000040 ac000000 01efcdab ad000080 7fae4523
; OBJ-LE: 0x00000050 c17faf00 00000000 00f07fb0 ab896745
; OBJ-LE: 0x00000060 2301f87f
