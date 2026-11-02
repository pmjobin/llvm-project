// RUN: %clang -### -target shle-unknown-none-elf -m4-340 -c %s 2>&1 | FileCheck %s --check-prefix=ST340
// RUN: %clang -### -target shle-unknown-none-elf -m4-400 -c %s 2>&1 | FileCheck %s --check-prefix=ST400
// RUN: %clang -### -target shle-unknown-none-elf -m4-500 -c %s 2>&1 | FileCheck %s --check-prefix=ST500

// ST340: "-cc1"
// ST340-SAME: "-target-cpu" "sh4"
// ST340-SAME: "-target-feature" "-mmu"
// ST340-SAME: "-target-feature" "-single-fpu"
// ST340-NOT: "-target-feature" "+st40-300"
// ST340-SAME: "-tune-cpu" "st40-340"
// ST340-SAME: "-D__SH4_340__"
// ST340-SAME: "-D__SH_ATOMIC_MODEL_NONE__"
// ST340-SAME: "-msoft-float"
// ST340-SAME: "-mfloat-abi" "soft"
// ST400: "-cc1"
// ST400-SAME: "-target-cpu" "sh4"
// ST400-SAME: "-target-feature" "-mmu"
// ST400-SAME: "-target-feature" "-single-fpu"
// ST400-NOT: "-target-feature" "+st40-300"
// ST400-SAME: "-tune-cpu" "st40-400"
// ST400-SAME: "-D__SH4_400__"
// ST400-SAME: "-D__SH_ATOMIC_MODEL_NONE__"
// ST500: "-cc1"
// ST500-SAME: "-target-cpu" "sh4"
// ST500-NOT: "-target-feature" "-mmu"
// ST500-SAME: "-target-feature" "-single-fpu"
// ST500-NOT: "-target-feature" "+st40-300"
// ST500-SAME: "-tune-cpu" "st40-500"
// ST500-SAME: "-D__SH4_500__"
// ST500-SAME: "-D__SH_ATOMIC_MODEL_NONE__"

// RUN: %clang -### -target shle-unknown-none-elf -m4-300 -m4-340 -c %s 2>&1 | FileCheck %s --check-prefix=ST340
// RUN: %clang -### -target shle-unknown-none-elf -m4-340 -m4-400 -c %s 2>&1 | FileCheck %s --check-prefix=ST400
// RUN: %clang -### -target shle-unknown-none-elf -m4-400 -m4-500 -c %s 2>&1 | FileCheck %s --check-prefix=ST500
// RUN: %clang -### -target shle-unknown-none-elf -m4-500 -m4 -c %s 2>&1 | FileCheck %s --check-prefix=GENERIC
// RUN: %clang -### -target shle-unknown-none-elf -m4-400 -m3 -c %s 2>&1 | FileCheck %s --check-prefix=SH3

// GENERIC: "-target-cpu" "sh4"
// GENERIC-NOT: "-target-feature" "-mmu"
// GENERIC-NOT: "-target-feature" "-single-fpu"
// GENERIC-NOT: "-tune-cpu"
// SH3: "-target-cpu" "sh3"
// SH3-NOT: "-target-feature" "-mmu"
// SH3-NOT: "-tune-cpu"

// RUN: %clang -### -target shle-unknown-none-elf -m4 -mtune=st40-340 -c %s 2>&1 | FileCheck %s --check-prefix=TUNE340
// RUN: %clang -### -target shle-unknown-none-elf -m4-340 -mtune=st40-400 -c %s 2>&1 | FileCheck %s --check-prefix=TUNE400
// RUN: %clang -### -target shle-unknown-none-elf -m4-400 -mtune=st40-500 -c %s 2>&1 | FileCheck %s --check-prefix=TUNE500
// RUN: not %clang -### -target shle-unknown-none-elf -m3 -mtune=st40-340 -c %s 2>&1 | FileCheck %s --check-prefix=BAD-TUNE
// RUN: not %clang -### -target shle-unknown-none-elf -mcpu=st40-340 -c %s 2>&1 | FileCheck %s --check-prefix=BAD-CPU

// TUNE340: "-target-cpu" "sh4"
// TUNE340-NOT: "-target-feature" "-mmu"
// TUNE340-NOT: "-target-feature" "-single-fpu"
// TUNE340-SAME: "-tune-cpu" "st40-340"
// TUNE400: "-target-feature" "-mmu"
// TUNE400-SAME: "-tune-cpu" "st40-400"
// TUNE500: "-target-feature" "-mmu"
// TUNE500-SAME: "-tune-cpu" "st40-500"
// BAD-TUNE: error: unsupported option '-mtune=st40-340' for target 'shle-unknown-none-elf'
// BAD-CPU: error: unsupported argument 'st40-340' to option '-mcpu='

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-340 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-NOMMU
// RUN: env "PATH=" %clang -### -target sh-unknown-linux-gnu -m4-400 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-NOMMU-BE
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-500 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=AS-NOFPU

// AS-NOMMU: "sh4-linux-gnu-as" "--little" "--isa=sh4-nommu-nofpu"
// AS-NOMMU-BE: "sh-linux-gnu-as" "--big" "--isa=sh4-nommu-nofpu"
// AS-NOFPU: "sh4-linux-gnu-as" "--little" "--isa=sh4-nofpu"

int value;
