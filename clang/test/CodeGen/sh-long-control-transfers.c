// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O1 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -Os -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O1 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -Os -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O1 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -Os -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O1 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -Os -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s

#define STEP1 value += 3;
#define STEP2 STEP1 STEP1
#define STEP4 STEP2 STEP2
#define STEP8 STEP4 STEP4
#define STEP16 STEP8 STEP8
#define STEP32 STEP16 STEP16
#define STEP64 STEP32 STEP32
#define STEP128 STEP64 STEP64
#define STEP256 STEP128 STEP128
#define STEP512 STEP256 STEP256
#define STEP1024 STEP512 STEP512

// CHECK-LABEL: far_conditional:
// CHECK: bf	.LBB{{[0-9]+}}_1
// CHECK: mov.w	[[LITERAL:.Lsh_relative_branch[0-9]+]],r0
// CHECK-NEXT: braf	r0
// CHECK-NEXT: nop
// CHECK: [[LITERAL]]:
// CHECK-NEXT: .short	.LBB{{[0-9]+}}_2-[[LITERAL]]
// CHECK-NOT: bsr
unsigned far_conditional(unsigned selector) {
	volatile unsigned value = selector;
	if (selector) {
		STEP1024
	}
	return value;
}
