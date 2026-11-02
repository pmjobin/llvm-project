; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=STATIC
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=STATIC
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-dwarfdump --verify %t.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --verify %t.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-readobj --relocations %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --relocations %t.le.o | FileCheck %s --check-prefix=OBJECT

declare void @consume(ptr)

define i64 @large_frame_helpers(ptr %atomic, float %left, float %right, i64 %dividend, i64 %divisor) uwtable {
	%frame = alloca [131072 x i8], align 32
	%last = getelementptr [131072 x i8], ptr %frame, i32 0, i32 131068
	%sum = fadd float %left, %right
	store volatile float %sum, ptr %last, align 4
	%atomic.value = load atomic i16, ptr %atomic acquire, align 2
	%wide = zext i16 %atomic.value to i64
	%adjusted = add i64 %dividend, %wide
	%quotient = udiv i64 %adjusted, %divisor
	call void @consume(ptr %frame)
	ret i64 %quotient
}

; STATIC-LABEL: large_frame_helpers:
; STATIC: sub	r3,r15
; STATIC: mov	r14,r15
; STATIC: .long	__addsf3
; STATIC: .long	__atomic_load_2
; STATIC: .long	__udivdi3

; PIC-LABEL: large_frame_helpers:
; PIC: mov.l	r12,
; PIC: mov.l	@{{.*}},r12
; PIC: .long	__addsf3@PLT
; PIC: .long	__atomic_load_2@PLT
; PIC: .long	consume@PLT
; PIC: .long	__udivdi3@PLT

; VERIFY-NOT: error:
; VERIFY: No errors.

; OBJECT: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; OBJECT: R_SH_PLT32 __addsf3
; OBJECT: R_SH_PLT32 __atomic_load_2
; OBJECT: R_SH_PLT32 consume
; OBJECT: R_SH_PLT32 __udivdi3
; OBJECT-NOT: R_SH_DIR32 __atomic_load_2
