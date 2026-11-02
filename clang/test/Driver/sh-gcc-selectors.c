// RUN: %clang -### -target shle-unknown-none-elf -m1 -c %s 2>&1 | FileCheck %s --check-prefix=M1
// RUN: %clang -### -target shle-unknown-none-elf -m2 -c %s 2>&1 | FileCheck %s --check-prefix=M2
// RUN: %clang -### -target shle-unknown-none-elf -m2e -c %s 2>&1 | FileCheck %s --check-prefix=M2E
// RUN: %clang -### -target shle-unknown-none-elf -m3 -c %s 2>&1 | FileCheck %s --check-prefix=M3
// RUN: %clang -### -target shle-unknown-none-elf -m3e -c %s 2>&1 | FileCheck %s --check-prefix=M3E
// RUN: %clang -### -target shle-unknown-none-elf -m4 -c %s 2>&1 | FileCheck %s --check-prefix=M4
// RUN: %clang -### -target shle-unknown-none-elf -m4a -c %s 2>&1 | FileCheck %s --check-prefix=M4A
// RUN: %clang -### -target shle-unknown-none-elf -m4al -c %s 2>&1 | FileCheck %s --check-prefix=M4AL
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh4al-dsp -c %s 2>&1 | FileCheck %s --check-prefix=M4AL

// M1: "-cc1"
// M1-SAME: "-target-cpu" "sh1"
// M1-SAME: "-msoft-float"
// M1-SAME: "-mfloat-abi" "soft"
// M1-SAME: "-no-implicit-float"
// M2: "-cc1"
// M2-SAME: "-target-cpu" "sh2"
// M2-SAME: "-msoft-float"
// M2-SAME: "-mfloat-abi" "soft"
// M2-SAME: "-no-implicit-float"
// M2E: "-cc1"
// M2E-SAME: "-target-cpu" "sh2e"
// M2E-SAME: "-target-abi" "gnu-e-float"
// M2E-SAME: "-mfloat-abi" "hard"
// M3: "-cc1"
// M3-SAME: "-target-cpu" "sh3"
// M3-SAME: "-msoft-float"
// M3-SAME: "-mfloat-abi" "soft"
// M3-SAME: "-no-implicit-float"
// M3E: "-cc1"
// M3E-SAME: "-target-cpu" "sh3e"
// M3E-SAME: "-target-abi" "gnu-e-float"
// M3E-SAME: "-mfloat-abi" "hard"
// M4: "-cc1"
// M4-SAME: "-target-cpu" "sh4"
// M4-SAME: "-target-abi" "gnu-hard-float"
// M4-SAME: "-mfloat-abi" "hard"
// M4-NOT: "-msoft-float"
// M4-NOT: "-no-implicit-float"
// M4A: "-cc1"
// M4A-SAME: "-target-cpu" "sh4a"
// M4A-SAME: "-target-abi" "gnu-hard-float"
// M4A-SAME: "-mfloat-abi" "hard"
// M4A-NOT: "-msoft-float"
// M4AL: "-cc1"
// M4AL-SAME: "-target-cpu" "sh4al-dsp"
// M4AL-SAME: "-msoft-float"
// M4AL-SAME: "-mfloat-abi" "soft"
// M4AL-SAME: "-no-implicit-float"
// M4AL-NOT: "-target-abi" "gnu-hard-float"

// RUN: %clang -### -target shle-unknown-none-elf -m4-nofpu -c %s 2>&1 | FileCheck %s --check-prefix=M4-NOFPU
// RUN: %clang -### -target shle-unknown-none-elf -m4-single -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -m4-single-only -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE-ONLY

