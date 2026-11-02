// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -mconstructor-aliases -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefixes=CHECK,BE
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -mconstructor-aliases -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefixes=CHECK,LE
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -O1 -emit-obj -o %t.be.o %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -O1 -emit-obj -o %t.le.o %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -O2 -pic-level 2 -emit-obj -o %t.pic.be.o %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -std=c++17 -fexceptions -fcxx-exceptions -Oz -pic-level 2 -emit-obj -o %t.pic.le.o %s

// BE: target datalayout = "E-
// LE: target datalayout = "e-

struct Result { int words[3]; };

struct V {
	V(int);
	virtual ~V();
	virtual V *self();
	virtual int f();
	virtual Result result(int);
	int v;
};

struct A : virtual V {
	A(int);
	~A() override;
	int f() override;
	int a;
};

struct B : virtual V {
	B(int);
	~B() override;
	int f() override;
	int b;
};

struct D : A, B {
	D(int);
	~D() override;
	D *self() override;
	int f() override;
	Result result(int) override;
	int d;
};

V::V(int x) : v(x) {}
V::~V() {}
V *V::self() { return this; }
int V::f() { return v; }
Result V::result(int x) { return {{v, x, v + x}}; }
A::A(int x) : V(x), a(x) {}
A::~A() {}
int A::f() { return a; }
B::B(int x) : V(x), b(x) {}
B::~B() {}
int B::f() { return b; }
D::D(int x) : V(x), A(x), B(x), d(x) {}
D::~D() {}
D *D::self() { return this; }
int D::f() { return d; }
Result D::result(int x) { return {{d, x, d + x}}; }

static_assert(sizeof(V) == 8);
static_assert(sizeof(A) == 16);
static_assert(sizeof(B) == 16);
static_assert(sizeof(D) == 28);

int V::*data_member = &V::v;
int (V::*virtual_member)() = &V::f;
D global_object(1);

extern "C" D *cast_down(V *p) { return dynamic_cast<D *>(p); }
extern "C" int call_member(D *p) { return (p->*virtual_member)() + p->*data_member; }

// CHECK-DAG: @_ZTV1D = {{.*}}constant { [8 x ptr], [6 x ptr], [11 x ptr] } {{.*}}ptr inttoptr (i32 20 to ptr){{.*}}ptr inttoptr (i32 12 to ptr){{.*}}ptr inttoptr (i32 -8 to ptr){{.*}}ptr inttoptr (i32 -20 to ptr){{.*}}ptr @_ZTcv0_n16_v0_n12_N1D4selfEv{{.*}}ptr @_ZTv0_n24_N1D6resultEi
// CHECK-DAG: @_ZTT1D = {{.*}}constant [7 x ptr] [ptr getelementptr {{.*}} @_ZTV1D{{.*}}ptr getelementptr {{.*}} @_ZTC1D0_1A{{.*}}ptr getelementptr {{.*}} @_ZTC1D8_1B{{.*}}ptr getelementptr {{.*}} @_ZTV1D
// CHECK-DAG: @_ZTC1D0_1A = {{.*}}constant {{.*}}ptr inttoptr (i32 20 to ptr){{.*}}ptr inttoptr (i32 -20 to ptr){{.*}}ptr @_ZTI1A
// CHECK-DAG: @_ZTC1D8_1B = {{.*}}constant {{.*}}ptr inttoptr (i32 12 to ptr){{.*}}ptr inttoptr (i32 -12 to ptr){{.*}}ptr @_ZTI1B
// CHECK-DAG: @_ZTI1D = {{.*}}constant { ptr, ptr, i32, i32, ptr, i32, ptr, i32 } {{.*}}i32 2, i32 2, ptr @_ZTI1A, i32 2, ptr @_ZTI1B, i32 2050
// CHECK-DAG: @_ZTI1A = {{.*}}constant { ptr, ptr, i32, i32, ptr, i32 } {{.*}}ptr @_ZTI1V, i32 -3069
// CHECK-DAG: @data_member = {{.*}}global i32 4
// CHECK-DAG: @virtual_member = {{.*}}global { i32, i32 } { i32 13, i32 0 }

// CHECK-DAG: define {{.*}}void @_ZN1AC2Ei(ptr {{.*}}%this, ptr {{.*}}%vtt, i32 {{.*}}%x)
// CHECK-DAG: define {{.*}}void @_ZN1AC1Ei(ptr {{.*}}%this, i32 {{.*}}%x)
// CHECK-DAG: define {{.*}}void @_ZN1AD2Ev(ptr {{.*}}%this, ptr {{.*}}%vtt)
// CHECK-DAG: define {{.*}}void @_ZN1AD1Ev(ptr {{.*}}%this)
// CHECK-DAG: define {{.*}}void @_ZN1AD0Ev(ptr {{.*}}%this)
// CHECK-DAG: define {{.*}}ptr @_ZTcv0_n16_v0_n12_N1D4selfEv(ptr
// CHECK-DAG: define {{.*}}i32 @_ZTv0_n20_N1D1fEv(ptr
// CHECK-DAG: define {{.*}}void @_ZTv0_n24_N1D6resultEi(ptr {{.*}}sret(%struct.Result){{.*}}, ptr {{.*}}%this, i32
// CHECK-DAG: call ptr @__dynamic_cast(ptr {{.*}}, ptr @_ZTI1V, ptr @_ZTI1D, i32 -1)
