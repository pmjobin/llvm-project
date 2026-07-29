; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

define i32 @constant_zero() nounwind {
; ISEL-LABEL: name:            constant_zero
; ISEL: MOVri 0
; ASM-LABEL: constant_zero:
; ASM-NEXT: mov	#0,r0
	ret i32 0
}

define i32 @constant_one() nounwind {
; ISEL-LABEL: name:            constant_one
; ISEL: MOVri 1
; ASM-LABEL: constant_one:
; ASM-NEXT: mov	#1,r0
	ret i32 1
}

define i32 @constant_127() nounwind {
; ISEL-LABEL: name:            constant_127
; ISEL: MOVri 127
; ASM-LABEL: constant_127:
; ASM-NEXT: mov	#127,r0
	ret i32 127
}

define i32 @constant_128() nounwind {
; ISEL-LABEL: name:            constant_128
; ISEL: [[SIGNED:%[0-9]+]]:gpr = MOVri -128
; ISEL-NEXT: [[VALUE:%[0-9]+]]:gpr = EXTUB [[SIGNED]]
; ASM-LABEL: constant_128:
; ASM-NEXT: mov	#-128,r0
; ASM-NEXT: extu.b	r0,r0
	ret i32 128
}

define i32 @constant_255() nounwind {
; ISEL-LABEL: name:            constant_255
; ISEL: [[SIGNED:%[0-9]+]]:gpr = MOVri -1
; ISEL-NEXT: [[VALUE:%[0-9]+]]:gpr = EXTUB [[SIGNED]]
; ASM-LABEL: constant_255:
; ASM-NEXT: mov	#-1,r0
; ASM-NEXT: extu.b	r0,r0
	ret i32 255
}

define i32 @constant_256() nounwind {
; ISEL-LABEL: name:            constant_256
; ISEL: [[HIGH:%[0-9]+]]:gpr = MOVri 1
; ISEL-NEXT: [[VALUE:%[0-9]+]]:gpr = SHLL8 [[HIGH]]
; ASM-LABEL: constant_256:
; ASM-NEXT: mov	#1,r0
; ASM-NEXT: shll8	r0
	ret i32 256
}

define i32 @constant_32767() nounwind {
; ISEL-LABEL: name:            constant_32767
; ISEL: [[HIGH:%[0-9]+]]:gpr = MOVri 127
; ISEL-NEXT: [[SHIFTED:%[0-9]+]]:gpr = SHLL8 [[HIGH]]
; ISEL-NEXT: [[SIGNED:%[0-9]+]]:gpr = MOVri -1
; ISEL-NEXT: [[LOW:%[0-9]+]]:gpr = EXTUB [[SIGNED]]
; ISEL-NEXT: [[VALUE:%[0-9]+]]:gpr = ORrr [[SHIFTED]], [[LOW]]
; ASM-LABEL: constant_32767:
	ret i32 32767
}

define i32 @constant_32768() nounwind {
; ISEL-LABEL: name:            constant_32768
; ISEL: MOVri -128
; ISEL: EXTUB
; ISEL: SHLL8
; ASM-LABEL: constant_32768:
	ret i32 32768
}

define i32 @constant_65535() nounwind {
; ISEL-LABEL: name:            constant_65535
; ISEL: MOVri -1
; ISEL: EXTUB
; ISEL: SHLL8
; ISEL: MOVri -1
; ISEL: EXTUB
; ISEL: ORrr
; ASM-LABEL: constant_65535:
	ret i32 65535
}

define i32 @constant_minus_one() nounwind {
; ISEL-LABEL: name:            constant_minus_one
; ISEL: MOVri -1
; ASM-LABEL: constant_minus_one:
; ASM-NEXT: mov	#-1,r0
	ret i32 -1
}

define i32 @constant_minus_128() nounwind {
; ISEL-LABEL: name:            constant_minus_128
; ISEL: MOVri -128
; ASM-LABEL: constant_minus_128:
; ASM-NEXT: mov	#-128,r0
	ret i32 -128
}

define i32 @constant_minus_129() nounwind {
; ISEL-LABEL: name:            constant_minus_129
; ISEL: MOVri -1
; ISEL: EXTUB
; ISEL: SHLL8
; ISEL: ORrr
; ISEL: SHLL8
; ISEL: ORrr
; ISEL: SHLL8
; ISEL: ORrr
; ASM-LABEL: constant_minus_129:
	ret i32 -129
}

