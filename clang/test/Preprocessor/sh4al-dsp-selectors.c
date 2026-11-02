// RUN: %clang -target shle-unknown-none-elf -m4al -dM -E -x c /dev/null | FileCheck %s --check-prefix=ARCH
// RUN: %clang -target shle-unknown-none-elf -mcpu=sh4al-dsp -dM -E -x c /dev/null | FileCheck %s --check-prefix=ARCH
// RUN: %clang -target shle-unknown-none-elf -m3 -mtune=sh4al-dsp -dM -E -x c /dev/null | FileCheck %s --check-prefix=TUNE-ONLY

// ARCH-DAG: #define __SH4A__ 1
// ARCH-DAG: #define __SH4_NOFPU__ 1
// ARCH-DAG: #define __SH_ATOMIC_MODEL_NONE__ 1
// ARCH-DAG: #define __SIZEOF_DOUBLE__ 8
// ARCH-DAG: #define __SIZEOF_FLOAT__ 4
// ARCH-DAG: #define __SIZEOF_INT__ 4
// ARCH-DAG: #define __SIZEOF_LONG_DOUBLE__ 8
// ARCH-DAG: #define __SIZEOF_LONG__ 4
// ARCH-DAG: #define __SIZEOF_POINTER__ 4
// ARCH-DAG: #define __sh__ 1
// ARCH-NOT: __SH4AL_DSP__
// ARCH-NOT: __SH4__
// ARCH-NOT: __SH4_SINGLE__
// ARCH-NOT: __SH4_SINGLE_ONLY__
// ARCH-NOT: __SH_FPU_ANY__
// ARCH-NOT: __SH_FPU_DOUBLE__

// TUNE-ONLY-DAG: #define __SH3__ 1
// TUNE-ONLY-DAG: #define __sh3__ 1
// TUNE-ONLY-NOT: __SH4A__
// TUNE-ONLY-NOT: __SH4_NOFPU__
// TUNE-ONLY-NOT: __SH4AL_DSP__
