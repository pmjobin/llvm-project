// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <sh4_freestanding.h>
#include <stdint.h>

extern "C" int platform_write(const char *, size_t);
extern "C" __attribute__((noreturn)) void platform_exit(int);
extern "C" int __cxa_atexit(void (*)(void *), void *, void *);
extern "C" void *__dso_handle;
extern "C" int __cxa_guard_acquire(uint64_t *);
extern "C" void __cxa_guard_release(uint64_t *);
extern "C" void __cxa_guard_abort(uint64_t *);

static int constructions, sum, destructions, destructor_order;
static int allocations, deallocations, callbacks, callback_error;
static int finalize_probe_order, finalize_probe_error, probe_dso;
alignas(16) static unsigned char heap[8192];
static size_t used;

extern "C" void *sh4_allocate(size_t bytes, size_t alignment) {
  size_t start = (used + alignment - 1) & ~(alignment - 1);
  if (bytes > sizeof(heap) || start > sizeof(heap) - bytes)
    return nullptr;
  used = start + bytes;
  ++allocations;
  return heap + start;
}
extern "C" void sh4_deallocate(void *pointer) {
  if (pointer)
    ++deallocations;
}
extern "C" void sh4_panic() { platform_exit(90); }

struct Object {
  int id;
  Object(int value = 4) : id(value) {
    ++constructions;
    sum += id;
  }
  virtual int value() const { return id * 7; }
  virtual ~Object() {
    ++destructions;
    destructor_order = destructor_order * 10 + id;
  }
};
Object global_object(1);

static Object &local_object() {
  static Object object(2);
  return object;
}
__attribute__((noinline)) int virtual_value(const Object *p) {
  return p->value();
}

static void callback(void *argument) {
  ++callbacks;
  if ((uintptr_t)argument != (unsigned)(59 - callbacks))
    callback_error = 1;
}

static void finalize_probe(void *argument) {
  finalize_probe_order = finalize_probe_order * 10 + (uintptr_t)argument;
}

static void register_during_finalize(void *) {
  finalize_probe((void *)(uintptr_t)1);
  if (__cxa_atexit(finalize_probe, (void *)(uintptr_t)2, &probe_dso) ||
      __cxa_atexit(finalize_probe, (void *)(uintptr_t)3, &probe_dso))
    finalize_probe_error = 1;
}

extern "C" int main() {
  if (constructions != 1 || sum != 1 || global_object.value() != 7)
    return 31;
  if (local_object().id != 2 || local_object().id != 2 || constructions != 2)
    return 32;
  Object *p = new Object(3);
  if (virtual_value(p) != 21 || constructions != 3)
    return 33;
  delete p;
  Object *array = new Object[2];
  if (array[0].id != 4 || array[1].id != 4 || constructions != 5 || sum != 14)
    return 34;
  delete[] array;
  if (destructions != 3 || destructor_order != 344 || deallocations != 2)
    return 35;

  uint64_t guard = 0;
  if (__cxa_guard_acquire(&guard) != 1)
    return 36;
  __cxa_guard_abort(&guard);
  if (__cxa_guard_acquire(&guard) != 1)
    return 37;
  __cxa_guard_release(&guard);
  if (__cxa_guard_acquire(&guard) != 0)
    return 38;

  // A destructor can register more destructors. They must run before the older
  // registrations, and a second finalization must not call any of them again.
  if (__cxa_atexit(finalize_probe, (void *)(uintptr_t)4, &probe_dso) ||
      __cxa_atexit(register_during_finalize, nullptr, &probe_dso))
    return 44;
  __cxa_finalize(&probe_dso);
  if (finalize_probe_order != 1324 || finalize_probe_error || destructions != 3)
    return 45;
  __cxa_finalize(&probe_dso);
  if (finalize_probe_order != 1324)
    return 46;

  // Two language-generated registrations and four probe registrations consume
  // six lifetime slots. Fill the remaining 58 and verify overflow and order.
  for (unsigned i = 1; i != 59; ++i)
    if (__cxa_atexit(callback, (void *)(uintptr_t)i, __dso_handle))
      return 39;
  if (__cxa_atexit(callback, nullptr, __dso_handle) != -1)
    return 40;
  return 0;
}

extern "C" int after_fini(int result) {
  if (result)
    return result;
  if (destructions != 5 || destructor_order != 34421 || allocations != 2 ||
      deallocations != 2 || callbacks != 58 || callback_error)
    return 41;
  __cxa_finalize(nullptr);
  if (destructions != 5 || callbacks != 58)
    return 42;
  static const char message[] = "SH4 C++ lifecycle PASS\n";
  return platform_write(message, sizeof(message) - 1) == sizeof(message) - 1
             ? 0
             : 43;
}
