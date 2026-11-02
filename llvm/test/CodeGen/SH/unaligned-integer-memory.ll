; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR

@two_bytes = external global [2 x i8], align 1
@four_bytes = external global [4 x i8], align 1
@eight_bytes = external global [8 x i8], align 1

; COMMON-LABEL: load_i16_a1:
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON-COUNT-2: mov.b
; COMMON-NOT: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
define i32 @load_i16_a1(ptr %p) {
	%value = load i16, ptr %p, align 1
	%result = zext i16 %value to i32
	ret i32 %result
}

; The high byte is sign-extended by mov.b; only the low byte is zero-extended.
; COMMON-LABEL: load_s16_a1:
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: mov.b
; COMMON: mov.b
; COMMON-NOT: mov.b
; COMMON: rts
define i32 @load_s16_a1(ptr %p) {
	%value = load i16, ptr %p, align 1
	%result = sext i16 %value to i32
	ret i32 %result
}

; COMMON-LABEL: store_i16_a1:
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: mov.b
; COMMON-NOT: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
; COMMON-NEXT: mov.b	{{r[0-9]+}},@r4
define void @store_i16_a1(ptr %p, i32 %value) {
	%narrow = trunc i32 %value to i16
	store i16 %narrow, ptr %p, align 1
	ret void
}

; COMMON-LABEL: load_i32_a1:
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON-COUNT-4: mov.b
; COMMON-NOT: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
; MIR-LABEL: name: load_i32_a1
; MIR-DAG: MOVB_load_reg {{.*}} :: (load (s8) from %ir.p)
; MIR-DAG: MOVB_load_reg {{.*}} :: (load (s8) from %ir.p + 1)
; MIR-DAG: MOVB_load_reg {{.*}} :: (load (s8) from %ir.p + 2)
; MIR-DAG: MOVB_load_reg {{.*}} :: (load (s8) from %ir.p + 3)
define i32 @load_i32_a1(ptr %p) {
	%value = load i32, ptr %p, align 1
	ret i32 %value
}

; COMMON-LABEL: load_i32_a2:
; COMMON-NOT: mov.b
; COMMON-NOT: mov.l
; COMMON-COUNT-2: mov.w
; COMMON-NOT: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
define i32 @load_i32_a2(ptr %p) {
	%value = load i32, ptr %p, align 2
	ret i32 %value
}

; COMMON-LABEL: store_i32_a1:
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON-COUNT-3: mov.b
; COMMON-NOT: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
; COMMON-NEXT: mov.b	{{r[0-9]+}},@r4
; MIR-LABEL: name: store_i32_a1
; MIR-DAG: MOVB_store_reg {{.*}} :: (store (s8) into %ir.p)
; MIR-DAG: MOVB_store_reg {{.*}} :: (store (s8) into %ir.p + 1)
; MIR-DAG: MOVB_store_reg {{.*}} :: (store (s8) into %ir.p + 2)
; MIR-DAG: MOVB_store_reg {{.*}} :: (store (s8) into %ir.p + 3)
define void @store_i32_a1(ptr %p, i32 %value) {
	store i32 %value, ptr %p, align 1
	ret void
}

; COMMON-LABEL: store_i32_a2:
; COMMON-NOT: mov.b
; COMMON-NOT: mov.l
; COMMON-COUNT-1: mov.w
; COMMON-NOT: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
; COMMON-NEXT: mov.w	{{r[0-9]+}},@r4
define void @store_i32_a2(ptr %p, i32 %value) {
	store i32 %value, ptr %p, align 2
	ret void
}

; COMMON-LABEL: load_i64_a1:
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON-COUNT-8: mov.b
; COMMON-NOT: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
define i64 @load_i64_a1(ptr %p) {
	%value = load i64, ptr %p, align 1
	ret i64 %value
}

; COMMON-LABEL: load_i64_a2:
; COMMON-NOT: mov.b
; COMMON-NOT: mov.l
; COMMON-COUNT-4: mov.w
; COMMON-NOT: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
define i64 @load_i64_a2(ptr %p) {
	%value = load i64, ptr %p, align 2
	ret i64 %value
}

; COMMON-LABEL: store_i64_a1:
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON-COUNT-7: mov.b
; COMMON-NOT: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
; COMMON-NEXT: mov.b	{{r[0-9]+}},@r4
define void @store_i64_a1(ptr %p, i64 %value) {
	store i64 %value, ptr %p, align 1
	ret void
}

