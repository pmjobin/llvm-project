; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=atomic-expand < %s | FileCheck %s --check-prefix=EXPAND
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ABI
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --relocations --symbols %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --relocations --symbols %t.le.o | FileCheck %s --check-prefix=OBJECT

@global8 = global i8 0, align 1
@global16 = global i16 0, align 2

; EXPAND-LABEL: define i32 @load_store_orderings
; EXPAND: call i8 @__atomic_load_1(ptr %p8, i32 0)
; EXPAND: call i8 @__atomic_load_1(ptr %p8, i32 0)
; EXPAND: call i8 @__atomic_load_1(ptr %p8, i32 2)
; EXPAND: call i8 @__atomic_load_1(ptr %p8, i32 5)
; EXPAND: call void @__atomic_store_2(ptr %p16, i16 %value, i32 0)
; EXPAND: call void @__atomic_store_2(ptr %p16, i16 %value, i32 0)
; EXPAND: call void @__atomic_store_2(ptr %p16, i16 %value, i32 3)
; EXPAND: call void @__atomic_store_2(ptr %p16, i16 %value, i32 5)
define i32 @load_store_orderings(ptr %p8, ptr %p16, i32 %input) {
	%value = trunc i32 %input to i16
	%a = load atomic i8, ptr %p8 unordered, align 1
	%b = load atomic i8, ptr %p8 monotonic, align 1
	%c = load atomic i8, ptr %p8 acquire, align 1
	%d = load atomic i8, ptr %p8 seq_cst, align 1
	store atomic i16 %value, ptr %p16 unordered, align 2
	store atomic i16 %value, ptr %p16 monotonic, align 2
	store atomic i16 %value, ptr %p16 release, align 2
	store atomic i16 %value, ptr %p16 seq_cst, align 2
	%ab = xor i8 %a, %b
	%cd = xor i8 %c, %d
	%result8 = xor i8 %ab, %cd
	%result = zext i8 %result8 to i32
	ret i32 %result
}

; EXPAND-LABEL: define i32 @exchange_orderings
; EXPAND: call i16 @__atomic_exchange_2(ptr %p, i16 %value, i32 0)
; EXPAND: call i16 @__atomic_exchange_2(ptr %p, i16 %value, i32 2)
; EXPAND: call i16 @__atomic_exchange_2(ptr %p, i16 %value, i32 3)
; EXPAND: call i16 @__atomic_exchange_2(ptr %p, i16 %value, i32 4)
; EXPAND: call i16 @__atomic_exchange_2(ptr %p, i16 %value, i32 5)
define i32 @exchange_orderings(ptr %p, i32 %input) {
	%value = trunc i32 %input to i16
	%a = atomicrmw xchg ptr %p, i16 %value monotonic, align 2
	%b = atomicrmw xchg ptr %p, i16 %value acquire, align 2
	%c = atomicrmw xchg ptr %p, i16 %value release, align 2
	%d = atomicrmw xchg ptr %p, i16 %value acq_rel, align 2
	%e = atomicrmw xchg ptr %p, i16 %value seq_cst, align 2
	%ab = xor i16 %a, %b
	%cd = xor i16 %c, %d
	%abcd = xor i16 %ab, %cd
	%result16 = xor i16 %abcd, %e
	%result = zext i16 %result16 to i32
	ret i32 %result
}

