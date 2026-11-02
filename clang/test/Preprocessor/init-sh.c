// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh1 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH1,SH < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH1,NOSH,BARE < %t
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh1 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH1,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH1,NOSHLE,BARE < %t
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH2,SH < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH2,NOSH,BARE < %t
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH2,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH2,NOSHLE,BARE < %t
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH2,SH,LINUX < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH2,NOSH < %t
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH2,SHLE,LINUX < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH2,NOSHLE < %t
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2e -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=E-COMMON,SH2E,SH < %t
// RUN: FileCheck %s --allow-empty --check-prefix=E-ABSENT < %t
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2e -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=E-COMMON,SH2E,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefix=E-ABSENT < %t
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh3 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH3,SH < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH3,NOSH,BARE < %t
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh3 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH3,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH3,NOSHLE,BARE < %t
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh3e -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=E-COMMON,SH3E,SH < %t
// RUN: FileCheck %s --allow-empty --check-prefix=E-ABSENT < %t
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh3e -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=E-COMMON,SH3E,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefix=E-ABSENT < %t
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh4 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH4,SH < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH4,NOSH,BARE < %t
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH4,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH4,NOSHLE,BARE < %t
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh4 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH4,SH,LINUX < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH4,NOSH < %t
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH4,SHLE,LINUX < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH4,NOSHLE < %t
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -fgnuc-version=4.2.1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH4,SHLE,LINUX < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH4,NOSHLE,HARD-ABI-ABSENT < %t
// RUN: %clang -target shle-unknown-none-elf -m1 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH1,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH1,NOSHLE,BARE < %t
// RUN: %clang -target shle-unknown-none-elf -m2 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH2,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH2,NOSHLE,BARE < %t
// RUN: %clang -target shle-unknown-none-elf -m3 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH3,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH3,NOSHLE,BARE < %t
// RUN: %clang -target shle-unknown-none-elf -m2e -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=E-COMMON,SH2E,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefix=E-ABSENT < %t
// RUN: %clang -target shle-unknown-none-elf -m3e -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=E-COMMON,SH3E,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefix=E-ABSENT < %t
// RUN: %clang -target shle-unknown-none-elf -m4 -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=COMMON,SH4,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=ABSENT-SH4,NOSHLE,BARE,HARD-ABI-ABSENT < %t
// RUN: %clang -target shle-unknown-none-elf -m4-nofpu -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=SH4-NOFPU,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=SH4-NOFPU-ABSENT,HARD-ABI-ABSENT < %t
// RUN: %clang -target shle-unknown-none-elf -m4-single -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=SH4-SINGLE,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=SH4-SINGLE-ABSENT,HARD-ABI-ABSENT < %t
// RUN: %clang -target shle-unknown-none-elf -m4-single-only -dM -E -x c /dev/null > %t
// RUN: FileCheck %s --check-prefixes=SH4-SINGLE-ONLY,SHLE < %t
// RUN: FileCheck %s --allow-empty --check-prefixes=SH4-SINGLE-ONLY-ABSENT,HARD-ABI-ABSENT < %t

