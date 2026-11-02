; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=PERF
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=PERF
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=SH4
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj -o /dev/null < %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj -o /dev/null < %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj -o /dev/null < %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj -o /dev/null < %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh1 -O2 -verify-machineinstrs -filetype=obj -o /dev/null < %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2e -O2 -verify-machineinstrs -filetype=obj -o /dev/null < %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2a -O2 -verify-machineinstrs -filetype=obj -o /dev/null < %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -filetype=obj -o /dev/null < %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3e -O2 -verify-machineinstrs -filetype=obj -o /dev/null < %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -filetype=obj -o /dev/null < %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4a -O2 -verify-machineinstrs -filetype=obj -o /dev/null < %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4al-dsp -O2 -verify-machineinstrs -filetype=obj -o /dev/null < %s

declare i8 @llvm.fshl.i8(i8, i8, i8)
declare i16 @llvm.fshr.i16(i16, i16, i16)
declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)
declare { i8, i1 } @llvm.uadd.with.overflow.i8(i8, i8)
declare { i16, i1 } @llvm.ssub.with.overflow.i16(i16, i16)
declare { i32, i1 } @llvm.uadd.with.overflow.i32(i32, i32)
declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32)
declare { i32, i1 } @llvm.usub.with.overflow.i32(i32, i32)
declare { i32, i1 } @llvm.ssub.with.overflow.i32(i32, i32)
declare i32 @helper(i32)

@tls_value = external thread_local global i32

define i32 @rotate_left_1(i32 %value) {
; ASM-LABEL: rotate_left_1:
; ASM: rotl
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 1)
	ret i32 %result
}

define i32 @rotate_left_7(i32 %value) {
; ASM-LABEL: rotate_left_7:
; ASM-COUNT-7: rotl
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 7)
	ret i32 %result
}

define i32 @rotate_left_8(i32 %value) {
; ASM-LABEL: rotate_left_8:
; ASM: shlr16
; ASM: shlr8
; ASM: shll8
; ASM: or
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 8)
	ret i32 %result
}

define i32 @rotate_left_9(i32 %value) {
; ASM-LABEL: rotate_left_9:
; ASM: swap.w
; ASM-COUNT-7: rotr
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 9)
	ret i32 %result
}

define i32 @rotate_left_16(i32 %value) {
; ASM-LABEL: rotate_left_16:
; ASM: swap.w
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 16)
	ret i32 %result
}

define i32 @rotate_right_13(i32 %value) {
; ASM-LABEL: rotate_right_13:
; ASM: swap.w
; ASM-COUNT-3: rotl
	%result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 13)
	ret i32 %result
}

define i32 @rotate_left_24(i32 %value) {
; ASM-LABEL: rotate_left_24:
; ASM: shlr8
; ASM: shll16
; ASM: shll8
; ASM: or
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 24)
	ret i32 %result
}

define i32 @rotate_left_25(i32 %value) {
; ASM-LABEL: rotate_left_25:
; ASM-COUNT-7: rotr
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 25)
	ret i32 %result
}

define i32 @rotate_left_30(i32 %value) {
; ASM-LABEL: rotate_left_30:
; ASM-COUNT-2: rotr
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 30)
	ret i32 %result
}

define i32 @rotate_right_31(i32 %value) {
; ASM-LABEL: rotate_right_31:
; ASM: rotl
	%result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 31)
	ret i32 %result
}

define i32 @rotate_modulo_32(i32 %value) {
; ASM-LABEL: rotate_modulo_32:
; ASM-NOT: rot
; ASM: rts
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 32)
	ret i32 %result
}

define i32 @rotate_modulo_33(i32 %value) {
; ASM-LABEL: rotate_modulo_33:
; ASM: rotl
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 33)
	ret i32 %result
}

define i32 @rotate_modulo_63(i32 %value) {
; ASM-LABEL: rotate_modulo_63:
; ASM: rotr
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 63)
	ret i32 %result
}

