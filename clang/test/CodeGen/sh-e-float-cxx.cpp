// RUN: %clang --target=shle-unknown-linux-gnu -m4-single-only -std=c++17 -fcxx-exceptions -fexceptions -O0 -S -emit-llvm %s -o - | FileCheck %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4-single-only -std=c++17 -fcxx-exceptions -fexceptions -O1 -S -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang --target=shle-unknown-linux-gnu -m4-single-only -std=c++17 -fcxx-exceptions -fexceptions -O1 -S -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang --target=shle-unknown-linux-gnu -m2e -std=c++17 -fcxx-exceptions -fexceptions -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=SH2E
// RUN: %clang --target=shle-unknown-linux-gnu -m3e -std=c++17 -fcxx-exceptions -fexceptions -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=SH3E
// RUN: %clang --target=sh-unknown-linux-gnu -m2e -std=c++17 -fcxx-exceptions -fexceptions -O1 -S -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang --target=shle-unknown-linux-gnu -m3e -std=c++17 -fcxx-exceptions -fexceptions -O1 -S -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang --target=sh-unknown-linux-gnu -m2e -std=c++17 -fcxx-exceptions -fexceptions -fPIC -g -O1 -c %s -o %t.sh2e.be.o
// RUN: %clang --target=shle-unknown-linux-gnu -m2e -std=c++17 -fcxx-exceptions -fexceptions -fPIC -g -O1 -c %s -o %t.sh2e.le.o
// RUN: %clang --target=sh-unknown-linux-gnu -m3e -std=c++17 -fcxx-exceptions -fexceptions -fPIC -g -O1 -c %s -o %t.sh3e.be.o
// RUN: %clang --target=shle-unknown-linux-gnu -m3e -std=c++17 -fcxx-exceptions -fexceptions -fPIC -g -O1 -c %s -o %t.sh3e.le.o
// RUN: llvm-dwarfdump --verify %t.sh2e.be.o %t.sh2e.le.o %t.sh3e.be.o %t.sh3e.le.o

struct value {
	float single;
	long double wide;
};

struct base {
	virtual float transform(float single, long double wide);
	virtual ~base();
};

struct derived : base {
	float bias;
	derived(float value) : bias(value) {}
	float transform(float single, long double wide) override {
		return single + (float)wide + bias;
	}
};

float indirect(base *object, float single, long double wide) {
	return object->transform(single, wide);
}

float concrete(float bias, float single, long double wide) {
	derived object(bias);
	return object.transform(single, wide);
}

value exception_path(value input, bool fail) {
	try {
		if (fail)
			throw input;
		return input;
	} catch (value caught) {
		return caught;
	}
}

// CHECK-LABEL: define dso_local noundef float @_Z8indirectP4basefe(ptr noundef %object, float noundef %single, double noundef %wide)
// CHECK: call noundef float %{{[0-9]+}}(ptr noundef nonnull align 4 dereferenceable(4) %{{[0-9]+}}, float noundef %{{[0-9]+}}, double noundef %{{[0-9]+}})
// CHECK-LABEL: define dso_local noundef float @_Z8concreteffe(float noundef %bias, float noundef %single, double noundef %wide)
// CHECK: call void @_ZN7derivedC2Ef(ptr noundef nonnull align 4 dereferenceable(8) %object, float noundef %{{[0-9]+}})
// CHECK: define linkonce_odr {{.*}}void @_ZN7derivedC2Ef(ptr noundef nonnull align 4 dereferenceable(8) %this, float noundef %value)
// CHECK: define linkonce_odr {{.*}}float @_ZN7derived9transformEfe(ptr noundef nonnull align 4 dereferenceable(8) %this, float noundef %single, double noundef %wide)
// CHECK-LABEL: define dso_local void @_Z14exception_path5valueb(ptr dead_on_unwind noalias writable sret(%struct.value) align 4 %agg.result, ptr noundef byval(%struct.value) align 4 %input, i1 noundef zeroext %fail) {{.*}}personality ptr @__gxx_personality_v0
// CHECK: attributes #[[ATTR:[0-9]+]] = { {{.*}} "target-abi"="gnu-e-float" "target-cpu"="sh4" "target-features"="{{[^"]*}}+fpu-single-mode{{[^"]*}}+single-fpu{{[^"]*}}-double-fpu{{[^"]*}}" }
// CHECK: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-e-float"}
// SH2E: declare ptr @__cxa_allocate_exception(i32) #[[SH2E_GENERATED:[0-9]+]]
// SH2E: attributes #[[ATTR:[0-9]+]] = { {{.*}} "target-abi"="gnu-e-float" "target-cpu"="sh2e" "target-features"="+fpu-single-mode,+sh2,+single-fpu" }
// SH2E: attributes #[[SH2E_GENERATED]] = { "target-cpu"="sh2e" "target-features"="+fpu-single-mode,+sh2,+single-fpu" }
// SH2E: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-e-float"}
// SH3E: declare ptr @__cxa_allocate_exception(i32) #[[SH3E_GENERATED:[0-9]+]]
// SH3E: attributes #[[ATTR:[0-9]+]] = { {{.*}} "target-abi"="gnu-e-float" "target-cpu"="sh3e" "target-features"="{{[^"]*}}+fsqrt{{[^"]*}}+single-fpu{{[^"]*}}" }
// SH3E: attributes #[[SH3E_GENERATED]] = { "target-cpu"="sh3e" "target-features"="{{[^"]*}}+fsqrt{{[^"]*}}+single-fpu{{[^"]*}}" }
// SH3E: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-e-float"}
