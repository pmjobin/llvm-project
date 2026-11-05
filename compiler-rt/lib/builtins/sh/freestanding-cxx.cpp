// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "include/sh4_freestanding.h"
#include <stdint.h>

// A bounded, single-threaded subset of the Itanium C++ ABI. This is not an
// alternative to libc++abi for hosted programs, exceptions or threading.
namespace {
struct Destructor {
  void (*function)(void *);
  void *argument;
  void *dso;
};
Destructor destructors[64];
size_t count;
} // namespace

extern "C" {
void *__dso_handle = &__dso_handle;

int __cxa_atexit(void (*function)(void *), void *argument, void *dso) {
  if (count == 64)
    return -1;
  destructors[count++] = {function, argument, dso};
  return 0;
}

void __cxa_finalize(void *dso) {
  for (size_t i = count; i != 0;) {
    Destructor &d = destructors[--i];
    if (d.function && (!dso || dso == d.dso)) {
      auto function = d.function;
      d.function = nullptr;
      function(d.argument);
      // A callback may register new destructors. Revisit the end of the table
      // so they run before older registrations. Cleared entries are skipped;
      // the lifetime limit bounds the number of callbacks and rescans.
      i = count;
    }
  }
}

int __cxa_guard_acquire(uint64_t *guard) {
  auto *bytes = reinterpret_cast<unsigned char *>(guard);
  if (bytes[0])
    return 0;
  if (bytes[1])
    sh4_panic(); // Recursive local static initialization.
  bytes[1] = 1;
  return 1;
}

void __cxa_guard_release(uint64_t *guard) {
  auto *bytes = reinterpret_cast<unsigned char *>(guard);
  bytes[0] = 1;
  bytes[1] = 0;
}

void __cxa_guard_abort(uint64_t *guard) {
  reinterpret_cast<unsigned char *>(guard)[1] = 0;
}

void __cxa_pure_virtual() { sh4_panic(); }
void __cxa_deleted_virtual() { sh4_panic(); }
}

void *operator new(size_t size) {
  if (void *p = sh4_allocate(size ? size : 1, __STDCPP_DEFAULT_NEW_ALIGNMENT__))
    return p;
  sh4_panic();
}

void *operator new[](size_t size) { return ::operator new(size); }
void operator delete(void *pointer) noexcept { sh4_deallocate(pointer); }
void operator delete[](void *pointer) noexcept { sh4_deallocate(pointer); }
void operator delete(void *pointer, size_t) noexcept {
  sh4_deallocate(pointer);
}
void operator delete[](void *pointer, size_t) noexcept {
  sh4_deallocate(pointer);
}
