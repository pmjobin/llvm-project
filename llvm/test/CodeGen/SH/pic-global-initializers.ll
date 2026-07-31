; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -function-sections -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -function-sections -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --file-headers --sections --symbols --relocations %t.be.o | FileCheck %s --check-prefixes=OBJECT,BE
; RUN: llvm-readobj --file-headers --sections --symbols --relocations %t.le.o | FileCheck %s --check-prefixes=OBJECT,LE

%Pointers = type { ptr, ptr, ptr }

@local_data = internal global i32 1, align 4
@hidden_data = hidden global i32 2, align 4
@default_data = global i32 3, align 4
@external_data = external global i32

@local_pointer = global ptr @local_data, align 4
@hidden_pointer = global ptr @hidden_data, align 4
@default_pointer = global ptr @default_data, align 4
@external_pointer = global ptr @external_data, align 4
@addend_pointer = global ptr getelementptr (i8, ptr @hidden_data, i32 12), align 4
@local_function_pointer = global ptr @local_function, align 4
@hidden_function_pointer = global ptr @hidden_function, align 4
@external_function_pointer = global ptr @external_function, align 4
@pointer_array = global [3 x ptr] [ptr @local_data, ptr @external_data, ptr @external_function], align 4
@pointer_aggregate = global %Pointers { ptr @hidden_data, ptr @default_data, ptr @hidden_function }, align 4
@block_pointer = global ptr blockaddress(@block_owner, %target), align 4

declare i32 @external_function(i32)

define internal i32 @local_function(i32 %x) {
entry:
	ret i32 %x
}

define hidden i32 @hidden_function(i32 %x) {
entry:
	ret i32 %x
}

define i32 @block_owner(i32 %value) {
entry:
	%condition = icmp eq i32 %value, 0
	br i1 %condition, label %target, label %other

target:
	ret i32 1

other:
	ret i32 0
}

; BE: Format: elf32-sh
; BE: DataEncoding: BigEndian
; LE: Format: elf32-shl
; LE: DataEncoding: LittleEndian
; OBJECT: Machine: EM_SH
; OBJECT: Section {{.*}} .rela.data {
; OBJECT-DAG: R_SH_DIR32 local_data
; OBJECT-DAG: R_SH_DIR32 hidden_data
; OBJECT-DAG: R_SH_DIR32 default_data
; OBJECT-DAG: R_SH_DIR32 external_data
; OBJECT-DAG: R_SH_DIR32 hidden_data 0x0
; OBJECT-DAG: R_SH_DIR32 local_function
; OBJECT-DAG: R_SH_DIR32 hidden_function
; OBJECT-DAG: R_SH_DIR32 external_function
; OBJECT-DAG: R_SH_DIR32 .Ltmp
; OBJECT: }
; OBJECT-NOT: Section {{.*}} .rela.text
; OBJECT-DAG: Name: local_data
; OBJECT-DAG: Binding: Local
; OBJECT-DAG: Name: hidden_data
; OBJECT-DAG: STV_HIDDEN
; OBJECT-DAG: Name: default_data
; OBJECT-DAG: Name: external_data
; OBJECT-DAG: Section: Undefined
; OBJECT-DAG: Name: hidden_function
; OBJECT-DAG: STV_HIDDEN
; OBJECT-DAG: Name: external_function
; OBJECT-DAG: Section: Undefined
