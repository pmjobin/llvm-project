// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=BIG
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=LITTLE
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -Os -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -Os -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -mllvm -stop-after=prolog-epilog -o - %s | FileCheck %s --check-prefixes=ABI,FRAG
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -mllvm -stop-after=prolog-epilog -o - %s | FileCheck %s --check-prefixes=ABI,FRAG
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -mllvm -stop-after=prolog-epilog -o - %s | FileCheck %s --check-prefix=ABI
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -mllvm -stop-after=prolog-epilog -o - %s | FileCheck %s --check-prefix=ABI

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct __attribute__((packed)) Inner3 {
	u8 tag;
	u16 value;
};

struct __attribute__((packed)) EightPacked {
	struct Inner3 inner;
	u32 word;
	u8 tail;
};

// The split aggregate's second word and the following scalar remain at entry
// SP+0 and SP+4. Native aggregate memory lowering needs one additional spill
// on big endian and saves r10 on little endian at O0. The resulting frames are
// 68 and 72 bytes: 60 local bytes plus two or three four-byte callee saves.
// Keep the exact allocations, incoming offsets, and balanced restores checked.
// IR: %struct.EightPacked = type <{ %struct.Inner3, i32, i8 }>
// IR-NEXT: %struct.Inner3 = type <{ i8, i16 }>
// IR-LABEL: define{{.*}} i32 @boundary_packed_min(i32{{.*}}, i32{{.*}}, i32{{.*}}, %struct.EightPacked %{{.*}}, i32{{.*}})
// BIG-LABEL: boundary_packed_min:
// BIG: mov.l	r8,@-r15
// BIG-NEXT: mov.l	r9,@-r15
// BIG-NEXT: add	#-60,r15
// BIG: mov	#68,r0
// BIG-NEXT: mov.l	@(r0,r15),r1
// BIG: mov	r7,
// BIG: mov	#72,r0
// BIG-NEXT: mov.l	@(r0,r15),r0
// BIG: add	#60,r15
// BIG-NEXT: mov.l	@r15+,r9
// BIG-NEXT: rts
// BIG-NEXT: mov.l	@r15+,r8
// LITTLE-LABEL: boundary_packed_min:
// LITTLE: mov.l	r8,@-r15
// LITTLE-NEXT: mov.l	r9,@-r15
// LITTLE-NEXT: mov.l	r10,@-r15
// LITTLE-NEXT: add	#-60,r15
// LITTLE-NEXT: mov	r7,
// LITTLE: mov	#72,r0
// LITTLE-NEXT: mov.l	@(r0,r15),r2
// LITTLE: mov	#76,r0
// LITTLE-NEXT: mov.l	@(r0,r15),r0
// LITTLE: add	#60,r15
// LITTLE-NEXT: mov.l	@r15+,r10
// LITTLE-NEXT: mov.l	@r15+,r9
// LITTLE-NEXT: rts
// LITTLE-NEXT: mov.l	@r15+,r8
u32 boundary_packed_min(u32 a, u32 b, u32 c, struct EightPacked value, u32 tail) {
	return a + b + c + value.inner.value + value.word + value.tail + tail;
}

struct __attribute__((packed)) Packed5 { u8 value[5]; };
struct __attribute__((packed)) Packed6 { u8 value[6]; };
struct __attribute__((packed)) Packed7 { u8 value[7]; };
struct Natural8 { u32 first; u32 second; };

u32 boundary5(u32 a, u32 b, u32 c, struct Packed5 value, u32 tail) {
	return a + b + c + value.value[0] + value.value[4] + tail;
}

u32 boundary6(u32 a, u32 b, u32 c, struct Packed6 value, u32 tail) {
	return a + b + c + value.value[0] + value.value[5] + tail;
}

u32 boundary7(u32 a, u32 b, u32 c, struct Packed7 value, u32 tail) {
	return a + b + c + value.value[0] + value.value[6] + tail;
}

u32 boundary8(u32 a, u32 b, u32 c, struct Natural8 value, u32 tail) {
	return a + b + c + value.first + value.second + tail;
}

u32 packed8_earlier(u32 a, struct EightPacked value, u32 tail) {
	return a + value.inner.value + value.word + value.tail + tail;
}

u32 packed8_stack(u32 a, u32 b, u32 c, u32 d, struct EightPacked value, u32 tail) {
	return a + b + c + d + value.inner.value + value.word + value.tail + tail;
}

u32 consecutive_boundary(u32 a, u32 b, u32 c, struct Packed5 first, struct Packed7 second, u32 tail) {
	return a + b + c + first.value[0] + first.value[4] + second.value[0] + second.value[6] + tail;
}

