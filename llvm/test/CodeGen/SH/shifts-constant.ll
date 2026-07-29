; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define i32 @shl_0(i32 %value) nounwind {
; CHECK-LABEL: shl_0:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: rts
	%result = shl i32 %value, 0
	ret i32 %result
}

define i32 @shl_1(i32 %value) nounwind {
; CHECK-LABEL: shl_1:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shll	r0
	%result = shl i32 %value, 1
	ret i32 %result
}

define i32 @shl_2(i32 %value) nounwind {
; CHECK-LABEL: shl_2:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shll2	r0
	%result = shl i32 %value, 2
	ret i32 %result
}

define i32 @shl_3(i32 %value) nounwind {
; CHECK-LABEL: shl_3:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shll2	r0
; CHECK-NEXT: shll	r0
	%result = shl i32 %value, 3
	ret i32 %result
}

define i32 @shl_7(i32 %value) nounwind {
; CHECK-LABEL: shl_7:
; CHECK-NEXT: mov	r4,r0
; CHECK-COUNT-3: shll2	r0
; CHECK-NEXT: shll	r0
	%result = shl i32 %value, 7
	ret i32 %result
}

define i32 @shl_8(i32 %value) nounwind {
; CHECK-LABEL: shl_8:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shll8	r0
	%result = shl i32 %value, 8
	ret i32 %result
}

define i32 @shl_15(i32 %value) nounwind {
; CHECK-LABEL: shl_15:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shll8	r0
; CHECK-COUNT-3: shll2	r0
; CHECK-NEXT: shll	r0
	%result = shl i32 %value, 15
	ret i32 %result
}

define i32 @shl_16(i32 %value) nounwind {
; CHECK-LABEL: shl_16:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shll16	r0
	%result = shl i32 %value, 16
	ret i32 %result
}

define i32 @shl_17(i32 %value) nounwind {
; CHECK-LABEL: shl_17:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shll16	r0
; CHECK-NEXT: shll	r0
	%result = shl nuw i32 %value, 17
	ret i32 %result
}

define i32 @shl_24(i32 %value) nounwind {
; CHECK-LABEL: shl_24:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shll16	r0
; CHECK-NEXT: shll8	r0
	%result = shl nsw i32 %value, 24
	ret i32 %result
}

define i32 @shl_31(i32 %value) nounwind {
; CHECK-LABEL: shl_31:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shll16	r0
; CHECK-NEXT: shll8	r0
; CHECK-COUNT-3: shll2	r0
; CHECK-NEXT: shll	r0
	%result = shl i32 %value, 31
	ret i32 %result
}

define i32 @lshr_0(i32 %value) nounwind {
; CHECK-LABEL: lshr_0:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: rts
	%result = lshr i32 %value, 0
	ret i32 %result
}

define i32 @lshr_1(i32 %value) nounwind {
; CHECK-LABEL: lshr_1:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shlr	r0
	%result = lshr i32 %value, 1
	ret i32 %result
}

define i32 @lshr_2(i32 %value) nounwind {
; CHECK-LABEL: lshr_2:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shlr2	r0
	%result = lshr i32 %value, 2
	ret i32 %result
}

define i32 @lshr_3(i32 %value) nounwind {
; CHECK-LABEL: lshr_3:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shlr2	r0
; CHECK-NEXT: shlr	r0
	%result = lshr exact i32 %value, 3
	ret i32 %result
}

define i32 @lshr_7(i32 %value) nounwind {
; CHECK-LABEL: lshr_7:
; CHECK-NEXT: mov	r4,r0
; CHECK-COUNT-3: shlr2	r0
; CHECK-NEXT: shlr	r0
	%result = lshr i32 %value, 7
	ret i32 %result
}

define i32 @lshr_8(i32 %value) nounwind {
; CHECK-LABEL: lshr_8:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shlr8	r0
	%result = lshr i32 %value, 8
	ret i32 %result
}

define i32 @lshr_15(i32 %value) nounwind {
; CHECK-LABEL: lshr_15:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shlr8	r0
; CHECK-COUNT-3: shlr2	r0
; CHECK-NEXT: shlr	r0
	%result = lshr i32 %value, 15
	ret i32 %result
}

