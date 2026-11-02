; The ordinary pipeline does not force an extra size-only merge-functions pass.
; RUN: opt -mtriple=shle-unknown-none-elf -passes='default<O2>' -S %s -o - | FileCheck %s --check-prefix=NONE
; RUN: opt -mtriple=shle-unknown-none-elf -passes='default<O0>' -S %s -o - | FileCheck %s --check-prefix=NONE

define i32 @first(i32 %value) #0 {
entry:
	%result = add i32 %value, 7
	ret i32 %result
}

define i32 @second(i32 %value) #0 {
entry:
	%result = add i32 %value, 7
	ret i32 %result
}

attributes #0 = { noinline optsize }

; SIZE-COUNT-1: add i32
; SIZE-COUNT-1: tail call i32

; NONE-COUNT-2: add i32
; NONE-NOT: tail call i32
