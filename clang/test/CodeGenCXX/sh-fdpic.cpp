// RUN: %clang_cc1 -triple sh2-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 1 -std=c++17 -O2 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple sh2eb-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 1 -std=c++17 -O2 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple sh2-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 1 -std=c++17 -O2 -emit-obj -o %t.le.o %s
// RUN: %clang_cc1 -triple sh2eb-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 1 -std=c++17 -O2 -emit-obj -o %t.be.o %s
// RUN: llvm-readobj --file-headers --relocations %t.le.o %t.be.o | FileCheck %s --check-prefix=OBJECT

struct Base {
	int value;
	virtual int method(int argument);
	virtual ~Base();
	int ordinary(int argument);
};

int Base::method(int argument) { return value + argument; }
Base::~Base() {}
int Base::ordinary(int argument) { return value - argument; }

struct Side {
	virtual int side(int argument);
};

int Side::side(int argument) { return argument + 1; }

struct Derived : Base, Side {
	int method(int argument) override;
	__attribute__((noinline)) int side(int argument) override;
};

int Derived::method(int argument) { return argument + 2; }
int Derived::side(int argument) { return value + argument + 3; }

struct Abstract {
	virtual void pure() = 0;
	virtual ~Abstract();
};

Abstract::~Abstract() {}

struct Anchor {
	int anchor_value;
	virtual int anchor();
};

struct CovBase {
	virtual CovBase *copy();
};

struct CovDerived : Anchor, CovBase {
	int anchor() override;
	__attribute__((noinline)) CovDerived *copy() override;
};

int Anchor::anchor() { return anchor_value + 8; }
CovBase *CovBase::copy() { return this; }
int CovDerived::anchor() { return anchor_value + 9; }
CovDerived *CovDerived::copy() { return anchor_value ? this : nullptr; }

struct VirtualBase {
	virtual int virtual_adjust(int argument);
};

struct VirtualDerived : Anchor, virtual VirtualBase {
	__attribute__((noinline)) int virtual_adjust(int argument) override;
};

int VirtualBase::virtual_adjust(int argument) { return argument + 10; }
int VirtualDerived::virtual_adjust(int argument) {
	return anchor_value + argument + 11;
}

int free_function(int argument) { return argument + 4; }
int overloaded(int argument) { return argument + 5; }
int overloaded(int left, int right) { return left + right; }

struct Utility {
	static int static_method(int argument);
};

int Utility::static_method(int argument) { return argument + 6; }

int lifecycle_count;

struct Lifecycle {
	Lifecycle();
	virtual ~Lifecycle();
};

Lifecycle::Lifecycle() { ++lifecycle_count; }
Lifecycle::~Lifecycle() { ++lifecycle_count; }

int (*global_function_pointer)(int) = &free_function;
int (*global_overload_pointer)(int) = static_cast<int (*)(int)>(&overloaded);
int (*global_static_pointer)(int) = &Utility::static_method;
int (Base::*nonvirtual_member_pointer)(int) = &Base::ordinary;
int (Base::*virtual_member_pointer)(int) = &Base::method;

struct MemberHolder {
	int (Base::*member)(int);
};

MemberHolder member_holder = {&Base::ordinary};
int (Base::*member_array[2])(int) = {nullptr, &Base::method};
Derived global_derived;
CovDerived global_covariant;
VirtualDerived global_virtual_derived;
Lifecycle global_lifecycle;

int call_virtual(Base *object, int argument) {
	return object->method(argument);
}

int call_member(Base *object, int (Base::*member)(int), int argument) {
	return (object->*member)(argument);
}

int (Base::*return_member())(int) {
	return &Base::ordinary;
}

bool member_is_null(int (Base::*member)(int)) {
	return member == nullptr;
}

bool members_equal(int (Base::*left)(int), int (Base::*right)(int)) {
	return left == right;
}

Base *rtti_cast(Side *object) {
	return dynamic_cast<Base *>(object);
}

extern int runtime_initializer();

int guarded_local_static() {
	static int value = runtime_initializer();
	return value;
}

// ASM-LABEL: _ZN4Base8ordinaryEi:
// ASM: mov.l	@(4,r4),

// ASM-LABEL: _ZThn8_N7Derived4sideEi:
// ASM: add	#-8,r4
// ASM: bsrf

// ASM-LABEL: _ZTchn8_h8_N10CovDerived4copyEv:
// ASM: add	#-8,r4
// ASM: bsrf
// ASM: add	#8,

// ASM-LABEL: _ZTv0_n12_N14VirtualDerived14virtual_adjustEi:
// ASM: mov.l	@r4,[[VIRTUAL_VTABLE:r[0-9]+]]
// ASM: add	#-12,[[VIRTUAL_VTABLE]]
// ASM: mov.l	@[[VIRTUAL_VTABLE]],[[VIRTUAL_ADJUST:r[0-9]+]]
// ASM: add	[[VIRTUAL_ADJUST]],r4
// ASM: bsrf

