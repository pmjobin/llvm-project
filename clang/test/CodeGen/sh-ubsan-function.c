// RUN: %clang_cc1 -triple sh2-linux-gnu -fsanitize=function -emit-llvm -o - %s | FileCheck %s --check-prefix=ORDINARY
// RUN: %clang_cc1 -triple sh2-linux-muslfdpic -fsanitize=function -emit-llvm -o - %s | FileCheck %s --check-prefix=FDPIC

typedef void (*function_pointer)(int);

void call(function_pointer callee) {
	callee(0);
}

// ORDINARY-LABEL: define{{.*}} void @call(ptr noundef %callee)
// ORDINARY-NOT: ubsan.function.entry
// ORDINARY: [[CALLEE:%.*]] = load ptr, ptr %callee.addr, align 4
// ORDINARY: getelementptr <{ i32, i32 }>, ptr [[CALLEE]], i32 -1, i32 0

// FDPIC-LABEL: define{{.*}} void @call(ptr noundef %callee)
// FDPIC: [[DESCRIPTOR:%.*]] = load ptr, ptr %callee.addr, align 4
// FDPIC: %ubsan.function.entry = load ptr, ptr [[DESCRIPTOR]], align 4
// FDPIC: getelementptr <{ i32, i32 }>, ptr %ubsan.function.entry, i32 -1, i32 0
