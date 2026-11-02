// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -S -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -S -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -O0 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -O0 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O1 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -O1 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -O1 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O2 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O2 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -O2 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -O2 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -Os -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -Os -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -Os -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -Os -emit-obj -o /dev/null %s

struct __attribute__((packed)) PackedPointer {
	unsigned char tag;
	const void *pointer;
	unsigned char tail;
};

struct AlignedPointer {
	unsigned before;
	const void *pointer;
	unsigned after;
};

struct __attribute__((packed)) NestedPointer {
	unsigned char prefix;
	struct PackedPointer inner;
	void (*function)(void);
};

struct __attribute__((packed)) MixedFields {
	_Bool flag;
	unsigned short short_value;
	const void *pointer;
	unsigned after;
};

struct __attribute__((packed)) MultiplePointers {
	const void *first;
	unsigned char middle;
	void (*second)(void);
};

union PointerUnion {
	const void *pointer;
	unsigned value;
};

extern int external_object;
extern void external_function(void);
int default_object;
__attribute__((visibility("hidden"))) int hidden_object;
static int internal_object;
static unsigned char anchor[8];

static void local_function(void) {}

_Thread_local struct PackedPointer tls_record = {0x12, &external_object, 0x34};
_Thread_local struct PackedPointer tls_default = {0x21, &default_object, 0x22};
_Thread_local struct PackedPointer tls_hidden = {0x31, &hidden_object, 0x32};
static _Thread_local struct PackedPointer tls_internal = {0x41, &internal_object, 0x42};
_Thread_local struct PackedPointer tls_external_function = {0x51, external_function, 0x52};
_Thread_local struct PackedPointer tls_local_function = {0x61, local_function, 0x62};
_Thread_local struct PackedPointer tls_addend = {0x71, anchor + 3, 0x72};
_Thread_local struct AlignedPointer tls_aligned = {0x11223344, &external_object, 0x55667788};
_Thread_local struct MixedFields tls_mixed = {1, 0x1234, &external_object, 0x55667788};
_Thread_local struct MultiplePointers tls_multiple = {&default_object, 0xdd, external_function};
_Thread_local union PointerUnion tls_union = {.pointer = &external_object};
_Thread_local struct NestedPointer tls_nested = {0x81, {0x82, &external_object, 0x83}, external_function};
_Thread_local struct PackedPointer tls_array[2] = {{0x91, &default_object, 0x92}, {0xa1, &hidden_object, 0xa2}};
_Thread_local struct PackedPointer tls_null = {0xb1, 0, 0xb2};

const void *use_internal_tls(void) {
	return tls_internal.pointer;
}

// CHECK-LABEL: tls_record:
// CHECK-NEXT: .byte 18
// CHECK-NEXT: .ualong external_object
// CHECK-NEXT: .byte 52
// CHECK-NEXT: .size tls_record, 6

// CHECK-LABEL: tls_default:
// CHECK-NEXT: .byte 33
// CHECK-NEXT: .ualong default_object
// CHECK-NEXT: .byte 34

// CHECK-LABEL: tls_hidden:
// CHECK-NEXT: .byte 49
// CHECK-NEXT: .ualong hidden_object
// CHECK-NEXT: .byte 50

// CHECK-LABEL: tls_external_function:
// CHECK-NEXT: .byte 81
// CHECK-NEXT: .ualong external_function
// CHECK-NEXT: .byte 82

// CHECK-LABEL: tls_local_function:
// CHECK-NEXT: .byte 97
// CHECK-NEXT: .ualong local_function
// CHECK-NEXT: .byte 98

// CHECK-LABEL: tls_addend:
// CHECK-NEXT: .byte 113
// CHECK-NEXT: .ualong anchor+3
// CHECK-NEXT: .byte 114

// CHECK-LABEL: tls_aligned:
// CHECK-NEXT: .long 287454020
// CHECK-NEXT: .long external_object
// CHECK-NEXT: .long 1432778632

// CHECK-LABEL: tls_mixed:
// CHECK-NEXT: .byte 1
// CHECK-NEXT: .uaword 4660
// CHECK-NEXT: .ualong external_object
// CHECK-NEXT: .ualong 1432778632

// CHECK-LABEL: tls_multiple:
// CHECK-NEXT: .ualong default_object
// CHECK-NEXT: .byte 221
// CHECK-NEXT: .ualong external_function

// CHECK-LABEL: tls_union:
// CHECK-NEXT: .long external_object

// CHECK-LABEL: tls_nested:
// CHECK-NEXT: .byte 129
// CHECK-NEXT: .byte 130
// CHECK-NEXT: .ualong external_object
// CHECK-NEXT: .byte 131
// CHECK-NEXT: .ualong external_function

// CHECK-LABEL: tls_array:
// CHECK-NEXT: .byte 145
// CHECK-NEXT: .ualong default_object
// CHECK-NEXT: .byte 146
// CHECK-NEXT: .byte 161
// CHECK-NEXT: .ualong hidden_object
// CHECK-NEXT: .byte 162

// CHECK-LABEL: tls_null:
// CHECK-NEXT: .byte 177
// CHECK-NEXT: .ualong 0
// CHECK-NEXT: .byte 178

// CHECK-LABEL: tls_internal:
// CHECK-NEXT: .byte 65
// CHECK-NEXT: .ualong internal_object
// CHECK-NEXT: .byte 66
