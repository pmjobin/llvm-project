; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=prolog-epilog %s -o - | FileCheck %s --check-prefix=PEI
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=prolog-epilog %s -o - | FileCheck %s --check-prefix=PEI
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=ASM

%R8 = type { i32, i32 }
%B8 = type [8 x i8]
%P5 = type <{ i8, i8, i8, i8, i8 }>

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_end.p0(ptr)
declare i32 @nested5(i32, i32, i32, i32, i32)
declare i32 @nested8(i32, i32, i32, i32, i32, i32, i32, i32)

; PEI-LABEL: name:            nonleaf_save12
; PEI: stackSize:       20
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -16, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -12, size: 12, alignment: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -12
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -4
; PEI: JSR
; PEI-NEXT: $r15 = ADDri $r15, 4
; PEI: $r15 = frame-destroy ADDri $r15, 4
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 12
;
; ASM-LABEL: nonleaf_save12:
; ASM-NEXT: add	#-12,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-4,r15
; ASM: mov.l	r7,@(16,r15)
; ASM: mov.l	r6,@(12,r15)
; ASM: mov.l	r5,@(8,r15)
; ASM: add	#-4,r15
; ASM: jsr
; ASM-NEXT: nop
; ASM-NEXT: add	#4,r15
; ASM: add	#4,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
; ASM-NEXT: rts
; ASM-NEXT: nop
define i32 @nonleaf_save12(i32 %fixed, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 %fixed, i32 3, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%sum = add i32 %called, %second
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %sum
}

; PEI-LABEL: name:            nonleaf_save8
; PEI: stackSize:       16
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -12, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -8, size: 8, alignment: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -8
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -4
; PEI: JSR
; PEI-NEXT: $r15 = ADDri $r15, 4
; PEI: $r15 = frame-destroy ADDri $r15, 4
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 8
;
; ASM-LABEL: nonleaf_save8:
; ASM-NEXT: add	#-8,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-4,r15
; ASM: mov.l	r7,@(12,r15)
; ASM: mov.l	r6,@(8,r15)
; ASM: add	#-4,r15
; ASM: jsr
; ASM-NEXT: nop
; ASM-NEXT: add	#4,r15
; ASM: add	#4,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#8,r15
; ASM-NEXT: rts
; ASM-NEXT: nop
define i32 @nonleaf_save8(i32 %a, i32 %b, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 %a, i32 %b, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%sum = add i32 %called, %second
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %sum
}

; PEI-LABEL: name:            nonleaf_save4
; PEI: stackSize:       12
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -8, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -4, size: 4, alignment: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -4
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -4
; PEI: JSR
; PEI-NEXT: $r15 = ADDri $r15, 4
; PEI: $r15 = frame-destroy ADDri $r15, 4
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 4
;
; ASM-LABEL: nonleaf_save4:
; ASM-NEXT: add	#-4,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-4,r15
; ASM: mov.l	r7,@(8,r15)
; ASM: add	#-4,r15
; ASM: jsr
; ASM-NEXT: nop
; ASM-NEXT: add	#4,r15
; ASM: add	#4,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#4,r15
; ASM-NEXT: rts
; ASM-NEXT: nop
define i32 @nonleaf_save4(i32 %a, i32 %b, i32 %c, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 %a, i32 %b, i32 %c, i32 5)
	%second = va_arg ptr %ap, i32
	%sum = add i32 %called, %second
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %sum
}

; PEI-LABEL: name:            nonleaf_save0
; PEI: stackSize:       8
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -4, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: 0, size: 4, alignment: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -4
; PEI: JSR
; PEI-NEXT: $r15 = ADDri $r15, 4
; PEI: $r15 = frame-destroy ADDri $r15, 4
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: RTS
;
; ASM-LABEL: nonleaf_save0:
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-4,r15
; ASM: add	#-4,r15
; ASM: jsr
; ASM-NEXT: nop
; ASM-NEXT: add	#4,r15
; ASM: add	#4,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: rts
; ASM-NEXT: nop
define i32 @nonleaf_save0(i32 %a, i32 %b, i32 %c, i32 %d, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 %a, i32 %b, i32 %c, i32 %d)
	%second = va_arg ptr %ap, i32
	%sum = add i32 %called, %second
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %sum
}

