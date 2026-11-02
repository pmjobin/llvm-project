// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -target-feature +sh2 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh4 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh4 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -fsyntax-only -verify %s
// expected-no-diagnostics

_Static_assert(sizeof(char) == 1, "char width");
_Static_assert(sizeof(short) == 2, "short width");
_Static_assert(sizeof(int) == 4, "int width");
_Static_assert(sizeof(long) == 4, "long width");
_Static_assert(sizeof(long long) == 8, "long long width");
_Static_assert(sizeof(void *) == 4, "pointer width");
_Static_assert(sizeof(float) == 4, "float width");
_Static_assert(sizeof(double) == 8, "double width");
_Static_assert(sizeof(long double) == 8, "long double width");

_Static_assert(_Alignof(long long) == 4, "long long alignment");
_Static_assert(_Alignof(double) == 4, "double alignment");
_Static_assert(_Alignof(long double) == 4, "long double alignment");
_Static_assert(_Alignof(void *) == 4, "pointer alignment");

_Static_assert(__builtin_types_compatible_p(__SIZE_TYPE__, unsigned int), "size_t type");
_Static_assert(__builtin_types_compatible_p(__PTRDIFF_TYPE__, int), "ptrdiff_t type");
_Static_assert(__builtin_types_compatible_p(__INTPTR_TYPE__, int), "intptr_t type");
_Static_assert(__builtin_types_compatible_p(__WCHAR_TYPE__, long), "wchar_t type");
_Static_assert(__builtin_types_compatible_p(__WINT_TYPE__, unsigned int), "wint_t type");
_Static_assert(__builtin_types_compatible_p(__builtin_va_list, void *), "va_list type");

struct layout {
	char byte;
	long long wide;
};

_Static_assert(sizeof(struct layout) == 12, "aggregate size");
_Static_assert(_Alignof(struct layout) == 4, "aggregate alignment");

__thread int tls_value;
_Atomic(int) atomic_value;

_Static_assert(!__atomic_always_lock_free(1, 0), "byte atomics require a library");
_Static_assert(!__atomic_always_lock_free(4, 0), "word atomics require a library");
_Static_assert(!__atomic_always_lock_free(8, 0), "wide atomics require a library");

void valid_registers(int value) {
	register int r0 __asm__("r0") = value;
	__asm__ volatile("" : "+r"(r0));
	__asm__ volatile("" ::: "r1", "r2", "r3", "r4", "r5", "r6", "r7");
	__asm__ volatile("" ::: "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");
	__asm__ volatile("" ::: "pc", "pr", "gbr", "vbr", "mach", "macl", "sr");
	__asm__ volatile("" ::: "sp", "fp");
}
