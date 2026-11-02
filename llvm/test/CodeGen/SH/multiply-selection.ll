; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh3 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh3 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o /dev/null

declare i64 @consume_i64(i64)

define i32 @multiply_unsigned_word_mask(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_unsigned_word_mask:
; CHECK-NOT: extu
; CHECK: mulu.w
; CHECK-NEXT: rts
; CHECK-NEXT: sts	macl,
	%left = and i32 %a, 65535
	%right = and i32 %b, 65535
	%product = mul i32 %left, %right
	ret i32 %product
}

define i32 @multiply_unsigned_word_extend(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_unsigned_word_extend:
; CHECK-NOT: extu
; CHECK: mulu.w
; CHECK-NEXT: rts
; CHECK-NEXT: sts	macl,
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%left = zext i16 %a16 to i32
	%right = zext i16 %b16 to i32
	%product = mul i32 %left, %right
	ret i32 %product
}

define i32 @multiply_signed_word_extend(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_signed_word_extend:
; CHECK-NOT: exts
; CHECK: muls.w
; CHECK-NEXT: rts
; CHECK-NEXT: sts	macl,
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%left = sext i16 %a16 to i32
	%right = sext i16 %b16 to i32
	%product = mul i32 %left, %right
	ret i32 %product
}

define i32 @multiply_i16_low(ptr %left_address, ptr %right_address) {
; CHECK-LABEL: multiply_i16_low:
; CHECK: mov.w
; CHECK: mov.w
; CHECK: mulu.w
; CHECK-NEXT: sts	macl,
; CHECK: rts
	%left = load i16, ptr %left_address, align 2
	%right = load i16, ptr %right_address, align 2
	%product = mul i16 %left, %right
	%result = zext i16 %product to i32
	ret i32 %result
}

define i32 @multiply_unsigned_too_wide(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_unsigned_too_wide:
; CHECK-NOT: mulu.w
; CHECK-NOT: muls.w
; CHECK: mul.l
; CHECK-NEXT: rts
; CHECK-NEXT: sts	macl,
	%left = and i32 %a, 131071
	%right = and i32 %b, 65535
	%product = mul i32 %left, %right
	ret i32 %product
}

define i32 @multiply_mixed_word_sign(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_mixed_word_sign:
; CHECK-NOT: mulu.w
; CHECK-NOT: muls.w
; CHECK: mul.l
; CHECK-NEXT: rts
; CHECK-NEXT: sts	macl,
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%left = sext i16 %a16 to i32
	%right = zext i16 %b16 to i32
	%product = mul i32 %left, %right
	ret i32 %product
}

define i32 @multiply_signed_too_wide(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_signed_too_wide:
; CHECK-NOT: mulu.w
; CHECK-NOT: muls.w
; CHECK: mul.l
; CHECK-NEXT: rts
; CHECK-NEXT: sts	macl,
	%left_shifted = shl i32 %a, 15
	%right_shifted = shl i32 %b, 15
	%left = ashr exact i32 %left_shifted, 15
	%right = ashr exact i32 %right_shifted, 15
	%product = mul i32 %left, %right
	ret i32 %product
}

define i64 @multiply_unsigned_full(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_unsigned_full:
; CHECK: dmulu.l
; CHECK-NEXT: sts	macl,[[ULO:r[0-9]+]]
; CHECK-NEXT: rts
; CHECK-NEXT: sts	mach,[[UHI:r[0-9]+]]
	%left = zext i32 %a to i64
	%right = zext i32 %b to i64
	%product = mul i64 %left, %right
	ret i64 %product
}

define i64 @multiply_signed_full(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_signed_full:
; CHECK: dmuls.l
; CHECK-NEXT: sts	macl,[[SLO:r[0-9]+]]
; CHECK-NEXT: rts
; CHECK-NEXT: sts	mach,[[SHI:r[0-9]+]]
	%left = sext i32 %a to i64
	%right = sext i32 %b to i64
	%product = mul i64 %left, %right
	ret i64 %product
}

define i32 @multiply_unsigned_high(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_unsigned_high:
; CHECK: dmulu.l
; CHECK-NOT: sts	macl,
; CHECK-NEXT: rts
; CHECK-NEXT: sts	mach,
	%left = zext i32 %a to i64
	%right = zext i32 %b to i64
	%product = mul i64 %left, %right
	%shifted = lshr i64 %product, 32
	%high = trunc i64 %shifted to i32
	ret i32 %high
}

