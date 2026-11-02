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

// CHECK-LABEL: main:
// CHECK-COUNT-4: jsr	@{{r[0-9]+}}
// CHECK-NOT: bsr
#include "Inputs/sh-long-calls/runtime-main.c"