; COMMON-LABEL: store_i64_a2:
; COMMON-NOT: mov.b
; COMMON-NOT: mov.l
; COMMON-COUNT-3: mov.w
; COMMON-NOT: mov.b
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
; COMMON-NEXT: mov.w	{{r[0-9]+}},@r4
define void @store_i64_a2(ptr %p, i64 %value) {
	store i64 %value, ptr %p, align 2
	ret void
}

; BE-LABEL: store_i16_pattern:
; BE: mov #18,[[B0:r[0-9]+]]
; BE-NEXT: mov.b [[B0]],@r4
; BE-NEXT: add #1,r4
; BE-NEXT: mov #52,[[B1:r[0-9]+]]
; BE-NEXT: mov.b [[B1]],@r4
; LE-LABEL: store_i16_pattern:
; LE: mov #52,[[B0:r[0-9]+]]
; LE-NEXT: mov.b [[B0]],@r4
; LE-NEXT: add #1,r4
; LE-NEXT: mov #18,[[B1:r[0-9]+]]
; LE-NEXT: mov.b [[B1]],@r4
define void @store_i16_pattern(ptr %p) {
	store volatile i16 4660, ptr %p, align 1
	ret void
}

; BE-LABEL: store_i32_pattern:
; BE: mov #18,[[B0:r[0-9]+]]
; BE-NEXT: mov.b [[B0]],@[[P0:r[0-9]+]]
; BE: mov #52,[[B1:r[0-9]+]]
; BE-NEXT: mov.b [[B1]],@[[P1:r[0-9]+]]
; BE: mov #86,[[B2:r[0-9]+]]
; BE-NEXT: mov.b [[B2]],@[[P2:r[0-9]+]]
; BE: mov #120,[[B3:r[0-9]+]]
; BE-NEXT: mov.b [[B3]],@[[P3:r[0-9]+]]
; LE-LABEL: store_i32_pattern:
; LE: mov #120,[[B0:r[0-9]+]]
; LE-NEXT: mov.b [[B0]],@[[P0:r[0-9]+]]
; LE: mov #86,[[B1:r[0-9]+]]
; LE-NEXT: mov.b [[B1]],@[[P1:r[0-9]+]]
; LE: mov #52,[[B2:r[0-9]+]]
; LE-NEXT: mov.b [[B2]],@[[P2:r[0-9]+]]
; LE: mov #18,[[B3:r[0-9]+]]
; LE-NEXT: mov.b [[B3]],@[[P3:r[0-9]+]]
define void @store_i32_pattern(ptr %p) {
	store volatile i32 305419896, ptr %p, align 1
	ret void
}

; COMMON-LABEL: store_i16_zero:
; COMMON: mov #0,[[ZERO:r[0-9]+]]
; COMMON-COUNT-2: mov.b [[ZERO]],
define void @store_i16_zero(ptr %p) {
	store volatile i16 0, ptr %p, align 1
	ret void
}

; COMMON-LABEL: store_i32_ones:
; COMMON: mov #-1,[[ONES:r[0-9]+]]
; COMMON-COUNT-4: mov.b [[ONES]],
define void @store_i32_ones(ptr %p) {
	store volatile i32 -1, ptr %p, align 2
	ret void
}

; COMMON-LABEL: store_i16_high_bit:
; COMMON-COUNT-2: mov.b
; COMMON: rts
define void @store_i16_high_bit(ptr %p) {
	store volatile i16 -32768, ptr %p, align 1
	ret void
}

; COMMON-LABEL: load_i16_boundary:
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l @
; COMMON-COUNT-2: mov.b
; COMMON: rts
define i32 @load_i16_boundary() {
	%value = load i16, ptr @two_bytes, align 1
	%result = zext i16 %value to i32
	ret i32 %result
}

; COMMON-LABEL: load_i32_boundary:
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l @
; COMMON-COUNT-4: mov.b
; COMMON: rts
define i32 @load_i32_boundary() {
	%value = load i32, ptr @four_bytes, align 1
	ret i32 %value
}

; COMMON-LABEL: load_i64_boundary:
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l @
; COMMON-COUNT-8: mov.b
; COMMON: rts
define i64 @load_i64_boundary() {
	%value = load i64, ptr @eight_bytes, align 1
	ret i64 %value
}

