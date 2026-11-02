// RUN: %clang --target=shle-unknown-linux-gnu -m4al -std=c++17 -fcxx-exceptions -fexceptions -O1 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang --target=sh-unknown-linux-gnu -m4al -std=c++17 -fcxx-exceptions -fexceptions -g -O1 -c -mllvm -verify-machineinstrs -o %t.be.o %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4al -std=c++17 -fcxx-exceptions -fexceptions -g -O1 -c -mllvm -verify-machineinstrs -o %t.le.o %s
// RUN: llvm-readobj --file-headers %t.be.o %t.le.o | FileCheck %s --check-prefix=ELF-DIRECT
// RUN: llvm-dwarfdump --verify %t.be.o %t.le.o 2>&1 | FileCheck %s --check-prefix=DWARF
// RUN: %clang --target=shle-unknown-linux-gnu -m4al -std=c++17 -fcxx-exceptions -fexceptions -O1 -flto -c -o %t.bc %s
// RUN: llvm-lto -O1 -exported-symbol=_Z12call_virtualP4Base9Aggregate -o %t.lto.o %t.bc
// RUN: llvm-readobj --file-headers %t.lto.o | FileCheck %s --check-prefix=ELF-LTO

// IR-DAG: define{{.*}} @_ZN4BaseC2Ei(ptr{{.*}} %this, i32{{.*}} %value){{.*}} #[[CTOR:[0-9]+]]
// IR-DAG: define{{.*}} @_ZN4BaseD2Ev(ptr{{.*}} %this){{.*}} #[[DTOR:[0-9]+]]
// IR-DAG: define{{.*}} @_ZN4Base6methodE9Aggregatei(ptr{{.*}}sret(%struct.Aggregate){{.*}}, ptr{{.*}}%this, ptr{{.*}}byval(%struct.Aggregate){{.*}}, i32{{.*}}){{.*}} #[[METHOD:[0-9]+]]
// IR-DAG: define{{.*}} @_Z12call_virtualP4Base9Aggregate(ptr{{.*}}sret(%struct.Aggregate){{.*}}, ptr{{.*}}, ptr{{.*}}byval(%struct.Aggregate){{.*}}){{.*}} #[[CALL:[0-9]+]] personality ptr @__gxx_personality_v0
// IR-DAG: @_ZTV4Base = {{.*}}constant
// IR-DAG: @_ZTI4Base = {{.*}}constant
// IR-DAG: declare i32 @__gxx_personality_v0(...) #[[DECL:[0-9]+]]
// IR-DAG: attributes #[[CTOR]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4al-dsp{{[^"]*}}"{{.*}} }
// IR-DAG: attributes #[[DTOR]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4al-dsp{{[^"]*}}"{{.*}} }
// IR-DAG: attributes #[[METHOD]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4al-dsp{{[^"]*}}"{{.*}} }
// IR-DAG: attributes #[[CALL]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4al-dsp{{[^"]*}}"{{.*}} }
// IR-DAG: attributes #[[DECL]] = { "target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4al-dsp{{[^"]*}}" }
// IR: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}
// ELF-DIRECT-COUNT-2: Flags [ (0x6)
// ELF-LTO: Flags [ (0x6)
// DWARF: No errors.

struct Aggregate {
	int first;
	double second;
};

class Base {
public:
	Base(int value);
	virtual ~Base();
	virtual Aggregate method(Aggregate value, int extra);

private:
	int value;
};

Base::Base(int value) : value(value) {}
Base::~Base() = default;

Aggregate Base::method(Aggregate input, int extra) {
	input.first += value + extra;
	return input;
}

Aggregate call_virtual(Base *object, Aggregate value) {
	try {
		return object->method(value, 3);
	} catch (int) {
		return {0, 0.0};
	}
}
