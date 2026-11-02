; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SOFT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SOFT
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -verify-machineinstrs -stop-after=greedy < %s | FileCheck %s --check-prefix=PRESSURE

; Word-aligned offsets at either end of the signed byte range use an index.
define void @indexed_offsets(ptr %source, ptr %destination) nounwind {
; CHECK-LABEL: indexed_offsets:
; CHECK: mov #124,r0
; SOFT: mov r0,[[INDEX:r[0-9]+]]
; SOFT: lds r0,fpscr
; SOFT-NEXT: mov [[INDEX]],r0
; CHECK: fmov @(r0,r4),[[VALUE:fr[0-9]+]]
; CHECK-DAG: fneg [[VALUE]]
; CHECK-DAG: mov #-128,r0
; CHECK: fmov [[VALUE]],@(r0,r5)
	%in = getelementptr i8, ptr %source, i32 124
	%out = getelementptr i8, ptr %destination, i32 -128
	%value = load float, ptr %in, align 4
	%negated = fneg float %value
	store float %negated, ptr %out, align 4
	ret void
}

; The root may be an arbitrary computed register value.
define void @computed_root(i32 %encoded, i32 %key, ptr %destination) nounwind {
; CHECK-LABEL: computed_root:
; CHECK-DAG: xor r5,[[ROOT:r[0-9]+]]
; CHECK-DAG: mov #12,r0
; SOFT: mov r0,[[COMPUTED_INDEX:r[0-9]+]]
; SOFT: lds r0,fpscr
; SOFT-NEXT: mov [[COMPUTED_INDEX]],r0
; CHECK: fmov @(r0,[[ROOT]]),[[COMPUTED:fr[0-9]+]]
; CHECK: fneg [[COMPUTED]]
; CHECK: fmov [[COMPUTED]],@r6
	%decoded = xor i32 %encoded, %key
	%base = inttoptr i32 %decoded to ptr
	%address = getelementptr i8, ptr %base, i32 12
	%value = load float, ptr %address, align 4
	%negated = fneg float %value
	store float %negated, ptr %destination, align 4
	ret void
}

declare void @clobber()

; Preserve the root across the call; recreate the architectural index after it.
define float @call_crossing(ptr %base) nounwind optsize {
; CHECK-LABEL: call_crossing:
; SOFT: lds r0,fpscr
; CHECK: mov #8,r0
; CHECK: fmov @(r0,[[SAVED:r[0-9]+]]),
; CHECK: jsr
; SOFT: lds r0,fpscr
; CHECK: mov #8,r0
; CHECK: fmov @(r0,[[SAVED]]),
	%address = getelementptr i8, ptr %base, i32 8
	%first = load volatile float, ptr %address, align 4
	call void @clobber()
	%second = load volatile float, ptr %address, align 4
	%result = fadd float %first, %second
	ret float %result
}

; Sixteen adjusted addresses used in a loop must not become sixteen live GPR
; values. This checks the CodeGenPrepare/selection boundary as well as spills.
define float @shared_root_loop(ptr %base, i32 %count) nounwind optsize {
; CHECK-LABEL: shared_root_loop:
; CHECK: fmov @(r0,r4),
; PRESSURE-LABEL: name: shared_root_loop
; PRESSURE-NOT: %stack.
; PRESSURE: FMOV_S_LOAD_INDEXED
; PRESSURE-NOT: %stack.
	%p1 = getelementptr float, ptr %base, i32 1
	%p2 = getelementptr float, ptr %base, i32 2
	%p3 = getelementptr float, ptr %base, i32 3
	%p4 = getelementptr float, ptr %base, i32 4
	%p5 = getelementptr float, ptr %base, i32 5
	%p6 = getelementptr float, ptr %base, i32 6
	%p7 = getelementptr float, ptr %base, i32 7
	%p8 = getelementptr float, ptr %base, i32 8
	%p9 = getelementptr float, ptr %base, i32 9
	%p10 = getelementptr float, ptr %base, i32 10
	%p11 = getelementptr float, ptr %base, i32 11
	%p12 = getelementptr float, ptr %base, i32 12
	%p13 = getelementptr float, ptr %base, i32 13
	%p14 = getelementptr float, ptr %base, i32 14
	%p15 = getelementptr float, ptr %base, i32 15
	%p16 = getelementptr float, ptr %base, i32 16
	br label %loop

loop:
	%remaining = phi i32 [ %count, %0 ], [ %next, %loop ]
	%v1 = load volatile float, ptr %p1, align 4
	%v2 = load volatile float, ptr %p2, align 4
	%s2 = fadd float %v1, %v2
	%v3 = load volatile float, ptr %p3, align 4
	%s3 = fadd float %s2, %v3
	%v4 = load volatile float, ptr %p4, align 4
	%s4 = fadd float %s3, %v4
	%v5 = load volatile float, ptr %p5, align 4
	%s5 = fadd float %s4, %v5
	%v6 = load volatile float, ptr %p6, align 4
	%s6 = fadd float %s5, %v6
	%v7 = load volatile float, ptr %p7, align 4
	%s7 = fadd float %s6, %v7
	%v8 = load volatile float, ptr %p8, align 4
	%s8 = fadd float %s7, %v8
	%v9 = load volatile float, ptr %p9, align 4
	%s9 = fadd float %s8, %v9
	%v10 = load volatile float, ptr %p10, align 4
	%s10 = fadd float %s9, %v10
	%v11 = load volatile float, ptr %p11, align 4
	%s11 = fadd float %s10, %v11
	%v12 = load volatile float, ptr %p12, align 4
	%s12 = fadd float %s11, %v12
	%v13 = load volatile float, ptr %p13, align 4
	%s13 = fadd float %s12, %v13
	%v14 = load volatile float, ptr %p14, align 4
	%s14 = fadd float %s13, %v14
	%v15 = load volatile float, ptr %p15, align 4
	%s15 = fadd float %s14, %v15
	%v16 = load volatile float, ptr %p16, align 4
	%sum = fadd float %s15, %v16
	%next = sub i32 %remaining, 1
	%more = icmp ne i32 %next, 0
	br i1 %more, label %loop, label %exit

exit:
	ret float %sum
}
