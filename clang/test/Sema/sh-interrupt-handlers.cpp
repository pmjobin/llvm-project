// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -fsyntax-only -verify %s

__attribute__((interrupt_handler)) void interrupt_ok(void);
__attribute__((trapa_handler)) void trapa_ok(void);

__attribute__((interrupt_handler)) void redeclared(void);
void redeclared(void) {}

using handler_pointer = void (*)(void);
handler_pointer interrupt_vector = &interrupt_ok;
handler_pointer trapa_vector = &trapa_ok;

struct handlers {
	__attribute__((interrupt_handler)) static void interrupt_static(void);
	__attribute__((trapa_handler)) static void trapa_static(void);

	__attribute__((interrupt_handler)) void interrupt_member(void); // expected-error {{'interrupt_handler' attribute requires a non-variadic free or static member function with type 'void (void)'; non-static member functions have an implicit 'this' argument}}
	__attribute__((trapa_handler)) virtual void trapa_virtual(void); // expected-error {{'trapa_handler' attribute requires a non-variadic free or static member function with type 'void (void)'; non-static member functions have an implicit 'this' argument}}
};

__attribute__((interrupt_handler)) int bad_return(void); // expected-error {{'interrupt_handler' attribute requires a non-variadic free or static member function with type 'void (void)'; function has a non-void return type}}
__attribute__((trapa_handler)) void bad_argument(int); // expected-error {{'trapa_handler' attribute requires a non-variadic free or static member function with type 'void (void)'; function has parameters}}
__attribute__((interrupt_handler)) void bad_varargs(...); // expected-error {{'interrupt_handler' attribute requires a non-variadic free or static member function with type 'void (void)'; function is variadic}}

__attribute__((interrupt_handler, trapa_handler)) void conflicting_kind(void); // expected-error {{'trapa_handler' and 'interrupt_handler' attributes are not compatible}} expected-note {{conflicting attribute is here}}
__attribute__((interrupt_handler, naked)) void conflicting_naked(void); // expected-error {{'naked' and 'interrupt_handler' attributes are not compatible}} expected-note {{conflicting attribute is here}}
__attribute__((trapa_handler, always_inline)) void conflicting_inline(void) {} // expected-error {{'always_inline' and 'trapa_handler' attributes are not compatible}} expected-note {{conflicting attribute is here}}

int invalid_placement __attribute__((interrupt_handler)); // expected-error {{'interrupt_handler' attribute only applies to non-K&R-style functions}}

void direct_calls(void) {
	interrupt_ok(); // expected-error {{hardware SuperH handler cannot be called directly because it returns with RTE}}
	trapa_ok(); // expected-error {{hardware SuperH handler cannot be called directly because it returns with RTE}}
}
