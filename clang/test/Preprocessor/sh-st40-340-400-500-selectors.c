// RUN: %clang -target shle-unknown-none-elf -m4-340 -dM -E -x c /dev/null | FileCheck %s --check-prefix=ST340
// RUN: %clang -target shle-unknown-none-elf -m4-400 -dM -E -x c /dev/null | FileCheck %s --check-prefix=ST400
// RUN: %clang -target shle-unknown-none-elf -m4-500 -dM -E -x c /dev/null | FileCheck %s --check-prefix=ST500
// RUN: %clang -target shle-unknown-none-elf -m4 -mtune=st40-340 -dM -E -x c /dev/null | FileCheck %s --check-prefix=TUNE-ONLY

// ST340-DAG: #define __SH4_340__ 1
// ST340-DAG: #define __SH4_NOFPU__ 1
// ST340-DAG: #define __SH3__ 1
// ST340-DAG: #define __sh3__ 1
// ST340-DAG: #define __SH_ATOMIC_MODEL_NONE__ 1
// ST340-NOT: __SH4_300__
// ST340-NOT: __SH_FPU_ANY__
// ST400-DAG: #define __SH4_400__ 1
// ST400-DAG: #define __SH4_NOFPU__ 1
// ST400-DAG: #define __SH_ATOMIC_MODEL_NONE__ 1
// ST400-NOT: __SH4_300__
// ST400-NOT: __SH_FPU_ANY__
// ST500-DAG: #define __SH4_500__ 1
// ST500-DAG: #define __SH4_NOFPU__ 1
// ST500-DAG: #define __SH_ATOMIC_MODEL_NONE__ 1
// ST500-NOT: __SH4_300__
// ST500-NOT: __SH_FPU_ANY__
// TUNE-ONLY-DAG: #define __SH4__ 1
// TUNE-ONLY-NOT: __SH4_340__
// TUNE-ONLY-NOT: __SH4_400__
// TUNE-ONLY-NOT: __SH4_500__
