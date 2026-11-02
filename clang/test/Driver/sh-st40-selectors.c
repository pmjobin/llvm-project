// RUN: %clang -### -target shle-unknown-none-elf -m4-100 -c %s 2>&1 | FileCheck %s --check-prefix=ST100-FULL
// RUN: %clang -### -target shle-unknown-none-elf -m4-100-nofpu -c %s 2>&1 | FileCheck %s --check-prefix=ST100-NOFPU
// RUN: %clang -### -target shle-unknown-none-elf -m4-100-single -c %s 2>&1 | FileCheck %s --check-prefix=ST100-SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -m4-100-single-only -c %s 2>&1 | FileCheck %s --check-prefix=ST100-SINGLE-ONLY
// RUN: %clang -### -target shle-unknown-none-elf -m4-200 -c %s 2>&1 | FileCheck %s --check-prefix=ST200-FULL
// RUN: %clang -### -target shle-unknown-none-elf -m4-200-nofpu -c %s 2>&1 | FileCheck %s --check-prefix=ST200-NOFPU
// RUN: %clang -### -target shle-unknown-none-elf -m4-200-single -c %s 2>&1 | FileCheck %s --check-prefix=ST200-SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -m4-200-single-only -c %s 2>&1 | FileCheck %s --check-prefix=ST200-SINGLE-ONLY

// ST100-FULL: "-cc1"
// ST100-FULL-SAME: "-target-cpu" "sh4"
// ST100-FULL-SAME: "-tune-cpu" "st40-100"
// ST100-FULL-SAME: "-D__SH4_100__"
// ST100-FULL-SAME: "-target-abi" "gnu-hard-float"
// ST100-FULL-SAME: "-mfloat-abi" "hard"
// ST100-NOFPU: "-cc1"
// ST100-NOFPU-SAME: "-target-cpu" "sh4"
// ST100-NOFPU-SAME: "-target-feature" "-single-fpu"
// ST100-NOFPU-SAME: "-target-feature" "-double-fpu"
// ST100-NOFPU-SAME: "-target-feature" "-fpu-single-mode"
// ST100-NOFPU-SAME: "-tune-cpu" "st40-100"
// ST100-NOFPU-SAME: "-D__SH4_100__"
// ST100-NOFPU-SAME: "-mfloat-abi" "soft"
// ST100-SINGLE: "-cc1"
// ST100-SINGLE-SAME: "-target-cpu" "sh4"
// ST100-SINGLE-SAME: "-target-feature" "+double-fpu"
// ST100-SINGLE-SAME: "-target-feature" "+fpu-single-mode"
// ST100-SINGLE-SAME: "-tune-cpu" "st40-100"
// ST100-SINGLE-SAME: "-D__SH4_100__"
// ST100-SINGLE-SAME: "-target-abi" "gnu-hard-float-single"
// ST100-SINGLE-ONLY: "-cc1"
// ST100-SINGLE-ONLY-SAME: "-target-cpu" "sh4"
// ST100-SINGLE-ONLY-SAME: "-target-feature" "+single-fpu"
// ST100-SINGLE-ONLY-SAME: "-target-feature" "-double-fpu"
// ST100-SINGLE-ONLY-SAME: "-target-feature" "+fpu-single-mode"
// ST100-SINGLE-ONLY-SAME: "-tune-cpu" "st40-100"
// ST100-SINGLE-ONLY-SAME: "-D__SH4_100__"
// ST100-SINGLE-ONLY-SAME: "-target-abi" "gnu-e-float"

