// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -fsyntax-only -verify=static %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -fsyntax-only -verify=static %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh4 -pic-level 1 -DTEST_PIC -fsyntax-only -verify=pic %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -pic-level 2 -pic-is-pie -DTEST_PIC -fsyntax-only -verify=pic %s

#if !__has_attribute(sp_switch)
#error sp_switch must be available on SH
#endif

#ifdef TEST_PIC
__attribute__((interrupt_handler, sp_switch("alternate_stack"))) void pic_switch(void); // pic-error {{'sp_switch' attribute is unsupported for position-independent SuperH code}}
#else
extern void *alternate_stack;
extern unsigned long second_stack;

__attribute__((interrupt_handler, sp_switch("alternate_stack"))) void switched(void);
__attribute__((sp_switch("second_stack"), interrupt_handler)) void switched_reverse(void);

__attribute__((sp_switch("alternate_stack"))) void missing_handler(void); // static-error {{'sp_switch' attribute requires 'interrupt_handler' on the same SuperH function}}
__attribute__((trapa_handler, sp_switch("alternate_stack"))) void trapa_switch(void); // static-error {{'sp_switch' attribute requires 'interrupt_handler' on the same SuperH function}}
__attribute__((interrupt_handler, sp_switch(""))) void empty_symbol(void); // static-error {{'sp_switch' attribute requires a non-empty SuperH stack-pointer object symbol}}
__attribute__((interrupt_handler, sp_switch(4))) void nonstring_symbol(void); // static-error {{expected string literal as argument of 'sp_switch' attribute}}
__attribute__((interrupt_handler, sp_switch("alternate_stack"), naked)) void naked_switch(void); // static-error {{'naked' and 'interrupt_handler' attributes are not compatible}} static-note {{conflicting attribute is here}}
__attribute__((interrupt_handler, sp_switch("alternate_stack"), always_inline)) void inline_switch(void); // static-error {{'always_inline' and 'interrupt_handler' attributes are not compatible}} static-note {{conflicting attribute is here}}
#endif