; COMMON-LABEL: volatile_i16_rmw:
; COMMON-COUNT-2: mov.b @
; COMMON-COUNT-2: mov.b {{r[0-9]+}},@
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
; MIR-LABEL: name: volatile_i16_rmw
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p)
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p + 1)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p + 1)
define i32 @volatile_i16_rmw(ptr %p) {
	%value = load volatile i16, ptr %p, align 1
	%wide = zext i16 %value to i32
	%next = add i32 %wide, 1
	%narrow = trunc i32 %next to i16
	store volatile i16 %narrow, ptr %p, align 1
	ret i32 %next
}

; COMMON-LABEL: volatile_i32_rmw:
; COMMON-COUNT-4: mov.b @
; COMMON-COUNT-4: mov.b {{r[0-9]+}},@
; COMMON-NOT: mov.w
; COMMON-NOT: mov.l
; COMMON: rts
; MIR-LABEL: name: volatile_i32_rmw
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p)
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p + 1)
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p + 2)
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p + 3)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p + 1)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p + 2)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p + 3)
define i32 @volatile_i32_rmw(ptr %p) {
	%value = load volatile i32, ptr %p, align 1
	%next = add i32 %value, 1
	store volatile i32 %next, ptr %p, align 1
	ret i32 %next
}

; COMMON-LABEL: volatile_i64_rmw:
; COMMON-COUNT-8: mov.b @
; COMMON-COUNT-8: mov.b {{r[0-9]+}},@
; COMMON-NOT: mov.w
; COMMON: rts
; MIR-LABEL: name: volatile_i64_rmw
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p)
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p + 1)
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p + 2)
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p + 3)
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p + 4)
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p + 5)
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p + 6)
; MIR: MOVB_load_reg {{.*}} :: (volatile load (s8) from %ir.p + 7)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p + 1)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p + 2)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p + 3)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p + 4)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p + 5)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p + 6)
; MIR: MOVB_store_reg {{.*}} :: (volatile store (s8) into %ir.p + 7)
define i64 @volatile_i64_rmw(ptr %p) {
	%value = load volatile i64, ptr %p, align 1
	%next = add i64 %value, 1
	store volatile i64 %next, ptr %p, align 1
	ret i64 %next
}

; MIR-LABEL: name: metadata_i32
; MIR-COUNT-4: MOVB_load_reg {{.*}} :: (load (s8) from %ir.src{{.*}}!tbaa {{![0-9]+}}, !alias.scope {{![0-9]+}})
; MIR-COUNT-4: MOVB_store_reg {{.*}} :: (store (s8) into %ir.dst{{.*}}!tbaa {{![0-9]+}}, !noalias {{![0-9]+}})
define void @metadata_i32(ptr %dst, ptr %src) {
	%value = load i32, ptr %src, align 1, !tbaa !3, !alias.scope !4
	store i32 %value, ptr %dst, align 1, !tbaa !3, !noalias !4
	ret void
}

; COMMON-LABEL: load_i16_aligned:
; COMMON: mov.w @r4,r0
define i32 @load_i16_aligned(ptr %p) {
	%value = load i16, ptr %p, align 2
	%result = zext i16 %value to i32
	ret i32 %result
}

; COMMON-LABEL: load_i32_aligned:
; COMMON: mov.l @r4,r0
define i32 @load_i32_aligned(ptr %p) {
	%value = load i32, ptr %p, align 4
	ret i32 %value
}

; COMMON-LABEL: load_i64_aligned:
; COMMON-COUNT-2: mov.l
define i64 @load_i64_aligned(ptr %p) {
	%value = load i64, ptr %p, align 4
	ret i64 %value
}

; COMMON-LABEL: store_i16_aligned:
; COMMON: mov.w r5,@r4
define void @store_i16_aligned(ptr %p, i32 %value) {
	%narrow = trunc i32 %value to i16
	store i16 %narrow, ptr %p, align 2
	ret void
}

; COMMON-LABEL: store_i32_aligned:
; COMMON: mov.l r5,@r4
define void @store_i32_aligned(ptr %p, i32 %value) {
	store i32 %value, ptr %p, align 4
	ret void
}

; COMMON-LABEL: store_i64_aligned:
; COMMON-COUNT-2: mov.l
define void @store_i64_aligned(ptr %p, i64 %value) {
	store i64 %value, ptr %p, align 4
	ret void
}

!0 = !{!"Simple C/C++ TBAA"}
!1 = !{!"omnipotent char", !0, i64 0}
!2 = !{!"int", !1, i64 0}
!3 = !{!2, !2, i64 0}
!4 = !{!5}
!5 = distinct !{!5, !6, !"scope"}
!6 = distinct !{!6, i1 false, !"domain"}
