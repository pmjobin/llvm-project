// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -O2 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s

// CHECK-LABEL: sh_inline_asm_repeat:
// CHECK: bf [[NEAR:.LBB[0-9]+_[0-9]+]]
// CHECK: mov.w [[LITERAL:.Lsh_relative_branch[0-9]+]],r0
// CHECK-NEXT: braf r0
// CHECK-NEXT: nop
// CHECK: [[LITERAL]]:
// CHECK-NEXT: .short [[FAR:.LBB[0-9]+_[0-9]+]]-[[LITERAL]]
// CHECK: [[NEAR]]:
// CHECK: !APP
// CHECK-COUNT-2500: nop
// CHECK: !NO_APP
// CHECK: [[FAR]]:

int sh_inline_asm_repeat(int condition) {
	if (!condition)
		return 19;
	__asm__ volatile(".rept 2500\n\tnop\n\t.endr");
	return 17;
}
