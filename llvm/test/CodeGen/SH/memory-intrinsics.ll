; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=SIZE

declare void @llvm.memcpy.p0.p0.i32(ptr, ptr, i32, i1 immarg)
declare void @llvm.memmove.p0.p0.i32(ptr, ptr, i32, i1 immarg)
declare void @llvm.memset.p0.i32(ptr, i8, i32, i1 immarg)

; CHECK-LABEL: memcpy12_align4:
; CHECK: mov.l	@r5,
; CHECK: mov.l	{{.*}},@r4
; CHECK: mov.l	@(4,r5),
; CHECK: mov.l	{{.*}},@(4,r4)
; CHECK: mov.l	@(8,r5),
; CHECK-NOT: jsr
; CHECK: rts
; CHECK-NEXT: mov.l	{{.*}},@(8,r4)
define void @memcpy12_align4(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i32(ptr align 4 %dst, ptr align 4 %src, i32 12, i1 false)
	ret void
}

; MIR-LABEL: name:            memcpy12_align4
; MIR: MOVL_load_reg
; MIR-NEXT: MOVL_store_reg
; MIR: MOVL_load_disp {{.*}}, 4
; MIR-NEXT: MOVL_store_disp {{.*}}, 4
; MIR: MOVL_load_disp {{.*}}, 8
; MIR-NEXT: MOVL_store_disp {{.*}}, 8
; MIR-NOT: JSR
; MIR: RTS
; MIR: RTS

; CHECK-LABEL: memcpy7_align2:
; CHECK-COUNT-6: mov.w
; CHECK-COUNT-2: mov.b
; CHECK-NOT: jsr
define void @memcpy7_align2(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i32(ptr align 2 %dst, ptr align 2 %src, i32 7, i1 false)
	ret void
}

; CHECK-LABEL: memcpy5_align1:
; CHECK-COUNT-10: mov.b
; CHECK-NOT: jsr
; CHECK: rts
define void @memcpy5_align1(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 %dst, ptr align 1 %src, i32 5, i1 false)
	ret void
}

; SIZE-LABEL: size_memcpy4_align1:
; SIZE-NOT: jsr
; SIZE: rts
define void @size_memcpy4_align1(ptr %dst, ptr %src) #0 {
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 %dst, ptr align 1 %src, i32 4, i1 false)
	ret void
}

; SIZE-LABEL: size_memcpy5_align1:
; SIZE: jsr	@{{r[0-9]+}}
; SIZE: rts
define void @size_memcpy5_align1(ptr %dst, ptr %src) #0 {
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 %dst, ptr align 1 %src, i32 5, i1 false)
	ret void
}

; All source words precede every store, making the fixed-size move overlap
; safe.
;
; CHECK-LABEL: memmove16_align4:
; CHECK: mov.l	@(4,r5),
; CHECK: mov.l	@(8,r5),
; CHECK: mov.l	@(12,r5),
; CHECK: mov.l	@r5,
; CHECK: mov.l	{{.*}},@r4
; CHECK: mov.l	{{.*}},@(12,r4)
; CHECK: mov.l	{{.*}},@(8,r4)
; CHECK-NOT: jsr
; CHECK: rts
; CHECK-NEXT: mov.l	{{.*}},@(4,r4)
define void @memmove16_align4(ptr %dst, ptr %src) {
	call void @llvm.memmove.p0.p0.i32(ptr align 4 %dst, ptr align 4 %src, i32 16, i1 false)
	ret void
}

; MIR-LABEL: name:            memmove16_align4
; MIR: MOVL_load_disp
; MIR-NEXT: MOVL_load_disp
; MIR-NEXT: MOVL_load_disp
; MIR-NEXT: {{.*}}MOVL_load_reg
; MIR-NEXT: MOVL_store_reg
; MIR-NEXT: MOVL_store_disp
; MIR-NEXT: MOVL_store_disp
; MIR-NEXT: MOVL_store_disp
; MIR-NOT: JSR
; MIR: RTS

; CHECK-LABEL: memset12_align4:
; CHECK-COUNT-2: mov.l	{{.*}},@
; CHECK-NOT: jsr
; CHECK: rts
; CHECK-NEXT: mov.l	{{.*}},@
define void @memset12_align4(ptr %dst, i32 %input) {
	%value = trunc i32 %input to i8
	call void @llvm.memset.p0.i32(ptr align 4 %dst, i8 %value, i32 12, i1 false)
	ret void
}

; CHECK-LABEL: memset7_align2:
; CHECK: mov.b
; CHECK-COUNT-3: mov.w
; CHECK-NOT: jsr
define void @memset7_align2(ptr %dst, i32 %input) {
	%value = trunc i32 %input to i8
	call void @llvm.memset.p0.i32(ptr align 2 %dst, i8 %value, i32 7, i1 false)
	ret void
}

; CHECK-LABEL: memset5_align1:
; CHECK-COUNT-5: mov.b
; CHECK-NOT: jsr
define void @memset5_align1(ptr %dst, i32 %input) {
	%value = trunc i32 %input to i8
	call void @llvm.memset.p0.i32(ptr align 1 %dst, i8 %value, i32 5, i1 false)
	ret void
}

; CHECK-LABEL: zero_length:
; CHECK-NOT: jsr
; CHECK: .size zero_length
define void @zero_length(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i32(ptr align 4 %dst, ptr align 4 %src, i32 0, i1 false)
	call void @llvm.memmove.p0.p0.i32(ptr align 4 %dst, ptr align 4 %src, i32 0, i1 false)
	call void @llvm.memset.p0.i32(ptr align 4 %dst, i8 1, i32 0, i1 false)
	ret void
}

; CHECK-LABEL: maximum_memcpy:
; CHECK: .long memcpy
define void @maximum_memcpy(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 %dst, ptr align 1 %src, i32 60, i1 false)
	ret void
}

; CHECK-LABEL: maximum_memset:
; CHECK: .long memset
define void @maximum_memset(ptr %dst, i32 %input) {
	%value = trunc i32 %input to i8
	call void @llvm.memset.p0.i32(ptr align 4 %dst, i8 %value, i32 60, i1 false)
	ret void
}

; CHECK-LABEL: volatile_inline:
; CHECK-NOT: jsr
; CHECK: .size volatile_inline
define void @volatile_inline(ptr %dst, ptr %src, i32 %input) {
	%value = trunc i32 %input to i8
	call void @llvm.memcpy.p0.p0.i32(ptr align 4 %dst, ptr align 4 %src, i32 12, i1 true)
	call void @llvm.memmove.p0.p0.i32(ptr align 2 %dst, ptr align 2 %src, i32 16, i1 true)
	call void @llvm.memset.p0.i32(ptr align 1 %dst, i8 %value, i32 15, i1 true)
	ret void
}

attributes #0 = { optsize }

; MIR-LABEL: name:            volatile_inline
; MIR: volatile load
; MIR: volatile store
; MIR-NOT: JSR
; MIR: RTS
