; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs < %s -o /dev/null

declare i8 @llvm.fshl.i8(i8, i8, i8)
declare i16 @llvm.fshr.i16(i16, i16, i16)
declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i64 @llvm.fshr.i64(i64, i64, i64)
declare { i64, i1 } @llvm.uadd.with.overflow.i64(i64, i64)
declare { i64, i1 } @llvm.ssub.with.overflow.i64(i64, i64)

define i32 @funnel_distinct(i32 %a, i32 %b, i32 %count) {
	%result = call i32 @llvm.fshl.i32(i32 %a, i32 %b, i32 %count)
	ret i32 %result
}

define i64 @funnel_i64(i64 %a, i64 %b, i64 %count) {
	%result = call i64 @llvm.fshr.i64(i64 %a, i64 %b, i64 %count)
	ret i64 %result
}

define i64 @overflow_i64(i64 %a, i64 %b, ptr %flags) {
	%add = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %a, i64 %b)
	%sub = call { i64, i1 } @llvm.ssub.with.overflow.i64(i64 %a, i64 %b)
	%add.value = extractvalue { i64, i1 } %add, 0
	%add.flag = extractvalue { i64, i1 } %add, 1
	%sub.value = extractvalue { i64, i1 } %sub, 0
	%sub.flag = extractvalue { i64, i1 } %sub, 1
	store i1 %add.flag, ptr %flags, align 1
	%other = getelementptr i8, ptr %flags, i32 1
	store i1 %sub.flag, ptr %other, align 1
	%result = xor i64 %add.value, %sub.value
	ret i64 %result
}

define i32 @narrow_ops(i32 %a, i32 %b, i32 %count) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%c8 = trunc i32 %count to i8
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%c16 = trunc i32 %count to i16
	%udiv = udiv i8 %a8, %b8
	%urem = urem i8 %a8, %b8
	%sdiv = sdiv i16 %a16, %b16
	%srem = srem i16 %a16, %b16
	%shl = shl i8 %a8, %c8
	%ashr = ashr i16 %a16, %c16
	%fshl = call i8 @llvm.fshl.i8(i8 %a8, i8 %b8, i8 %c8)
	%fshr = call i16 @llvm.fshr.i16(i16 %a16, i16 %b16, i16 %c16)
	%p0 = zext i8 %udiv to i32
	%p1 = zext i8 %urem to i32
	%p2 = sext i16 %sdiv to i32
	%p3 = sext i16 %srem to i32
	%p4 = zext i8 %shl to i32
	%p5 = sext i16 %ashr to i32
	%p6 = zext i8 %fshl to i32
	%p7 = sext i16 %fshr to i32
	%r0 = xor i32 %p0, %p1
	%r1 = xor i32 %p2, %p3
	%r2 = xor i32 %p4, %p5
	%r3 = xor i32 %p6, %p7
	%r4 = xor i32 %r0, %r1
	%r5 = xor i32 %r2, %r3
	%result = xor i32 %r4, %r5
	ret i32 %result
}
