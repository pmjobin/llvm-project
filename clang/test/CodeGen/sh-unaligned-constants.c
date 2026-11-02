// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -S -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -S -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -O0 -S -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -O0 -S -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -O0 -pic-level 2 -S -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -O0 -pic-level 2 -S -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -debug-info-kind=standalone -dwarf-version=4 -S -o - %s | FileCheck %s --check-prefix=DEBUG
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -debug-info-kind=standalone -dwarf-version=4 -S -o - %s | FileCheck %s --check-prefix=DEBUG
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -S -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O1 -S -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -O1 -S -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -O1 -S -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O2 -S -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O2 -S -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -O2 -S -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -O2 -S -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -Os -S -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -Os -S -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -Os -S -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -Os -S -o /dev/null %s

// DEBUG:      .section .debug_info
// DEBUG:      .ualong
// DEBUG:      .uaword

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

struct __attribute__((packed)) P16 {
	u8 tag;
	u16 value;
};

struct __attribute__((packed)) P32 {
	u8 tag;
	u32 value;
};

struct __attribute__((packed)) P64 {
	u8 tag;
	u64 value;
};

struct __attribute__((packed)) Nested {
	u8 prefix;
	struct P16 inner;
	u32 tail;
};

struct __attribute__((packed)) Floating {
	u8 tag;
	float value;
	u8 middle;
	double wide;
};

struct __attribute__((packed)) PointerPacked {
	u8 tag;
	const int *object;
	u8 middle;
	void (*function)(void);
	u8 suffix;
	const char *addend;
	u8 tail;
};

struct __attribute__((packed)) Inner {
	u8 tag;
	u16 value;
};

struct __attribute__((packed)) PackedEight {
	struct Inner inner;
	u32 word;
	u8 tail;
};

extern void consume_packed(struct PackedEight value);

void pass_packed_constant(void) {
	struct PackedEight value = {{9, 0x1234}, 0x10203040, 10};
	consume_packed(value);
}

extern int external_object;
extern void external_function(void);
static const char anchor[] = "anchor";

const struct P16 p16 = {0xa1, 0x1234};
struct P32 writable_p32 = {0xa2, 0x12345678};
const struct P64 p64 = {0xa3, 0x0123456789abcdefULL};
const struct Nested nested = {0xa4, {0xa5, 0x5678}, 0x10203040};
const struct P32 packed_array[] = {{1, 0}, {2, 1}, {3, 0xffffffff}};
const struct Floating floating = {0xa6, -0.0f, 0xa7, -1.0};
const struct PointerPacked pointers = {0xa8, &external_object, 0xa9, external_function, 0xaa, anchor + 3, 0xab};
_Thread_local struct P32 tls_p32 = {0xac, 0x0badcafe};

const void *local_static_address(void) {
	static const struct P64 local_static = {0xad, 0x8000000000000000ULL};
	return &local_static;
}

// CHECK-LABEL: .L__const.pass_packed_constant.value:
// CHECK-NEXT: .byte 9
// CHECK-NEXT: .uaword 4660
// CHECK-NEXT: .ualong 270544960
// CHECK-NEXT: .byte 10
// CHECK-NEXT: .size .L__const.pass_packed_constant.value, 8

// CHECK-LABEL: p16:
// CHECK-NEXT: .byte 161
// CHECK-NEXT: .uaword 4660
// CHECK-NEXT: .size p16, 3

// CHECK-LABEL: writable_p32:
// CHECK-NEXT: .byte 162
// CHECK-NEXT: .ualong 305419896
// CHECK-NEXT: .size writable_p32, 5

// CHECK-LABEL: p64:
// CHECK-NEXT: .byte 163
// CHECK-NEXT: .uaquad 81985529216486895
// CHECK-NEXT: .size p64, 9

// CHECK-LABEL: nested:
// CHECK-NEXT: .byte 164
// CHECK-NEXT: .byte 165
// CHECK-NEXT: .short 22136
// CHECK-NEXT: .long 270544960
// CHECK-NEXT: .size nested, 8

// CHECK-LABEL: packed_array:
// CHECK-NEXT: .byte 1
// CHECK-NEXT: .ualong 0
// CHECK-NEXT: .byte 2
// CHECK-NEXT: .ualong 1
// CHECK-NEXT: .byte 3
// CHECK-NEXT: .ualong 4294967295
// CHECK-NEXT: .size packed_array, 15

// CHECK-LABEL: floating:
// CHECK-NEXT: .byte 166
// CHECK-NEXT: .ualong 2147483648
// CHECK-NEXT: .byte 167
// CHECK-NEXT: .uaquad -4616189618054758400
// CHECK-NEXT: .size floating, 14

// CHECK-LABEL: pointers:
// CHECK-NEXT: .byte 168
// CHECK-NEXT: .ualong external_object
// CHECK-NEXT: .byte 169
// CHECK-NEXT: .ualong external_function
// CHECK-NEXT: .byte 170
// CHECK-NEXT: .ualong anchor+3
// CHECK-NEXT: .byte 171
// CHECK-NEXT: .size pointers, 16

// CHECK-LABEL: tls_p32:
// CHECK-NEXT: .byte 172
// CHECK-NEXT: .ualong 195939070
// CHECK-NEXT: .size tls_p32, 5

// CHECK-LABEL: local_static_address.local_static:
// CHECK-NEXT: .byte 173
// CHECK-NEXT: .uaquad -9223372036854775808
// CHECK-NEXT: .size local_static_address.local_static, 9