; PEI-LABEL: name:            leaf_save12
; PEI: stackSize:       16
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: default, offset: -12, size: 12, alignment: 4
; PEI-LABEL: body:
; PEI-NOT: STS_L_PR
; PEI-NOT: LDS_L_PR
; PEI: $r15 = frame-setup ADDri $r15, -16
; PEI: $r15 = frame-destroy ADDri $r15, 16
;
; ASM-LABEL: leaf_save12:
; ASM-NOT: sts.l
; ASM: add	#-16,r15
; ASM: mov.l	r7,@(12,r15)
; ASM: mov.l	r6,@(8,r15)
; ASM: mov.l	r0,@(4,r15)
; ASM-NOT: lds.l
; ASM: add	#16,r15
; ASM-NEXT: rts
define i32 @leaf_save12(i32 %fixed, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %value
}

; PEI-LABEL: name:            leaf_save0
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: default, offset: 0, size: 4, alignment: 4
; PEI-LABEL: body:
; PEI-NOT: STS_L_PR
; PEI-NOT: LDS_L_PR
;
; ASM-LABEL: leaf_save0:
; ASM-NOT: sts.l
; ASM-NOT: lds.l
; ASM: rts
define i32 @leaf_save0(i32 %a, i32 %b, i32 %c, i32 %d, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %value
}

