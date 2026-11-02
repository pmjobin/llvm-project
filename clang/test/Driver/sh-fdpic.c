// RUN: %clang -### -target sh2-linux-muslfdpic -c %s 2>&1 | FileCheck %s --check-prefixes=COMMON,NATURAL,LE
// RUN: %clang -### -target sh2eb-linux-muslfdpic -c %s 2>&1 | FileCheck %s --check-prefixes=COMMON,NATURAL,BE
// RUN: %clang -### -target sh2-linux-musl -mfdpic -c %s 2>&1 | FileCheck %s --check-prefixes=COMMON,NATURAL,EXPLICIT
// RUN: %clang -### -target sh2-linux-muslfdpic -fno-pic -c %s 2>&1 | FileCheck %s --check-prefixes=COMMON,NO-PIE
// RUN: %clang -### -target sh2-linux-muslfdpic -fPIC -c %s 2>&1 | FileCheck %s --check-prefix=LARGE-PIC
// RUN: %clang -### -target sh2-linux-muslfdpic -fPIE -c %s 2>&1 | FileCheck %s --check-prefixes=LARGE-PIC,PIE
// RUN: %clang -### -target sh2-linux-muslfdpic -mno-fdpic -c %s 2>&1 | FileCheck %s --check-prefix=ORDINARY

// LE-DAG: "-triple" "sh2-unknown-linux-muslfdpic"
// BE-DAG: "-triple" "sh2eb-unknown-linux-muslfdpic"
// EXPLICIT-DAG: "-triple" "sh2-unknown-linux-musl"
// COMMON-DAG: "-mrelocation-model" "pic"
// COMMON-DAG: "-pic-level" "1"
// COMMON-DAG: "-target-cpu" "sh2"
// COMMON-DAG: "-target-abi" "gnu-fdpic"
// NATURAL-NOT: "-pic-is-pie"
// NO-PIE-NOT: "-pic-is-pie"
// LARGE-PIC-DAG: "-mrelocation-model" "pic"
// LARGE-PIC-DAG: "-pic-level" "2"
// LARGE-PIC-DAG: "-target-cpu" "sh2"
// LARGE-PIC-DAG: "-target-abi" "gnu-fdpic"
// PIE-DAG: "-pic-is-pie"
// ORDINARY: "-target-abi" "gnu"
// ORDINARY-NOT: "-target-abi" "gnu-fdpic"

// RUN: env "PATH=" %clang -### -target sh2-linux-muslfdpic -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=LE-AS
// RUN: env "PATH=" %clang -### -target sh2eb-linux-muslfdpic -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=BE-AS
// LE-AS: "sh4-linux-gnu-as" "--little" "--fdpic" "--isa=sh2"
// BE-AS: "sh-linux-gnu-as" "--big" "--fdpic" "--isa=sh2"

// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/bin %t.dir/sysroot
// RUN: touch %t.dir/bin/sh4-linux-gnu-ld %t.dir/bin/sh-linux-gnu-ld
// RUN: chmod +x %t.dir/bin/sh4-linux-gnu-ld %t.dir/bin/sh-linux-gnu-ld
// RUN: env "PATH=" %clang -### -target sh2-linux-muslfdpic -B%t.dir/bin --sysroot=%t.dir/sysroot -nostdlib %s 2>&1 | FileCheck %s --check-prefix=LE-LD
// RUN: env "PATH=" %clang -### -target sh2eb-linux-muslfdpic -B%t.dir/bin --sysroot=%t.dir/sysroot -nostdlib %s 2>&1 | FileCheck %s --check-prefix=BE-LD
// RUN: env "PATH=" %clang -### -target sh2-linux-gnu -mfdpic -B%t.dir/bin --sysroot=%t.dir/sysroot -nostdlib %s 2>&1 | FileCheck %s --check-prefix=GNU-LD
// LE-LD: "{{.*}}sh4-linux-gnu-ld"
// LE-LD-SAME: "-m" "shlelf_fd"
// LE-LD-SAME: "-dynamic-linker" "/lib/ld-musl-sh-nofpu-fdpic.so.1"
// LE-LD-NOT: "-pie"
// BE-LD: "{{.*}}sh-linux-gnu-ld"
// BE-LD-SAME: "-m" "shelf_fd"
// BE-LD-SAME: "-dynamic-linker" "/lib/ld-musl-sheb-nofpu-fdpic.so.1"
// BE-LD-NOT: "-pie"
// GNU-LD: "{{.*}}sh4-linux-gnu-ld"
// GNU-LD-SAME: "-m" "shlelf_fd"
// GNU-LD-SAME: "-dynamic-linker" "/lib/ld-linux.so.2"
// GNU-LD-NOT: ld-musl

// RUN: %clang -target sh2-linux-muslfdpic -x c++ -c %s -o /dev/null
// RUN: %clang -target sh2eb-linux-muslfdpic -x c++ -c %s -o /dev/null
// RUN: not %clang -target sh2-unknown-none-musl -mfdpic -x c -c %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=NONLINUX
// NONLINUX: error: the clang compiler does not support 'SH FDPIC outside Linux'

int value;