; EXPAND-LABEL: define i32 @cmpxchg_orderings
; EXPAND: call zeroext i1 @__atomic_compare_exchange_1(ptr %p, ptr {{.*}}, i8 %desired, i32 0, i32 0)
; EXPAND: call zeroext i1 @__atomic_compare_exchange_1(ptr %p, ptr {{.*}}, i8 %desired, i32 2, i32 2)
; EXPAND: call zeroext i1 @__atomic_compare_exchange_1(ptr %p, ptr {{.*}}, i8 %desired, i32 3, i32 0)
; EXPAND: call zeroext i1 @__atomic_compare_exchange_1(ptr %p, ptr {{.*}}, i8 %desired, i32 4, i32 2)
; EXPAND: call zeroext i1 @__atomic_compare_exchange_1(ptr %p, ptr {{.*}}, i8 %desired, i32 5, i32 5)
define i32 @cmpxchg_orderings(ptr %p, i32 %expected.input, i32 %desired.input) {
	%expected = trunc i32 %expected.input to i8
	%desired = trunc i32 %desired.input to i8
	%a = cmpxchg ptr %p, i8 %expected, i8 %desired monotonic monotonic, align 1
	%b = cmpxchg weak ptr %p, i8 %expected, i8 %desired acquire acquire, align 1
	%c = cmpxchg ptr %p, i8 %expected, i8 %desired release monotonic, align 1
	%d = cmpxchg weak ptr %p, i8 %expected, i8 %desired acq_rel acquire, align 1
	%e = cmpxchg ptr %p, i8 %expected, i8 %desired seq_cst seq_cst, align 1
	%av = extractvalue { i8, i1 } %a, 0
	%bv = extractvalue { i8, i1 } %b, 0
	%cv = extractvalue { i8, i1 } %c, 0
	%dv = extractvalue { i8, i1 } %d, 0
	%ev = extractvalue { i8, i1 } %e, 0
	%ab = xor i8 %av, %bv
	%cd = xor i8 %cv, %dv
	%abcd = xor i8 %ab, %cd
	%result8 = xor i8 %abcd, %ev
	%result = zext i8 %result8 to i32
	ret i32 %result
}

; EXPAND-LABEL: define i32 @rmw_i8
; EXPAND: call i8 @__atomic_fetch_add_1
; EXPAND: call i8 @__atomic_fetch_sub_1
; EXPAND: call i8 @__atomic_fetch_and_1
; EXPAND: call i8 @__atomic_fetch_or_1
; EXPAND: call i8 @__atomic_fetch_xor_1
; EXPAND: call i8 @__atomic_fetch_nand_1
define i32 @rmw_i8(ptr %p, i32 %input) {
	%value = trunc i32 %input to i8
	%a = atomicrmw add ptr %p, i8 %value monotonic, align 1
	%b = atomicrmw sub ptr %p, i8 %value acquire, align 1
	%c = atomicrmw and ptr %p, i8 %value release, align 1
	%d = atomicrmw or ptr %p, i8 %value acq_rel, align 1
	%e = atomicrmw xor ptr %p, i8 %value seq_cst, align 1
	%f = atomicrmw nand ptr %p, i8 %value monotonic, align 1
	%ab = xor i8 %a, %b
	%cd = xor i8 %c, %d
	%ef = xor i8 %e, %f
	%abcd = xor i8 %ab, %cd
	%result8 = xor i8 %abcd, %ef
	%result = zext i8 %result8 to i32
	ret i32 %result
}

; EXPAND-LABEL: define i32 @rmw_i16_minmax
; EXPAND: call i16 @__atomic_load_2
; EXPAND: call zeroext i1 @__atomic_compare_exchange_2
; EXPAND: call i16 @__atomic_load_2
; EXPAND: call zeroext i1 @__atomic_compare_exchange_2
; EXPAND: call i16 @__atomic_load_2
; EXPAND: call zeroext i1 @__atomic_compare_exchange_2
; EXPAND: call i16 @__atomic_load_2
; EXPAND: call zeroext i1 @__atomic_compare_exchange_2
define i32 @rmw_i16_minmax(ptr %p, i32 %input) {
	%value = trunc i32 %input to i16
	%a = atomicrmw min ptr %p, i16 %value seq_cst, align 2
	%b = atomicrmw max ptr %p, i16 %value seq_cst, align 2
	%c = atomicrmw umin ptr %p, i16 %value seq_cst, align 2
	%d = atomicrmw umax ptr %p, i16 %value seq_cst, align 2
	%ab = xor i16 %a, %b
	%cd = xor i16 %c, %d
	%result16 = xor i16 %ab, %cd
	%result = zext i16 %result16 to i32
	ret i32 %result
}

