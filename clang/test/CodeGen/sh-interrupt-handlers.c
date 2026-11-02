// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -fsanitize=integer-divide-by-zero -stack-protector 3 -emit-llvm -o - %s | FileCheck %s --check-prefix=SAFETY
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -fsanitize=integer-divide-by-zero -stack-protector 3 -emit-llvm -o - %s | FileCheck %s --check-prefix=SAFETY
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh4 -O0 -fsanitize=address -emit-llvm -o - %s | FileCheck %s --check-prefix=ASAN
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -fPIE -O2 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=PIE

volatile int global;

__attribute__((interrupt_handler)) void interrupt_definition(void) {
	volatile char buffer[32];
	buffer[0] = global / (global | 1);
}

__attribute__((trapa_handler)) void trapa_definition(void) {}

__attribute__((interrupt_handler)) void interrupt_redeclared(void);
void interrupt_redeclared(void) {}

__attribute__((weak, trapa_handler)) void trapa_weak(void) {}

void interrupt_alias(void) __attribute__((alias("interrupt_definition")));

typedef void (*handler_pointer)(void);
handler_pointer const vector_table[] = {
	interrupt_definition,
	trapa_definition,
	interrupt_alias,
};

// IR-DAG: @interrupt_alias ={{.*}} alias void (), ptr @interrupt_definition
// IR-DAG: @vector_table ={{.*}}constant [3 x ptr] [ptr @interrupt_definition, ptr @trapa_definition, ptr @interrupt_alias]
// IR: define{{.*}} void @interrupt_definition(){{.*}} #[[INT:[0-9]+]]
// IR: define{{.*}} void @trapa_definition(){{.*}} #[[TRAPA:[0-9]+]]
// IR: define{{.*}} void @interrupt_redeclared(){{.*}} #[[INT_REDECL:[0-9]+]]
// IR: define weak{{.*}} void @trapa_weak(){{.*}} #[[TRAPA_WEAK:[0-9]+]]
// IR: attributes #[[INT]] = { {{.*}}disable_sanitizer_instrumentation{{.*}}noinline{{.*}}nounwind{{.*}}"disable-tail-calls"="true"{{.*}}"sh-interrupt-handler"="interrupt"
// IR: attributes #[[TRAPA]] = { {{.*}}disable_sanitizer_instrumentation{{.*}}noinline{{.*}}nounwind{{.*}}"disable-tail-calls"="true"{{.*}}"sh-interrupt-handler"="trapa"
// IR: attributes #[[INT_REDECL]] = { {{.*}}"sh-interrupt-handler"="interrupt"
// IR: attributes #[[TRAPA_WEAK]] = { {{.*}}"sh-interrupt-handler"="trapa"

// SAFETY-NOT: __ubsan
// SAFETY: attributes #[[SAFETY_INT:[0-9]+]] = { {{.*}}disable_sanitizer_instrumentation{{.*}}sspreq{{.*}}"sh-interrupt-handler"="interrupt"
// SAFETY: attributes #[[SAFETY_TRAPA:[0-9]+]] = { {{.*}}disable_sanitizer_instrumentation{{.*}}sspreq{{.*}}"sh-interrupt-handler"="trapa"

// ASAN-LABEL: define{{.*}} void @interrupt_definition()
// ASAN-NOT: __asan
// ASAN: ret void
// ASAN-LABEL: define{{.*}} void @trapa_definition()
// ASAN-NOT: __asan
// ASAN: ret void

// PIE-LABEL: interrupt_definition:
// PIE: mov.l r12,
// PIE: mova [[GOT:.LCPI[0-9]+_[0-9]+_[0-9]+]],r0
// PIE-NEXT: mov.l [[GOT]],r12
// PIE-NEXT: add r0,r12
// PIE: mov.l {{.*}},r12
// PIE: rte
// PIE-NEXT: nop
