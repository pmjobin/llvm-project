; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs < %s -o /dev/null

%struct.SourceLoc = type { i16, i16 }

define ptr @adjacent_large_frame_stores(ptr %CIntType, ptr %VoidType, ptr %FloatType, ptr %IntType, ptr %BooleanType, ptr %CFloatType, ptr %0, ptr %1, ptr %2, ptr %3, ptr %4, ptr %location) nounwind {
entry:
	%dummy = alloca %struct.SourceLoc, align 4
	store ptr null, ptr %CIntType, align 4
	%call5 = tail call ptr null(i32 0, i32 0)
	store ptr null, ptr %VoidType, align 4
	store ptr null, ptr %FloatType, align 4
	store ptr null, ptr %IntType, align 4
	%call8 = tail call ptr null(i32 0, i32 1)
	store ptr null, ptr %BooleanType, align 4
	%call9 = tail call ptr null(ptr %0, i32 1, i32 4096)
	%call10 = tail call ptr null(ptr null, i32 2, i32 0)
	%call11 = tail call ptr null(ptr null, i32 3, i32 0)
	%call12 = tail call ptr null(ptr null, i32 4, i32 0)
	%call13 = tail call ptr null(ptr %1, i32 0, i32 4096)
	%call21 = tail call ptr null(ptr %2, i32 0, i32 0)
	%call22 = tail call ptr null(ptr %3, i32 2, i32 0)
	%call23 = tail call ptr null(ptr %4, i32 3, i32 0)
	%call24 = tail call ptr null(ptr null, i32 4, i32 0)
	%call32 = call ptr null(ptr %CFloatType, ptr null, i32 0, ptr %location, i32 0)
	%call33 = call ptr null(ptr null, ptr null, i32 313, ptr %BooleanType, i32 0)
	%call34 = call ptr null(ptr %CIntType, ptr null, i32 0, ptr %CIntType, i32 1)
	%call80 = call ptr null(ptr null, ptr null, i32 0, ptr %VoidType, i32 0)
	store i32 0, ptr %FloatType, align 4
	store ptr %CIntType, ptr null, align 4
	store ptr null, ptr %CIntType, align 4
	ret ptr %CIntType
}

; OPT-LABEL: adjacent_large_frame_stores:
; OPT: sts.l	pr,@-r15
; OPT-NEXT: mov.l	r8,@-r15
; OPT-NEXT: mov.l	r9,@-r15
; OPT-NEXT: mov.l	r10,@-r15
; OPT-NEXT: mov.l	r11,@-r15
; OPT-NEXT: mov.l	r12,@-r15
; OPT-NEXT: mov.l	r13,@-r15
; OPT-NEXT: mov.l	r14,@-r15
; OPT-NEXT: add	#-36,r15
; OPT: add	#36,r15
; OPT: mov.l	@r15+,r8
; OPT-NEXT: lds.l	@r15+,pr
; OPT-NEXT: rts
; OPT-NEXT: nop