define i32 @lshr_16(i32 %value) nounwind {
; CHECK-LABEL: lshr_16:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shlr16	r0
	%result = lshr i32 %value, 16
	ret i32 %result
}

define i32 @lshr_17(i32 %value) nounwind {
; CHECK-LABEL: lshr_17:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shlr16	r0
; CHECK-NEXT: shlr	r0
	%result = lshr i32 %value, 17
	ret i32 %result
}

define i32 @lshr_24(i32 %value) nounwind {
; CHECK-LABEL: lshr_24:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shlr16	r0
; CHECK-NEXT: shlr8	r0
	%result = lshr i32 %value, 24
	ret i32 %result
}

define i32 @lshr_31(i32 %value) nounwind {
; CHECK-LABEL: lshr_31:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shlr16	r0
; CHECK-NEXT: shlr8	r0
; CHECK-COUNT-3: shlr2	r0
; CHECK-NEXT: shlr	r0
	%result = lshr i32 %value, 31
	ret i32 %result
}

define i32 @ashr_0(i32 %value) nounwind {
; CHECK-LABEL: ashr_0:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: rts
	%result = ashr i32 %value, 0
	ret i32 %result
}

define i32 @ashr_1(i32 %value) nounwind {
; CHECK-LABEL: ashr_1:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shar	r0
	%result = ashr i32 %value, 1
	ret i32 %result
}

define i32 @ashr_2(i32 %value) nounwind {
; CHECK-LABEL: ashr_2:
; CHECK-NEXT: mov	r4,r0
; CHECK-COUNT-2: shar	r0
	%result = ashr i32 %value, 2
	ret i32 %result
}

define i32 @ashr_3(i32 %value) nounwind {
; CHECK-LABEL: ashr_3:
; CHECK-NEXT: mov	r4,r0
; CHECK-COUNT-3: shar	r0
	%result = ashr exact i32 %value, 3
	ret i32 %result
}

define i32 @ashr_7(i32 %value) nounwind {
; CHECK-LABEL: ashr_7:
; CHECK-NEXT: mov	r4,r0
; CHECK-COUNT-7: shar	r0
	%result = ashr i32 %value, 7
	ret i32 %result
}

define i32 @ashr_8(i32 %value) nounwind {
; CHECK-LABEL: ashr_8:
; CHECK-NEXT: mov	r4,r0
; CHECK-COUNT-8: shar	r0
	%result = ashr i32 %value, 8
	ret i32 %result
}

define i32 @ashr_15(i32 %value) nounwind {
; CHECK-LABEL: ashr_15:
; CHECK-NEXT: mov	r4,r0
; CHECK-COUNT-15: shar	r0
	%result = ashr i32 %value, 15
	ret i32 %result
}

define i32 @ashr_16(i32 %value) nounwind {
; CHECK-LABEL: ashr_16:
; CHECK-NEXT: shlr16	[[REG:r[0-9]+]]
; CHECK-NEXT: exts.w	[[REG]],r0
	%result = ashr i32 %value, 16
	ret i32 %result
}

define i32 @ashr_17(i32 %value) nounwind {
; CHECK-LABEL: ashr_17:
; CHECK-NEXT: shlr16	[[REG:r[0-9]+]]
; CHECK-NEXT: exts.w	[[REG]],r0
; CHECK-NEXT: shar	r0
	%result = ashr i32 %value, 17
	ret i32 %result
}

define i32 @ashr_24(i32 %value) nounwind {
; CHECK-LABEL: ashr_24:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shlr16	r0
; CHECK-NEXT: shlr8	r0
; CHECK-NEXT: exts.b	r0,r0
	%result = ashr i32 %value, 24
	ret i32 %result
}

define i32 @ashr_31(i32 %value) nounwind {
; CHECK-LABEL: ashr_31:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: shlr16	r0
; CHECK-NEXT: shlr8	r0
; CHECK-NEXT: exts.b	r0,r0
; CHECK-COUNT-7: shar	r0
	%result = ashr i32 %value, 31
	ret i32 %result
}
