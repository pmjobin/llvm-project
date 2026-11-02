; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,BE

define double @load_double(ptr %address) {
; COMMON-LABEL: load_double:
; COMMON-NOT: sts fpscr
; COMMON: mov #4,r0
; LE-NEXT: fmov.s @r4,fr1
; BE-NEXT: fmov.s @r4,fr0
; COMMON-NEXT: rts
; LE-NEXT: fmov.s @(r0,r4),fr0
; BE-NEXT: fmov.s @(r0,r4),fr1
	%value = load double, ptr %address, align 4
	ret double %value
}

define void @store_double(ptr %address, double %value) {
; COMMON-LABEL: store_double:
; COMMON-NOT: sts fpscr
; COMMON: mov #4,r0
; LE-NEXT: fmov.s fr5,@r4
; BE-NEXT: fmov.s fr4,@r4
; COMMON-NEXT: rts
; LE-NEXT: fmov.s fr4,@(r0,r4)
; BE-NEXT: fmov.s fr5,@(r0,r4)
	store double %value, ptr %address, align 4
	ret void
}

define void @copy_double(ptr %destination, ptr %source) {
; COMMON-LABEL: copy_double:
; COMMON-NOT: sts fpscr
; COMMON: mov #4,r0
; LE-NEXT: fmov.s @r5,fr1
; BE-NEXT: fmov.s @r5,fr0
; LE-NEXT: fmov.s @(r0,r5),fr0
; LE-NEXT: fmov.s fr1,@r4
; LE-NEXT: rts
; LE-NEXT: fmov.s fr0,@(r0,r4)
; BE-NEXT: fmov.s @(r0,r5),fr1
; BE-NEXT: fmov.s fr0,@r4
; BE-NEXT: rts
; BE-NEXT: fmov.s fr1,@(r0,r4)
	%value = load double, ptr %source, align 4
	store double %value, ptr %destination, align 4
	ret void
}

define void @copy_float(ptr %destination, ptr %source) {
; COMMON-LABEL: copy_float:
; COMMON-NOT: fpscr
; COMMON: fmov @r5,fr0
; COMMON-NEXT: rts
; COMMON-NEXT: fmov fr0,@r4
	%value = load float, ptr %source, align 4
	store float %value, ptr %destination, align 4
	ret void
}

define void @negate_float(ptr %destination, ptr %source) {
; COMMON-LABEL: negate_float:
; COMMON-NOT: fpscr
; COMMON: fmov @r5,fr0
; COMMON-NEXT: fneg fr0
; COMMON-NEXT: rts
; COMMON-NEXT: fmov fr0,@r4
	%value = load float, ptr %source, align 4
	%negated = fneg float %value
	store float %negated, ptr %destination, align 4
	ret void
}

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}
