// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -fsyntax-only -verify %s
// expected-no-diagnostics

typedef __builtin_va_list va_list;

_Static_assert(sizeof(va_list) == 20, "GNU SH hard-float va_list size");
_Static_assert(_Alignof(va_list) == 4, "GNU SH hard-float va_list alignment");
_Static_assert(!__builtin_types_compatible_p(va_list, void *), "hard-float va_list is not the soft ABI cursor");

void use_va_list(va_list *values) {
	(void)values;
}
