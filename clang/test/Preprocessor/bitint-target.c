// RUN: %clang_cc1 -triple shle-unknown-none-elf -E -dM %s | FileCheck %s --check-prefix=UNSUPPORTED
// RUN: %clang_cc1 -triple sh-unknown-none-elf -E -dM %s | FileCheck %s --check-prefix=UNSUPPORTED
// RUN: %clang_cc1 -triple x86_64-unknown-none-elf -E -dM %s | FileCheck %s --check-prefix=SUPPORTED
// The width macro must not advertise a type the target rejects. libc++ uses
// this macro to decide whether it can declare _BitInt overloads.
// UNSUPPORTED-NOT: __BITINT_MAXWIDTH__
// SUPPORTED: #define __BITINT_MAXWIDTH__
