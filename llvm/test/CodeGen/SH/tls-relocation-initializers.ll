; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --sections --symbols --relocations --hex-dump=.tdata %t.be.o | FileCheck %s --check-prefixes=OBJ,OBJ-BE
; RUN: llvm-readobj --sections --symbols --relocations --hex-dump=.tdata %t.le.o | FileCheck %s --check-prefixes=OBJ,OBJ-LE

%Packed = type <{ i8, ptr, i8 }>
%Aligned = type { i32, ptr, i32 }
%Nested = type <{ i8, %Packed, ptr }>
%Offset2 = type <{ i16, ptr, i8 }>
%Offset3 = type <{ i16, i8, ptr, i8 }>
%Multiple = type <{ ptr, i8, ptr }>

@external_object = external global i32
@default_object = global i32 1
@hidden_object = hidden global i32 2
@internal_object = internal global i32 3
@anchor = internal global [8 x i8] zeroinitializer

@packed_external = thread_local global %Packed <{ i8 18, ptr @external_object, i8 52 }>, align 1
@packed_default = thread_local global %Packed <{ i8 33, ptr @default_object, i8 34 }>, align 1
@packed_hidden = thread_local global %Packed <{ i8 49, ptr @hidden_object, i8 50 }>, align 1
@packed_internal = internal thread_local global %Packed <{ i8 65, ptr @internal_object, i8 66 }>, align 1
@packed_external_function = thread_local global %Packed <{ i8 81, ptr @external_function, i8 82 }>, align 1
@packed_local_function = thread_local global %Packed <{ i8 97, ptr @local_function, i8 98 }>, align 1
@packed_addend = thread_local global %Packed <{ i8 113, ptr getelementptr ([8 x i8], ptr @anchor, i32 0, i32 3), i8 114 }>, align 1
@aligned_external = thread_local global %Aligned { i32 287454020, ptr @external_object, i32 1432778632 }, align 4
@offset_two = thread_local global %Offset2 <{ i16 4660, ptr @external_object, i8 86 }>, align 1
@offset_three = thread_local global %Offset3 <{ i16 22136, i8 -102, ptr @hidden_object, i8 -68 }>, align 1
@multiple = thread_local global %Multiple <{ ptr @default_object, i8 -35, ptr @external_function }>, align 1
@nested = thread_local global %Nested <{ i8 -127, %Packed <{ i8 -126, ptr @external_object, i8 -125 }>, ptr @external_function }>, align 1
@packed_array = thread_local global [2 x %Packed] [%Packed <{ i8 -111, ptr @default_object, i8 -110 }>, %Packed <{ i8 -95, ptr @hidden_object, i8 -94 }>], align 1
@mixed_null = thread_local global %Packed <{ i8 -79, ptr null, i8 -78 }>, align 1

declare void @external_function()

define internal void @local_function() {
	ret void
}

; ASM-LABEL: packed_external:
; ASM-NEXT: .byte 18
; ASM-NEXT: .ualong external_object
; ASM-NEXT: .byte 52
; ASM-NEXT: .size packed_external, 6

; ASM-LABEL: packed_default:
; ASM-NEXT: .byte 33
; ASM-NEXT: .ualong default_object
; ASM-NEXT: .byte 34

; ASM-LABEL: packed_hidden:
; ASM-NEXT: .byte 49
; ASM-NEXT: .ualong hidden_object
; ASM-NEXT: .byte 50

; ASM-LABEL: packed_internal:
; ASM-NEXT: .byte 65
; ASM-NEXT: .ualong internal_object
; ASM-NEXT: .byte 66

; ASM-LABEL: packed_external_function:
; ASM-NEXT: .byte 81
; ASM-NEXT: .ualong external_function
; ASM-NEXT: .byte 82

; ASM-LABEL: packed_local_function:
; ASM-NEXT: .byte 97
; ASM-NEXT: .ualong local_function
; ASM-NEXT: .byte 98

; ASM-LABEL: packed_addend:
; ASM-NEXT: .byte 113
; ASM-NEXT: .ualong anchor+3
; ASM-NEXT: .byte 114

; ASM-LABEL: aligned_external:
; ASM-NEXT: .long 287454020
; ASM-NEXT: .long external_object
; ASM-NEXT: .long 1432778632

; ASM-LABEL: offset_two:
; ASM-NEXT: .uaword 4660
; ASM-NEXT: .ualong external_object
; ASM-NEXT: .byte 86

; ASM-LABEL: offset_three:
; ASM-NEXT: .uaword 22136
; ASM-NEXT: .byte 154
; ASM-NEXT: .ualong hidden_object
; ASM-NEXT: .byte 188

; ASM-LABEL: multiple:
; ASM-NEXT: .ualong default_object
; ASM-NEXT: .byte 221
; ASM-NEXT: .ualong external_function

; ASM-LABEL: nested:
; ASM-NEXT: .byte 129
; ASM-NEXT: .byte 130
; ASM-NEXT: .ualong external_object
; ASM-NEXT: .byte 131
; ASM-NEXT: .ualong external_function

; ASM-LABEL: packed_array:
; ASM-NEXT: .byte 145
; ASM-NEXT: .ualong default_object
; ASM-NEXT: .byte 146
; ASM-NEXT: .byte 161
; ASM-NEXT: .ualong hidden_object
; ASM-NEXT: .byte 162

; ASM-LABEL: mixed_null:
; ASM-NEXT: .byte 177
; ASM-NEXT: .ualong 0
; ASM-NEXT: .byte 178

; OBJ: Name: .tdata
; OBJ: SHF_TLS
; OBJ-DAG: 0x1 R_SH_DIR32 external_object
; OBJ-DAG: R_SH_DIR32 default_object
; OBJ-DAG: R_SH_DIR32 hidden_object
; OBJ-DAG: R_SH_DIR32 internal_object
; OBJ-DAG: R_SH_DIR32 external_function
; OBJ-DAG: R_SH_DIR32 local_function
; OBJ-DAG: R_SH_DIR32 anchor
; OBJ: Name: packed_external
; OBJ-NEXT: Value: 0x0
; OBJ-NEXT: Size: 6
; OBJ: Type: TLS
; OBJ-BE: Hex dump of section '.tdata':
; OBJ-BE: 0x00000000 12000000 0034
; OBJ-LE: Hex dump of section '.tdata':
; OBJ-LE: 0x00000000 12000000 0034