// M4-NOFPU: "-cc1"
// M4-NOFPU-SAME: "-target-cpu" "sh4"
// M4-NOFPU-SAME: "-target-feature" "-single-fpu"
// M4-NOFPU-SAME: "-target-feature" "-double-fpu"
// M4-NOFPU-SAME: "-target-feature" "-fpu-single-mode"
// M4-NOFPU-SAME: "-msoft-float"
// M4-NOFPU-SAME: "-mfloat-abi" "soft"
// M4-NOFPU-SAME: "-no-implicit-float"
// M4-SINGLE: "-cc1"
// M4-SINGLE-SAME: "-target-cpu" "sh4"
// M4-SINGLE-SAME: "-target-feature" "+double-fpu"
// M4-SINGLE-SAME: "-target-feature" "+fpu-single-mode"
// M4-SINGLE-SAME: "-target-abi" "gnu-hard-float-single"
// M4-SINGLE-SAME: "-mfloat-abi" "hard"
// M4-SINGLE-ONLY: "-cc1"
// M4-SINGLE-ONLY-SAME: "-target-cpu" "sh4"
// M4-SINGLE-ONLY-SAME: "-target-feature" "+single-fpu"
// M4-SINGLE-ONLY-SAME: "-target-feature" "+fpu-size-mode"
// M4-SINGLE-ONLY-SAME: "-target-feature" "-double-fpu"
// M4-SINGLE-ONLY-SAME: "-target-feature" "+fpu-register-bank"
// M4-SINGLE-ONLY-SAME: "-target-feature" "+fpu-vector"
// M4-SINGLE-ONLY-SAME: "-target-feature" "+fpu-single-mode"
// M4-SINGLE-ONLY-SAME: "-target-abi" "gnu-e-float"
// M4-SINGLE-ONLY-SAME: "-mfloat-abi" "hard"

// RUN: %clang -### -target shle-unknown-none-elf -m2a -c %s 2>&1 | FileCheck %s --check-prefix=M2A
// RUN: %clang -### -target shle-unknown-none-elf -m2a-nofpu -c %s 2>&1 | FileCheck %s --check-prefix=M2A-NOFPU
// RUN: %clang -### -target shle-unknown-none-elf -m2a-single -c %s 2>&1 | FileCheck %s --check-prefix=M2A-SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -m2a-single-only -c %s 2>&1 | FileCheck %s --check-prefix=M2A-SINGLE-ONLY
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh2a -c %s 2>&1 | FileCheck %s --check-prefix=M2A

// M2A: "-cc1"
// M2A-SAME: "-target-cpu" "sh2a"
// M2A-SAME: "-target-abi" "gnu-hard-float"
// M2A-SAME: "-mfloat-abi" "hard"
// M2A-NOT: "-msoft-float"
// M2A-NOT: "-no-implicit-float"
// M2A-NOFPU: "-cc1"
// M2A-NOFPU-SAME: "-target-cpu" "sh2a"
// M2A-NOFPU-SAME: "-target-feature" "-single-fpu"
// M2A-NOFPU-SAME: "-target-feature" "-double-fpu"
// M2A-NOFPU-SAME: "-target-feature" "-fpu-single-mode"
// M2A-NOFPU-SAME: "-target-abi" "gnu"
// M2A-NOFPU-SAME: "-msoft-float"
// M2A-NOFPU-SAME: "-mfloat-abi" "soft"
// M2A-NOFPU-SAME: "-no-implicit-float"
// M2A-SINGLE: "-cc1"
// M2A-SINGLE-SAME: "-target-cpu" "sh2a"
// M2A-SINGLE-SAME: "-target-feature" "+single-fpu"
// M2A-SINGLE-SAME: "-target-feature" "+double-fpu"
// M2A-SINGLE-SAME: "-target-feature" "-fpu-register-bank"
// M2A-SINGLE-SAME: "-target-feature" "-fpu-vector"
// M2A-SINGLE-SAME: "-target-feature" "+fpu-single-mode"
// M2A-SINGLE-SAME: "-target-abi" "gnu-hard-float-single"
// M2A-SINGLE-SAME: "-mfloat-abi" "hard"
// M2A-SINGLE-ONLY: "-cc1"
// M2A-SINGLE-ONLY-SAME: "-target-cpu" "sh2a"
// M2A-SINGLE-ONLY-SAME: "-target-feature" "+single-fpu"
// M2A-SINGLE-ONLY-SAME: "-target-feature" "-double-fpu"
// M2A-SINGLE-ONLY-SAME: "-target-feature" "-fpu-register-bank"
// M2A-SINGLE-ONLY-SAME: "-target-feature" "-fpu-vector"
// M2A-SINGLE-ONLY-SAME: "-target-feature" "+fpu-single-mode"
// M2A-SINGLE-ONLY-SAME: "-target-abi" "gnu-e-float"
// M2A-SINGLE-ONLY-SAME: "-mfloat-abi" "hard"

