; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s

declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)

define i32 @variable_shl(i32 %value, i32 %count) {
; CHECK-LABEL: variable_shl:
; CHECK: mov	r4,[[DST:r[0-9]+]]
; CHECK-NEXT: rts
; CHECK-NEXT: shld	r5,[[DST]]
; CHECK-NOT: shll
	%result = shl i32 %value, %count
	ret i32 %result
}

define i32 @variable_lshr(i32 %value, i32 %count) {
; CHECK-LABEL: variable_lshr:
; CHECK: mov	r4,[[DST:r[0-9]+]]
; CHECK-NEXT: neg	r5,[[NEGATED:r[0-9]+]]
; CHECK-NEXT: rts
; CHECK-NEXT: shld	[[NEGATED]],[[DST]]
; CHECK-NOT: shlr
	%result = lshr i32 %value, %count
	ret i32 %result
}

define i32 @variable_ashr(i32 %value, i32 %count) {
; CHECK-LABEL: variable_ashr:
; CHECK: mov	r4,[[DST:r[0-9]+]]
; CHECK-NEXT: neg	r5,[[NEGATED:r[0-9]+]]
; CHECK-NEXT: rts
; CHECK-NEXT: shad	[[NEGATED]],[[DST]]
; CHECK-NOT: shar
	%result = ashr i32 %value, %count
	ret i32 %result
}

define i32 @constant_shl_7(i32 %value) {
; CHECK-LABEL: constant_shl_7:
; CHECK-DAG: mov	#7,[[COUNT:r[0-9]+]]
; CHECK-DAG: mov	r4,[[DST:r[0-9]+]]
; CHECK: rts
; CHECK-NEXT: shld	[[COUNT]],[[DST]]
	%result = shl i32 %value, 7
	ret i32 %result
}

define i32 @constant_lshr_19(i32 %value) {
; CHECK-LABEL: constant_lshr_19:
; CHECK-DAG: mov	#-19,[[COUNT:r[0-9]+]]
; CHECK-DAG: mov	r4,[[DST:r[0-9]+]]
; CHECK: rts
; CHECK-NEXT: shld	[[COUNT]],[[DST]]
	%result = lshr i32 %value, 19
	ret i32 %result
}

define i32 @constant_ashr_7(i32 %value) {
; CHECK-LABEL: constant_ashr_7:
; CHECK-DAG: mov	#-7,[[COUNT:r[0-9]+]]
; CHECK-DAG: mov	r4,[[DST:r[0-9]+]]
; CHECK: rts
; CHECK-NEXT: shad	[[COUNT]],[[DST]]
	%result = ashr i32 %value, 7
	ret i32 %result
}

; Two static shifts remain no larger than materializing a dynamic count.

define i32 @constant_shl_4(i32 %value) {
; CHECK-LABEL: constant_shl_4:
; CHECK: mov	r4,[[DST:r[0-9]+]]
; CHECK-NEXT: shll2	[[DST]]
; CHECK-NEXT: rts
; CHECK-NEXT: shll2	[[DST]]
; CHECK-NOT: shld
	%result = shl i32 %value, 4
	ret i32 %result
}

define i32 @variable_rotl(i32 %value, i32 %count) {
; CHECK-LABEL: variable_rotl:
; CHECK: mov	#31,[[COUNT:r[0-9]+]]
; CHECK: and
; CHECK-DAG: add	#-32,
; CHECK-DAG: shld
; CHECK-DAG: shld
; CHECK: or
; CHECK-NOT: {{^ *rotl }}
; CHECK-NOT: {{^ *rotr }}
; CHECK-NOT: {{^ *dt }}
; CHECK-NOT: {{^ *bf }}
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 %count)
	ret i32 %result
}

define i32 @variable_rotr(i32 %value, i32 %count) {
; CHECK-LABEL: variable_rotr:
; CHECK: mov	#31,[[COUNT:r[0-9]+]]
; CHECK: and
; CHECK: neg
; CHECK-DAG: add	#32,
; CHECK-DAG: shld
; CHECK-DAG: shld
; CHECK: or
; CHECK-NOT: {{^ *rotl }}
; CHECK-NOT: {{^ *rotr }}
; CHECK-NOT: {{^ *dt }}
; CHECK-NOT: {{^ *bf }}
	%result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 %count)
	ret i32 %result
}
