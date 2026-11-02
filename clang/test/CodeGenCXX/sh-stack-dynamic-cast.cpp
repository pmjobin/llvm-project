// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -mconstructor-aliases -O0 -emit-obj -o %t.be.o %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -mconstructor-aliases -O0 -emit-obj -o %t.le.o %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -mconstructor-aliases -O0 -pic-level 2 -emit-obj -o %t.pic.be.o %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -mconstructor-aliases -O0 -pic-level 2 -emit-obj -o %t.pic.le.o %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -mconstructor-aliases -O0 -pic-level 2 -pic-is-pie -emit-obj -o %t.pie.be.o %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -mconstructor-aliases -O0 -pic-level 2 -pic-is-pie -emit-obj -o %t.pie.le.o %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -mconstructor-aliases -O0 -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s

struct Base {
	virtual void anchor();
};

struct Derived : Base {};

void Base::anchor() {}

int main() {
	Derived object;
	Base *base = &object;
	return dynamic_cast<Derived *>(base) != &object;
}

// CHECK-LABEL: define {{.*}}i32 @main()
// CHECK: [[OBJECT:%.*]] = alloca %struct.Derived, align 4
// CHECK: [[BASE:%.*]] = alloca ptr, align 4
// CHECK: store ptr [[OBJECT]], ptr [[BASE]], align 4
// CHECK: [[SOURCE:%.*]] = load ptr, ptr [[BASE]], align 4
// CHECK: call ptr @__dynamic_cast(ptr [[SOURCE]], ptr @_ZTI4Base, ptr @_ZTI7Derived, i32 0)
// CHECK: icmp ne ptr {{.*}}, [[OBJECT]]
