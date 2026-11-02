// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -fcxx-exceptions -fexceptions -std=c++17 -O0 -flto-unit -emit-llvm-bc -o %t.le-o0.bc %s
// RUN: llvm-dis %t.le-o0.bc -o - | FileCheck %s --check-prefix=IR
// RUN: llvm-lto -O0 -exported-symbol=_Z9reproducev -o %t.le-o0.o %t.le-o0.bc
// RUN: llvm-readobj --file-headers %t.le-o0.o | FileCheck %s --check-prefix=SH4
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -fcxx-exceptions -fexceptions -std=c++17 -O2 -flto-unit -emit-llvm-bc -o %t.le-o2.bc %s
// RUN: llvm-lto -O2 -exported-symbol=_Z9reproducev -o %t.le-o2.o %t.le-o2.bc
// RUN: llvm-readobj --file-headers %t.le-o2.o | FileCheck %s --check-prefix=SH4
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -fcxx-exceptions -fexceptions -std=c++17 -O0 -flto-unit -emit-llvm-bc -o %t.be-o0.bc %s
// RUN: llvm-lto -O0 -exported-symbol=_Z9reproducev -o %t.be-o0.o %t.be-o0.bc
// RUN: llvm-readobj --file-headers %t.be-o0.o | FileCheck %s --check-prefix=SH4
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -fcxx-exceptions -fexceptions -std=c++17 -O2 -flto-unit -emit-llvm-bc -o %t.be-o2.bc %s
// RUN: llvm-lto -O2 -exported-symbol=_Z9reproducev -o %t.be-o2.o %t.be-o2.bc
// RUN: llvm-readobj --file-headers %t.be-o2.o | FileCheck %s --check-prefix=SH4
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -fcxx-exceptions -fexceptions -std=c++17 -O0 -flto -c -o %t.alias-full.bc %s
// RUN: llvm-dis %t.alias-full.bc -o - | FileCheck %s --check-prefix=IR
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -fcxx-exceptions -fexceptions -std=c++17 -O0 -flto=thin -c -o %t.alias-thin.bc %s
// RUN: llvm-dis %t.alias-thin.bc -o - | FileCheck %s --check-prefix=IR

// IR: define{{.*}} void @_Z9reproducev() #[[USER:[0-9]+]]
// IR: declare ptr @__cxa_allocate_exception(i32) #[[RUNTIME:[0-9]+]]
// IR: declare void @__cxa_throw(ptr, ptr, ptr) #[[RUNTIME]]
// IR: attributes #[[USER]] = { {{.*}}"target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="{{[^"]*}}+double-fpu{{[^"]*}}"{{.*}} }
// IR: attributes #[[RUNTIME]] = { "target-cpu"="sh4" "target-features"="{{[^"]*}}+double-fpu{{[^"]*}}" }
// IR: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-hard-float"}

// SH4: Flags [ (0x9)

void reproduce() {
	throw 1;
}