define i32 @rotate_left_variable(i32 %value, i32 %count) {
; ASM-LABEL: rotate_left_variable:
; ASM: mov	#31,[[COUNT:r[0-9]+]]
; ASM-NEXT: and	[[COUNT]],{{r[0-9]+}}
; ASM: rotl
; ASM-NEXT: dt
; ASM: bf
; ISEL-LABEL: name:            rotate_left_variable
; ISEL: [[ROTATED:%[0-9]+]]:gpr = ROTL {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: [[NEXT_COUNT:%[0-9]+]]:gpr = DT {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: BF {{.*}}, implicit $tbit
; PERF-LABEL: rotate_left_variable:
; PERF-COUNT-2: jsr
; PERF: or
; PERF-DAG: .long __ashlsi3
; PERF-DAG: .long __lshrsi3
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 %count)
	ret i32 %result
}

define i32 @rotate_right_variable(i32 %value, i32 %count) {
; ASM-LABEL: rotate_right_variable:
; ASM: mov	#31,[[COUNT:r[0-9]+]]
; ASM-NEXT: and	[[COUNT]],{{r[0-9]+}}
; ASM: rotr
; ASM-NEXT: dt
; ASM: bf
; PERF-LABEL: rotate_right_variable:
; PERF-COUNT-2: jsr
; PERF: or
; PERF-DAG: .long __ashlsi3
; PERF-DAG: .long __lshrsi3
	%result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 %count)
	ret i32 %result
}

define i32 @clear_bit_mask(i32 %count) optsize {
; ASM-LABEL: clear_bit_mask:
; ASM: mov	#31,{{r[0-9]+}}
; ASM: and
; ASM: .long	__ashlsi3
; ASM-NOT: .long	__lshrsi3
; PERF-LABEL: clear_bit_mask:
; PERF: mov	#31,{{r[0-9]+}}
; PERF: and
; PERF: .long	__ashlsi3
; PERF-NOT: .long	__lshrsi3
; SH4-LABEL: clear_bit_mask:
; SH4: mov	#31,[[COUNT:r[0-9]+]]
; SH4: and	{{r[0-9]+}},[[COUNT]]
; SH4: mov	#1,[[MASK:r[0-9]+]]
; SH4-COUNT-1: shld	[[COUNT]],[[MASK]]
; SH4: rts
; SH4-NEXT: not	[[MASK]],r0
; SH4-NOT: {{^ *or	}}
	%result = call i32 @llvm.fshl.i32(i32 -2, i32 -2, i32 %count)
	ret i32 %result
}

define i32 @rotate_left_variable_short(i32 %value, i32 %count) {
; PERF-LABEL: rotate_left_variable_short:
; PERF: and
; PERF-NOT: dt
; PERF-COUNT-15: rotl
; PERF-NOT: dt
; SH4-LABEL: rotate_left_variable_short:
; SH4-COUNT-2: shld
; SH4: or
; SH4-NOT: rotl
	%short = and i32 %count, 15
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 %short)
	ret i32 %result
}

define i32 @rotate_right_variable_short(i32 %value, i32 %count) {
; PERF-LABEL: rotate_right_variable_short:
; PERF: and
; PERF-NOT: dt
; PERF-COUNT-15: rotr
; PERF-NOT: dt
; SH4-LABEL: rotate_right_variable_short:
; SH4-COUNT-2: shld
; SH4: or
; SH4-NOT: rotr
	%short = and i32 %count, 15
	%result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 %short)
	ret i32 %result
}

define i32 @rotate_left_variable_short_optsize(i32 %value, i32 %count) optsize {
; PERF-LABEL: rotate_left_variable_short_optsize:
; PERF: and
; PERF: tst
; PERF: rotl
; PERF-NEXT: dt
; PERF: bf
	%short = and i32 %count, 15
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 %short)
	ret i32 %result
}

define i32 @rotate_left_variable_7(i32 %value, i32 %count) {
; PERF-LABEL: rotate_left_variable_7:
; PERF: and
; PERF: tst
; PERF: rotl
; PERF-NEXT: dt
; PERF-NEXT: bf
	%short = and i32 %count, 7
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 %short)
	ret i32 %result
}

define i32 @rotate_i8(i32 %value_arg, i32 %count_arg) {
; ASM-LABEL: rotate_i8:
	%value = trunc i32 %value_arg to i8
	%count = trunc i32 %count_arg to i8
	%rotated = call i8 @llvm.fshl.i8(i8 %value, i8 %value, i8 %count)
	%result = zext i8 %rotated to i32
	ret i32 %result
}

define i32 @rotate_i16(i32 %value_arg, i32 %count_arg) {
; ASM-LABEL: rotate_i16:
	%value = trunc i32 %value_arg to i16
	%count = trunc i32 %count_arg to i16
	%rotated = call i16 @llvm.fshr.i16(i16 %value, i16 %value, i16 %count)
	%result = zext i16 %rotated to i32
	ret i32 %result
}

