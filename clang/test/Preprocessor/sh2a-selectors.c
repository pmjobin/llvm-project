// RUN: %clang -target shle-unknown-none-elf -m2a -dM -E -x c /dev/null | FileCheck %s --check-prefixes=COMMON,DOUBLE
// RUN: %clang -target shle-unknown-none-elf -mcpu=sh2a -dM -E -x c /dev/null | FileCheck %s --check-prefixes=COMMON,DOUBLE
// RUN: %clang -target shle-unknown-none-elf -m2a-nofpu -dM -E -x c /dev/null | FileCheck %s --check-prefixes=COMMON,NOFPU
// RUN: %clang -target shle-unknown-none-elf -m2a-single -dM -E -x c /dev/null | FileCheck %s --check-prefixes=COMMON,SINGLE
// RUN: %clang -target shle-unknown-none-elf -m2a-single-only -dM -E -x c /dev/null | FileCheck %s --check-prefixes=COMMON,SINGLE-ONLY

// COMMON-DAG: #define __SH2A__ 1
// COMMON-DAG: #define __SH_ATOMIC_MODEL_NONE__ 1
// COMMON-DAG: #define __sh__ 1

// DOUBLE-DAG: #define __DBL_MANT_DIG__ 53
// DOUBLE-DAG: #define __LDBL_MANT_DIG__ 53
// DOUBLE-DAG: #define __SH2A_DOUBLE__ 1
// DOUBLE-DAG: #define __SH_FPU_ANY__ 1
// DOUBLE-DAG: #define __SH_FPU_DOUBLE__ 1
// DOUBLE-DAG: #define __SIZEOF_DOUBLE__ 8
// DOUBLE-NOT: __SH2A_NOFPU__
// DOUBLE-NOT: __SH2A_SINGLE__
// DOUBLE-NOT: __SH2A_SINGLE_ONLY__

// NOFPU-DAG: #define __DBL_MANT_DIG__ 53
// NOFPU-DAG: #define __LDBL_MANT_DIG__ 53
// NOFPU-DAG: #define __SH2A_NOFPU__ 1
// NOFPU-DAG: #define __SIZEOF_DOUBLE__ 8
// NOFPU-NOT: __SH2A_DOUBLE__
// NOFPU-NOT: __SH2A_SINGLE__
// NOFPU-NOT: __SH2A_SINGLE_ONLY__
// NOFPU-NOT: __SH_FPU_ANY__
// NOFPU-NOT: __SH_FPU_DOUBLE__

// SINGLE-DAG: #define __DBL_MANT_DIG__ 53
// SINGLE-DAG: #define __LDBL_MANT_DIG__ 53
// SINGLE-DAG: #define __SH2A_SINGLE__ 1
// SINGLE-DAG: #define __SH_FPU_ANY__ 1
// SINGLE-DAG: #define __SH_FPU_DOUBLE__ 1
// SINGLE-DAG: #define __SIZEOF_DOUBLE__ 8
// SINGLE-NOT: __SH2A_DOUBLE__
// SINGLE-NOT: __SH2A_NOFPU__
// SINGLE-NOT: __SH2A_SINGLE_ONLY__

// SINGLE-ONLY-DAG: #define __DBL_MANT_DIG__ 24
// SINGLE-ONLY-DAG: #define __LDBL_MANT_DIG__ 53
// SINGLE-ONLY-DAG: #define __SH2A_SINGLE_ONLY__ 1
// SINGLE-ONLY-DAG: #define __SH_FPU_ANY__ 1
// SINGLE-ONLY-DAG: #define __SIZEOF_DOUBLE__ 4
// SINGLE-ONLY-NOT: __SH2A_DOUBLE__
// SINGLE-ONLY-NOT: __SH2A_NOFPU__
// SINGLE-ONLY-NOT: __SH2A_SINGLE__
// SINGLE-ONLY-NOT: __SH_FPU_DOUBLE__