// RUN: %clang -### -target shle-unknown-none-elf -m4 -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=M4-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -mfloat-abi=soft -m4 -c %s 2>&1 | FileCheck %s --check-prefix=M4-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m4 -msoft-float -c %s 2>&1 | FileCheck %s --check-prefix=M4-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -msoft-float -m4 -c %s 2>&1 | FileCheck %s --check-prefix=M4-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m4 -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=M4
// RUN: %clang -### -target shle-unknown-none-elf -m4 -mhard-float -c %s 2>&1 | FileCheck %s --check-prefix=M4
// RUN: %clang -### -target shle-unknown-none-elf -m4 -mno-soft-float -c %s 2>&1 | FileCheck %s --check-prefix=M4

// M4-SOFT: "-cc1"
// M4-SOFT-SAME: "-target-cpu" "sh4"
// M4-SOFT-SAME: "-msoft-float"
// M4-SOFT-SAME: "-mfloat-abi" "soft"
// M4-SOFT-SAME: "-no-implicit-float"
// M4-SOFT-NOT: "-target-abi" "gnu-hard-float"

// RUN: not %clang -### -target shle-unknown-none-elf -m2 -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=HARD-SH2
// RUN: not %clang -### -target shle-unknown-none-elf -m1 -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=HARD-SH1
// RUN: not %clang -### -target shle-unknown-none-elf -m3 -mhard-float -c %s 2>&1 | FileCheck %s --check-prefix=HARD-SH3
// HARD-SH1: error: unsupported option '-mfloat-abi=hard' for target 'shle-unknown-none-elf'
// HARD-SH2: error: unsupported option '-mfloat-abi=hard' for target 'shle-unknown-none-elf'
// HARD-SH3: error: unsupported option '-mhard-float' for target 'shle-unknown-none-elf'