define i32 @unsigned_add_both(i32 %a, i32 %b) {
; ASM-LABEL: unsigned_add_both:
; ASM: clrt
; ASM-NOT: cmp
; ASM: addc
; ASM-NEXT: movt
; ISEL-LABEL: name:            unsigned_add_both
; ISEL: CLRT implicit-def $tbit
; ISEL-NEXT: [[SUM:%[0-9]+]]:gpr = ADDC {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit, implicit $tbit
; ISEL-NEXT: [[CARRY:%[0-9]+]]:gpr = MOVT implicit $tbit
	%pair = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
	%value = extractvalue { i32, i1 } %pair, 0
	%flag = extractvalue { i32, i1 } %pair, 1
	%flag32 = zext i1 %flag to i32
	%result = add i32 %value, %flag32
	ret i32 %result
}

define i32 @unsigned_sub_both(i32 %a, i32 %b) {
; ASM-LABEL: unsigned_sub_both:
; ASM: clrt
; ASM-NOT: cmp
; ASM: subc
; ASM-NEXT: movt
; ISEL-LABEL: name:            unsigned_sub_both
; ISEL: CLRT implicit-def $tbit
; ISEL-NEXT: [[DIFFERENCE:%[0-9]+]]:gpr = SUBC {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit, implicit $tbit
; ISEL-NEXT: [[BORROW:%[0-9]+]]:gpr = MOVT implicit $tbit
	%pair = call { i32, i1 } @llvm.usub.with.overflow.i32(i32 %a, i32 %b)
	%value = extractvalue { i32, i1 } %pair, 0
	%flag = extractvalue { i32, i1 } %pair, 1
	%flag32 = zext i1 %flag to i32
	%result = add i32 %value, %flag32
	ret i32 %result
}

define i32 @signed_add_both(i32 %a, i32 %b) {
; ASM-LABEL: signed_add_both:
; ASM: addv
; ASM-NEXT: movt
; ISEL-LABEL: name:            signed_add_both
; ISEL: [[SUM:%[0-9]+]]:gpr = ADDV {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: [[OVERFLOW:%[0-9]+]]:gpr = MOVT implicit $tbit
	%pair = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
	%value = extractvalue { i32, i1 } %pair, 0
	%flag = extractvalue { i32, i1 } %pair, 1
	%flag32 = zext i1 %flag to i32
	%result = add i32 %value, %flag32
	ret i32 %result
}

define i32 @signed_sub_both(i32 %a, i32 %b) {
; ASM-LABEL: signed_sub_both:
; ASM: subv
; ASM-NEXT: movt
; ISEL-LABEL: name:            signed_sub_both
; ISEL: [[DIFFERENCE:%[0-9]+]]:gpr = SUBV {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit
; ISEL-NEXT: [[OVERFLOW:%[0-9]+]]:gpr = MOVT implicit $tbit
	%pair = call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 %a, i32 %b)
	%value = extractvalue { i32, i1 } %pair, 0
	%flag = extractvalue { i32, i1 } %pair, 1
	%flag32 = zext i1 %flag to i32
	%result = add i32 %value, %flag32
	ret i32 %result
}

define i32 @unsigned_add_flag_only(i32 %a, i32 %b) {
; ASM-LABEL: unsigned_add_flag_only:
; ASM: clrt
; ASM: addc
; ASM-NEXT: rts
; ASM-NEXT: movt
	%pair = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
	%flag = extractvalue { i32, i1 } %pair, 1
	%result = zext i1 %flag to i32
	ret i32 %result
}

define i32 @unsigned_add_result_only(i32 %a, i32 %b) {
; ASM-LABEL: unsigned_add_result_only:
; ASM-NOT: addc
; ASM: add
	%pair = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
	%result = extractvalue { i32, i1 } %pair, 0
	ret i32 %result
}

define i32 @overflow_branch(i32 %a, i32 %b) {
; ASM-LABEL: overflow_branch:
; ASM: addv
; ASM-NEXT: movt
	%pair = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
	%flag = extractvalue { i32, i1 } %pair, 1
	br i1 %flag, label %overflow, label %normal

overflow:
	ret i32 1

normal:
	ret i32 0
}

define void @overflow_bool_store(i32 %a, i32 %b, ptr %address) {
; ASM-LABEL: overflow_bool_store:
; ASM: subv
; ASM-NEXT: movt
; ASM: mov.b
	%pair = call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 %a, i32 %b)
	%flag = extractvalue { i32, i1 } %pair, 1
	store i1 %flag, ptr %address, align 1
	ret void
}

define i32 @multiple_unsigned_adds(i32 %a, i32 %b, i32 %c) {
; ASM-LABEL: multiple_unsigned_adds:
; ISEL-LABEL: name:            multiple_unsigned_adds
; ISEL: CLRT implicit-def $tbit
; ISEL-NEXT: [[FIRST_SUM:%[0-9]+]]:gpr = ADDC {{%[0-9]+}}, {{%[0-9]+}}, implicit-def $tbit, implicit $tbit
; ISEL-NEXT: [[FIRST_CARRY:%[0-9]+]]:gpr = MOVT implicit $tbit
; ISEL-NEXT: CLRT implicit-def $tbit
; ISEL-NEXT: [[SECOND_SUM:%[0-9]+]]:gpr = ADDC [[FIRST_SUM]], {{%[0-9]+}}, implicit-def $tbit, implicit $tbit
; ISEL-NEXT: [[SECOND_CARRY:%[0-9]+]]:gpr = MOVT implicit $tbit
	%first = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
	%first_value = extractvalue { i32, i1 } %first, 0
	%first_flag = extractvalue { i32, i1 } %first, 1
	%second = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %first_value, i32 %c)
	%second_value = extractvalue { i32, i1 } %second, 0
	%second_flag = extractvalue { i32, i1 } %second, 1
	%either = or i1 %first_flag, %second_flag
	%either32 = zext i1 %either to i32
	%result = add i32 %second_value, %either32
	ret i32 %result
}

define i32 @narrow_overflow(i32 %a_arg, i32 %b_arg, i32 %c_arg, i32 %d_arg) {
; ASM-LABEL: narrow_overflow:
	%a = trunc i32 %a_arg to i8
	%b = trunc i32 %b_arg to i8
	%c = trunc i32 %c_arg to i16
	%d = trunc i32 %d_arg to i16
	%add = call { i8, i1 } @llvm.uadd.with.overflow.i8(i8 %a, i8 %b)
	%add_value = extractvalue { i8, i1 } %add, 0
	%add_flag = extractvalue { i8, i1 } %add, 1
	%sub = call { i16, i1 } @llvm.ssub.with.overflow.i16(i16 %c, i16 %d)
	%sub_value = extractvalue { i16, i1 } %sub, 0
	%sub_flag = extractvalue { i16, i1 } %sub, 1
	%add32 = zext i8 %add_value to i32
	%sub32 = sext i16 %sub_value to i32
	%add_flag32 = zext i1 %add_flag to i32
	%sub_flag32 = zext i1 %sub_flag to i32
	%values = add i32 %add32, %sub32
	%flags = add i32 %add_flag32, %sub_flag32
	%result = add i32 %values, %flags
	ret i32 %result
}

define internal fastcc i32 @internal_rotate_overflow(i32 %value, i32 %count, i32 %addend) noinline {
; ASM-LABEL: internal_rotate_overflow:
; ASM: movt
	%pair = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %value, i32 %addend)
	%sum = extractvalue { i32, i1 } %pair, 0
	%flag = extractvalue { i32, i1 } %pair, 1
	%rotated = call i32 @llvm.fshl.i32(i32 %sum, i32 %sum, i32 %count)
	%flag32 = zext i1 %flag to i32
	%result = add i32 %rotated, %flag32
	ret i32 %result
}

