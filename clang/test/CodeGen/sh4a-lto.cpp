// RUN: %clang --target=shle-unknown-linux-gnu -m4a -fcxx-exceptions -fexceptions -std=c++17 -O2 -flto -c -o %t.le.bc %s
// RUN: llvm-dis %t.le.bc -o - | FileCheck %s --check-prefix=IR
// RUN: llvm-lto -O2 -exported-symbol=_Z12sh4a_throwerv -o %t.le.o %t.le.bc
// RUN: llvm-readobj --file-headers %t.le.o | FileCheck %s --check-prefix=ELF
// RUN: %clang --target=sh-unknown-linux-gnu -m4a -fcxx-exceptions -fexceptions -std=c++17 -O2 -flto -c -o %t.be.bc %s
// RUN: llvm-lto -O2 -exported-symbol=_Z12sh4a_throwerv -o %t.be.o %t.be.bc
// RUN: llvm-readobj --file-headers %t.be.o | FileCheck %s --check-prefix=ELF
// RUN: %clang --target=shle-unknown-linux-gnu -m4a -fcxx-exceptions -fexceptions -std=c++17 -O2 -flto=thin -c -o %t.thin.bc %s
// RUN: llvm-dis %t.thin.bc -o - | FileCheck %s --check-prefix=IR
// RUN: llvm-lto -O2 --thinlto-action=run -exported-symbol=_Z12sh4a_throwerv %t.thin.bc
// RUN: llvm-readobj --file-headers %t.thin.bc.thinlto.o | FileCheck %s --check-prefix=ELF

// IR: define{{.*}} void @_Z12sh4a_throwerv(){{.*}} #[[USER:[0-9]+]]
// IR: declare ptr @__cxa_allocate_exception(i32){{.*}} #[[ALLOC:[0-9]+]]
// IR: declare void @__cxa_throw(ptr, ptr, ptr){{.*}} #[[THROW:[0-9]+]]
// IR: attributes #[[USER]] = { {{.*}}"target-abi"="gnu-hard-float" "target-cpu"="sh4a" "target-features"="{{[^"]*}}+sh4a{{[^"]*}}+sh4a-common{{[^"]*}}" }
// IR: attributes #[[ALLOC]] = { "target-cpu"="sh4a" "target-features"="{{[^"]*}}+sh4a{{[^"]*}}+sh4a-common{{[^"]*}}" }
// IR: attributes #[[THROW]] = { {{.*}}"target-cpu"="sh4a" "target-features"="{{[^"]*}}+sh4a{{[^"]*}}+sh4a-common{{[^"]*}}" }
// IR: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-hard-float"}

// ELF: Flags [ (0xC)

void sh4a_thrower() {
	throw 1;
}