// RUN: %clang -### -target shle-unknown-none-elf -m1 -m2 -c %s 2>&1 | FileCheck %s --check-prefix=M2
// RUN: %clang -### -target shle-unknown-none-elf -m2 -m1 -c %s 2>&1 | FileCheck %s --check-prefix=M1
// RUN: %clang -### -target shle-unknown-none-elf -m1 -m2e -c %s 2>&1 | FileCheck %s --check-prefix=M2E
// RUN: %clang -### -target shle-unknown-none-elf -m3 -m1 -c %s 2>&1 | FileCheck %s --check-prefix=M1
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh1 -m3 -c %s 2>&1 | FileCheck %s --check-prefix=M3
// RUN: %clang -### -target shle-unknown-none-elf -m4 -mcpu=sh1 -c %s 2>&1 | FileCheck %s --check-prefix=M1
// RUN: %clang -### -target shle-unknown-none-elf -m2 -m3 -c %s 2>&1 | FileCheck %s --check-prefix=M3
// RUN: %clang -### -target shle-unknown-none-elf -m3 -m2 -c %s 2>&1 | FileCheck %s --check-prefix=M2
// RUN: %clang -### -target shle-unknown-none-elf -m4 -mcpu=sh3 -c %s 2>&1 | FileCheck %s --check-prefix=M3
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh3 -m4 -c %s 2>&1 | FileCheck %s --check-prefix=M4
// RUN: %clang -### -target shle-unknown-none-elf -m4 -m4-nofpu -c %s 2>&1 | FileCheck %s --check-prefix=M4-NOFPU
// RUN: %clang -### -target shle-unknown-none-elf -m4-nofpu -m4 -c %s 2>&1 | FileCheck %s --check-prefix=M4
// RUN: %clang -### -target shle-unknown-none-elf -m4-single -m4-single-only -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE-ONLY
// RUN: %clang -### -target shle-unknown-none-elf -m4-single-only -m4-single -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh3 -m4-single -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -m4-single -mcpu=sh3 -c %s 2>&1 | FileCheck %s --check-prefix=M3
// RUN: %clang -### -target shle-unknown-none-elf -m2 -m2e -c %s 2>&1 | FileCheck %s --check-prefix=M2E
// RUN: %clang -### -target shle-unknown-none-elf -m2e -m2 -c %s 2>&1 | FileCheck %s --check-prefix=M2
// RUN: %clang -### -target shle-unknown-none-elf -m3 -m3e -c %s 2>&1 | FileCheck %s --check-prefix=M3E
// RUN: %clang -### -target shle-unknown-none-elf -m3e -m3 -c %s 2>&1 | FileCheck %s --check-prefix=M3
// RUN: %clang -### -target shle-unknown-none-elf -m2e -m3e -c %s 2>&1 | FileCheck %s --check-prefix=M3E
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh2e -m3 -c %s 2>&1 | FileCheck %s --check-prefix=M3
// RUN: %clang -### -target shle-unknown-none-elf -m3e -mcpu=sh2 -c %s 2>&1 | FileCheck %s --check-prefix=M2
// RUN: %clang -### -target shle-unknown-none-elf -m2 -m2a -c %s 2>&1 | FileCheck %s --check-prefix=M2A
// RUN: %clang -### -target shle-unknown-none-elf -m2a -m2 -c %s 2>&1 | FileCheck %s --check-prefix=M2
// RUN: %clang -### -target shle-unknown-none-elf -m2e -m2a -c %s 2>&1 | FileCheck %s --check-prefix=M2A
// RUN: %clang -### -target shle-unknown-none-elf -m2a -m3 -c %s 2>&1 | FileCheck %s --check-prefix=M3
// RUN: %clang -### -target shle-unknown-none-elf -m4-300 -m2a -c %s 2>&1 | FileCheck %s --check-prefix=M2A
// RUN: %clang -### -target shle-unknown-none-elf -m2a -m4-400 -c %s 2>&1 | FileCheck %s --check-prefix=M4-400
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh2a -m3 -c %s 2>&1 | FileCheck %s --check-prefix=M3
// RUN: %clang -### -target shle-unknown-none-elf -m2 -mtune=sh2a -c %s 2>&1 | FileCheck %s --check-prefix=TUNE-SH2A
// RUN: %clang -### -target shle-unknown-none-elf -m4 -m4al -c %s 2>&1 | FileCheck %s --check-prefix=M4AL
// RUN: %clang -### -target shle-unknown-none-elf -m4a -m4al -c %s 2>&1 | FileCheck %s --check-prefix=M4AL
// RUN: %clang -### -target shle-unknown-none-elf -m4al -m4a -c %s 2>&1 | FileCheck %s --check-prefix=M4A
// RUN: %clang -### -target shle-unknown-none-elf -m4-300 -m4al -c %s 2>&1 | FileCheck %s --check-prefix=M4AL
// RUN: %clang -### -target shle-unknown-none-elf -m4al -m4-400 -c %s 2>&1 | FileCheck %s --check-prefix=M4-400
// RUN: %clang -### -target shle-unknown-none-elf -m4al -m3 -c %s 2>&1 | FileCheck %s --check-prefix=M3
// RUN: %clang -### -target shle-unknown-none-elf -m3 -mtune=sh4al-dsp -c %s 2>&1 | FileCheck %s --check-prefix=TUNE-SH4AL
// M4-400: "-target-cpu" "sh4"
// M4-400-SAME: "-target-feature" "-mmu"
// M4-400-SAME: "-target-feature" "-single-fpu"
// M4-400-SAME: "-tune-cpu" "st40-400"
// M4-400-SAME: "-mfloat-abi" "soft"
// TUNE-SH2A: "-cc1"
// TUNE-SH2A-SAME: "-target-cpu" "sh2"
// TUNE-SH2A-SAME: "-tune-cpu" "sh2a"
// TUNE-SH2A-NOT: "-target-feature" "+sh2a"
// TUNE-SH4AL: "-cc1"
// TUNE-SH4AL-SAME: "-target-cpu" "sh3"
// TUNE-SH4AL-SAME: "-tune-cpu" "sh4al-dsp"
// TUNE-SH4AL-NOT: "-target-feature" "+dsp"
// TUNE-SH4AL-NOT: "-target-feature" "+sh4al-dsp"

