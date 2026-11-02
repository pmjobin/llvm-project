// RUN: not %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -target-feature +sh4 -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=SH4
// RUN: not %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -target-feature -sh2 -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=NO-SH2
// RUN: not %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -target-feature +dynamic-shift -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=PARTIAL-SH2
// RUN: not %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh3 -target-feature -mmu -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=PARTIAL-SH3
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -target-feature +double-fpu -fsyntax-only %s
// RUN: not %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -target-feature -double-fpu -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=PARTIAL-SH4
// RUN: not %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -target-feature +sh4a -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=SH4A

// SH4: error: invalid feature combination: unsupported SH target feature '+sh4'
// NO-SH2: error: invalid feature combination: unsupported SH target feature '-sh2'
// PARTIAL-SH2: error: invalid feature combination: unsupported SH target feature '+dynamic-shift'
// PARTIAL-SH3: error: invalid feature combination: unsupported SH target feature '-mmu'
// PARTIAL-SH4: error: invalid feature combination: unsupported SH target feature '-double-fpu'
// SH4A: error: invalid feature combination: unsupported SH target feature '+sh4a'

int value;
