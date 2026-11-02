// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -std=c++17 -mconstructor-aliases -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefixes=CHECK,BE
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -std=c++17 -mconstructor-aliases -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefixes=CHECK,LE
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -std=c++17 -mconstructor-aliases -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefix=CHECK
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -std=c++17 -mconstructor-aliases -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefix=CHECK
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh3 -std=c++17 -mconstructor-aliases -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefixes=CHECK,BE
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh3 -std=c++17 -mconstructor-aliases -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefixes=CHECK,LE
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh3 -std=c++17 -mconstructor-aliases -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefix=CHECK
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh3 -std=c++17 -mconstructor-aliases -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefix=CHECK

// BE: target datalayout = "E-
// LE: target datalayout = "e-

namespace names {
int overload(int value) {
	return value;
}

int overload(long value) {
	return value;
}
}

struct Large {
	int values[3];
};

struct Object {
	int value;
	Object(int initial);
	virtual ~Object();
	int method(int argument) const volatile;
	virtual Object *self();
	Large large(int argument) const;
};

Object::Object(int initial) : value(initial) {}
Object::~Object() {}

int Object::method(int argument) const volatile {
	return value + argument;
}

Object *Object::self() {
	return this;
}

struct Abstract {
	virtual void pure() = 0;
	virtual ~Abstract();
};

Abstract::~Abstract() {}

struct Side {
	int side_value;
	virtual int side() const;
};

struct Multi : Object, Side {
	Multi();
	int side() const override;
};

int Side::side() const {
	return side_value;
}

Multi::Multi() : Object(1), Side() {}

int Multi::side() const {
	return value + side_value;
}

Large Object::large(int argument) const {
	return {{value, argument, value + argument}};
}

int Object::*data_null = nullptr;
int Object::*data_value = &Object::value;
int (Object::*nonvirtual_member)(int) const volatile = &Object::method;
Object *(Object::*virtual_member)() = &Object::self;

template <typename T> T twice(T value) {
	return value + value;
}

int instantiate(int value) {
	return twice(value);
}

Object &local_object() {
	static Object object(3);
	return object;
}

Object global_object(7);

extern "C" int c_function(int value) {
	return value;
}

// CHECK-DAG: @_ZTV6Object = {{.*}}constant { [5 x ptr] } { [5 x ptr] [ptr null, ptr @_ZTI6Object, ptr @_ZN6ObjectD1Ev, ptr @_ZN6ObjectD0Ev, ptr @_ZN6Object4selfEv] }
// CHECK-DAG: @_ZTI6Object = {{.*}}constant { ptr, ptr }
// CHECK-DAG: @_ZTS6Object = {{.*}}constant [8 x i8] c"6Object\00"
// CHECK-DAG: @_ZTV5Multi = {{.*}}constant {{.*}}ptr inttoptr (i32 -8 to ptr){{.*}}ptr @_ZThn8_NK5Multi4sideEv
// CHECK-DAG: @_ZTV8Abstract = {{.*}}constant { [5 x ptr] } { [5 x ptr] [ptr null, ptr @_ZTI8Abstract, ptr @__cxa_pure_virtual, ptr @_ZN8AbstractD1Ev, ptr @_ZN8AbstractD0Ev] }
// CHECK-DAG: @data_null = {{.*}}global i32 -1
// CHECK-DAG: @data_value = {{.*}}global i32 4
// CHECK-DAG: @nonvirtual_member = {{.*}}global { i32, i32 } { i32 ptrtoint (ptr @_ZNVK6Object6methodEi to i32), i32 0 }
// CHECK-DAG: @virtual_member = {{.*}}global { i32, i32 } { i32 9, i32 0 }
// CHECK-DAG: @_ZGVZ12local_objectvE6object = internal global i64 0, align 4
// CHECK-DAG: @llvm.global_ctors = appending global

// CHECK-DAG: @_ZN6ObjectC1Ei = {{.*}}alias void (ptr, i32), ptr @_ZN6ObjectC2Ei
// CHECK-DAG: @_ZN6ObjectD1Ev = {{.*}}alias void (ptr), ptr @_ZN6ObjectD2Ev
// CHECK-DAG: define {{.*}}i32 @_ZN5names8overloadEi(i32
// CHECK-DAG: define {{.*}}i32 @_ZN5names8overloadEl(i32
// CHECK-DAG: define {{.*}}i32 @_ZNVK6Object6methodEi(ptr
// CHECK-DAG: define {{.*}}void @_ZNK6Object5largeEi(ptr {{.*}}sret(%struct.Large){{.*}}, ptr {{.*}}%this, i32
// CHECK-DAG: define {{.*}}i32 @_ZThn8_NK5Multi4sideEv(ptr
// CHECK-DAG: define {{.*}}void @_ZN8AbstractD0Ev(ptr
// CHECK-DAG: call void @llvm.trap()
// CHECK-DAG: define linkonce_odr {{.*}}i32 @_Z5twiceIiET_S0_(i32 {{.*}}){{.*}} comdat
// CHECK-DAG: define {{.*}}i32 @c_function(i32

// CHECK-DAG: call i32 @__cxa_guard_acquire(ptr @_ZGVZ12local_objectvE6object)
// CHECK-DAG: call void @__cxa_guard_release(ptr @_ZGVZ12local_objectvE6object)
// CHECK-DAG: call i32 @__cxa_atexit(ptr @_ZN6ObjectD1Ev, ptr @_ZZ12local_objectvE6object, ptr @__dso_handle)
// CHECK-DAG: call i32 @__cxa_atexit(ptr @_ZN6ObjectD1Ev, ptr @global_object, ptr @__dso_handle)
