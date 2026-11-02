// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -O2 -emit-llvm -o - %s | FileCheck %s

__attribute__((interrupt_handler, trap_exit(0))) void trap_min(void) {}
__attribute__((interrupt_handler, trap_exit(255))) void trap_max(void) {}

// CHECK: define{{.*}} void @trap_min(){{.*}} #[[MIN:[0-9]+]]
// CHECK: define{{.*}} void @trap_max(){{.*}} #[[MAX:[0-9]+]]
// CHECK: attributes #[[MIN]] = { {{.*}}"sh-interrupt-handler"="interrupt"{{.*}}"sh-trap-exit"="0"
// CHECK: attributes #[[MAX]] = { {{.*}}"sh-interrupt-handler"="interrupt"{{.*}}"sh-trap-exit"="255"
