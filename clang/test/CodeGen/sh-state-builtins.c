// RUN: %clang_cc1 -triple shle -target-cpu sh4 -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh -target-cpu sh4 -O2 -S -o /dev/null %s
// RUN: %clang_cc1 -triple shle -target-cpu sh4 -O0 -S -o /dev/null %s
// RUN: %clang_cc1 -triple shle -target-cpu sh4 -x c++ -O2 -emit-llvm -o /dev/null %s

#if !__has_builtin(__builtin_set_thread_pointer) || \
    !__has_builtin(__builtin_sh_get_fpscr) || \
    !__has_builtin(__builtin_sh_set_fpscr)
#error Missing SH state builtin
#endif

#ifdef __cplusplus
static_assert(__is_same(decltype(__builtin_thread_pointer()), void *));
static_assert(__is_same(decltype(__builtin_set_thread_pointer(nullptr)), void));
static_assert(__is_same(decltype(__builtin_sh_get_fpscr()), unsigned int));
static_assert(__is_same(decltype(__builtin_sh_set_fpscr(0u)), void));
#else
_Static_assert(__builtin_types_compatible_p(
    __typeof__(__builtin_thread_pointer()), void *), "getter type");
_Static_assert(__builtin_types_compatible_p(
    __typeof__(__builtin_set_thread_pointer((void *)0)), void), "setter type");
_Static_assert(__builtin_types_compatible_p(
    __typeof__(__builtin_sh_get_fpscr()), unsigned int), "FPSCR getter type");
_Static_assert(__builtin_types_compatible_p(
    __typeof__(__builtin_sh_set_fpscr(0u)), void), "FPSCR setter type");
#endif

void set(void *p) { __builtin_set_thread_pointer(p); }

// CHECK-LABEL: define{{.*}} @roundtrip(
// CHECK: [[BEFORE:%.*]] = {{.*}}call ptr @llvm.sh.get.thread.pointer()
// CHECK: call void @llvm.sh.set.thread.pointer(ptr {{.*}})
// CHECK: [[AFTER:%.*]] = {{.*}}call ptr @llvm.sh.get.thread.pointer()
// CHECK: store ptr [[BEFORE]]
// CHECK: ret ptr [[AFTER]]
void *roundtrip(void *p, void **saved) {
  void *before = __builtin_thread_pointer();
  __builtin_set_thread_pointer(p);
  void *after = __builtin_thread_pointer();
  *saved = before;
  return after;
}

extern void writer(void *);
// CHECK-LABEL: define{{.*}} @across_call(
// CHECK: call ptr @llvm.sh.get.thread.pointer()
// CHECK: call void @writer(
// CHECK: call ptr @llvm.sh.get.thread.pointer()
void *across_call(void *p, void **saved) {
  *saved = __builtin_thread_pointer();
  writer(p);
  return __builtin_thread_pointer();
}

// CHECK-LABEL: define{{.*}} @fpscr(
// CHECK: call i32 @llvm.sh.get.fpscr()
// CHECK: call void @llvm.sh.set.fpscr(i32
// CHECK: call i32 @llvm.sh.get.fpscr()
unsigned fpscr(unsigned v, unsigned *saved) {
  *saved = __builtin_sh_get_fpscr();
  __builtin_sh_set_fpscr(v);
  return __builtin_sh_get_fpscr();
}

#pragma STDC FENV_ACCESS ON
// CHECK-LABEL: define{{.*}} @strict_control(
// CHECK: call i32 @llvm.sh.get.fpscr()
// CHECK: call void @llvm.sh.set.fpscr(i32
void strict_control(void) {
  unsigned v = __builtin_sh_get_fpscr();
  __builtin_sh_set_fpscr(v);
}
