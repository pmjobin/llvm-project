; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL

declare void @consume(i32)

define i32 @immediate_min() nounwind {
; ASM-LABEL: immediate_min:
; ASM: mov	#-128,r0
	ret i32 -128
}

define i32 @immediate_max() nounwind {
; ASM-LABEL: immediate_max:
; ASM: mov	#127,r0
	ret i32 127
}

define i32 @immediate_below() nounwind {
; ASM-LABEL: immediate_below:
; ASM: mov	#-1,r0
; ASM: shll8	r0
; ASM: add	#127,r0
	ret i32 -129
}

define i32 @immediate_above() nounwind {
; ASM-LABEL: immediate_above:
; ASM: mov	#-128,r0
; ASM: extu.b	r0,r0
	ret i32 128
}

define i32 @word_synthesis() nounwind {
; ISEL-LABEL: name:            word_synthesis
; ISEL: [[HIGH:%[0-9]+]]:gpr = MOVri 18
; ISEL-NEXT: [[SHIFTED:%[0-9]+]]:gpr = SHLL8 [[HIGH]]
; ISEL-NEXT: [[VALUE:%[0-9]+]]:gpr = ADDri [[SHIFTED]], 52
; ASM-LABEL: word_synthesis:
; ASM: mov	#18,r0
; ASM: shll8	r0
; ASM: add	#52,r0
	ret i32 4660
}

define i32 @high_bit_synthesis() nounwind {
; ASM-LABEL: high_bit_synthesis:
; ASM: mov	#-128,r0
; ASM: shll16	r0
; ASM: shll8	r0
	ret i32 -2147483648
}

define i32 @arbitrary_literal() nounwind {
; ISEL-LABEL: name:            arbitrary_literal
; ISEL: MOVL_load_pc_island %const.0, -1
; ASM-LABEL: arbitrary_literal:
; ASM: mov.l	[[LITERAL:.LCPI[0-9]+_[0-9]+_[0-9]+]],r0
; ASM: [[LITERAL]]:
; ASM: .long	305419896
	ret i32 305419896
}

define i32 @speed_six_instructions() nounwind {
; ASM-LABEL: speed_six_instructions:
; ASM-NOT: mov.l
; ASM: mov	#18,r0
; ASM: shll8	r0
; ASM: add	#52,r0
; ASM: shll8	r0
; ASM: add	#86,r0
; ASM: shll8	r0
	ret i32 305419776
}

define i32 @size_six_instructions() minsize nounwind {
; ASM-LABEL: size_six_instructions:
; ASM: mov.l	[[LITERAL:.LCPI[0-9]+_[0-9]+_[0-9]+]],r0
; ASM: [[LITERAL]]:
; ASM: .long	305419776
	ret i32 305419776
}

define i32 @size_three_instructions() minsize nounwind {
; ASM-LABEL: size_three_instructions:
; ASM-NOT: mov.l
; ASM: mov	#18,r0
; ASM-NEXT: shll8	r0
; ASM: add	#52,r0
	ret i32 4660
}

define i32 @size_four_instructions() minsize nounwind {
; ASM-LABEL: size_four_instructions:
; ASM: mov.l	[[FOUR_STEP_LITERAL:.LCPI[0-9]+_[0-9]+_[0-9]+]],r0
; ASM: [[FOUR_STEP_LITERAL]]:
; ASM-NEXT: .long	1192960
	ret i32 1192960
}

define i32 @optsize_two_instructions() optsize nounwind {
; ASM-LABEL: optsize_two_instructions:
; ASM: mov.w	[[TWO_STEP_LITERAL:.LCPI[0-9]+_[0-9]+_[0-9]+]],r0
; ASM: [[TWO_STEP_LITERAL]]:
; ASM-NEXT: .short	256
	ret i32 256
}

define i32 @optsize_negative_word() optsize nounwind {
; ASM-LABEL: optsize_negative_word:
; ASM: mov.w	[[NEGATIVE_WORD:.LCPI[0-9]+_[0-9]+_[0-9]+]],r0
; ASM: [[NEGATIVE_WORD]]:
; ASM-NEXT: .short	65236
	ret i32 -300
}

define i32 @minsize_two_instructions() minsize nounwind {
; ASM-LABEL: minsize_two_instructions:
; ASM-NOT: mov.l
; ASM: mov	#1,r0
; ASM: rts
; ASM-NEXT: shll8	r0
	ret i32 256
}

define i32 @repeated_literal(i32 %lhs, i32 %rhs) nounwind {
; ISEL-LABEL: name:            repeated_literal
; ISEL-COUNT-1: MOVL_load_pc_island
	%left = xor i32 %lhs, 305419896
	%right = xor i32 %rhs, 305419896
	%result = add i32 %left, %right
	ret i32 %result
}

define i32 @constant_after_call(i32 %value) nounwind {
; ASM-LABEL: constant_after_call:
; ASM: mov	#16,[[ARG:r[0-9]+]]
; ASM: jsr
; ASM-NEXT: shll8	[[ARG]]
; ASM: mov	#15,[[MASK:r[0-9]+]]
; ASM-NEXT: add	#1,[[MASK]]
; ASM-NEXT: shll8	[[MASK]]
; ASM-NEXT: add	#-1,[[MASK]]
; ASM: and	{{r[0-9]+}},[[MASK]]
	call void @consume(i32 4096)
	%masked = and i32 %value, 4095
	ret i32 %masked
}
