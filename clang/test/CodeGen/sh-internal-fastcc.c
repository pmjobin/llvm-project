// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O1 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O2 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O2 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -Os -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -Os -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -emit-llvm -o - %s | FileCheck %s

typedef unsigned int u32;

__attribute__((noinline)) static u32 internal_compare(u32 a, u32 b) {
	return a < b;
}

// CHECK-LABEL: define{{.*}} i32 @call_internal_compare(
// CHECK: call fastcc i32 @internal_compare(
u32 call_internal_compare(u32 a, u32 b) {
	return internal_compare(a, b) + 7;
}

// CHECK-LABEL: define internal fastcc{{.*}} i32 @internal_compare(
