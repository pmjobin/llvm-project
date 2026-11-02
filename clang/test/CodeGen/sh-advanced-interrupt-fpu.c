// RUN: %clang --target=sh-unknown-none-elf -m4 -O2 -S -o - %s | FileCheck %s --check-prefix=HARD
// RUN: %clang --target=shle-unknown-none-elf -m4 -O2 -S -o - %s | FileCheck %s --check-prefix=HARD
// RUN: %clang --target=sh-unknown-none-elf -m2e -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -m3e -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -m4-single -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -m4-single-only -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -m4a -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -m4-300 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -m4-nofpu -O2 -S -o - %s | FileCheck %s --check-prefix=SOFT

volatile float value;
extern float helper(float);

__attribute__((interrupt_handler)) void empty(void) {}

__attribute__((interrupt_handler)) void leaf(void) {
	value += 1.0f;
}

__attribute__((interrupt_handler)) void nonleaf(void) {
	value = helper(value);
}

// HARD-LABEL: empty:
// HARD-NOT: fpscr
// HARD-NOT: fpul
// HARD: rte
// HARD-NEXT: nop

// HARD-LABEL: leaf:
// HARD: sts.l fpscr,@-r15
// HARD: sts.l fpul,@-r15
// HARD: flds fr{{[0-9]+}},fpul
// HARD: lds.l @r0+,fpscr
// HARD: fsts fpul,fr{{[0-9]+}}
// HARD: lds.l @r0+,fpul
// HARD: rte
// HARD-NEXT: nop

// HARD-LABEL: nonleaf:
// HARD: sts.l fpscr,@-r15
// HARD: sts.l fpul,@-r15
// HARD-COUNT-12: sts.l fpul,@-r15
// HARD: jsr
// HARD: lds.l @r0+,fpscr
// HARD-COUNT-12: fsts fpul,fr{{[0-9]+}}
// HARD: lds.l @r0+,fpul
// HARD: rte
// HARD-NEXT: nop

// SOFT-LABEL: empty:
// SOFT-NOT: fpscr
// SOFT-NOT: fpul
// SOFT: rte
// SOFT-NEXT: nop

// SOFT-LABEL: leaf:
// SOFT-NOT: fpscr
// SOFT-NOT: fpul
// SOFT: jsr
// SOFT: rte
// SOFT-NEXT: nop
