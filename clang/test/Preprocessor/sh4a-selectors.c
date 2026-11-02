// RUN: %clang -target shle-unknown-none-elf -m4a -dM -E -x c /dev/null | FileCheck %s --check-prefixes=COMMON,FULL
// RUN: %clang -target shle-unknown-none-elf -mcpu=sh4a -dM -E -x c /dev/null | FileCheck %s --check-prefixes=COMMON,FULL
// RUN: %clang -target shle-unknown-none-elf -m4a-nofpu -dM -E -x c /dev/null | FileCheck %s --check-prefixes=COMMON,NOFPU
// RUN: %clang -target shle-unknown-none-elf -m4a-single -dM -E -x c /dev/null | FileCheck %s --check-prefixes=COMMON,SINGLE
// RUN: %clang -target shle-unknown-none-elf -m4a-single-only -dM -E -x c /dev/null | FileCheck %s --check-prefixes=COMMON,SINGLE-ONLY
// RUN: %clang -target shle-unknown-none-elf -m3 -mtune=sh4a -dM -E -x c /dev/null | FileCheck %s --check-prefix=TUNE-ONLY

// COMMON-DAG: #define __SH4A__ 1
// COMMON-DAG: #define __SH_ATOMIC_MODEL_NONE__ 1
// COMMON-DAG: #define __sh__ 1

// FULL-DAG: #define __DBL_MANT_DIG__ 53
// FULL-DAG: #define __SH4__ 1
// FULL-DAG: #define __SH_FPU_ANY__ 1
// FULL-DAG: #define __SH_FPU_DOUBLE__ 1
// FULL-DAG: #define __SIZEOF_DOUBLE__ 8
// FULL-NOT: __SH4_NOFPU__
// FULL-NOT: __SH4_SINGLE__
// FULL-NOT: __SH4_SINGLE_ONLY__

// NOFPU-DAG: #define __SH4_NOFPU__ 1
// NOFPU-DAG: #define __SIZEOF_DOUBLE__ 8
// NOFPU-NOT: __SH4__
// NOFPU-NOT: __SH_FPU_ANY__
// NOFPU-NOT: __SH_FPU_DOUBLE__
// NOFPU-NOT: __sh3__

// SINGLE-DAG: #define __SH4_SINGLE__ 1
// SINGLE-DAG: #define __SH_FPU_ANY__ 1
// SINGLE-DAG: #define __SH_FPU_DOUBLE__ 1
// SINGLE-DAG: #define __SIZEOF_DOUBLE__ 8
// SINGLE-NOT: __SH4__
// SINGLE-NOT: __SH4_SINGLE_ONLY__

// SINGLE-ONLY-DAG: #define __DBL_MANT_DIG__ 24
// SINGLE-ONLY-DAG: #define __SH4_SINGLE_ONLY__ 1
// SINGLE-ONLY-DAG: #define __SH_FPU_ANY__ 1
// SINGLE-ONLY-DAG: #define __SIZEOF_DOUBLE__ 4
// SINGLE-ONLY-NOT: __SH4__
// SINGLE-ONLY-NOT: __SH4_SINGLE__
// SINGLE-ONLY-NOT: __SH_FPU_DOUBLE__

// TUNE-ONLY-DAG: #define __SH3__ 1
// TUNE-ONLY-DAG: #define __sh3__ 1
// TUNE-ONLY-NOT: __SH4A__
