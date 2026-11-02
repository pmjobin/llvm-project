// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
using CF = __complex__ float;
struct Natural {
  CF z;
};
struct Empty {};
struct EmptyDerived : Empty {
  CF z;
};
struct DerivedOnly : Natural {};
struct UserCtor {
  CF z;
  UserCtor();
};
struct NontrivialCopy {
  CF z;
  NontrivialCopy() = default;
  NontrivialCopy(const NontrivialCopy &);
};
struct NontrivialDtor {
  CF z;
  ~NontrivialDtor();
};
extern unsigned copies, destructors;
#define DECL(T)                                                                \
  unsigned take_##T(T);                                                        \
  T ret_##T(const T *);
DECL(Natural)
DECL(EmptyDerived) DECL(DerivedOnly) DECL(UserCtor) DECL(NontrivialCopy)
    DECL(NontrivialDtor) unsigned va_empty_even(int, ...);
unsigned va_empty_odd(float, int, ...);
unsigned va_inherited_even(int, ...);
unsigned va_inherited_odd(float, int, ...);