define i32 @constant_12345678() nounwind {
; ISEL-LABEL: name:            constant_12345678
; ISEL: MOVri 18
; ISEL: SHLL8
; ISEL: MOVri 52
; ISEL: ORrr
; ISEL: SHLL8
; ISEL: MOVri 86
; ISEL: ORrr
; ISEL: SHLL8
; ISEL: MOVri 120
; ISEL: ORrr
; ASM-LABEL: constant_12345678:
	ret i32 305419896
}

define i32 @constant_80000000() nounwind {
; ISEL-LABEL: name:            constant_80000000
; ISEL: MOVri -128
; ISEL: EXTUB
; ISEL-COUNT-3: SHLL8
; ASM-LABEL: constant_80000000:
	ret i32 -2147483648
}

define i32 @constant_deadbeef() nounwind {
; ISEL-LABEL: name:            constant_deadbeef
; ISEL: MOVri -34
; ISEL: EXTUB
; ISEL: SHLL8
; ISEL: MOVri -83
; ISEL: EXTUB
; ISEL: ORrr
; ISEL: SHLL8
; ISEL: MOVri -66
; ISEL: EXTUB
; ISEL: ORrr
; ISEL: SHLL8
; ISEL: MOVri -17
; ISEL: EXTUB
; ISEL: ORrr
; ASM-LABEL: constant_deadbeef:
	ret i32 -559038737
}

define i32 @constant_ffffffff() nounwind {
; ISEL-LABEL: name:            constant_ffffffff
; ISEL: MOVri -1
; ISEL-NOT: EXTUB
; ISEL-NOT: SHLL8
; ISEL-NOT: ORrr
; ASM-LABEL: constant_ffffffff:
	ret i32 -1
}

define i32 @constant_alu(i32 %value) nounwind {
; ISEL-LABEL: name:            constant_alu
; ISEL: MOVri 18
; ISEL: ORrr
; ISEL: ADDrr
; ASM-LABEL: constant_alu:
; ASM: add	{{r[0-9]+}},{{r[0-9]+}}
	%result = add i32 %value, 305419896
	ret i32 %result
}

define void @constant_store(ptr %address) nounwind {
; ISEL-LABEL: name:            constant_store
; ISEL: MOVri -34
; ISEL: MOVL_store_reg
; ASM-LABEL: constant_store:
; ASM: mov.l	{{r[0-9]+}},@r4
	store i32 -559038737, ptr %address, align 4
	ret void
}

define i32 @constant_compare(i32 %value) nounwind {
; ISEL-LABEL: name:            constant_compare
; ISEL: MOVri 18
; ISEL: CMP_EQ
; ASM-LABEL: constant_compare:
; ASM: cmp/eq
	%condition = icmp eq i32 %value, 305419896
	br i1 %condition, label %equal, label %different
different:
	ret i32 %value
equal:
	ret i32 1
}

define internal i32 @consume_constant(i32 %value) noinline nounwind {
	ret i32 %value
}

define i32 @constant_direct_call() nounwind {
; ISEL-LABEL: name:            constant_direct_call
; ISEL: MOVri -34
; ISEL: $r4 = COPY
; ISEL: BSR
; ASM-LABEL: constant_direct_call:
; ASM: mov	#-34,{{r[0-9]+}}
; ASM: bsr	consume_constant
	%result = call i32 @consume_constant(i32 -559038737)
	ret i32 %result
}

define i32 @constant_indirect_call(ptr %callee) nounwind {
; ISEL-LABEL: name:            constant_indirect_call
; ISEL: MOVri -34
; ISEL: $r4 = COPY
; ISEL: JSR
; ASM-LABEL: constant_indirect_call:
; ASM: mov	#-34,{{r[0-9]+}}
; ASM: jsr	@
	%result = call i32 %callee(i32 -559038737)
	ret i32 %result
}

define i32 @constant_pointer_offset(ptr %base) nounwind {
; ISEL-LABEL: name:            constant_pointer_offset
; ISEL: MOVri 18
; ISEL: ADDrr
; ISEL: MOVL_load_reg
; ASM-LABEL: constant_pointer_offset:
; ASM: add	{{r[0-9]+}},{{r[0-9]+}}
; ASM: mov.l	@{{r[0-9]+}},r0
	%address = getelementptr i8, ptr %base, i32 305419896
	%value = load i32, ptr %address, align 4
	ret i32 %value
}
