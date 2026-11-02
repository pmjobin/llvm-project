// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2a -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2a -O2 -emit-llvm -o - %s | FileCheck %s

__attribute__((interrupt_handler, resbank)) void banked(void) {}

// CHECK: define{{.*}} void @banked(){{.*}} #[[ATTRS:[0-9]+]]
// CHECK: attributes #[[ATTRS]] = { {{.*}}"sh-interrupt-handler"="interrupt"{{.*}}"sh-resbank"