define i32 @multiply_signed_high(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_signed_high:
; CHECK: dmuls.l
; CHECK-NOT: sts	macl,
; CHECK-NEXT: rts
; CHECK-NEXT: sts	mach,
	%left = sext i32 %a to i64
	%right = sext i32 %b to i64
	%product = mul i64 %left, %right
	%shifted = lshr i64 %product, 32
	%high = trunc i64 %shifted to i32
	ret i32 %high
}

define i64 @multiply_adjacent(i32 %a, i32 %b, i32 %c, i32 %d) {
; CHECK-LABEL: multiply_adjacent:
; CHECK: dmuls.l
; CHECK: sts	macl,
; CHECK-NEXT: sts	mach,
; CHECK: dmulu.l
; CHECK-NEXT: sts	macl,
; CHECK: sts	mach,
; CHECK: rts
	%ua = zext i32 %a to i64
	%ub = zext i32 %b to i64
	%unsigned = mul i64 %ua, %ub
	%sc = sext i32 %c to i64
	%sd = sext i32 %d to i64
	%signed = mul i64 %sc, %sd
	%result = add i64 %unsigned, %signed
	ret i64 %result
}

define i64 @multiply_mixed_state(i32 %a, i32 %b, i32 %c, i32 %d) {
; CHECK-LABEL: multiply_mixed_state:
; CHECK: mul.l
; CHECK: sts	macl,
; CHECK: dmulu.l
; CHECK-NEXT: sts	macl,
; CHECK: sts	mach,
; CHECK: rts
	%ua = zext i32 %a to i64
	%ub = zext i32 %b to i64
	%wide = mul i64 %ua, %ub
	%low = mul i32 %c, %d
	%low64 = zext i32 %low to i64
	%result = add i64 %wide, %low64
	ret i64 %result
}

define i64 @multiply_before_call(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_before_call:
; CHECK: dmulu.l
; CHECK: sts	macl,
; CHECK: jsr
; CHECK-NEXT: sts	mach,
; CHECK: rts
	%left = zext i32 %a to i64
	%right = zext i32 %b to i64
	%product = mul i64 %left, %right
	%result = call i64 @consume_i64(i64 %product)
	ret i64 %result
}

define i64 @multiply_before_asm(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_before_asm:
; CHECK: dmuls.l
; CHECK-NEXT: sts	macl,
; CHECK-NEXT: sts	mach,
; CHECK: !APP
; CHECK: !NO_APP
; CHECK: rts
	%left = sext i32 %a to i64
	%right = sext i32 %b to i64
	%product = mul i64 %left, %right
	call void asm sideeffect "", "~{mach},~{macl}"()
	ret i64 %product
}

define i32 @multiply_unsigned_range_i16(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_unsigned_range_i16:
; CHECK: mulu.w
; CHECK: sts	macl,
; CHECK: rts
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%left = zext i16 %a16 to i32
	%right = zext i16 %b16 to i32
	%product = mul i32 %left, %right
	%low = and i32 %product, 65535
	%overflow = icmp ugt i32 %product, 65535
	%overflow32 = zext i1 %overflow to i32
	%flag = shl i32 %overflow32, 16
	%result = or i32 %low, %flag
	ret i32 %result
}

define i32 @multiply_signed_range_i16(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_signed_range_i16:
; CHECK: muls.w
; CHECK: sts	macl,
; CHECK: rts
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%left = sext i16 %a16 to i32
	%right = sext i16 %b16 to i32
	%product = mul i32 %left, %right
	%low16 = trunc i32 %product to i16
	%low = zext i16 %low16 to i32
	%extended = sext i16 %low16 to i32
	%overflow = icmp ne i32 %product, %extended
	%overflow32 = zext i1 %overflow to i32
	%flag = shl i32 %overflow32, 16
	%result = or i32 %low, %flag
	ret i32 %result
}

define i64 @multiply_general_i64(i64 %a, i64 %b) {
; CHECK-LABEL: multiply_general_i64:
; CHECK-NOT: dmulu.l
; CHECK-NOT: dmuls.l
; CHECK: mul.l
; CHECK: rts
	%product = mul i64 %a, %b
	ret i64 %product
}
