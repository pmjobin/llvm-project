// RUN: %clang -### --target=sh-unknown-none-elf -c %s 2>&1 | FileCheck %s --check-prefix=ENABLED
// RUN: %clang -### --target=shle-unknown-linux-gnu -c %s 2>&1 | FileCheck %s --check-prefix=ENABLED
// RUN: %clang -### --target=shle-unknown-none-elf -fexceptions -c %s 2>&1 | FileCheck %s --check-prefix=ENABLED
// RUN: %clang -### --target=sh-unknown-linux-gnu -fno-exceptions -c %s 2>&1 | FileCheck %s --check-prefix=DISABLED
// RUN: %clang -### --target=shle-unknown-linux-gnu -fno-cxx-exceptions -c %s 2>&1 | FileCheck %s --check-prefix=DISABLED
// RUN: %clang -### --target=shle-unknown-linux-gnu -fno-exceptions -fcxx-exceptions -c %s 2>&1 | FileCheck %s --check-prefix=ENABLED
// RUN: %clang -### --target=shle-unknown-linux-gnu -fexceptions -fno-cxx-exceptions -c %s 2>&1 | FileCheck %s --check-prefix=C-ONLY

// ENABLED: "-cc1"{{.*}} "-fcxx-exceptions" "-fexceptions"

// DISABLED: "-cc1"
// DISABLED-NOT: "-fcxx-exceptions"
// DISABLED-NOT: "-fexceptions"

// C-ONLY: "-cc1"
// C-ONLY-NOT: "-fcxx-exceptions"
// C-ONLY: "-fexceptions"

void function() {}