extern u32 nested_callee(u32, u32, u32, struct EightPacked, u32);

u32 boundary_with_call(u32 a, u32 b, u32 c, struct EightPacked value, u32 tail) {
	volatile u8 local[80];
	local[0] = value.inner.tag;
	local[79] = value.tail;
	return nested_callee(a + local[0], b, c, value, tail + local[79]);
}

// Check the ABI coordinates independently of local allocation and register
// pressure, before final frame references lose their fixed-stack provenance.
// A short trailing aggregate fragment owns only its actual bytes, even though
// the next argument starts at the next four-byte boundary. The O0 fragment
// checks also reject a widened access into that padding or the following value.
// ABI-LABEL: name: boundary_packed_min
// ABI: fixedStack:
// ABI-NEXT: - { id: 0, type: default, offset: 4, size: 4, alignment: 4,
// ABI: - { id: 1, type: default, offset: 0, size: 4, alignment: 4,
// ABI-NOT: - { id:
// ABI: stack:

// ABI-LABEL: name: boundary5
// ABI: fixedStack:
// ABI-NEXT: - { id: 0, type: default, offset: 4, size: 4, alignment: 4,
// ABI: - { id: 1, type: default, offset: 0, size: 1, alignment: 4,
// ABI-NOT: - { id:
// ABI: stack:
// FRAG: body:
// FRAG-NOT: (load (s32) from %fixed-stack.1
// FRAG-NOT: (load (s16) from %fixed-stack.1
// FRAG: (load (s8) from %fixed-stack.1, align 4)
// FRAG-NOT: (load (s32) from %fixed-stack.1
// FRAG-NOT: (load (s16) from %fixed-stack.1

// ABI-LABEL: name: boundary6
// ABI: fixedStack:
// ABI-NEXT: - { id: 0, type: default, offset: 4, size: 4, alignment: 4,
// ABI: - { id: 1, type: default, offset: 0, size: 2, alignment: 4,
// ABI-NOT: - { id:
// ABI: stack:
// FRAG: body:
// FRAG-NOT: (load (s32) from %fixed-stack.1
// FRAG: (load (s16) from %fixed-stack.1, align 4)
// FRAG-NOT: (load (s32) from %fixed-stack.1

// ABI-LABEL: name: boundary7
// ABI: fixedStack:
// ABI-NEXT: - { id: 0, type: default, offset: 4, size: 4, alignment: 4,
// ABI: - { id: 1, type: default, offset: 0, size: 3, alignment: 4,
// ABI-NOT: - { id:
// ABI: stack:
// FRAG: body:
// FRAG-NOT: (load (s32) from %fixed-stack.1
// FRAG: (load (s16) from %fixed-stack.1, align 4)
// FRAG-NOT: (load (s32) from %fixed-stack.1
// FRAG: (load (s8) from %fixed-stack.1 + 2, align 2)
// FRAG-NOT: (load (s32) from %fixed-stack.1

// ABI-LABEL: name: boundary8
// ABI: fixedStack:
// ABI-NEXT: - { id: 0, type: default, offset: 4, size: 4, alignment: 4,
// ABI: - { id: 1, type: default, offset: 0, size: 4, alignment: 4,
// ABI-NOT: - { id:
// ABI: stack:

// ABI-LABEL: name: packed8_earlier
// ABI: fixedStack: []
// ABI-NEXT: stack:

// ABI-LABEL: name: packed8_stack
// ABI: fixedStack:
// ABI-NEXT: - { id: 0, type: default, offset: 8, size: 4, alignment: 4,
// ABI: - { id: 1, type: default, offset: 4, size: 4, alignment: 4,
// ABI: - { id: 2, type: default, offset: 0, size: 4, alignment: 4,
// ABI-NOT: - { id:
// ABI: stack:

// ABI-LABEL: name: consecutive_boundary
// ABI: fixedStack:
// ABI-NEXT: - { id: 0, type: default, offset: 12, size: 4, alignment: 4,
// ABI: - { id: 1, type: default, offset: 8, size: 3, alignment: 4,
// ABI: - { id: 2, type: default, offset: 4, size: 4, alignment: 4,
// ABI: - { id: 3, type: default, offset: 0, size: 1, alignment: 4,
// ABI-NOT: - { id:
// ABI: stack:

// ABI-LABEL: name: boundary_with_call
// ABI: fixedStack:
// ABI-NEXT: - { id: 0, type: spill-slot, offset: -4, size: 4, alignment: 4,
// ABI: - { id: 1, type: default, offset: 4, size: 4, alignment: 4,
// ABI: - { id: 2, type: default, offset: 0, size: 4, alignment: 4,
// ABI-NOT: - { id:
// ABI: stack:
