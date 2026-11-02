// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "cxx-runtime.h"
#include <stdio.h>
#define INIT(v)                                                                \
  __real__(v).z = 1.25f;                                                       \
  __imag__(v).z = -3.5f;
#define CHECK(z) (__real__(z) == 1.25f && __imag__(z) == -3.5f)
unsigned failed;
void record(const char *name, unsigned ok) {
  printf("%s:%s\n", name, ok ? "PASS" : "FAIL");
  failed |= !ok;
}
#define NAMED(T)                                                               \
  {                                                                            \
    T v;                                                                       \
    INIT(v);                                                                   \
    record("named-" #T, take_##T(v));                                          \
    T r = ret_##T(&v);                                                         \
    record("return-" #T, CHECK(r.z));                                          \
  }
int main() {
  NAMED(Natural)
  NAMED(EmptyDerived) NAMED(DerivedOnly) NAMED(UserCtor) NAMED(NontrivialCopy)
      record("copies", copies == 2);
  NAMED(NontrivialDtor)
  record("destructors", destructors == 3);
  EmptyDerived e;
  INIT(e);
  DerivedOnly d;
  INIT(d);
  record("va-empty-even", va_empty_even(0, e, 0x31415926U));
  record("va-empty-odd", va_empty_odd(7.0f, 0, e, 0x31415926U));
  record("va-inherited-even", va_inherited_even(0, d, 0x31415926U));
  record("va-inherited-odd", va_inherited_odd(7.0f, 0, d, 0x31415926U));
  return failed != 0;
}
