// RUN: %clang --target=sh2-linux-muslfdpic -mcpu=sh2 -std=c++17 -O2 -flto -c -o %t.le.bc %s
// RUN: %clang --target=sh2eb-linux-muslfdpic -mcpu=sh2 -std=c++17 -O2 -flto -c -o %t.be.bc %s
// RUN: llvm-dis %t.le.bc -o - | FileCheck %s --check-prefix=IR
// RUN: llvm-lto -O2 -j1 -exported-symbol=fdpic_language_entry -o %t.le.o %t.le.bc
// RUN: llvm-lto -O2 -j1 -exported-symbol=fdpic_language_entry -o %t.be.o %t.be.bc
// RUN: llvm-readobj --file-headers --relocations %t.le.o %t.be.o | FileCheck %s --check-prefix=OBJECT
// RUN: %clang --target=sh2-linux-muslfdpic -mcpu=sh2 -std=c++17 -O2 -flto=thin -c -o %t.le.thin.bc %s
// RUN: %clang --target=sh2eb-linux-muslfdpic -mcpu=sh2 -std=c++17 -O2 -flto=thin -c -o %t.be.thin.bc %s
// RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=fdpic_language_entry %t.le.thin.bc
// RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=fdpic_language_entry %t.be.thin.bc
// RUN: llvm-readobj --file-headers --relocations %t.le.thin.bc.thinlto.o %t.be.thin.bc.thinlto.o | FileCheck %s --check-prefix=OBJECT

struct Base {
	virtual int method(int argument);
	virtual ~Base();
};

int Base::method(int argument) { return argument + 1; }
Base::~Base() {}

extern thread_local int tls_value;
Base global_base;

static int plain_function(int argument) { return argument + 5; }
__attribute__((used)) int (*global_function_pointer)(int) = plain_function;

extern "C" int fdpic_language_entry(Base *object, int (Base::*member)(int), int argument) {
	try {
		if (argument == 0)
			throw 7;
	} catch (int value) {
		argument = value;
	}
	if (!object)
		object = &global_base;
	return object->method(argument) + (object->*member)(argument) +
		global_function_pointer(argument) + tls_value;
}

// IR: target triple = "sh2-unknown-linux-muslfdpic"
// IR: define{{.*}} @fdpic_language_entry{{.*}} #[[USER:[0-9]+]]
// IR: declare ptr @__cxa_allocate_exception(i32){{.*}} #[[ALLOC:[0-9]+]]
// IR: declare void @__cxa_throw(ptr, ptr, ptr){{.*}} #[[THROW:[0-9]+]]
// IR: attributes #[[USER]] = { {{.*}}"target-abi"="gnu-fdpic" "target-cpu"="sh2"{{.*}} }
// IR: attributes #[[ALLOC]] = { {{.*}}"target-cpu"="sh2"{{.*}} }
// IR: attributes #[[THROW]] = { {{.*}}"target-cpu"="sh2"{{.*}} }
// IR: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-fdpic"}

// OBJECT-COUNT-2: Flags [ (0x8002)
// OBJECT-DAG: R_SH_FUNCDESC _ZN4Base6methodEi
// OBJECT-DAG: R_SH_FUNCDESC _ZL14plain_functioni
// OBJECT-DAG: R_SH_GOTFUNCDESC __gxx_personality_v0
// OBJECT-DAG: R_SH_TLS_GD_32 tls_value
// OBJECT-DAG: R_SH_PLT32 __tls_get_addr
// OBJECT-DAG: R_SH_PLT32 __cxa_throw
