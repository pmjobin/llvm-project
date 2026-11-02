// RUN: %clang_cc1 -triple shle -target-cpu sh2 -O2 -S -o - %s | FileCheck %s --check-prefix=GBR
// RUN: not %clang_cc1 -triple shle -target-cpu sh2 -DFPSCR -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=NOFPU
// RUN: %clang_cc1 -triple shle -target-cpu sh4 -target-feature +single-fpu -mfloat-abi soft -DFPSCR -S -o /dev/null %s
// RUN: not %clang_cc1 -triple shle -target-cpu sh4 -DHANDLER -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=HANDLER

// GBR-LABEL: gbr:
// GBR: stc gbr,
// GBR: ldc {{.*}},gbr
// GBR: stc gbr,
void *gbr(void *p, void **old) {
  *old = __builtin_thread_pointer();
  __builtin_set_thread_pointer(p);
  return __builtin_thread_pointer();
}
#ifdef FPSCR
// NOFPU: needs target feature single-fpu
unsigned control(unsigned x) {
  __builtin_sh_set_fpscr(x);
  return __builtin_sh_get_fpscr();
}
#endif
#ifdef HANDLER
// HANDLER: SH FPSCR control intrinsic in a handler
__attribute__((interrupt_handler)) void handler(void) {
  __builtin_sh_set_fpscr(0);
}
#endif
