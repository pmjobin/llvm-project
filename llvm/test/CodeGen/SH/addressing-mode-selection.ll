; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL

declare i32 @get_index()

@predec_alias = external global ptr
@predec_hold = external global i8

; Generic register-address acceptance must leave the compact displacement
; alternatives available when their original folding conditions are met.
define i32 @long_disp_max(ptr %base) nounwind {
; ISEL-LABEL: name:            long_disp_max
; ISEL: MOVL_load_disp {{.*}}, 60
; ASM-LABEL: long_disp_max:
; ASM: mov.l @(60,r4),r0
	%address = getelementptr i8, ptr %base, i32 60
	%value = load i32, ptr %address, align 4
	ret i32 %value
}

define void @long_disp_store(ptr %base, i32 %value) nounwind {
; ISEL-LABEL: name:            long_disp_store
; ISEL: MOVL_store_disp {{.*}}, 60
; ASM-LABEL: long_disp_store:
; ASM: mov.l r5,@(60,r4)
	%address = getelementptr i8, ptr %base, i32 60
	store i32 %value, ptr %address, align 4
	ret void
}

define i32 @postinc_load(ptr %base, ptr %updated) nounwind {
; ISEL-LABEL: name:            postinc_load
; ISEL: [[VALUE:%[0-9]+]]:gpr, [[BASE:%[0-9]+]]:gpr = MOVL_load_postinc
; ISEL: MOVL_store_reg {{(killed )?}}[[BASE]]
; ASM-LABEL: postinc_load:
; ASM: mov.l	@r4+,r0
; ASM: mov.l	r4,@r5
	%value = load i32, ptr %base, align 4
	%next = getelementptr i8, ptr %base, i32 4
	store ptr %next, ptr %updated, align 4
	ret i32 %value
}

define i32 @postinc_load_u8(ptr %base, ptr %updated) nounwind {
; ISEL-LABEL: name:            postinc_load_u8
; ISEL: [[SIGNED:%[0-9]+]]:gpr, [[BASE:%[0-9]+]]:gpr = MOVB_load_postinc
; ISEL: MOVL_store_reg {{(killed )?}}[[BASE]]
; ISEL: [[VALUE:%[0-9]+]]:gpr = EXTUB {{(killed )?}}[[SIGNED]]
; ASM-LABEL: postinc_load_u8:
; ASM: mov.b	@r4+,r0
; ASM: extu.b	r0,r0
	%value = load i8, ptr %base, align 1
	%extended = zext i8 %value to i32
	%next = getelementptr i8, ptr %base, i32 1
	store ptr %next, ptr %updated, align 4
	ret i32 %extended
}

define i32 @postinc_load_u16(ptr %base, ptr %updated) nounwind {
; ISEL-LABEL: name:            postinc_load_u16
; ISEL: [[SIGNED:%[0-9]+]]:gpr, [[BASE:%[0-9]+]]:gpr = MOVW_load_postinc
; ISEL: MOVL_store_reg {{(killed )?}}[[BASE]]
; ISEL: [[VALUE:%[0-9]+]]:gpr = EXTUW {{(killed )?}}[[SIGNED]]
; ASM-LABEL: postinc_load_u16:
; ASM: mov.w	@r4+,r0
; ASM: extu.w	r0,r0
	%value = load i16, ptr %base, align 2
	%extended = zext i16 %value to i32
	%next = getelementptr i8, ptr %base, i32 2
	store ptr %next, ptr %updated, align 4
	ret i32 %extended
}

define i32 @postinc_negative(ptr %base, ptr %same) nounwind {
; ASM-LABEL: postinc_negative:
; ASM-NOT: @r4+
; ASM: mov.l	@r4,r0
; ASM: mov.l	r4,@r5
	%value = load i32, ptr %base, align 4
	store ptr %base, ptr %same, align 4
	ret i32 %value
}

define ptr @predec_store(ptr %base, i32 %value) nounwind {
; ISEL-LABEL: name:            predec_store
; ISEL: [[BASE:%[0-9]+]]:gpr = MOVL_store_predec
; ISEL: $r0 = COPY [[BASE]]
; ASM-LABEL: predec_store:
; ASM: rts
; ASM-NEXT: mov.l	r5,@-{{r[0-9]+}}
	%next = getelementptr i8, ptr %base, i32 -4
	store i32 %value, ptr %next, align 4
	ret ptr %next
}

