// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh3 -fsyntax-only -verify=banked %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh3e -fsyntax-only -verify=banked %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -fsyntax-only -verify=banked %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh4a -fsyntax-only -verify=banked %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh4al-dsp -fsyntax-only -verify=banked %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh4 -target-feature +st40-300 -fsyntax-only -verify=banked %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh1 -fsyntax-only -verify=unbanked %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -fsyntax-only -verify=unbanked %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2e -fsyntax-only -verify=unbanked %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2a -fsyntax-only -verify=sh2a %s

#if !__has_attribute(nosave_low_regs)
#error nosave_low_regs must be available on SH
#endif
#if !__has_attribute(resbank)
#error resbank must be available on SH
#endif

#if defined(__SH2A__)
__attribute__((interrupt_handler, resbank)) void resbank_ok(void);
__attribute__((resbank, interrupt_handler)) void resbank_reverse(void);
__attribute__((interrupt_handler, resbank, sp_switch("alternate_stack"))) void resbank_switched(void);
__attribute__((resbank)) void resbank_without_handler(void); // sh2a-error {{'resbank' attribute requires 'interrupt_handler' on the same SuperH function}}
__attribute__((trapa_handler, resbank)) void resbank_trapa(void); // sh2a-error {{'resbank' attribute requires 'interrupt_handler' on the same SuperH function}}
__attribute__((interrupt_handler, resbank, nosave_low_regs)) void resbank_nosave(void); // sh2a-error {{'nosave_low_regs' and 'resbank' attributes are not compatible}} sh2a-note {{conflicting attribute is here}}
#elif defined(__SH3__) || defined(__SH3E__) || defined(__SH4__) || defined(__SH4A__)
__attribute__((interrupt_handler, nosave_low_regs)) void nosave_ok(void);
__attribute__((nosave_low_regs, interrupt_handler)) void nosave_reverse(void);

__attribute__((nosave_low_regs)) void nosave_without_handler(void); // banked-error {{'nosave_low_regs' attribute requires 'interrupt_handler' on the same SuperH function}}
__attribute__((trapa_handler, nosave_low_regs)) void nosave_trapa(void); // banked-error {{'nosave_low_regs' attribute requires 'interrupt_handler' on the same SuperH function}}
__attribute__((interrupt_handler, nosave_low_regs, naked)) void nosave_naked(void); // banked-error {{'naked' and 'interrupt_handler' attributes are not compatible}} banked-note {{conflicting attribute is here}}
__attribute__((interrupt_handler, nosave_low_regs, always_inline)) void nosave_inline(void); // banked-error {{'always_inline' and 'interrupt_handler' attributes are not compatible}} banked-note {{conflicting attribute is here}}
#else
__attribute__((interrupt_handler, nosave_low_regs)) void nosave_bad_cpu(void); // unbanked-error {{'nosave_low_regs' attribute requires a SuperH target with hardware-banked r0-r7 registers}}
__attribute__((interrupt_handler, resbank)) void resbank_bad_cpu(void); // unbanked-error {{'resbank' attribute requires an SH2A target}}
#endif
