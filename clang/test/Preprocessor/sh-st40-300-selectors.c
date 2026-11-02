// RUN: %clang -target shle-unknown-none-elf -m4-300 -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST300,FULL
// RUN: %clang -target shle-unknown-none-elf -m4-300-nofpu -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST300,NOFPU
// RUN: %clang -target shle-unknown-none-elf -m4-300-single -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST300,SINGLE
// RUN: %clang -target shle-unknown-none-elf -m4-300-single-only -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST300,SINGLE-ONLY
// RUN: %clang -target shle-unknown-none-elf -m4 -mtune=st40-300 -dM -E -x c /dev/null | FileCheck %s --check-prefix=TUNE-ONLY

// ST300-DAG: #define __SH4_300__ 1
// FULL-DAG: #define __SH4__ 1
// FULL-DAG: #define __SH_FPU_DOUBLE__ 1
// NOFPU-DAG: #define __SH4_NOFPU__ 1
// NOFPU-NOT: __SH_FPU_ANY__
// SINGLE-DAG: #define __SH4_SINGLE__ 1
// SINGLE-DAG: #define __SH_FPU_DOUBLE__ 1
// SINGLE-ONLY-DAG: #define __SH4_SINGLE_ONLY__ 1
// SINGLE-ONLY-DAG: #define __SH_FPU_ANY__ 1
// SINGLE-ONLY-NOT: __SH_FPU_DOUBLE__
// TUNE-ONLY-DAG: #define __SH4__ 1
// TUNE-ONLY-NOT: __SH4_300__