define i32 @call_internal_rotate_overflow(i32 %value, i32 %count, i32 %addend) {
	%result = call fastcc i32 @internal_rotate_overflow(i32 %value, i32 %count, i32 %addend)
	ret i32 %result
}

define i32 @overflow_live_across_call(i32 %a, i32 %b) {
	%pair = call { i32, i1 } @llvm.usub.with.overflow.i32(i32 %a, i32 %b)
	%value = extractvalue { i32, i1 } %pair, 0
	%flag = extractvalue { i32, i1 } %pair, 1
	%called = call i32 @helper(i32 %value)
	%flag32 = zext i1 %flag to i32
	%result = add i32 %called, %flag32
	ret i32 %result
}

define i32 @rotate_live_across_call(i32 %value, i32 %count) {
	%rotated = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 %count)
	%called = call i32 @helper(i32 %value)
	%result = add i32 %called, %rotated
	ret i32 %result
}

define i32 @rotate_overflow_tls(i32 %value, i32 %count, i32 %addend) {
	%tls = load i32, ptr @tls_value, align 4
	%pair = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %value, i32 %addend)
	%sum = extractvalue { i32, i1 } %pair, 0
	%flag = extractvalue { i32, i1 } %pair, 1
	%called = call i32 @helper(i32 %tls)
	%rotated = call i32 @llvm.fshl.i32(i32 %sum, i32 %sum, i32 %count)
	%flag32 = zext i1 %flag to i32
	%with_rotate = add i32 %called, %rotated
	%result = add i32 %with_rotate, %flag32
	ret i32 %result
}

