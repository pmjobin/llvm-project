// RUN: %clang -### -target shle-unknown-none-elf -m4a -c %s 2>&1 | FileCheck %s --check-prefix=FULL
// RUN: %clang -### -target shle-unknown-none-elf -m4a-nofpu -c %s 2>&1 | FileCheck %s --check-prefix=NOFPU
// RUN: %clang -### -target shle-unknown-none-elf -m4a-single -c %s 2>&1 | FileCheck %s --check-prefix=SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -m4a-single-only -c %s 2>&1 | FileCheck %s --check-prefix=SINGLE-ONLY
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh4a -c %s 2>&1 | FileCheck %s --check-prefix=CANONICAL

// FULL: "-cc1"
// FULL-SAME: "-target-cpu" "sh4a"
// FULL-NOT: "-target-feature" "+st40-300"
// FULL-SAME: "-target-abi" "gnu-hard-float"
// FULL-SAME: "-mfloat-abi" "hard"
// NOFPU: "-target-cpu" "sh4a"
// NOFPU-SAME: "-target-feature" "-single-fpu"
// NOFPU-SAME: "-target-feature" "-double-fpu"
// NOFPU-SAME: "-target-abi" "gnu"
// NOFPU-SAME: "-msoft-float"
// SINGLE: "-target-cpu" "sh4a"
// SINGLE-SAME: "-target-feature" "+double-fpu"
// SINGLE-SAME: "-target-feature" "+fpu-single-mode"
// SINGLE-SAME: "-target-abi" "gnu-hard-float-single"
// SINGLE-ONLY: "-target-cpu" "sh4a"
// SINGLE-ONLY-SAME: "-target-feature" "+single-fpu"
// SINGLE-ONLY-SAME: "-target-feature" "+fpu-size-mode"
// SINGLE-ONLY-SAME: "-target-feature" "-double-fpu"
// SINGLE-ONLY-SAME: "-target-feature" "+fpu-register-bank"
// SINGLE-ONLY-SAME: "-target-feature" "+fpu-vector"
// SINGLE-ONLY-SAME: "-target-feature" "+fpu-single-mode"
// SINGLE-ONLY-SAME: "-target-abi" "gnu-e-float"
// CANONICAL: "-target-cpu" "sh4a"
// CANONICAL-SAME: "-target-abi" "gnu"
// CANONICAL-SAME: "-msoft-float"

// RUN: %clang -### -target shle-unknown-none-elf -m4-300 -m4a -c %s 2>&1 | FileCheck %s --check-prefix=FULL
// RUN: %clang -### -target shle-unknown-none-elf -m4a -m4-400 -c %s 2>&1 | FileCheck %s --check-prefix=ST400
// RUN: %clang -### -target shle-unknown-none-elf -m2a -m4a -c %s 2>&1 | FileCheck %s --check-prefix=FULL
// RUN: %clang -### -target shle-unknown-none-elf -m4a -m3 -c %s 2>&1 | FileCheck %s --check-prefix=SH3
// RUN: %clang -### -target shle-unknown-none-elf -m4a -mcpu=sh2a -c %s 2>&1 | FileCheck %s --check-prefix=SH2A
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh4a -m4 -c %s 2>&1 | FileCheck %s --check-prefix=SH4

// ST400: "-target-cpu" "sh4"
// ST400-NOT: "-target-feature" "+st40-300"
// ST400-SAME: "-tune-cpu" "st40-400"
// SH3: "-target-cpu" "sh3"
// SH3-NOT: "-target-feature" "+sh4a"
// SH3-NOT: "-tune-cpu"
// SH2A: "-target-cpu" "sh2a"
// SH2A-NOT: "-target-feature" "+sh4a"
// SH4: "-target-cpu" "sh4"
// SH4-NOT: "-target-feature" "+sh4a"

// RUN: %clang -### -target shle-unknown-none-elf -m3 -mtune=sh4a -c %s 2>&1 | FileCheck %s --check-prefix=TUNE-ONLY
// RUN: not %clang -### -target shle-unknown-none-elf -m4a -mtune=st40-300 -c %s 2>&1 | FileCheck %s --check-prefix=BAD-TUNE
// RUN: not %clang -### -target shle-unknown-none-elf -m4a-nofpu -mhard-float -c %s 2>&1 | FileCheck %s --check-prefix=BAD-FLOAT

// TUNE-ONLY: "-target-cpu" "sh3"
// TUNE-ONLY-NOT: "-target-feature" "+sh4a"
// TUNE-ONLY-SAME: "-tune-cpu" "sh4a"
// BAD-TUNE: error: unsupported option '-mtune=st40-300' for target
// BAD-FLOAT: error: unsupported option '-mhard-float' for target

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4a -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-FPU
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4a-nofpu -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-NOFPU
// RUN: env "PATH=" %clang -### -target sh-unknown-linux-gnu -m4a-single -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-BE
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4a-single-only -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-FPU

// AS-FPU: "sh4-linux-gnu-as" "--little" "--isa=sh4a"
// AS-NOFPU: "sh4-linux-gnu-as" "--little" "--isa=sh4a-nofpu"
// AS-BE: "sh-linux-gnu-as" "--big" "--isa=sh4a"

int value;