; PEI-LABEL: name:            nonleaf_pressure
; PEI: stackSize:       56
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -16, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -12, size: 12, alignment: 4
; PEI: stack:
; PEI: { id: 0, name: slot, type: default, offset: -48, size: 4
; PEI: { id: 1, name: ap, type: default, offset: -52, size: 4
; PEI: { id: 2, name: '', type: spill-slot, offset: -56, size: 4
; PEI-DAG: callee-saved-register: '$r8'
; PEI-DAG: callee-saved-register: '$r9'
; PEI-DAG: callee-saved-register: '$r10'
; PEI-DAG: callee-saved-register: '$r11'
; PEI-DAG: callee-saved-register: '$r12'
; PEI-DAG: callee-saved-register: '$r13'
; PEI-DAG: callee-saved-register: '$r14'
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -12
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -40
; PEI: MOVL_store_reg killed $r0, $r15 :: (store (s32) into %stack.2)
; PEI: $r15 = ADDri $r15, -16
; PEI: JSR
; PEI-NEXT: $r15 = ADDri $r15, 16
; PEI: $r15 = frame-destroy ADDri $r15, 40
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 12
;
; ASM-LABEL: nonleaf_pressure:
; ASM-NEXT: add	#-12,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-40,r15
; ASM-DAG: mov.l	r8,@(36,r15)
; ASM-DAG: mov.l	r14,@(12,r15)
; ASM: mov.l	r7,@(52,r15)
; ASM: mov.l	r6,@(48,r15)
; ASM: mov.l	r5,@(44,r15)
; ASM: mov.l	{{.*}},@r15
; ASM: add	#-16,r15
; ASM: jsr
; ASM-NEXT: nop
; ASM-NEXT: add	#16,r15
; ASM: add	#40,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
; ASM-NEXT: rts
define i32 @nonleaf_pressure(ptr %base, ...) {
	%slot = alloca i32, align 4
	%ap = alloca ptr, align 4
	store volatile i32 99, ptr %slot, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%p0 = getelementptr i8, ptr %base, i32 0
	%p1 = getelementptr i8, ptr %base, i32 4
	%p2 = getelementptr i8, ptr %base, i32 8
	%p3 = getelementptr i8, ptr %base, i32 12
	%p4 = getelementptr i8, ptr %base, i32 16
	%p5 = getelementptr i8, ptr %base, i32 20
	%p6 = getelementptr i8, ptr %base, i32 24
	%p7 = getelementptr i8, ptr %base, i32 28
	%v0 = load volatile i32, ptr %p0, align 4
	%v1 = load volatile i32, ptr %p1, align 4
	%v2 = load volatile i32, ptr %p2, align 4
	%v3 = load volatile i32, ptr %p3, align 4
	%v4 = load volatile i32, ptr %p4, align 4
	%v5 = load volatile i32, ptr %p5, align 4
	%v6 = load volatile i32, ptr %p6, align 4
	%v7 = load volatile i32, ptr %p7, align 4
	%called = call i32 @nested8(i32 %first, i32 %v0, i32 %v1, i32 %v2, i32 %v3, i32 %v4, i32 %v5, i32 %v6)
	%second = va_arg ptr %ap, i32
	%local = load volatile i32, ptr %slot, align 4
	%s0 = add i32 %called, %second
	%s1 = add i32 %s0, %local
	%s2 = add i32 %s1, %v0
	%s3 = add i32 %s2, %v1
	%s4 = add i32 %s3, %v2
	%s5 = add i32 %s4, %v3
	%s6 = add i32 %s5, %v4
	%s7 = add i32 %s6, %v5
	%s8 = add i32 %s7, %v6
	%result = add i32 %s8, %v7
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; PEI-LABEL: name:            nonleaf_frame_limit
; PEI: stackSize:       60
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -16, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -12, size: 12, alignment: 4
; PEI: stack:
; PEI: { id: 0, name: locals, type: default, offset: -56, size: 40
; PEI: { id: 1, name: ap, type: default, offset: -60, size: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -12
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -44
; PEI: $r15 = frame-destroy ADDri $r15, 44
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 12
;
; ASM-LABEL: nonleaf_frame_limit:
; ASM-NEXT: add	#-12,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-44,r15
; ASM: add	#44,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
; ASM-NEXT: rts
define i32 @nonleaf_frame_limit(i32 %fixed, ...) {
	%locals = alloca [10 x i32], align 4
	%last = getelementptr [10 x i32], ptr %locals, i32 0, i32 9
	%ap = alloca ptr, align 4
	store volatile i32 %fixed, ptr %locals, align 4
	store volatile i32 %fixed, ptr %last, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 2, i32 3, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%local = load volatile i32, ptr %last, align 4
	%sum = add i32 %called, %second
	%result = add i32 %sum, %local
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; PEI-LABEL: name:            nonleaf_i64_return
; PEI-LABEL: body:
; PEI: JSR
; PEI: $r15 = frame-destroy LDS_L_PR $r15
; PEI: RTS implicit $pr, implicit $r0, implicit $r1
;
; ASM-LABEL: nonleaf_i64_return:
; ASM: jsr
; ASM-NEXT: nop
; ASM: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
; ASM-NEXT: rts
define i64 @nonleaf_i64_return(i32 %fixed, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 2, i32 3, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%low = zext i32 %called to i64
	%high = zext i32 %second to i64
	%shifted = shl i64 %high, 32
	%result = or i64 %shifted, %low
	call void @llvm.va_end.p0(ptr %ap)
	ret i64 %result
}

; PEI-LABEL: name:            nonleaf_aggregate_return
; PEI-LABEL: body:
; PEI: JSR
; PEI: $r1 = MOVL_load_reg
; PEI: $r15 = frame-destroy LDS_L_PR $r15
; PEI: RTS implicit $pr, implicit $r0, implicit $r1
;
; ASM-LABEL: nonleaf_aggregate_return:
; ASM: jsr
; ASM-NEXT: nop
; ASM: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
; ASM-NEXT: rts
define %R8 @nonleaf_aggregate_return(i32 %fixed, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 2, i32 3, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%part = insertvalue %R8 poison, i32 %called, 0
	%result = insertvalue %R8 %part, i32 %second, 1
	call void @llvm.va_end.p0(ptr %ap)
	ret %R8 %result
}

; PEI-LABEL: name:            nonleaf_named_byval
; PEI: stackSize:       24
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -12, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -8, size: 8, alignment: 4
; PEI: stack:
; PEI: { id: 0, name: ap, type: default, offset: -16, size: 4
; PEI: { id: 1, name: '', type: default, offset: -24, size: 8
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -8
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -12
; PEI: MOVL_store_disp killed $r5, $r15, 4 :: (store (s32) into stack + 4)
; PEI-NEXT: MOVL_store_reg killed $r4, $r15 :: (store (s32) into stack)
; PEI: JSR
; PEI: MOVB_load_frame $r15, 7
; PEI: $r15 = frame-destroy ADDri $r15, 12
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 8
;
; ASM-LABEL: nonleaf_named_byval:
; ASM-NEXT: add	#-8,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-12,r15
; ASM: mov.l	r5,@(4,r15)
; ASM-NEXT: mov.l	r4,@r15
; ASM: jsr
; ASM-NEXT: nop
; ASM: mov.b	@(7,r15),r{{[0-9]+}}
; ASM: add	#12,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#8,r15
define i32 @nonleaf_named_byval(ptr byval(%B8) align 4 %named, ...) {
	%last = getelementptr %B8, ptr %named, i32 0, i32 7
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 2, i32 3, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%byte = load i8, ptr %last, align 1
	%extended = zext i8 %byte to i32
	%sum = add i32 %called, %second
	%result = add i32 %sum, %extended
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; PEI-LABEL: name:            nonleaf_packed_temporary
; PEI: stackSize:       36
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -16, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -12, size: 12, alignment: 4
; PEI: stack:
; PEI: { id: 0, name: vaarg.tmp{{[0-9]*}}, type: default, offset: -24, size: 5, alignment: 4
; PEI: { id: 1, name: vaarg.tmp, type: default, offset: -32, size: 5, alignment: 4
; PEI: { id: 2, name: ap, type: default, offset: -36, size: 4, alignment: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -12
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -20
; PEI: JSR
; PEI: $r15 = frame-destroy ADDri $r15, 20
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 12
;
; ASM-LABEL: nonleaf_packed_temporary:
; ASM-NEXT: add	#-12,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-20,r15
; ASM: jsr
; ASM-NEXT: nop
; ASM: add	#20,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
define i32 @nonleaf_packed_temporary(i32 %fixed, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, %P5
	%first.byte = extractvalue %P5 %first, 4
	%first.extended = zext i8 %first.byte to i32
	%called = call i32 @nested5(i32 %first.extended, i32 2, i32 3, i32 4, i32 5)
	%second = va_arg ptr %ap, %P5
	%second.byte = extractvalue %P5 %second, 4
	%second.extended = zext i8 %second.byte to i32
	%result = add i32 %called, %second.extended
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; PEI-LABEL: name:            nonleaf_named_byval_split
; PEI: stackSize:       16
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -4, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: 4, size: 4, alignment: 4
; PEI: - { id: 2, type: default, offset: 0, size: 4, alignment: 4
; PEI: stack:
; PEI: { id: 0, name: ap, type: default, offset: -8, size: 4
; PEI: { id: 1, name: '', type: default, offset: -16, size: 8
; PEI-LABEL: body:
; PEI: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -12
; PEI: $r1 = MOVL_load_disp $r15, 16 :: (load (s32) from %fixed-stack.2)
; PEI-NEXT: MOVL_store_disp killed $r1, $r15, 4 :: (store (s32) into stack + 4)
; PEI-NEXT: MOVL_store_reg killed $r7, $r15 :: (store (s32) into stack)
; PEI: JSR
; PEI: MOVB_load_frame $r15, 7
; PEI: $r15 = frame-destroy ADDri $r15, 12
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: RTS
;
; ASM-LABEL: nonleaf_named_byval_split:
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-12,r15
; ASM: mov.l	@(16,r15),r{{[0-9]+}}
; ASM-NEXT: mov.l	{{.*}},@(4,r15)
; ASM-NEXT: mov.l	r7,@r15
; ASM: jsr
; ASM-NEXT: nop
; ASM: mov.b	@(7,r15),r{{[0-9]+}}
; ASM: add	#12,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: rts
define i32 @nonleaf_named_byval_split(i32 %a, i32 %b, i32 %c, ptr byval(%B8) align 4 %named, ...) {
	%last = getelementptr %B8, ptr %named, i32 0, i32 7
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 %a, i32 %b, i32 %c, i32 5)
	%second = va_arg ptr %ap, i32
	%byte = load i8, ptr %last, align 1
	%extended = zext i8 %byte to i32
	%sum = add i32 %called, %second
	%result = add i32 %sum, %extended
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}
