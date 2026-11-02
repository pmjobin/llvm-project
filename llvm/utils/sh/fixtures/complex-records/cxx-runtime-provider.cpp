// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "cxx-runtime.h"
#include <stdarg.h>
unsigned copies, destructors;
UserCtor::UserCtor() {
  __real__ z = 1.25f;
  __imag__ z = -3.5f;
}
NontrivialCopy::NontrivialCopy(const NontrivialCopy &p) : z(p.z) { ++copies; }
NontrivialDtor::~NontrivialDtor() { ++destructors; }
#define CHECK(z) (__real__(z) == 1.25f && __imag__(z) == -3.5f)
#define DEF(T)                                                                 \
  unsigned take_##T(T v) { return CHECK(v.z); }                                \
  T ret_##T(const T *p) { return *p; }
DEF(Natural)
DEF(EmptyDerived) DEF(DerivedOnly) DEF(UserCtor) DEF(NontrivialCopy)
    DEF(NontrivialDtor)
#define VA(T, N, ...)                                                          \
  unsigned N(__VA_ARGS__, ...) {                                               \
    va_list ap, copy;                                                          \
    va_start(ap, tag);                                                         \
    va_copy(copy, ap);                                                         \
    T a = va_arg(ap, T);                                                       \
    unsigned tail = va_arg(ap, unsigned);                                      \
    T b = va_arg(copy, T);                                                     \
    unsigned tailcopy = va_arg(copy, unsigned);                                \
    va_end(copy);                                                              \
    va_end(ap);                                                                \
    return CHECK(a.z) && CHECK(b.z) && tail == 0x31415926U &&                  \
           tailcopy == tail;                                                   \
  }
        VA(EmptyDerived, va_empty_even, int tag)
            VA(EmptyDerived, va_empty_odd, float named, int tag)
                VA(DerivedOnly, va_inherited_even, int tag)
                    VA(DerivedOnly, va_inherited_odd, float named, int tag)
