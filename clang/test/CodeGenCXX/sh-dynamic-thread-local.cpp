// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -O1 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -O1 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -O1 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -O1 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh3 -std=c++17 -fexceptions -fcxx-exceptions -O1 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh3 -std=c++17 -fexceptions -fcxx-exceptions -O1 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh3 -std=c++17 -fexceptions -fcxx-exceptions -O1 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh3 -std=c++17 -fexceptions -fcxx-exceptions -O1 -emit-llvm -o - %s | FileCheck %s

extern int runtime_value();

struct Object {
	Object();
	~Object();
	int value;
};

// CHECK-DAG: @ordered = {{.*}}thread_local global i32 0, align 4
// CHECK-DAG: @object = {{.*}}thread_local global %struct.Object zeroinitializer, align 4
// CHECK-DAG: @_ZZ5localvE5value = internal thread_local global %struct.Object zeroinitializer, align 4
// CHECK-DAG: @_ZGVZ5localvE5value = internal thread_local {{.*}}global i1 false, align 1
// CHECK-DAG: @__tls_guard = internal thread_local global i8 0, align 1
// CHECK-DAG: @_ZTH7ordered = {{.*}}alias void (), ptr @__tls_init
// CHECK-DAG: @_ZTH6object = {{.*}}alias void (), ptr @__tls_init
thread_local int ordered = runtime_value();
thread_local Object object;

Object &local() {
	thread_local Object value;
	return value;
}

// CHECK: define internal {{.*}}void @__cxx_global_var_init
// CHECK: call void @_ZN6ObjectC1Ev
// CHECK: call i32 @__cxa_thread_atexit(ptr {{.*}}@_ZN6ObjectD1Ev, ptr {{.*}}@object, ptr {{.*}}@__dso_handle)
// CHECK: declare i32 @__cxa_thread_atexit(ptr, ptr, ptr)
// CHECK: define internal void @__tls_init()
// CHECK: define {{.*}}ptr @_ZTW7ordered()
// CHECK: define {{.*}}ptr @_ZTW6object()