define i32 @rotate_overflow_frame_pressure(i32 %value, i32 %count, i32 %addend) noinline optnone {
	%padding = alloca [80 x i8], align 4
	%words = alloca [24 x i32], align 4
	%pair = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %value, i32 %addend)
	%sum = extractvalue { i32, i1 } %pair, 0
	%flag = extractvalue { i32, i1 } %pair, 1
	%rotated = call i32 @llvm.fshl.i32(i32 %sum, i32 %sum, i32 %count)
	%padding_far = getelementptr inbounds [80 x i8], ptr %padding, i32 0, i32 79
	store volatile i8 1, ptr %padding_far, align 1
	%far = getelementptr inbounds [24 x i32], ptr %words, i32 0, i32 23
	store volatile i32 %rotated, ptr %far, align 4
	%reloaded = load volatile i32, ptr %far, align 4
	%called = call i32 @helper(i32 %reloaded)
	%flag32 = zext i1 %flag to i32
	%result = add i32 %called, %flag32
	ret i32 %result
}

define i32 @overflow_sign_extend(i32 %a, i32 %b) {
	%pair = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
	%flag = extractvalue { i32, i1 } %pair, 1
	%result = sext i1 %flag to i32
	ret i32 %result
}

define i32 @overflow_select(i32 %a, i32 %b, i32 %when_true, i32 %when_false) {
	%pair = call { i32, i1 } @llvm.usub.with.overflow.i32(i32 %a, i32 %b)
	%flag = extractvalue { i32, i1 } %pair, 1
	%result = select i1 %flag, i32 %when_true, i32 %when_false
	ret i32 %result
}

define i32 @overflow_phi(i32 %a, i32 %b, i32 %selector) {
	%choose_add = icmp ne i32 %selector, 0
	br i1 %choose_add, label %add, label %sub

add:
	%add_pair = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
	%add_flag = extractvalue { i32, i1 } %add_pair, 1
	br label %merge

sub:
	%sub_pair = call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 %a, i32 %b)
	%sub_flag = extractvalue { i32, i1 } %sub_pair, 1
	br label %merge

merge:
	%flag = phi i1 [ %add_flag, %add ], [ %sub_flag, %sub ]
	%result = zext i1 %flag to i32
	ret i32 %result
}

define i32 @overflow_store_reload(i32 %a, i32 %b, ptr %address) {
	%pair = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
	%flag = extractvalue { i32, i1 } %pair, 1
	store volatile i1 %flag, ptr %address, align 1
	%reloaded = load volatile i1, ptr %address, align 1
	%result = zext i1 %reloaded to i32
	ret i32 %result
}

define i32 @mixed_comparison_rotate_overflow(i32 %a, i32 %b, i32 %count) {
	%before = icmp ult i32 %a, %b
	%add = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
	%sum = extractvalue { i32, i1 } %add, 0
	%carry = extractvalue { i32, i1 } %add, 1
	%rotated = call i32 @llvm.fshr.i32(i32 %sum, i32 %sum, i32 %count)
	%signed = call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 %rotated, i32 %b)
	%difference = extractvalue { i32, i1 } %signed, 0
	%signed_flag = extractvalue { i32, i1 } %signed, 1
	%after = icmp uge i32 %difference, %a
	%before32 = zext i1 %before to i32
	%carry32 = zext i1 %carry to i32
	%signed32 = zext i1 %signed_flag to i32
	%after32 = zext i1 %after to i32
	%first_flags = or i32 %before32, %carry32
	%second_flags = xor i32 %signed32, %after32
	%flags = and i32 %first_flags, %second_flags
	%result = add i32 %difference, %flags
	ret i32 %result
}