define ptr @predec_store_i8(ptr %base, i32 %value) nounwind {
; ISEL-LABEL: name:            predec_store_i8
; ISEL: [[BASE:%[0-9]+]]:gpr = MOVB_store_predec
; ASM-LABEL: predec_store_i8:
; ASM: mov.b	{{r[0-9]+}},@-{{r[0-9]+}}
	%next = getelementptr i8, ptr %base, i32 -1
	%narrow = trunc i32 %value to i8
	store i8 %narrow, ptr %next, align 1
	ret ptr %next
}

define ptr @predec_store_i16(ptr %base, i32 %value) nounwind {
; ISEL-LABEL: name:            predec_store_i16
; ISEL: [[BASE:%[0-9]+]]:gpr = MOVW_store_predec
; ASM-LABEL: predec_store_i16:
; ASM: mov.w	{{r[0-9]+}},@-{{r[0-9]+}}
	%next = getelementptr i8, ptr %base, i32 -2
	%narrow = trunc i32 %value to i16
	store i16 %narrow, ptr %next, align 2
	ret ptr %next
}

define i8 @predec_store_address_user(ptr %base, i8 %value) nounwind {
; ISEL-LABEL: name:            predec_store_address_user
; ISEL: [[BASE:%[0-9]+]]:gpr = MOVB_store_predec
; ISEL: MOVB_load_reg {{(killed )?}}[[BASE]]
; ASM-LABEL: predec_store_address_user:
; ASM: mov.b	{{r[0-9]+}},@-[[BASE:r[0-9]+]]
; ASM: mov.b	@[[BASE]],{{r[0-9]+}}
entry:
	%address = getelementptr i8, ptr %base, i32 -1
	store i8 %value, ptr %address, align 1
	store ptr %address, ptr @predec_alias, align 4
	%loaded = load i8, ptr %address, align 1
	store i8 %loaded, ptr @predec_hold, align 1
	ret i8 %loaded
}

define ptr @predec_negative(ptr %base, i32 %value) nounwind {
; ASM-LABEL: predec_negative:
; ASM-NOT: @-{{r[0-9]+}}
; ASM: add	#-8,[[ADDRESS:r[0-9]+]]
; ASM: mov.l	r5,@[[ADDRESS]]
	%address = getelementptr i8, ptr %base, i32 -8
	%next = getelementptr i8, ptr %base, i32 -4
	store i32 %value, ptr %address, align 4
	ret ptr %next
}

define i32 @postinc_loop(ptr %base, i32 %count) nounwind {
; ASM-LABEL: postinc_loop:
; ASM: mov.l	@{{r[0-9]+}}+,{{r[0-9]+}}
; ASM: dt	{{r[0-9]+}}
; ASM-NOT: add	#-1
; ASM-NOT: tst
entry:
	br label %loop

loop:
	%pointer = phi ptr [ %base, %entry ], [ %next, %loop ]
	%remaining = phi i32 [ %count, %entry ], [ %decremented, %loop ]
	%sum = phi i32 [ 0, %entry ], [ %next_sum, %loop ]
	%value = load i32, ptr %pointer, align 4
	%next_sum = add i32 %sum, %value
	%next = getelementptr i8, ptr %pointer, i32 4
	%decremented = add i32 %remaining, -1
	%done = icmp eq i32 %decremented, 0
	br i1 %done, label %exit, label %loop

exit:
	ret i32 %next_sum
}

