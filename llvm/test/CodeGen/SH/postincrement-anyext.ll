; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,ENABLE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,ENABLE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,ENABLE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,ENABLE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,DISABLE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh1 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,DISABLE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,DISABLE
; RUN: llc -enable-new-pm -mtriple=shle-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=CHECK,ENABLE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs %s -o /dev/null

declare void @observe(ptr, ptr, i32)

; Only low bits of the loaded value are needed by narrow arithmetic. A signed
; load implements any extension; the final extension still belongs after add.
define i32 @byte_low(ptr %p, ptr %updated, i32 %index) {
; CHECK-LABEL: name:            byte_low
; ENABLE: early-clobber [[BYTE:%[0-9]+]]:gpr, [[NEXT:%[0-9]+]]:gpr = MOVB_load_postinc
; ENABLE: MOVL_store_reg {{(killed )?}}[[NEXT]]
; ENABLE: ADDrr [[BYTE]]
; ENABLE: EXTUB
; DISABLE-NOT: MOVB_load_postinc
	%x = load i8, ptr %p, align 1
	%i = trunc i32 %index to i8
	%y = add i8 %x, %i
	%z = zext i8 %y to i32
	%q = getelementptr i8, ptr %p, i32 1
	store ptr %q, ptr %updated, align 4
	ret i32 %z
}

define i32 @half_low(ptr %p, ptr %updated, i32 %index) {
; CHECK-LABEL: name:            half_low
; ENABLE: early-clobber {{%[0-9]+}}:gpr, {{%[0-9]+}}:gpr = MOVW_load_postinc
; ENABLE: EXTUW
; DISABLE-NOT: MOVW_load_postinc
	%x = load i16, ptr %p, align 2
	%i = trunc i32 %index to i16
	%y = add i16 %x, %i
	%z = zext i16 %y to i32
	%q = getelementptr i16, ptr %p, i32 1
	store ptr %q, ptr %updated, align 4
	ret i32 %z
}

; The old pointer and updated pointer are distinct SSA results. Register
; allocation must preserve both when passing them across a call boundary.
define void @both_bases(ptr %p, i32 %index) {
; CHECK-LABEL: name:            both_bases
; ENABLE: [[OLD:%[0-9]+]]:gpr = COPY $r4
; ENABLE: early-clobber {{%[0-9]+}}:gpr, [[NEW:%[0-9]+]]:gpr = MOVB_load_postinc [[OLD]]
; ENABLE-DAG: $r4 = COPY [[OLD]]
; ENABLE-DAG: $r5 = COPY [[NEW]]
; ENABLE: JSR
	%x = load i8, ptr %p, align 1
	%i = trunc i32 %index to i8
	%y = add i8 %x, %i
	%z = zext i8 %y to i32
	%q = getelementptr i8, ptr %p, i32 1
	call void @observe(ptr %p, ptr %q, i32 %z)
	ret void
}

define i32 @old_base_only(ptr %p, ptr %saved, i32 %index) {
; CHECK-LABEL: name:            old_base_only
; CHECK-NOT: MOVB_load_postinc
	%x = load i8, ptr %p, align 1
	%i = trunc i32 %index to i8
	%y = add i8 %x, %i
	%z = zext i8 %y to i32
	store ptr %p, ptr %saved, align 4
	ret i32 %z
}

define i32 @aliasing(ptr %p, ptr %updated, i32 %index) {
; CHECK-LABEL: name:            aliasing
; CHECK: MOVB_load_
; CHECK: MOVB_store_
; CHECK: MOVL_store_
	%x = load i8, ptr %p, align 1
	%i = trunc i32 %index to i8
	%y = add i8 %x, %i
	%z = zext i8 %y to i32
	store i8 %y, ptr %p, align 1
	%q = getelementptr i8, ptr %p, i32 1
	store ptr %q, ptr %updated, align 4
	ret i32 %z
}

define i32 @volatile_byte(ptr %p, ptr %updated, i32 %index) {
; CHECK-LABEL: name:            volatile_byte
; ENABLE: MOVB_load_postinc {{.*}} :: (volatile load (s8)
	%x = load volatile i8, ptr %p, align 1
	%i = trunc i32 %index to i8
	%y = add i8 %x, %i
	%z = zext i8 %y to i32
	%q = getelementptr i8, ptr %p, i32 1
	store ptr %q, ptr %updated, align 4
	ret i32 %z
}

define i32 @atomic_byte(ptr %p, ptr %updated, i32 %index) {
; CHECK-LABEL: name:            atomic_byte
; CHECK-NOT: MOVB_load_postinc
	%x = load atomic i8, ptr %p monotonic, align 1
	%i = trunc i32 %index to i8
	%y = add i8 %x, %i
	%z = zext i8 %y to i32
	%q = getelementptr i8, ptr %p, i32 1
	store ptr %q, ptr %updated, align 4
	ret i32 %z
}

define i32 @asm_barrier(ptr %p, ptr %updated, i32 %index) {
; CHECK-LABEL: name:            asm_barrier
; CHECK: MOVB_load_
; CHECK: INLINEASM
; CHECK: MOVL_store_
	%x = load i8, ptr %p, align 1
	call void asm sideeffect "", "~{memory},~{r0},~{t}"()
	%i = trunc i32 %index to i8
	%y = add i8 %x, %i
	%z = zext i8 %y to i32
	%q = getelementptr i8, ptr %p, i32 1
	store ptr %q, ptr %updated, align 4
	ret i32 %z
}

define i32 @size_byte(ptr %p, ptr %updated, i32 %index) optsize {
; CHECK-LABEL: name:            size_byte
; CHECK-NOT: MOVB_load_postinc
	%x = load i8, ptr %p, align 1
	%i = trunc i32 %index to i8
	%y = add i8 %x, %i
	%z = zext i8 %y to i32
	%q = getelementptr i8, ptr %p, i32 1
	store ptr %q, ptr %updated, align 4
	ret i32 %z
}

define i32 @minsize_byte(ptr %p, ptr %updated, i32 %index) minsize {
; CHECK-LABEL: name:            minsize_byte
; CHECK-NOT: MOVB_load_postinc
	%x = load i8, ptr %p, align 1
	%i = trunc i32 %index to i8
	%y = add i8 %x, %i
	%z = zext i8 %y to i32
	%q = getelementptr i8, ptr %p, i32 1
	store ptr %q, ptr %updated, align 4
	ret i32 %z
}

define i32 @optnone_byte(ptr %p, ptr %updated, i32 %index) noinline optnone {
; CHECK-LABEL: name:            optnone_byte
; CHECK-NOT: MOVB_load_postinc
	%x = load i8, ptr %p, align 1
	%i = trunc i32 %index to i8
	%y = add i8 %x, %i
	%z = zext i8 %y to i32
	%q = getelementptr i8, ptr %p, i32 1
	store ptr %q, ptr %updated, align 4
	ret i32 %z
}
