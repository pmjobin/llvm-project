// RUN: %clang -### -target shle-unknown-none-elf -m4-300 -c %s 2>&1 | FileCheck %s --check-prefix=FULL
// RUN: %clang -### -target shle-unknown-none-elf -m4-300-nofpu -c %s 2>&1 | FileCheck %s --check-prefix=NOFPU
// RUN: %clang -### -target shle-unknown-none-elf -m4-300-single -c %s 2>&1 | FileCheck %s --check-prefix=SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -m4-300-single-only -c %s 2>&1 | FileCheck %s --check-prefix=SINGLE-ONLY

// FULL: "-cc1"
// FULL-SAME: "-target-cpu" "sh4"
// FULL-SAME: "-target-feature" "+st40-300"
// FULL-SAME: "-tune-cpu" "st40-300"
// FULL-SAME: "-target-abi" "gnu-hard-float"
// FULL-SAME: "-mfloat-abi" "hard"
// NOFPU: "-cc1"
// NOFPU-SAME: "-target-cpu" "sh4"
// NOFPU-SAME: "-target-feature" "+st40-300"
// NOFPU-SAME: "-target-feature" "-single-fpu"
// NOFPU-SAME: "-target-feature" "-double-fpu"
// NOFPU-SAME: "-tune-cpu" "st40-300"
// NOFPU-SAME: "-msoft-float"
// NOFPU-SAME: "-mfloat-abi" "soft"
// SINGLE: "-cc1"
// SINGLE-SAME: "-target-cpu" "sh4"
// SINGLE-SAME: "-target-feature" "+st40-300"
// SINGLE-SAME: "-target-feature" "+double-fpu"
// SINGLE-SAME: "-target-feature" "+fpu-single-mode"
// SINGLE-SAME: "-tune-cpu" "st40-300"
// SINGLE-SAME: "-target-abi" "gnu-hard-float-single"
// SINGLE-ONLY: "-cc1"
// SINGLE-ONLY-SAME: "-target-cpu" "sh4"
// SINGLE-ONLY-SAME: "-target-feature" "+st40-300"
// SINGLE-ONLY-SAME: "-target-feature" "+single-fpu"
// SINGLE-ONLY-SAME: "-target-feature" "-double-fpu"
// SINGLE-ONLY-SAME: "-target-feature" "+fpu-single-mode"
// SINGLE-ONLY-SAME: "-tune-cpu" "st40-300"
// SINGLE-ONLY-SAME: "-target-abi" "gnu-e-float"

// RUN: %clang -### -target shle-unknown-none-elf -m4-300 -m3 -c %s 2>&1 | FileCheck %s --check-prefix=SH3
// RUN: %clang -### -target shle-unknown-none-elf -m4-100 -m4-300 -c %s 2>&1 | FileCheck %s --check-prefix=FULL
// RUN: %clang -### -target shle-unknown-none-elf -m4-300 -m4-100 -c %s 2>&1 | FileCheck %s --check-prefix=ST100
// RUN: %clang -### -target shle-unknown-none-elf -m4-200 -m4-300-single -c %s 2>&1 | FileCheck %s --check-prefix=SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -m4-300-single-only -mcpu=sh4 -c %s 2>&1 | FileCheck %s --check-prefix=GENERIC

// SH3: "-target-cpu" "sh3"
// SH3-NOT: "-target-feature" "+st40-300"
// SH3-NOT: "-tune-cpu"
// ST100: "-target-cpu" "sh4"
// ST100-NOT: "-target-feature" "+st40-300"
// ST100-SAME: "-tune-cpu" "st40-100"
// GENERIC: "-target-cpu" "sh4"
// GENERIC-NOT: "-target-feature" "+st40-300"
// GENERIC-NOT: "-tune-cpu"

// RUN: %clang -### -target shle-unknown-none-elf -m4-300 -mtune=st40-100 -c %s 2>&1 | FileCheck %s --check-prefix=TUNE100
// RUN: %clang -### -target shle-unknown-none-elf -m4-300 -mtune=st40-200 -c %s 2>&1 | FileCheck %s --check-prefix=TUNE200
// RUN: %clang -### -target shle-unknown-none-elf -m4 -mtune=st40-300 -c %s 2>&1 | FileCheck %s --check-prefix=TUNE300-ONLY
// RUN: not %clang -### -target shle-unknown-none-elf -m3 -mtune=st40-300 -c %s 2>&1 | FileCheck %s --check-prefix=BAD-PAIR
// RUN: not %clang -### -target shle-unknown-none-elf -mcpu=st40-300 -c %s 2>&1 | FileCheck %s --check-prefix=BAD-CPU

// TUNE100: "-target-cpu" "sh4"
// TUNE100-SAME: "-target-feature" "+st40-300"
// TUNE100-SAME: "-tune-cpu" "st40-100"
// TUNE200: "-target-cpu" "sh4"
// TUNE200-SAME: "-target-feature" "+st40-300"
// TUNE200-SAME: "-tune-cpu" "st40-200"
// TUNE300-ONLY: "-target-cpu" "sh4"
// TUNE300-ONLY-NOT: "-target-feature" "+st40-300"
// TUNE300-ONLY-SAME: "-tune-cpu" "st40-300"
// BAD-PAIR: error: unsupported option '-mtune=st40-300' for target 'shle-unknown-none-elf'
// BAD-CPU: error: unsupported argument 'st40-300' to option '-mcpu='

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-300 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-FPU
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-300-nofpu -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-NOFPU
// RUN: env "PATH=" %clang -### -target sh-unknown-linux-gnu -m4-300-single -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-BE
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-300-single-only -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-FPU

// AS-FPU: "sh4-linux-gnu-as" "--little" "--isa=sh4a"
// AS-NOFPU: "sh4-linux-gnu-as" "--little" "--isa=sh4a-nofpu"
// AS-BE: "sh-linux-gnu-as" "--big" "--isa=sh4a"

int value;
