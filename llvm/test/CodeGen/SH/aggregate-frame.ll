; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=LE

%M = type { i8, i16, i32 }
%P = type <{ i8, i32 }>
%B60 = type [60 x i8]

declare void @llvm.memset.p0.i32(ptr, i8, i32, i1 immarg)

define i32 @fixed_m(i32 %a, i32 %b) {
	%slot = alloca %M, align 4
	%x0 = insertvalue %M poison, i8 1, 0
	%x1 = insertvalue %M %x0, i16 2, 1
	%x2 = insertvalue %M %x1, i32 %a, 2
	store volatile %M %x2, ptr %slot, align 4
	%loaded = load volatile %M, ptr %slot, align 4
	%result = extractvalue %M %loaded, 2
	ret i32 %result
}

define i32 @fixed_packed(i32 %value) {
	%slot = alloca %P, align 1
	%x0 = insertvalue %P poison, i8 1, 0
	%x1 = insertvalue %P %x0, i32 %value, 1
	store volatile %P %x1, ptr %slot, align 1
	%loaded = load volatile %P, ptr %slot, align 1
	%result = extractvalue %P %loaded, 1
	ret i32 %result
}

; BE-LABEL: fixed_sixty:
; BE: add	#-60,r15
; BE: mov.l	{{.*}},@(56,r15)
; BE: add	#60,r15
;
; LE-LABEL: fixed_sixty:
; LE: add	#-60,r15
; LE: mov.l	{{.*}},@(56,r15)
; LE: add	#60,r15
define void @fixed_sixty() {
	%slot = alloca %B60, align 4
	call void @llvm.memset.p0.i32(ptr align 4 %slot, i8 90, i32 60, i1 true)
	ret void
}
