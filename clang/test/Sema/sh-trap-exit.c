// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh4 -fsyntax-only -verify=base %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -pic-level 1 -fsyntax-only -verify=base %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2a -fsyntax-only -verify=sh2a %s

#if !__has_attribute(trap_exit)
#error trap_exit must be available on SH
#endif

enum { trap_max = 255 };
extern int dynamic_trap;

__attribute__((interrupt_handler, trap_exit(0))) void trap_min(void);
__attribute__((trap_exit(trap_max), interrupt_handler)) void trap_maximum(void);

__attribute__((trap_exit(1))) void missing_handler(void); // base-error {{'trap_exit' attribute requires 'interrupt_handler' on the same SuperH function}} sh2a-error {{'trap_exit' attribute requires 'interrupt_handler' on the same SuperH function}}
__attribute__((trapa_handler, trap_exit(2))) void trapa_exit(void); // base-error {{'trap_exit' attribute requires 'interrupt_handler' on the same SuperH function}} sh2a-error {{'trap_exit' attribute requires 'interrupt_handler' on the same SuperH function}}
__attribute__((interrupt_handler, trap_exit(-1))) void below_range(void); // base-error {{'trap_exit' attribute requires integer constant between 0 and 255 inclusive}} sh2a-error {{'trap_exit' attribute requires integer constant between 0 and 255 inclusive}}
__attribute__((interrupt_handler, trap_exit(256))) void above_range(void); // base-error {{'trap_exit' attribute requires integer constant between 0 and 255 inclusive}} sh2a-error {{'trap_exit' attribute requires integer constant between 0 and 255 inclusive}}
__attribute__((interrupt_handler, trap_exit(dynamic_trap))) void nonconstant(void); // base-error {{'trap_exit' attribute requires an integer constant}} sh2a-error {{'trap_exit' attribute requires an integer constant}}

#if defined(__SH2A__)
__attribute__((interrupt_handler, trap_exit(3), resbank)) void banked_exit(void); // sh2a-error {{'resbank' and 'trap_exit' attributes are not compatible}} sh2a-note {{conflicting attribute is here}}
#else
extern void *alternate_stack;
__attribute__((interrupt_handler, trap_exit(3), sp_switch("alternate_stack"))) void switched_exit(void); // base-error {{'sp_switch' and 'trap_exit' attributes are not compatible}} base-note {{conflicting attribute is here}}
__attribute__((interrupt_handler, trap_exit(4), nosave_low_regs)) void nosave_exit(void);
#endif
