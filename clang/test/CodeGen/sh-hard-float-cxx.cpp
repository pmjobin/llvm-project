// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -std=c++17 -O0 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -std=c++17 -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -std=c++17 -O0 -emit-llvm -S -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -std=c++17 -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s

struct Base {
	virtual float method(float value);
};

struct Left : virtual Base {};
struct Right : virtual Base {};

struct Derived : Left, Right {
	Derived();
	float method(float value) override;
	double mixed(int integer, double value);
};

// CHECK: define{{.*}} float @_ZN7Derived6methodEf(ptr{{.*}} %this, float{{.*}} %value)
float Derived::method(float value) {
	return value;
}

// CHECK: define{{.*}} double @_ZN7Derived5mixedEid(ptr{{.*}} %this, i32{{.*}} %integer, double{{.*}} %value)
double Derived::mixed(int integer, double value) {
	return value + integer;
}

// CHECK: define{{.*}} float @_Z12call_virtualP4Basef(ptr{{.*}} %object, float{{.*}} %value)
// CHECK: call{{.*}} float %{{.*}}(ptr{{.*}}, float{{.*}})
float call_virtual(Base *object, float value) {
	return object->method(value);
}

// CHECK: define{{.*}} void @_ZN7DerivedC2Ev(ptr{{.*}} %this, ptr{{.*}} %vtt)
// CHECK: define{{.*}} void @_ZN7DerivedC1Ev(ptr{{.*}} %this)
Derived::Derived() = default;