define i32 @volatile_postinc(ptr %base, ptr %updated) nounwind {
; ISEL-LABEL: name:            volatile_postinc
; ISEL: MOVL_load_postinc {{.*}} :: (volatile load (s32)
; ASM-LABEL: volatile_postinc:
; ASM: mov.l	@r4+,r0
	%value = load volatile i32, ptr %base, align 4
	%next = getelementptr i8, ptr %base, i32 4
	store ptr %next, ptr %updated, align 4
	ret i32 %value
}

define i32 @atomic_boundary(ptr %base, ptr %updated) nounwind {
; ASM-LABEL: atomic_boundary:
; ASM-NOT: @r4+
	%value = load atomic i32, ptr %base seq_cst, align 4
	%next = getelementptr i8, ptr %base, i32 4
	store ptr %next, ptr %updated, align 4
	ret i32 %value
}

define i32 @byte_disp_max(ptr %base) nounwind {
; ISEL-LABEL: name:            byte_disp_max
; ISEL: {{%[0-9]+}}:r0reg = MOVB_load_disp {{.*}}, 15
; ASM-LABEL: byte_disp_max:
; ASM: mov.b	@(15,r4),r0
	%address = getelementptr i8, ptr %base, i32 15
	%value = load i8, ptr %address, align 1
	%extended = sext i8 %value to i32
	ret i32 %extended
}

define i32 @byte_disp_out_of_range(ptr %base) nounwind {
; ASM-LABEL: byte_disp_out_of_range:
; ASM-NOT: @(16,r4)
; ASM: add	#16,r4
; ASM: mov.b	@r4,r0
	%address = getelementptr i8, ptr %base, i32 16
	%value = load i8, ptr %address, align 1
	%extended = sext i8 %value to i32
	ret i32 %extended
}

define i32 @word_disp_max(ptr %base) nounwind {
; ISEL-LABEL: name:            word_disp_max
; ISEL: {{%[0-9]+}}:r0reg = MOVW_load_disp {{.*}}, 30
; ASM-LABEL: word_disp_max:
; ASM: mov.w	@(30,r4),r0
	%address = getelementptr i8, ptr %base, i32 30
	%value = load i16, ptr %address, align 2
	%extended = sext i16 %value to i32
	ret i32 %extended
}

define i32 @word_disp_out_of_range(ptr %base) nounwind {
; ASM-LABEL: word_disp_out_of_range:
; ASM-NOT: @(32,r4)
; ASM: add	#32,r4
; ASM: mov.w	@r4,r0
	%address = getelementptr i8, ptr %base, i32 32
	%value = load i16, ptr %address, align 2
	%extended = sext i16 %value to i32
	ret i32 %extended
}

define i32 @r0_indexed_load(ptr %base) nounwind {
; ISEL-LABEL: name:            r0_indexed_load
; ISEL: JSR
; ISEL: MOVL_load_indexed
; ASM-LABEL: r0_indexed_load:
; ASM: jsr
; ASM: mov.l	@(r0,{{r[0-9]+}}),r0
	%index = call i32 @get_index()
	%address = getelementptr i8, ptr %base, i32 %index
	%value = load i32, ptr %address, align 4
	ret i32 %value
}

define i32 @r0_indexed_requires_copy(ptr %base, i32 %index) nounwind {
; ISEL-LABEL: name:            r0_indexed_requires_copy
; ISEL: $r0 = COPY
; ISEL: MOVL_load_indexed
; ASM-LABEL: r0_indexed_requires_copy:
; ASM-NOT: add
; ASM: mov	{{r[0-9]+}},r0
; ASM-NEXT: rts
; ASM-NEXT: mov.l	@(r0,{{r[0-9]+}}),r0
	%address = getelementptr i8, ptr %base, i32 %index
	%value = load i32, ptr %address, align 4
	ret i32 %value
}

define i32 @r0_indexed_byte_load(ptr %base, i32 %index) nounwind {
; ISEL-LABEL: name:            r0_indexed_byte_load
; ISEL: MOVB_load_indexed
; ASM-LABEL: r0_indexed_byte_load:
; ASM: mov.b	@(r0,{{r[0-9]+}}),{{r[0-9]+}}
	%byte_address = getelementptr i8, ptr %base, i32 %index
	%byte = load i8, ptr %byte_address, align 1
	%byte_extended = zext i8 %byte to i32
	ret i32 %byte_extended
}

define i32 @r0_indexed_word_load(ptr %base, i32 %index) nounwind {
; ISEL-LABEL: name:            r0_indexed_word_load
; ISEL: MOVW_load_indexed
; ASM-LABEL: r0_indexed_word_load:
; ASM: mov.w	@(r0,{{r[0-9]+}}),{{r[0-9]+}}
	%word_address = getelementptr i8, ptr %base, i32 %index
	%word = load i16, ptr %word_address, align 2
	%word_extended = zext i16 %word to i32
	ret i32 %word_extended
}

define void @r0_indexed_store(ptr %base, i32 %index, i32 %value) nounwind {
; ISEL-LABEL: name:            r0_indexed_store
; ISEL: $r0 = COPY
; ISEL: MOVL_store_indexed
; ASM-LABEL: r0_indexed_store:
; ASM-NOT: add
; ASM: mov.l	{{r[0-9]+}},@(r0,{{r[0-9]+}})
	%address = getelementptr i8, ptr %base, i32 %index
	store i32 %value, ptr %address, align 4
	ret void
}

define i32 @r0_indexed_multiuse_negative(ptr %base, i32 %index) nounwind {
; ASM-LABEL: r0_indexed_multiuse_negative:
; ASM-NOT: @(r0,
; ASM: add	{{r[0-9]+}},{{r[0-9]+}}
	store ptr %base, ptr @predec_alias, align 4
	%address = getelementptr i8, ptr %base, i32 %index
	%value = load i32, ptr %address, align 4
	%sum = add i32 %value, %index
	ret i32 %sum
}