// ST200-FULL: "-cc1"
// ST200-FULL-SAME: "-target-cpu" "sh4"
// ST200-FULL-SAME: "-tune-cpu" "st40-200"
// ST200-FULL-SAME: "-D__SH4_200__"
// ST200-FULL-SAME: "-target-abi" "gnu-hard-float"
// ST200-FULL-SAME: "-mfloat-abi" "hard"
// ST200-NOFPU: "-cc1"
// ST200-NOFPU-SAME: "-target-cpu" "sh4"
// ST200-NOFPU-SAME: "-target-feature" "-single-fpu"
// ST200-NOFPU-SAME: "-target-feature" "-double-fpu"
// ST200-NOFPU-SAME: "-target-feature" "-fpu-single-mode"
// ST200-NOFPU-SAME: "-tune-cpu" "st40-200"
// ST200-NOFPU-SAME: "-D__SH4_200__"
// ST200-NOFPU-SAME: "-mfloat-abi" "soft"
// ST200-SINGLE: "-cc1"
// ST200-SINGLE-SAME: "-target-cpu" "sh4"
// ST200-SINGLE-SAME: "-target-feature" "+double-fpu"
// ST200-SINGLE-SAME: "-target-feature" "+fpu-single-mode"
// ST200-SINGLE-SAME: "-tune-cpu" "st40-200"
// ST200-SINGLE-SAME: "-D__SH4_200__"
// ST200-SINGLE-SAME: "-target-abi" "gnu-hard-float-single"
// ST200-SINGLE-ONLY: "-cc1"
// ST200-SINGLE-ONLY-SAME: "-target-cpu" "sh4"
// ST200-SINGLE-ONLY-SAME: "-target-feature" "+single-fpu"
// ST200-SINGLE-ONLY-SAME: "-target-feature" "-double-fpu"
// ST200-SINGLE-ONLY-SAME: "-target-feature" "+fpu-single-mode"
// ST200-SINGLE-ONLY-SAME: "-tune-cpu" "st40-200"
// ST200-SINGLE-ONLY-SAME: "-D__SH4_200__"
// ST200-SINGLE-ONLY-SAME: "-target-abi" "gnu-e-float"

// RUN: %clang -### -target shle-unknown-none-elf -m4-100 -m4-200 -c %s 2>&1 | FileCheck %s --check-prefix=ST200-FULL
// RUN: %clang -### -target shle-unknown-none-elf -m4-200 -m4-100 -c %s 2>&1 | FileCheck %s --check-prefix=ST100-FULL
// RUN: %clang -### -target shle-unknown-none-elf -m4 -m4-100 -c %s 2>&1 | FileCheck %s --check-prefix=ST100-FULL
// RUN: %clang -### -target shle-unknown-none-elf -m4-100 -m4 -c %s 2>&1 | FileCheck %s --check-prefix=GENERIC-SH4
// RUN: %clang -### -target shle-unknown-none-elf -m4-200 -m3 -c %s 2>&1 | FileCheck %s --check-prefix=GENERIC-SH3
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh3 -m4-100 -c %s 2>&1 | FileCheck %s --check-prefix=ST100-FULL
// RUN: %clang -### -target shle-unknown-none-elf -m4-100 -mcpu=sh3 -c %s 2>&1 | FileCheck %s --check-prefix=GENERIC-SH3

// GENERIC-SH4: "-cc1"
// GENERIC-SH4-SAME: "-target-cpu" "sh4"
// GENERIC-SH4-NOT: "-tune-cpu"
// GENERIC-SH4-NOT: "-D__SH4_"
// GENERIC-SH3: "-cc1"
// GENERIC-SH3-SAME: "-target-cpu" "sh3"
// GENERIC-SH3-NOT: "-tune-cpu"
// GENERIC-SH3-NOT: "-D__SH4_"

// RUN: %clang -### -target shle-unknown-none-elf -m4 -mtune=st40-100 -c %s 2>&1 | FileCheck %s --check-prefix=TUNE100
// RUN: %clang -### -target shle-unknown-none-elf -m4-100 -mtune=st40-200 -c %s 2>&1 | FileCheck %s --check-prefix=TUNE200-SELECTOR100
// RUN: not %clang -### -target shle-unknown-none-elf -m3 -mtune=st40-100 -c %s 2>&1 | FileCheck %s --check-prefix=BAD-PAIR
// RUN: not %clang -### -target shle-unknown-none-elf -m4 -mtune=not-a-cpu -c %s 2>&1 | FileCheck %s --check-prefix=BAD-TUNE
// RUN: not %clang -### -target shle-unknown-none-elf -mcpu=st40-100 -c %s 2>&1 | FileCheck %s --check-prefix=BAD-CPU

