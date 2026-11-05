// RUN: %clang -target shle-unknown-none-elf -mcpu=sh4 -mfloat-abi=soft -E -dM %s | FileCheck %s --check-prefix=SOFT
// RUN: %clang -target sh-unknown-none-elf -mcpu=sh4 -mfloat-abi=soft -E -dM %s | FileCheck %s --check-prefix=SOFT
// RUN: %clang -target shle-unknown-none-elf -mcpu=sh4 -mfloat-abi=hard -E -dM %s | FileCheck %s --check-prefix=HARD
// Hardware capability and argument ABI are independent: soft arguments do not
// imply a no-FPU CPU implementation. A target sysroot must distinguish them.
// SOFT: #define __SH_FLOAT_ABI_SOFT__ 1
// SOFT-NOT: __SH_FLOAT_ABI_HARD__
// HARD: #define __SH_FLOAT_ABI_HARD__ 1
// HARD-NOT: __SH_FLOAT_ABI_SOFT__
