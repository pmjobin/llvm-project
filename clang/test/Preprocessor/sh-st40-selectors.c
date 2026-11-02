// RUN: %clang -target shle-unknown-none-elf -m4-100 -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST100,FULL
// RUN: %clang -target shle-unknown-none-elf -m4-100-nofpu -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST100,NOFPU
// RUN: %clang -target shle-unknown-none-elf -m4-100-single -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST100,SINGLE
// RUN: %clang -target shle-unknown-none-elf -m4-100-single-only -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST100,SINGLE-ONLY
// RUN: %clang -target shle-unknown-none-elf -m4-200 -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST200,FULL
// RUN: %clang -target shle-unknown-none-elf -m4-200-nofpu -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST200,NOFPU
// RUN: %clang -target shle-unknown-none-elf -m4-200-single -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST200,SINGLE
// RUN: %clang -target shle-unknown-none-elf -m4-200-single-only -dM -E -x c /dev/null | FileCheck %s --check-prefixes=ST200,SINGLE-ONLY
// RUN: %clang -target shle-unknown-none-elf -m4 -mtune=st40-100 -dM -E -x c /dev/null | FileCheck %s --check-prefix=TUNE-ONLY

// ST100-DAG: #define __SH4_100__ 1
// ST100-NOT: __SH4_200__
// ST200-DAG: #define __SH4_200__ 1
// ST200-NOT: __SH4_100__
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
// TUNE-ONLY-NOT: __SH4_100__
// TUNE-ONLY-NOT: __SH4_200__