// TUNE100: "-target-cpu" "sh4"
// TUNE100-SAME: "-tune-cpu" "st40-100"
// TUNE100-NOT: "-D__SH4_100__"
// TUNE200-SELECTOR100: "-target-cpu" "sh4"
// TUNE200-SELECTOR100-SAME: "-tune-cpu" "st40-200"
// TUNE200-SELECTOR100-SAME: "-D__SH4_100__"
// BAD-PAIR: error: unsupported option '-mtune=st40-100' for target 'shle-unknown-none-elf'
// BAD-TUNE: error: unsupported argument 'not-a-cpu' to option '-mtune='
// BAD-CPU: error: unsupported argument 'st40-100' to option '-mcpu='

// RUN: %clang -### -target shle-unknown-none-elf -m4-100 -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=ST100-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m4-100 -msoft-float -c %s 2>&1 | FileCheck %s --check-prefix=ST100-SOFT
// RUN: not %clang -### -target shle-unknown-none-elf -m4-100-nofpu -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=HARD-NOFPU
// RUN: not %clang -### -target shle-unknown-none-elf -m4-100-nofpu -mhard-float -c %s 2>&1 | FileCheck %s --check-prefix=HARD-NOFPU
// RUN: not %clang -### -target shle-unknown-none-elf -m4-100-nofpu -mno-soft-float -c %s 2>&1 | FileCheck %s --check-prefix=HARD-NOFPU
// RUN: %clang -### -target shle-unknown-none-elf -m4-200-single -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=ST200-SINGLE-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m4-200-single -msoft-float -c %s 2>&1 | FileCheck %s --check-prefix=ST200-SINGLE-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m4-200-single-only -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=ST200-SINGLE-ONLY-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m4-200-single-only -msoft-float -c %s 2>&1 | FileCheck %s --check-prefix=ST200-SINGLE-ONLY-SOFT

// ST100-SOFT: "-target-cpu" "sh4"
// ST100-SOFT-SAME: "-tune-cpu" "st40-100"
// ST100-SOFT-SAME: "-mfloat-abi" "soft"
// ST100-SOFT-NOT: "-target-abi" "gnu-hard-float"
// HARD-NOFPU: error: unsupported option '{{-mfloat-abi=hard|-mhard-float|-mno-soft-float}}' for target 'shle-unknown-none-elf'
// ST200-SINGLE-SOFT: "-target-cpu" "sh4"
// ST200-SINGLE-SOFT-SAME: "-tune-cpu" "st40-200"
// ST200-SINGLE-SOFT-SAME: "-mfloat-abi" "soft"
// ST200-SINGLE-SOFT-NOT: "-target-abi" "gnu-hard-float-single"
// ST200-SINGLE-ONLY-SOFT: "-target-cpu" "sh4"
// ST200-SINGLE-ONLY-SOFT-SAME: "-tune-cpu" "st40-200"
// ST200-SINGLE-ONLY-SOFT-SAME: "-mfloat-abi" "soft"
// ST200-SINGLE-ONLY-SOFT-NOT: "-target-abi" "gnu-e-float"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-100 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-SH4
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-100-nofpu -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-SH4-NOFPU
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-100-single -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-SH4
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-100-single-only -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-SH4
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-200 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-SH4
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-200-nofpu -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-SH4-NOFPU
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-200-single -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-SH4
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-200-single-only -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-SH4

// AS-SH4: "sh4-linux-gnu-as" "--little" "--isa=sh4"
// AS-SH4-NOT: "--isa=sh4a"
// AS-SH4-NOFPU: "sh4-linux-gnu-as" "--little" "--isa=sh4-nofpu"
// AS-SH4-NOFPU-NOT: "--isa=sh4a"

int value;
