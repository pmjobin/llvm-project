// RUN: %clang -### --target=sh2-linux-gnu -mcpu=sh2 -resource-dir=%S/Inputs/resource_dir_with_per_target_subdir --sysroot=%S/Inputs/basic_linux_tree -fuse-ld=lld -fsanitize=undefined %s 2>&1 | FileCheck %s --check-prefix=LE
// RUN: %clang -### --target=sh2eb-linux-gnu -mcpu=sh2 -resource-dir=%S/Inputs/resource_dir_with_per_target_subdir --sysroot=%S/Inputs/basic_linux_tree -fuse-ld=lld -fsanitize=undefined %s 2>&1 | FileCheck %s --check-prefix=BE
// RUN: %clang -### --target=sh2-linux-muslfdpic -mcpu=sh2 -resource-dir=%S/Inputs/resource_dir_with_per_target_subdir --sysroot=%S/Inputs/basic_linux_tree -fuse-ld=lld -fsanitize=undefined %s 2>&1 | FileCheck %s --check-prefix=FDPICLE
// RUN: %clang -### --target=sh2eb-linux-muslfdpic -mcpu=sh2 -resource-dir=%S/Inputs/resource_dir_with_per_target_subdir --sysroot=%S/Inputs/basic_linux_tree -fuse-ld=lld -fsanitize=undefined %s 2>&1 | FileCheck %s --check-prefix=FDPICBE
// RUN: %clang -### --target=sh2-linux-gnu -mcpu=sh2 -resource-dir=%S/Inputs/resource_dir_with_per_target_subdir --sysroot=%S/Inputs/basic_linux_tree -fuse-ld=lld -fsanitize=undefined -fsanitize-minimal-runtime %s 2>&1 | FileCheck %s --check-prefix=MINIMAL
// RUN: %clang -### --target=sh2-linux-muslfdpic -mcpu=sh2 -resource-dir=%S/Inputs/resource_dir_with_per_target_subdir --sysroot=%S/Inputs/basic_linux_tree -fuse-ld=lld -fsanitize=undefined -fsanitize-trap=undefined %s 2>&1 | FileCheck %s --check-prefix=TRAP
// RUN: %clangxx -### --target=sh2eb-linux-muslfdpic -mcpu=sh2 -resource-dir=%S/Inputs/resource_dir_with_per_target_subdir --sysroot=%S/Inputs/basic_linux_tree -fuse-ld=lld -fsanitize=vptr -x c++ %s 2>&1 | FileCheck %s --check-prefix=CXX

// LE: "--whole-archive" "{{.*}}/lib/sh2-unknown-linux-gnu/libclang_rt.ubsan_standalone.a" "--no-whole-archive"
// LE-NOT: /lib/x86_64
// LE-NOT: /lib/sh2eb-

// BE: "--whole-archive" "{{.*}}/lib/sh2eb-unknown-linux-gnu/libclang_rt.ubsan_standalone.a" "--no-whole-archive"
// BE-NOT: /lib/x86_64
// BE-NOT: /lib/sh2-

// FDPICLE: "--whole-archive" "{{.*}}/lib/sh2-unknown-linux-muslfdpic/libclang_rt.ubsan_standalone.a" "--no-whole-archive"
// FDPICLE-NOT: /lib/x86_64
// FDPICLE-NOT: /lib/sh2eb-
// FDPICLE-NOT: /lib/sh2-unknown-linux-gnu/

// FDPICBE: "--whole-archive" "{{.*}}/lib/sh2eb-unknown-linux-muslfdpic/libclang_rt.ubsan_standalone.a" "--no-whole-archive"
// FDPICBE-NOT: /lib/x86_64
// FDPICBE-NOT: /lib/sh2-unknown-
// FDPICBE-NOT: /lib/sh2eb-unknown-linux-gnu/

// MINIMAL: "--whole-archive" "{{.*}}/lib/sh2-unknown-linux-gnu/libclang_rt.ubsan_minimal.a" "--no-whole-archive"
// MINIMAL-NOT: libclang_rt.ubsan_standalone
// MINIMAL-NOT: /lib/x86_64

// TRAP-NOT: libclang_rt.ubsan

// CXX: "--whole-archive" "{{.*}}/lib/sh2eb-unknown-linux-muslfdpic/libclang_rt.ubsan_standalone.a" "--no-whole-archive"
// CXX: "--whole-archive" "{{.*}}/lib/sh2eb-unknown-linux-muslfdpic/libclang_rt.ubsan_standalone_cxx.a" "--no-whole-archive"
// CXX-NOT: /lib/x86_64
