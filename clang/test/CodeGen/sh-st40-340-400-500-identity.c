// RUN: %clang --target=shle-unknown-none-elf -m4-340 -O1 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=ST340
// RUN: %clang --target=shle-unknown-none-elf -m4-400 -O1 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=ST400
// RUN: %clang --target=shle-unknown-none-elf -m4-500 -O1 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=ST500
// RUN: %clang --target=shle-unknown-none-elf -m4-340 -O1 -flto -c -o %t.340.bc %s
// RUN: llvm-dis %t.340.bc -o - | FileCheck %s --check-prefix=ST340
// RUN: %clang --target=shle-unknown-none-elf -m4-400 -O1 -flto -c -o %t.400.bc %s
// RUN: llvm-dis %t.400.bc -o - | FileCheck %s --check-prefix=ST400
// RUN: %clang --target=shle-unknown-none-elf -m4-500 -O1 -flto -c -o %t.500.bc %s
// RUN: llvm-dis %t.500.bc -o - | FileCheck %s --check-prefix=ST500
// RUN: %clang --target=shle-unknown-none-elf -m4-340 -O1 -flto=thin -c -o %t.340.thin.bc %s
// RUN: llvm-dis %t.340.thin.bc -o - | FileCheck %s --check-prefix=ST340
// RUN: %clang --target=shle-unknown-none-elf -m4-400 -O1 -flto=thin -c -o %t.400.thin.bc %s
// RUN: llvm-dis %t.400.thin.bc -o - | FileCheck %s --check-prefix=ST400
// RUN: %clang --target=shle-unknown-none-elf -m4-500 -O1 -flto=thin -c -o %t.500.thin.bc %s
// RUN: llvm-dis %t.500.thin.bc -o - | FileCheck %s --check-prefix=ST500

// ST340-DAG: define{{.*}} @entry({{.*}}){{.*}}#[[ST340_DEFAULT:[0-9]+]]
// ST340-DAG: declare{{.*}} @external({{.*}}){{.*}}#[[ST340_DECL:[0-9]+]]
// ST340-DAG: define{{.*}} @retuned({{.*}}){{.*}}#[[ST340_RETUNED:[0-9]+]]
// ST340-DAG: attributes #[[ST340_DEFAULT]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4" "target-features"="{{[^"]*}}-mmu{{[^"]*}}-single-fpu{{[^"]*}}" "tune-cpu"="st40-340"{{.*}} }
// ST340-DAG: attributes #[[ST340_DECL]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4" "target-features"="{{[^"]*}}-mmu{{[^"]*}}-single-fpu{{[^"]*}}" "tune-cpu"="st40-340"{{.*}} }
// ST340-DAG: attributes #[[ST340_RETUNED]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4" "target-features"="{{[^"]*}}-mmu{{[^"]*}}-single-fpu{{[^"]*}}" "tune-cpu"="st40-500"{{.*}} }
// ST340: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}

// ST400-DAG: define{{.*}} @entry({{.*}}){{.*}}#[[ST400_DEFAULT:[0-9]+]]
// ST400-DAG: declare{{.*}} @external({{.*}}){{.*}}#[[ST400_DECL:[0-9]+]]
// ST400-DAG: define{{.*}} @retuned({{.*}}){{.*}}#[[ST400_RETUNED:[0-9]+]]
// ST400-DAG: attributes #[[ST400_DEFAULT]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4" "target-features"="{{[^"]*}}-mmu{{[^"]*}}-single-fpu{{[^"]*}}" "tune-cpu"="st40-400"{{.*}} }
// ST400-DAG: attributes #[[ST400_DECL]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4" "target-features"="{{[^"]*}}-mmu{{[^"]*}}-single-fpu{{[^"]*}}" "tune-cpu"="st40-400"{{.*}} }
// ST400-DAG: attributes #[[ST400_RETUNED]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4" "target-features"="{{[^"]*}}-mmu{{[^"]*}}-single-fpu{{[^"]*}}" "tune-cpu"="st40-500"{{.*}} }
// ST400: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}

// ST500-DAG: define{{.*}} @entry({{.*}}){{.*}}#[[ST500_DEFAULT:[0-9]+]]
// ST500-DAG: declare{{.*}} @external({{.*}}){{.*}}#[[ST500_DECL:[0-9]+]]
// ST500-DAG: define{{.*}} @retuned({{.*}}){{.*}}#[[ST500_RETUNED:[0-9]+]]
// ST500-DAG: attributes #[[ST500_DEFAULT]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4" "target-features"="{{[^"]*}}+mmu{{[^"]*}}-single-fpu{{[^"]*}}" "tune-cpu"="st40-500"{{.*}} }
// ST500-DAG: attributes #[[ST500_DECL]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4" "target-features"="{{[^"]*}}+mmu{{[^"]*}}-single-fpu{{[^"]*}}" "tune-cpu"="st40-500"{{.*}} }
// ST500-DAG: attributes #[[ST500_RETUNED]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4" "target-features"="{{[^"]*}}+mmu{{[^"]*}}-single-fpu{{[^"]*}}" "tune-cpu"="st40-500"{{.*}} }
// ST500: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}

extern int external(int);

int entry(int value) {
	return external(value) + 1;
}

__attribute__((target("tune=st40-500"))) int retuned(int value) {
	return value + 2;
}