// RUN: not %clang -### -target shle-unknown-none-elf -m4-nofpu -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=HARD-NOFPU
// RUN: not %clang -### -target shle-unknown-none-elf -m4-nofpu -mhard-float -c %s 2>&1 | FileCheck %s --check-prefix=HARD-NOFPU
// RUN: not %clang -### -target shle-unknown-none-elf -m4-nofpu -mno-soft-float -c %s 2>&1 | FileCheck %s --check-prefix=HARD-NOFPU
// HARD-NOFPU: error: unsupported option '{{-mfloat-abi=hard|-mhard-float|-mno-soft-float}}' for target 'shle-unknown-none-elf'
// RUN: %clang -### -target shle-unknown-none-elf -m4-nofpu -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=M4-NOFPU
// RUN: %clang -### -target shle-unknown-none-elf -m4-nofpu -msoft-float -c %s 2>&1 | FileCheck %s --check-prefix=M4-NOFPU
// RUN: %clang -### -target shle-unknown-none-elf -m4-single -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m4-single -msoft-float -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m4-single-only -msoft-float -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE-ONLY-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m4-single-only -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE-ONLY-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m4-single -mhard-float -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -m4-single -mno-soft-float -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -m4-single -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE
// RUN: %clang -### -target shle-unknown-none-elf -m4-single-only -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE-ONLY
// RUN: %clang -### -target shle-unknown-none-elf -m4-single-only -mhard-float -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE-ONLY
// RUN: %clang -### -target shle-unknown-none-elf -m4-single-only -mno-soft-float -c %s 2>&1 | FileCheck %s --check-prefix=M4-SINGLE-ONLY
// M4-SINGLE-SOFT: "-target-cpu" "sh4"
// M4-SINGLE-SOFT-SAME: "-target-feature" "+fpu-single-mode"
// M4-SINGLE-SOFT-SAME: "-mfloat-abi" "soft"
// M4-SINGLE-SOFT-NOT: "-target-abi" "gnu-hard-float-single"
// M4-SINGLE-ONLY-SOFT: "-target-cpu" "sh4"
// M4-SINGLE-ONLY-SOFT-SAME: "-target-feature" "+fpu-single-mode"
// M4-SINGLE-ONLY-SOFT-SAME: "-mfloat-abi" "soft"
// M4-SINGLE-ONLY-SOFT-NOT: "-target-abi" "gnu-e-float"
// RUN: %clang -### -target shle-unknown-none-elf -m2e -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=M2E-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m2e -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=M2E
// RUN: %clang -### -target shle-unknown-none-elf -m3e -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=M3E-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m3e -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=M3E
// M2E-SOFT: "-target-cpu" "sh2e"
// M2E-SOFT-SAME: "-target-abi" "gnu"
// M2E-SOFT-SAME: "-mfloat-abi" "soft"
// M3E-SOFT: "-target-cpu" "sh3e"
// M3E-SOFT-SAME: "-target-abi" "gnu"
// M3E-SOFT-SAME: "-mfloat-abi" "soft"
// RUN: %clang -### -target shle-unknown-none-elf -m2a -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=M2A-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m2a-single -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=M2A-SINGLE-SOFT
// RUN: %clang -### -target shle-unknown-none-elf -m2a-single-only -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=M2A-SINGLE-ONLY-SOFT
// RUN: not %clang -### -target shle-unknown-none-elf -m2a-nofpu -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=HARD-NOFPU
// M2A-SOFT: "-target-cpu" "sh2a"
// M2A-SOFT-SAME: "-target-abi" "gnu"
// M2A-SOFT-SAME: "-mfloat-abi" "soft"
// M2A-SINGLE-SOFT: "-target-cpu" "sh2a"
// M2A-SINGLE-SOFT-SAME: "-target-feature" "+fpu-single-mode"
// M2A-SINGLE-SOFT-SAME: "-target-abi" "gnu"
// M2A-SINGLE-SOFT-SAME: "-mfloat-abi" "soft"
// M2A-SINGLE-ONLY-SOFT: "-target-cpu" "sh2a"
// M2A-SINGLE-ONLY-SOFT-SAME: "-target-feature" "+fpu-single-mode"
// M2A-SINGLE-ONLY-SOFT-SAME: "-target-abi" "gnu"
// M2A-SINGLE-ONLY-SOFT-SAME: "-mfloat-abi" "soft"