; EXPAND-LABEL: define i32 @misaligned_i16
; EXPAND: call void @__atomic_load(i32 2, ptr %p, ptr {{.*}}, i32 2)
; EXPAND: call void @__atomic_store(i32 2, ptr %p, ptr {{.*}}, i32 3)
; EXPAND: call void @__atomic_exchange(i32 2, ptr %p, ptr {{.*}}, ptr {{.*}}, i32 4)
; EXPAND: call zeroext i1 @__atomic_compare_exchange(i32 2, ptr %p, ptr {{.*}}, ptr {{.*}}, i32 5, i32 2)
; EXPAND: call void @__atomic_load(i32 2, ptr %p, ptr {{.*}}, i32 0)
; EXPAND: call zeroext i1 @__atomic_compare_exchange(i32 2, ptr %p, ptr {{.*}}, ptr {{.*}}, i32 5, i32 5)
define i32 @misaligned_i16(ptr %p, i32 %input) {
	%value = trunc i32 %input to i16
	%a = load atomic i16, ptr %p acquire, align 1
	store atomic i16 %value, ptr %p release, align 1
	%b = atomicrmw xchg ptr %p, i16 %value acq_rel, align 1
	%c = cmpxchg ptr %p, i16 %a, i16 %value seq_cst acquire, align 1
	%d = extractvalue { i16, i1 } %c, 0
	%e = atomicrmw add ptr %p, i16 %value seq_cst, align 1
	%ab = xor i16 %a, %b
	%cd = xor i16 %d, %e
	%result16 = xor i16 %ab, %cd
	%result = zext i16 %result16 to i32
	ret i32 %result
}

; EXPAND-LABEL: define i32 @stack_atomic_i8
; EXPAND: call void @__atomic_store_1
; EXPAND: call i8 @__atomic_load_1
; ABI-LABEL: stack_atomic_i8:
; ABI: jsr	@{{r[0-9]+}}
; ABI: mov.l	.LCPI{{[0-9_]+}},[[LOAD:r[0-9]+]]
; ABI-NEXT: mov	#2,r5
; ABI-NEXT: jsr	@[[LOAD]]
; ABI-NEXT: mov	r8,r4
; ABI: extu.b	r0,r0
define i32 @stack_atomic_i8(i32 %input) {
	%object = alloca i8, align 1
	%value = trunc i32 %input to i8
	store atomic i8 %value, ptr %object release, align 1
	%loaded = load atomic i8, ptr %object acquire, align 1
	%result = zext i8 %loaded to i32
	ret i32 %result
}

; EXPAND-LABEL: define i32 @select_atomic_i16
; EXPAND: call i16 @__atomic_exchange_2
define i32 @select_atomic_i16(ptr %local, i32 %condition.input, i32 %input) {
	%condition = icmp ne i32 %condition.input, 0
	%object = select i1 %condition, ptr %local, ptr @global16
	%value = trunc i32 %input to i16
	%old = atomicrmw xchg ptr %object, i16 %value seq_cst, align 2
	%result = zext i16 %old to i32
	ret i32 %result
}

; EXPAND-LABEL: define i32 @phi_atomic_i8
; EXPAND: call zeroext i1 @__atomic_compare_exchange_1
define i32 @phi_atomic_i8(ptr %local, i32 %condition.input, i32 %input) {
entry:
	%condition = icmp ne i32 %condition.input, 0
	br i1 %condition, label %use.local, label %use.global

use.local:
	br label %join

use.global:
	br label %join

join:
	%object = phi ptr [ %local, %use.local ], [ @global8, %use.global ]
	%value = trunc i32 %input to i8
	%pair = cmpxchg ptr %object, i8 0, i8 %value acq_rel acquire, align 1
	%old = extractvalue { i8, i1 } %pair, 0
	%result = zext i8 %old to i32
	ret i32 %result
}

; ASM: .long	__atomic_load_1
; ASM: .long	__atomic_store_2
; ASM: .long	__atomic_exchange_2
; ASM: .long	__atomic_compare_exchange_1
; ASM: .long	__atomic_fetch_add_1
; ASM: .long	__atomic_fetch_sub_1
; ASM: .long	__atomic_fetch_and_1
; ASM: .long	__atomic_fetch_or_1
; ASM: .long	__atomic_fetch_xor_1
; ASM: .long	__atomic_fetch_nand_1
; ASM: .long	__atomic_load_2
; ASM: .long	__atomic_compare_exchange_2
; ASM: .long	__atomic_load
; ASM: .long	__atomic_store
; ASM: .long	__atomic_exchange
; ASM: .long	__atomic_compare_exchange
; ASM: .long	__atomic_store_1
; ASM-NOT: tas.b

; OBJECT: R_SH_PLT32 __atomic_load_1
; OBJECT: R_SH_PLT32 __atomic_store_2
; OBJECT: R_SH_PLT32 __atomic_compare_exchange_1
; OBJECT: R_SH_PLT32 __atomic_compare_exchange_2
; OBJECT: R_SH_PLT32 __atomic_compare_exchange
; OBJECT-NOT: R_SH_DIR32 __atomic
