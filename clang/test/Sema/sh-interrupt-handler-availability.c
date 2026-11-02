// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux -DNOT_SH -fsyntax-only -verify %s
// expected-no-diagnostics

#ifdef NOT_SH
#if __has_attribute(interrupt_handler)
#error interrupt_handler must be SH-only
#endif
#if __has_attribute(trapa_handler)
#error trapa_handler must be SH-only
#endif
#if __has_attribute(nosave_low_regs) || __has_attribute(resbank) || \
	__has_attribute(sp_switch) || __has_attribute(trap_exit)
#error advanced SuperH handler attributes must be SH-only
#endif
#else
#if !__has_attribute(interrupt_handler)
#error interrupt_handler must be available on SH
#endif
#if !__has_attribute(trapa_handler)
#error trapa_handler must be available on SH
#endif
#if !__has_attribute(nosave_low_regs) || !__has_attribute(resbank) || \
	!__has_attribute(sp_switch) || !__has_attribute(trap_exit)
#error advanced SuperH handler attributes must be available on SH
#endif
#endif
