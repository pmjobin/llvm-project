; RUN: llc -O0 -mtriple=shle-unknown-none-elf -mcpu=sh4 -verify-machineinstrs %s -o /dev/null
; RUN: llc -O2 -mtriple=shle-unknown-none-elf -mcpu=sh4 -verify-machineinstrs %s -o /dev/null
; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -verify-machineinstrs %s -o /dev/null
; RUN: llc -O3 -mtriple=sh-unknown-none-elf -mcpu=sh4 -verify-machineinstrs %s -o /dev/null
; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu -verify-machineinstrs %s -o /dev/null
; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs %s -o /dev/null
; RUN: llc -O3 -mtriple=sh-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs %s -o /dev/null
; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -target-abi=gnu -verify-machineinstrs %s -o /dev/null
; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -enable-new-pm -verify-machineinstrs %s -o /dev/null
; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -filetype=obj -verify-machineinstrs %s -o %t.o
; RUN: llc -O0 -mtriple=shle-unknown-none-elf -mcpu=sh4 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ISEL

; The particular choice is unspecified, but it must have a real definition
; before it can be copied to independently observable uses across clobbers.
; ISEL-LABEL: name: frozen_poison
; ISEL-NOT: IMPLICIT_DEF
; ISEL-LABEL: name: frozen_undef
; ISEL-NOT: IMPLICIT_DEF
; ISEL-LABEL: name: frozen_poison_only
; ISEL-NOT: IMPLICIT_DEF
; ISEL-LABEL: name: ordinary_undef
; ISEL-LABEL: name: twice
; ISEL-NOT: IMPLICIT_DEF
; ISEL-LABEL: name: twice_bits
; ISEL-NOT: IMPLICIT_DEF
; ISEL-LABEL: name: twice_float
; ISEL-NOT: IMPLICIT_DEF
; ISEL-LABEL: name: size_frozen

; Keep the original supported-IR failure, including its unused argument and
; the default GNU soft-argument ABI with an available FPU.
define double @frozen_poison(i1 %c, double %x) {
entry:
	%f = freeze i1 %c
	%s = select i1 %f, double poison, double undef
	%r = freeze double %s
	ret double %r
}

define double @frozen_undef() {
	%r = freeze double undef
	ret double %r
}

define double @frozen_poison_only() {
	%r = freeze double poison
	ret double %r
}

define double @ordinary_undef() {
	ret double undef
}

define double @defined(double %x) {
	%r = freeze double %x
	ret double %r
}

declare void @clobber()

; Two observable uses of one chosen value, separated by a call. The volatile
; outputs can be inspected as integer bits, including NaNs and signed zero.
define void @twice(ptr %a, ptr %b) {
	%r = freeze double undef
	store volatile double %r, ptr %a, align 4
	call void @clobber()
	store volatile double %r, ptr %b, align 4
	ret void
}

define void @twice_bits(ptr %a, ptr %b) {
	%r = freeze double poison
	%bits = bitcast double %r to i64
	store volatile i64 %bits, ptr %a, align 4
	call void @clobber()
	store volatile i64 %bits, ptr %b, align 4
	ret void
}

define void @twice_float(ptr %a, ptr %b) {
	%r = freeze float undef
	store volatile float %r, ptr %a, align 4
	call void @clobber()
	store volatile float %r, ptr %b, align 4
	ret void
}

define double @size_frozen() optsize {
	%r = freeze double undef
	ret double %r
}

define double @minsize_frozen() minsize optsize {
	%r = freeze double undef
	ret double %r
}

define double @optnone_frozen() noinline optnone {
	%r = freeze double undef
	ret double %r
}

define void @ordinary_undef_store(ptr %a) {
	store volatile double undef, ptr %a, align 4
	ret void
}

define void @defined_twice(ptr %a, ptr %b, double %x) {
	%r = freeze double %x
	%bits = bitcast double %r to i64
	store volatile i64 %bits, ptr %a, align 4
	call void @clobber()
	store volatile i64 %bits, ptr %b, align 4
	ret void
}

define void @partial_twice(ptr %a, ptr %b, i32 %known) {
	%v = insertelement <2 x i32> undef, i32 %known, i32 0
	%d = bitcast <2 x i32> %v to double
	%r = freeze double %d
	store volatile double %r, ptr %a, align 4
	call void @clobber()
	store volatile double %r, ptr %b, align 4
	ret void
}

define void @conditional_twice(ptr %a, ptr %b, i1 %c, double %x) {
entry:
	br i1 %c, label %left, label %right
left:
	br label %join
right:
	br label %join
join:
	%p = phi double [ %x, %left ], [ undef, %right ]
	%r = freeze double %p
	store volatile double %r, ptr %a, align 4
	call void @clobber()
	store volatile double %r, ptr %b, align 4
	ret void
}