// SH1-DAG: #define __SH1__ 1
// SH1-DAG: #define __SH_ATOMIC_MODEL_NONE__ 1
// SH1-DAG: #define __sh1__ 1
// SH2-DAG: #define __SH2__ 1
// SH2-DAG: #define __sh2__ 1
// SH2E-DAG: #define __SH2E__ 1
// SH3-DAG: #define __SH3__ 1
// SH3-DAG: #define __sh3__ 1
// SH3E-DAG: #define __SH3E__ 1
// SH4-DAG: #define __SH4__ 1
// SH4-DAG: #define __SH_FPU_ANY__ 1
// SH4-DAG: #define __SH_FPU_DOUBLE__ 1
// SH4-NOFPU-DAG: #define __SH3__ 1
// SH4-NOFPU-DAG: #define __SH4_NOFPU__ 1
// SH4-NOFPU-DAG: #define __SIZEOF_DOUBLE__ 8
// SH4-NOFPU-DAG: #define __sh3__ 1
// SH4-NOFPU-DAG: #define __sh__ 1
// SH4-SINGLE-DAG: #define __SH4_SINGLE__ 1
// SH4-SINGLE-DAG: #define __SH_FPU_ANY__ 1
// SH4-SINGLE-DAG: #define __SH_FPU_DOUBLE__ 1
// SH4-SINGLE-DAG: #define __SIZEOF_DOUBLE__ 8
// SH4-SINGLE-DAG: #define __sh__ 1
// SH4-SINGLE-ONLY-DAG: #define __DBL_MANT_DIG__ 24
// SH4-SINGLE-ONLY-DAG: #define __LDBL_MANT_DIG__ 53
// SH4-SINGLE-ONLY-DAG: #define __SH4_SINGLE_ONLY__ 1
// SH4-SINGLE-ONLY-DAG: #define __SH_FPU_ANY__ 1
// SH4-SINGLE-ONLY-DAG: #define __SIZEOF_DOUBLE__ 4
// SH4-SINGLE-ONLY-DAG: #define __SIZEOF_LONG_DOUBLE__ 8
// SH4-SINGLE-ONLY-DAG: #define __sh__ 1
// E-COMMON-DAG: #define __DBL_MANT_DIG__ 53
// E-COMMON-DAG: #define __LDBL_MANT_DIG__ 53
// E-COMMON-DAG: #define __SH_FPU_ANY__ 1
// E-COMMON-DAG: #define __SIZEOF_DOUBLE__ 8
// E-COMMON-DAG: #define __SIZEOF_LONG_DOUBLE__ 8
// E-COMMON-DAG: #define __sh__ 1
// COMMON-DAG: #define __CLANG_ATOMIC_INT_LOCK_FREE 1
// COMMON-DAG: #define __CLANG_ATOMIC_LLONG_LOCK_FREE 1
// COMMON-DAG: #define __CLANG_ATOMIC_POINTER_LOCK_FREE 1
// COMMON-DAG: #define __ORDER_BIG_ENDIAN__ 4321
// COMMON-DAG: #define __ORDER_LITTLE_ENDIAN__ 1234
// COMMON-DAG: #define __GCC_ATOMIC_INT_LOCK_FREE 1
// COMMON-DAG: #define __GCC_ATOMIC_LLONG_LOCK_FREE 1
// COMMON-DAG: #define __GCC_ATOMIC_POINTER_LOCK_FREE 1
// COMMON-DAG: #define __LDBL_MANT_DIG__ 53
// COMMON-DAG: #define __LDBL_MAX_EXP__ 1024
// COMMON-DAG: #define __SIZEOF_DOUBLE__ 8
// COMMON-DAG: #define __SIZEOF_INT__ 4
// COMMON-DAG: #define __SIZEOF_LONG_DOUBLE__ 8
// COMMON-DAG: #define __SIZEOF_LONG_LONG__ 8
// COMMON-DAG: #define __SIZEOF_LONG__ 4
// COMMON-DAG: #define __SIZEOF_POINTER__ 4
// COMMON-DAG: #define __SIZEOF_PTRDIFF_T__ 4
// COMMON-DAG: #define __SIZEOF_SIZE_T__ 4
// COMMON-DAG: #define __sh__ 1
// SH-DAG: #define __BIG_ENDIAN__ 1
// SH-DAG: #define __BYTE_ORDER__ __ORDER_BIG_ENDIAN__
// SHLE-DAG: #define __LITTLE_ENDIAN__ 1
// SHLE-DAG: #define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
// LINUX-DAG: #define __gnu_linux__ 1
// LINUX-DAG: #define __linux__ 1

// ABSENT-SH1-NOT: __SH2__
// ABSENT-SH1-NOT: __SH2E__
// ABSENT-SH1-NOT: __SH3__
// ABSENT-SH1-NOT: __SH3E__
// ABSENT-SH1-NOT: __SH4__
// ABSENT-SH1-NOT: __SH_FPU_ANY__
// ABSENT-SH1-NOT: __SH_FPU_DOUBLE__
// ABSENT-SH1-NOT: __SOFTFP__
// ABSENT-SH2-NOT: __SH2A__
// ABSENT-SH2-NOT: __SH3__
// ABSENT-SH2-NOT: __SH4__
// ABSENT-SH2-NOT: __SH_FPU_ANY__
// ABSENT-SH2-NOT: __SOFTFP__
// ABSENT-SH3-NOT: __SH2__
// ABSENT-SH3-NOT: __SH2A__
// ABSENT-SH3-NOT: __SH4__
// ABSENT-SH3-NOT: __SH_FPU_ANY__
// ABSENT-SH3-NOT: __SOFTFP__
// ABSENT-SH4-NOT: __SH2__
// ABSENT-SH4-NOT: __SH2A__
// ABSENT-SH4-NOT: __SH3__
// ABSENT-SH4-NOT: __SH4A__
// SH4-NOFPU-ABSENT-NOT: __SH4__
// SH4-NOFPU-ABSENT-NOT: __SH_FPU_ANY__
// SH4-NOFPU-ABSENT-NOT: __SH_FPU_DOUBLE__
// SH4-SINGLE-ABSENT-NOT: __SH4__
// SH4-SINGLE-ABSENT-NOT: __SH4_SINGLE_ONLY__
// SH4-SINGLE-ONLY-ABSENT-NOT: __SH4__
// SH4-SINGLE-ONLY-ABSENT-NOT: __SH4_SINGLE__
// SH4-SINGLE-ONLY-ABSENT-NOT: __SH_FPU_DOUBLE__
// ABSENT-SH4-NOT: __sh4__
// ABSENT-SH4-NOT: __SOFTFP__
// E-ABSENT-NOT: __SH2__
// E-ABSENT-NOT: __SH3__
// E-ABSENT-NOT: __SH4__
// E-ABSENT-NOT: __SH4_SINGLE__
// E-ABSENT-NOT: __SH4_SINGLE_ONLY__
// E-ABSENT-NOT: __SH_FPU_DOUBLE__
// HARD-ABI-ABSENT-NOT: __HARD_FLOAT__
// HARD-ABI-ABSENT-NOT: __SH_HARD_FLOAT__
// HARD-ABI-ABSENT-NOT: __SH4_HARD_FLOAT__
// NOSH-NOT: __LITTLE_ENDIAN__
// NOSHLE-NOT: __BIG_ENDIAN__
// BARE-NOT: #define __gnu_linux__ 1
// BARE-NOT: #define __linux__ 1
