// RUN: %clang --target=shle-unknown-linux-gnu -m4-single-only -O0 -S -emit-llvm %s -o - | FileCheck %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4-single-only -O1 -S -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang --target=shle-unknown-linux-gnu -m4-single-only -O1 -S -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang --target=shle-unknown-linux-gnu -m2e -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=SH2E
// RUN: %clang --target=shle-unknown-linux-gnu -m3e -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=SH3E
// RUN: %clang --target=sh-unknown-linux-gnu -m2e -O1 -S -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang --target=shle-unknown-linux-gnu -m2e -O1 -S -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang --target=sh-unknown-linux-gnu -m3e -O1 -S -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang --target=shle-unknown-linux-gnu -m3e -O1 -S -mllvm -verify-machineinstrs %s -o /dev/null

struct one_float {
	float value;
};

struct one_wide {
	long double value;
};

struct mixed {
	int tag;
	float single;
	long double wide;
};

float fixed(float single, long double wide, int integer) {
	return single + (float)wide + (float)integer;
}

float one_float_arg(struct one_float value) {
	return value.value;
}

long double one_wide_arg(struct one_wide value) {
	return value.value;
}

struct mixed aggregate_roundtrip(struct mixed value) {
	return value;
}

float vararg_float(int count, ...) {
	__builtin_va_list args;
	__builtin_va_start(args, count);
	float result = (float)__builtin_va_arg(args, double);
	__builtin_va_end(args);
	return result;
}

long double vararg_wide(int count, ...) {
	__builtin_va_list args;
	__builtin_va_start(args, count);
	long double result = __builtin_va_arg(args, long double);
	__builtin_va_end(args);
	return result;
}

int va_list_size(void) {
	return sizeof(__builtin_va_list);
}

extern void consume_varargs(int, ...);

void call_varargs(double single, long double wide) {
	consume_varargs(7, single, wide);
}

// CHECK: %struct.__va_list_tag = type { ptr, ptr, ptr, ptr, ptr }
// CHECK: define dso_local float @fixed(float noundef %single, double noundef %wide, i32 noundef %integer)
// CHECK: define dso_local float @one_float_arg(float %value.coerce)
// CHECK: define dso_local double @one_wide_arg(double %value.coerce)
// CHECK: define dso_local void @aggregate_roundtrip(ptr dead_on_unwind noalias writable sret(%struct.mixed) align 4 %agg.result, ptr noundef byval(%struct.mixed) align 4 %value)
// CHECK-LABEL: define dso_local float @vararg_float(i32 noundef %count, ...)
// CHECK: getelementptr inbounds nuw %struct.__va_list_tag, ptr %args, i32 0, i32 2
// CHECK: getelementptr inbounds nuw %struct.__va_list_tag, ptr %args, i32 0, i32 3
// CHECK: load float, ptr %vaarg.addr, align 4
// CHECK-LABEL: define dso_local double @vararg_wide(i32 noundef %count, ...)
// CHECK: getelementptr inbounds nuw %struct.__va_list_tag, ptr %args, i32 0, i32 0
// CHECK: getelementptr inbounds nuw %struct.__va_list_tag, ptr %args, i32 0, i32 1
// CHECK: load double, ptr %vaarg.addr, align 4
// CHECK-LABEL: define dso_local i32 @va_list_size()
// CHECK: ret i32 20
// CHECK-LABEL: define dso_local void @call_varargs(float noundef %single, double noundef %wide)
// CHECK: call void (i32, ...) @consume_varargs(i32 noundef 7, float noundef %{{[0-9]+}}, double noundef %{{[0-9]+}})
// CHECK: attributes #[[ATTR:[0-9]+]] = { {{.*}} "target-abi"="gnu-e-float" "target-cpu"="sh4" "target-features"="{{[^"]*}}+fpu-single-mode{{[^"]*}}+single-fpu{{[^"]*}}-double-fpu{{[^"]*}}" }
// CHECK: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-e-float"}
// SH2E: attributes #[[ATTR:[0-9]+]] = { {{.*}} "target-abi"="gnu-e-float" "target-cpu"="sh2e" "target-features"="+fpu-single-mode,+sh2,+single-fpu" }
// SH2E: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-e-float"}
// SH3E: attributes #[[ATTR:[0-9]+]] = { {{.*}} "target-abi"="gnu-e-float" "target-cpu"="sh3e" "target-features"="{{[^"]*}}+fsqrt{{[^"]*}}+single-fpu{{[^"]*}}" }
// SH3E: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-e-float"}