// ASM-LABEL: _Z12call_virtualP4Basei:
// ASM: mov.l	@r4,[[VTABLE:r[0-9]+]]
// ASM-NEXT: mov.l	@[[VTABLE]],[[DESC:r[0-9]+]]
// ASM-NEXT: mov.l	@[[DESC]],[[ENTRY:r[0-9]+]]
// ASM-NEXT: mov.l	@(4,[[DESC]]),r12
// ASM-NEXT: jsr	@[[ENTRY]]

// ASM-LABEL: _Z11call_memberP4BaseMS_FiiEi:
// ASM: mov	r5,r0
// ASM-NEXT: tst	#1,r0
// ASM-NEXT: bt/s
// ASM-NEXT: add	r6,r4
// ASM: mov.l	@r4,[[MEMBER_VTABLE:r[0-9]+]]
// ASM-NEXT: add	r0,[[MEMBER_VTABLE]]
// ASM-NEXT: add	#-1,[[MEMBER_VTABLE]]
// ASM-NEXT: mov.l	@[[MEMBER_VTABLE]],r0
// ASM: mov.l	@r0,[[MEMBER_ENTRY:r[0-9]+]]
// ASM-NEXT: mov.l	@(4,r0),r12
// ASM-NEXT: jsr	@[[MEMBER_ENTRY]]
// ASM-NEXT: mov	r7,r5

// ASM-LABEL: _Z13return_memberv:
// ASM: .long	_ZN4Base8ordinaryEi@GOTFUNCDESC

// ASM-LABEL: _Z14member_is_nullM4BaseFiiE:
// ASM: tst	r4,r4

// ASM-LABEL: _Z13members_equalM4BaseFiiES1_:
// ASM: cmp/eq

// ASM-LABEL: _Z9rtti_castP4Side:
// ASM: .long	_ZTI4Side@GOT
// ASM: .long	_ZTI4Base@GOT
// ASM: .long	__dynamic_cast@PLT

// ASM-LABEL: _Z20guarded_local_staticv:
// ASM: .long	__cxa_guard_acquire@PLT
// ASM: .long	_Z19runtime_initializerv@PLT
// ASM: .long	__cxa_guard_release@PLT

// ASM: global_function_pointer:
// ASM-NEXT: .long	_Z13free_functioni@FUNCDESC
// ASM: global_overload_pointer:
// ASM-NEXT: .long	_Z10overloadedi@FUNCDESC
// ASM: global_static_pointer:
// ASM-NEXT: .long	_ZN7Utility13static_methodEi@FUNCDESC
// ASM: nonvirtual_member_pointer:
// ASM-NEXT: .long	_ZN4Base8ordinaryEi@FUNCDESC
// ASM: virtual_member_pointer:
// ASM-NEXT: .long	1
// ASM-NEXT: .long	0
// ASM: _ZTV4Base:
// ASM: .long	_ZN4Base6methodEi@FUNCDESC
// ASM-NEXT: .long	_ZN4BaseD1Ev@FUNCDESC
// ASM-NEXT: .long	_ZN4BaseD0Ev@FUNCDESC
// ASM: _ZTV7Derived:
// ASM: .long	_ZN7Derived6methodEi@FUNCDESC
// ASM: .long	_ZThn8_N7Derived4sideEi@FUNCDESC
// ASM: _ZTV8Abstract:
// ASM: .long	__cxa_pure_virtual@FUNCDESC
// ASM: _ZTV10CovDerived:
// ASM: .long	_ZTchn8_h8_N10CovDerived4copyEv@FUNCDESC
// ASM: _ZTV14VirtualDerived:
// ASM: .long	_ZTv0_n12_N14VirtualDerived14virtual_adjustEi@FUNCDESC

// OBJECT-COUNT-2: Flags [ (0x8002)
// OBJECT-DAG: R_SH_FUNCDESC _Z13free_functioni
// OBJECT-DAG: R_SH_FUNCDESC _Z10overloadedi
// OBJECT-DAG: R_SH_FUNCDESC _ZN7Utility13static_methodEi
// OBJECT-DAG: R_SH_FUNCDESC _ZN4Base8ordinaryEi
// OBJECT-DAG: R_SH_FUNCDESC _ZN7Derived6methodEi
// OBJECT-DAG: R_SH_FUNCDESC _ZThn8_N7Derived4sideEi
// OBJECT-DAG: R_SH_FUNCDESC _ZTchn8_h8_N10CovDerived4copyEv
// OBJECT-DAG: R_SH_FUNCDESC _ZTv0_n12_N14VirtualDerived14virtual_adjustEi
// OBJECT-DAG: R_SH_FUNCDESC __cxa_pure_virtual
// OBJECT-DAG: R_SH_DIR32 _ZTI4Base
// OBJECT-DAG: R_SH_GOT32 _ZTI4Side
// OBJECT-DAG: R_SH_GOT32 _ZTI4Base
// OBJECT-DAG: R_SH_PLT32 __dynamic_cast
// OBJECT-DAG: R_SH_PLT32 __cxa_guard_acquire
// OBJECT-DAG: R_SH_PLT32 __cxa_guard_release
// OBJECT-DAG: R_SH_GOTFUNCDESC _ZN9LifecycleD1Ev
