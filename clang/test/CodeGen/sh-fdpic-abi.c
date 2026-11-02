// RUN: %clang_cc1 -triple sh2-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 1 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=ABI,FDPIC
// RUN: %clang_cc1 -triple sh2eb-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 1 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=ABI,FDPIC
// RUN: %clang_cc1 -triple sh2-unknown-linux-musl -target-cpu sh2 -target-abi gnu -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=ABI,ORDINARY
// RUN: %clang_cc1 -triple sh2-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 1 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh2eb-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 1 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s

typedef int (*callback_t)(int);
typedef int (*variadic_callback_t)(int, ...);

struct pair {
	int first;
	int second;
};

struct large {
	int words[6];
};

struct mixed {
	int *data;
	callback_t callback;
	int tag;
};

// ABI-DAG: define{{.*}} i32 @scalar_arguments(i8{{.*}} signext %a, i16{{.*}} signext %b, i32{{.*}} %c, ptr{{.*}} %p)
int scalar_arguments(signed char a, short b, int c, int *p) {
	return a + b + c + *p;
}

// ABI-DAG: define{{.*}} i64 @integer64_arguments(i64{{.*}} %a, i64{{.*}} %b)
long long integer64_arguments(long long a, long long b) {
	return a + b;
}

// ABI-DAG: define{{.*}} double @floating_arguments(float{{.*}} %a, double{{.*}} %b)
double floating_arguments(float a, double b) {
	return (double)a + b;
}

// ABI-DAG: define{{.*}} %struct.pair @pair_return(i32{{.*}} %a, i32{{.*}} %b)
struct pair pair_return(int a, int b) {
	struct pair value = {a, b};
	return value;
}

// ABI-DAG: define{{.*}} void @large_return(ptr{{.*}} sret(%struct.large) align 4 %agg.result, i32{{.*}} %seed)
struct large large_return(int seed) {
	struct large value = {{seed, seed + 1, seed + 2, seed + 3,
		seed + 4, seed + 5}};
	return value;
}

// ABI-DAG: define{{.*}} i32 @large_argument(ptr{{.*}} byval(%struct.large) align 4 %value)
int large_argument(struct large value) {
	return value.words[0] + value.words[5];
}

// ABI-DAG: define{{.*}} ptr @return_callback(ptr{{.*}} %callback)
callback_t return_callback(callback_t callback) {
	return callback;
}

// ABI-DAG: define{{.*}} void @mixed_return(ptr{{.*}} sret(%struct.mixed) align 4 %agg.result, ptr{{.*}} %data, ptr{{.*}} %callback, i32{{.*}} %tag)
struct mixed mixed_return(int *data, callback_t callback, int tag) {
	struct mixed value = {data, callback, tag};
	return value;
}

// ABI-DAG: define{{.*}} i32 @mixed_call(ptr{{.*}} byval(%struct.mixed) align 4 %value, i32{{.*}} %arg)
int mixed_call(struct mixed value, int arg) {
	return *value.data + value.callback(arg) + value.tag;
}

// ABI-DAG: define{{.*}} i32 @integer_varargs(i32{{.*}} %count, ...)
// ABI: call void @llvm.va_start.p0(ptr
// ABI: call void @llvm.va_end.p0(ptr
int integer_varargs(int count, ...) {
	__builtin_va_list args;
	__builtin_va_start(args, count);
	int result = __builtin_va_arg(args, int);
	__builtin_va_end(args);
	return result;
}

// ABI-DAG: define{{.*}} i32 @mixed_varargs(i32{{.*}} %count, ...)
int mixed_varargs(int count, ...) {
	__builtin_va_list args;
	__builtin_va_start(args, count);
	int integer = __builtin_va_arg(args, int);
	int *data = __builtin_va_arg(args, int *);
	callback_t callback = __builtin_va_arg(args, callback_t);
	double floating = __builtin_va_arg(args, double);
	__builtin_va_end(args);
	return integer + *data + callback(integer) + (int)floating;
}

// ABI-DAG: define{{.*}} i32 @call_mixed_varargs(ptr{{.*}} %data, ptr{{.*}} %callback, float{{.*}} %value)
// ABI-DAG: call i32 (i32, ...) @mixed_varargs({{.*}}double
int call_mixed_varargs(int *data, callback_t callback, float value) {
	return mixed_varargs(4, 1, data, callback, value);
}

// ABI-DAG: define{{.*}} i32 @call_variadic_callback(ptr{{.*}} %callback, i32{{.*}} %value)
int call_variadic_callback(variadic_callback_t callback, int value) {
	return callback(2, value, value + 1);
}

// FDPIC: !{i32 1, !"target-abi", !"gnu-fdpic"}
// ORDINARY-NOT: gnu-fdpic
