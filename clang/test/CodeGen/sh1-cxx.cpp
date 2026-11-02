// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh1 -target-abi gnu -msoft-float -std=c++17 -fcxx-exceptions -fexceptions -O0 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-linux-gnu -mcpu=sh1 -std=c++17 -fcxx-exceptions -fexceptions -fPIC -g -O1 -c %s -o %t.be.o
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh1 -std=c++17 -fcxx-exceptions -fexceptions -fPIC -g -O1 -c %s -o %t.le.o
// RUN: llvm-dwarfdump --verify %t.be.o %t.le.o

struct payload {
	int integer;
	float single;
	double wide;
};

struct base {
	virtual payload transform(payload value);
	virtual ~base();
};

struct derived : base {
	int bias;
	derived(int value) : bias(value) {}
	payload transform(payload value) override {
		value.integer += bias;
		return value;
	}
};

payload indirect(base *object, payload value) {
	return object->transform(value);
}

payload concrete(int bias, payload value) {
	derived object(bias);
	return object.transform(value);
}

payload function_pointer(payload (*callee)(payload), payload value) {
	return callee(value);
}

derived *rtti(base *object) {
	return dynamic_cast<derived *>(object);
}

payload exception_path(payload input, bool fail) {
	try {
		if (fail)
			throw input;
		return input;
	} catch (payload caught) {
		return caught;
	}
}

// CHECK-LABEL: define dso_local void @_Z8indirectP4base7payload(ptr dead_on_unwind noalias writable sret(%struct.payload) align 4 %agg.result, ptr noundef %object, ptr noundef byval(%struct.payload) align 4 %value)
// CHECK: call void %{{[0-9]+}}(ptr dead_on_unwind writable sret(%struct.payload) align 4 %agg.result, ptr noundef nonnull align 4 dereferenceable(4) %{{[0-9]+}}, ptr noundef byval(%struct.payload) align 4 %agg.tmp)
// CHECK-LABEL: define linkonce_odr void @_ZN7derivedC1Ei(ptr noundef nonnull align 4 dereferenceable(8) %this, i32 noundef %value)
// CHECK-LABEL: define linkonce_odr void @_ZN7derived9transformE7payload(ptr dead_on_unwind noalias writable sret(%struct.payload) align 4 %agg.result, ptr noundef nonnull align 4 dereferenceable(8) %this, ptr noundef byval(%struct.payload) align 4 %value)
// CHECK-LABEL: define dso_local noundef ptr @_Z4rttiP4base(ptr noundef %object)
// CHECK: call ptr @__dynamic_cast
// CHECK-LABEL: define dso_local void @_Z14exception_path7payloadb(ptr dead_on_unwind noalias writable sret(%struct.payload) align 4 %agg.result, ptr noundef byval(%struct.payload) align 4 %input, i1 noundef zeroext %fail) {{.*}}personality ptr @__gxx_personality_v0
// CHECK: declare ptr @__cxa_allocate_exception(i32) #[[GENERATED:[0-9]+]]
// CHECK: attributes #[[USER:[0-9]+]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh1" "use-soft-float"="true" }
// CHECK: attributes #[[GENERATED]] = { "target-cpu"="sh1" }
// CHECK: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}
