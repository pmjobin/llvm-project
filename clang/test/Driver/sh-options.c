// RUN: %clang -### -target shle-unknown-none-elf -c %s 2>&1 | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang -### -target sh-unknown-none-elf -mcpu=sh1 -mabi=gnu -msoft-float -c %s 2>&1 | FileCheck %s --check-prefix=SH1
// RUN: %clang -### -target sh-unknown-none-elf -mcpu=sh2 -mabi=gnu -msoft-float -c %s 2>&1 | FileCheck %s --check-prefix=EXPLICIT
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh2e -c %s 2>&1 | FileCheck %s --check-prefix=SH2E
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh2a -c %s 2>&1 | FileCheck %s --check-prefix=SH2A
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh3 -c %s 2>&1 | FileCheck %s --check-prefix=SH3
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh3e -c %s 2>&1 | FileCheck %s --check-prefix=SH3E
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh4 -c %s 2>&1 | FileCheck %s --check-prefix=SH4
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh4a -c %s 2>&1 | FileCheck %s --check-prefix=SH4A

// DEFAULT: "-cc1"
// DEFAULT-SAME: "-triple" "shle-unknown-none-elf"
// DEFAULT-SAME: "-target-cpu" "sh2"
// DEFAULT-SAME: "-msoft-float"
// DEFAULT-SAME: "-mfloat-abi" "soft"
// DEFAULT-SAME: "-no-implicit-float"

// SH1: "-cc1"
// SH1-SAME: "-triple" "sh-unknown-none-elf"
// SH1-SAME: "-target-cpu" "sh1"
// SH1-SAME: "-target-abi" "gnu"

// EXPLICIT: "-cc1"
// EXPLICIT-SAME: "-triple" "sh-unknown-none-elf"
// EXPLICIT-SAME: "-target-cpu" "sh2"
// EXPLICIT-SAME: "-target-abi" "gnu"

// SH2E: "-cc1"
// SH2E-SAME: "-target-cpu" "sh2e"
// SH2E-SAME: "-target-abi" "gnu-e-float"
// SH2E-SAME: "-mfloat-abi" "hard"

// SH2A: "-cc1"
// SH2A-SAME: "-target-cpu" "sh2a"
// SH2A-SAME: "-target-abi" "gnu-hard-float"
// SH2A-SAME: "-mfloat-abi" "hard"

// SH3: "-cc1"
// SH3-SAME: "-target-cpu" "sh3"
// SH3-SAME: "-msoft-float"
// SH3-SAME: "-mfloat-abi" "soft"
// SH3-SAME: "-no-implicit-float"

// SH3E: "-cc1"
// SH3E-SAME: "-target-cpu" "sh3e"
// SH3E-SAME: "-target-abi" "gnu-e-float"
// SH3E-SAME: "-mfloat-abi" "hard"

// SH4: "-cc1"
// SH4-SAME: "-target-cpu" "sh4"
// SH4-SAME: "-msoft-float"
// SH4-SAME: "-mfloat-abi" "soft"
// SH4-SAME: "-no-implicit-float"

// SH4A: "-cc1"
// SH4A-SAME: "-target-cpu" "sh4a"
// SH4A-SAME: "-target-abi" "gnu"
// SH4A-SAME: "-msoft-float"
// SH4A-SAME: "-mfloat-abi" "soft"
// SH4A-SAME: "-no-implicit-float"

// RUN: not %clang -### -target shle-unknown-none-elf -mcpu=misspelled -c %s 2>&1 | FileCheck %s --check-prefix=BAD-CPU-MISSPELLED
// BAD-CPU-MISSPELLED: error: unsupported argument 'misspelled' to option '-mcpu='

// RUN: not %clang -### -target shle-unknown-none-elf -mabi=fpu -c %s 2>&1 | FileCheck %s --check-prefix=BAD-ABI
// BAD-ABI: error: unsupported argument 'fpu' to option '-mabi='

// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh4 -mhard-float -c %s 2>&1 | FileCheck %s --check-prefix=HARD
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh4 -mno-soft-float -c %s 2>&1 | FileCheck %s --check-prefix=HARD
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh4 -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=HARD
// HARD: "-cc1"
// HARD-SAME: "-target-cpu" "sh4"
// HARD-SAME: "-target-abi" "gnu-hard-float"
// HARD-SAME: "-mfloat-abi" "hard"
// HARD-NOT: "-msoft-float"
// HARD-NOT: "-no-implicit-float"

// RUN: not %clang -### -target shle-unknown-none-elf -mcpu=sh2 -mfloat-abi=hard -c %s 2>&1 | FileCheck %s --check-prefix=HARD-SH2
// RUN: not %clang -### -target shle-unknown-none-elf -mcpu=sh3 -mhard-float -c %s 2>&1 | FileCheck %s --check-prefix=HARD-SH3
// HARD-SH2: error: unsupported option '-mfloat-abi=hard' for target 'shle-unknown-none-elf'
// HARD-SH3: error: unsupported option '-mhard-float' for target 'shle-unknown-none-elf'

// RUN: %clang -### -target shle-unknown-none-elf -mhard-float -msoft-float -c %s 2>&1 | FileCheck %s --check-prefix=SOFT-LAST
// RUN: %clang -### -target shle-unknown-none-elf -mcpu=sh4 -msoft-float -mhard-float -c %s 2>&1 | FileCheck %s --check-prefix=HARD-LAST
// SOFT-LAST: "-mfloat-abi" "soft"
// SOFT-LAST-NOT: error:
// HARD-LAST: "-target-abi" "gnu-hard-float"
// HARD-LAST-SAME: "-mfloat-abi" "hard"

// RUN: %clang -### -target shle-unknown-none-elf -mbig-endian -c %s 2>&1 | FileCheck %s --check-prefix=SELECT-BE
// RUN: %clang -### -target sh-unknown-none-elf -mlittle-endian -c %s 2>&1 | FileCheck %s --check-prefix=SELECT-LE
// SELECT-BE: "-cc1"
// SELECT-BE-SAME: "-triple" "sh-unknown-none-elf"
// SELECT-LE: "-cc1"
// SELECT-LE-SAME: "-triple" "shle-unknown-none-elf"

// RUN: %clang -### -target shle-unknown-linux-gnu -fPIC -c %s 2>&1 | FileCheck %s --check-prefix=PIC
// RUN: %clang -### -target shle-unknown-linux-gnu -fPIE -c %s 2>&1 | FileCheck %s --check-prefix=PIE-COMPILE
// PIC: "-mrelocation-model" "pic"
// PIC-SAME: "-pic-level" "2"
// PIE-COMPILE: "-mrelocation-model" "pic"
// PIE-COMPILE-SAME: "-pic-level" "2"
// PIE-COMPILE-SAME: "-pic-is-pie"

// RUN: %clang -### -target shle-unknown-linux-gnu -O2 -S %s 2>&1 | FileCheck %s --check-prefix=GNU-TEXT
// GNU-TEXT: "-cc1"
// GNU-TEXT-NOT: "-faddrsig"

// RUN: %clang -### -target sh-unknown-linux-gnu -O0 -c %s 2>&1 | FileCheck %s --check-prefix=FP-O0
// RUN: %clang -### -target shle-unknown-linux-gnu -O2 -c %s 2>&1 | FileCheck %s --check-prefix=FP-O2
// RUN: %clang -### -target shle-unknown-linux-gnu -O2 -fno-omit-frame-pointer -c %s 2>&1 | FileCheck %s --check-prefix=FP-EXPLICIT
// FP-O0: "-mframe-pointer=all"
// FP-O2: "-mframe-pointer=none"
// FP-EXPLICIT: "-mframe-pointer=all"

int value;
