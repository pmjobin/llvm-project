; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null

define i32 @optimizer_i33(i32 %value) {
entry:
	%wide = zext i32 %value to i33
	%product = mul i33 %wide, %wide
	%shifted = lshr i33 %product, 1
	%condition = icmp ugt i33 %shifted, 42
	%selected = select i1 %condition, i33 %shifted, i33 42
	br i1 %condition, label %left, label %right

left:
	br label %return

right:
	%summed = add i33 %selected, 3
	br label %return

return:
	%result = phi i33 [ %selected, %left ], [ %summed, %right ]
	%truncated = trunc i33 %result to i32
	ret i32 %truncated
}
