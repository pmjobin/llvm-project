// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh3 -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=IR

__attribute__((interrupt_handler, nosave_low_regs)) void nosave(void) {}
extern void *alternate_stack;
__attribute__((interrupt_handler, sp_switch("alternate_stack"))) void switched(void) {}
#ifdef __SH2A__
__attribute__((interrupt_handler, resbank)) void banked(void) {}
#endif

// IR: define{{.*}} void @nosave(){{.*}} #[[ATTRS:[0-9]+]]
// IR: define{{.*}} void @switched(){{.*}} #[[SWITCH:[0-9]+]]
// IR: attributes #[[ATTRS]] = { {{.*}}"sh-interrupt-handler"="interrupt"{{.*}}"sh-nosave-low-regs"
// IR: attributes #[[SWITCH]] = { {{.*}}"sh-interrupt-handler"="interrupt"{{.*}}"sh-sp-switch"="alternate_stack"