// RUN: not %clang -### -target shle-unknown-none-elf -m4al -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=HARD-SH4AL
// RUN: not %clang -### -target shle-unknown-none-elf -m4al -mhard-float -c %s 2>&1 | FileCheck %s --check-prefix=HARD-SH4AL
// RUN: %clang -### -target shle-unknown-none-elf -m4al -mfloat-abi=soft -c %s 2>&1 | FileCheck %s --check-prefix=M4AL
// RUN: %clang -### -target shle-unknown-none-elf -m4al -msoft-float -c %s 2>&1 | FileCheck %s --check-prefix=M4AL
// HARD-SH4AL: error: unsupported option '{{-mfloat-abi=hard|-mhard-float}}' for target 'shle-unknown-none-elf'

// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh1 -c %s 2>&1 | FileCheck %s --check-prefix=M1
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh2e -c %s 2>&1 | FileCheck %s --check-prefix=M2E
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh3e -c %s 2>&1 | FileCheck %s --check-prefix=M3E

// RUN: %clang --target=shle-unknown-none-elf -mcpu=help 2>&1 | FileCheck %s --check-prefix=CPU-HELP
// CPU-HELP: Available CPUs for this target:
// CPU-HELP: {{^ +}}sh1
// CPU-HELP-NEXT: {{^ +}}sh2
// CPU-HELP-NEXT: {{^ +}}sh2e
// CPU-HELP-NEXT: {{^ +}}sh2a
// CPU-HELP-NEXT: {{^ +}}sh3
// CPU-HELP-NEXT: {{^ +}}sh3e
// CPU-HELP-NEXT: {{^ +}}sh4
// CPU-HELP-NEXT: {{^ +}}sh4a
// CPU-HELP-NEXT: {{^ +}}sh4al-dsp
// CPU-HELP-NOT: sh4-nofpu
// CPU-HELP-NOT: sh4-single
// CPU-HELP-NOT: st40

// RUN: %clang -### -target shle-unknown-none-elf -m4al -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=EXTERNAL-LE
// RUN: %clang -### -target sh-unknown-none-elf -m4al -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=EXTERNAL-BE
// EXTERNAL-LE: "sh-elf-as" "--little" "--isa=sh4al-dsp"
// EXTERNAL-BE: "sh-elf-as" "--big" "--isa=sh4al-dsp"

// RUN: not %clang -### -target x86_64-unknown-linux-gnu -m4 -c %s 2>&1 | FileCheck %s --check-prefix=X86
// RUN: not %clang -### -target aarch64-unknown-linux-gnu -m4 -c %s 2>&1 | FileCheck %s --check-prefix=AARCH64
// X86: error: unsupported option '-m4' for target 'x86_64-unknown-linux-gnu'
// AARCH64: error: unsupported option '-m4' for target 'aarch64-unknown-linux-gnu'

int value;
