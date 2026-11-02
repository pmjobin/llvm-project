// RUN: %clang --target=shle-unknown-none-elf -m4-300 -O1 -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,FULL

// RUN: %clang --target=shle-unknown-none-elf -m4-300 -O1 -flto -c -o %t.full.bc %s
// RUN: llvm-dis %t.full.bc -o - | FileCheck %s --check-prefixes=COMMON,FULL
// RUN: %clang --target=shle-unknown-none-elf -m4-300-nofpu -O1 -flto -c -o %t.nofpu.bc %s
// RUN: llvm-dis %t.nofpu.bc -o - | FileCheck %s --check-prefixes=COMMON,NOFPU
// RUN: %clang --target=shle-unknown-none-elf -m4-300-single -O1 -flto -c -o %t.single.bc %s
// RUN: llvm-dis %t.single.bc -o - | FileCheck %s --check-prefixes=COMMON,SINGLE
// RUN: %clang --target=shle-unknown-none-elf -m4-300-single-only -O1 -flto -c -o %t.single-only.bc %s
// RUN: llvm-dis %t.single-only.bc -o - | FileCheck %s --check-prefixes=COMMON,SINGLE-ONLY

// RUN: %clang --target=shle-unknown-none-elf -m4-300 -O1 -flto=thin -c -o %t.full.thin.bc %s
// RUN: llvm-dis %t.full.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,FULL
// RUN: %clang --target=shle-unknown-none-elf -m4-300-nofpu -O1 -flto=thin -c -o %t.nofpu.thin.bc %s
// RUN: llvm-dis %t.nofpu.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,NOFPU
// RUN: %clang --target=shle-unknown-none-elf -m4-300-single -O1 -flto=thin -c -o %t.single.thin.bc %s
// RUN: llvm-dis %t.single.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,SINGLE
// RUN: %clang --target=shle-unknown-none-elf -m4-300-single-only -O1 -flto=thin -c -o %t.single-only.thin.bc %s
// RUN: llvm-dis %t.single-only.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,SINGLE-ONLY

// COMMON-DAG: define{{.*}} @entry({{.*}}){{.*}}#[[DEFAULT:[0-9]+]]
// COMMON-DAG: define{{.*}} @generic_sh4({{.*}}){{.*}}#[[GENERIC:[0-9]+]]
// COMMON-DAG: define{{.*}} @tune_st40_100({{.*}}){{.*}}#[[TUNE100:[0-9]+]]
// COMMON-DAG: define{{.*}} @tune_st40_200({{.*}}){{.*}}#[[TUNE200:[0-9]+]]
// COMMON-DAG: declare{{.*}} @external({{.*}}){{.*}}#[[DECL:[0-9]+]]
// COMMON-DAG: attributes #[[DEFAULT]] = { {{.*}}"target-cpu"="sh4"{{.*}}"target-features"="{{[^"]*}}+st40-300{{[^"]*}}"{{.*}}"tune-cpu"="st40-300"{{.*}} }
// COMMON-DAG: attributes #[[GENERIC]] = { {{.*}}"target-cpu"="sh4"{{.*}}"target-features"="{{[^"]*}}-st40-300{{[^"]*}}"{{.*}}"tune-cpu"="st40-300"{{.*}} }
// COMMON-DAG: attributes #[[TUNE100]] = { {{.*}}"target-features"="{{[^"]*}}+st40-300{{[^"]*}}"{{.*}}"tune-cpu"="st40-100"{{.*}} }
// COMMON-DAG: attributes #[[TUNE200]] = { {{.*}}"target-features"="{{[^"]*}}+st40-300{{[^"]*}}"{{.*}}"tune-cpu"="st40-200"{{.*}} }
// COMMON-DAG: attributes #[[DECL]] = { {{.*}}"target-cpu"="sh4"{{.*}}"target-features"="{{[^"]*}}+st40-300{{[^"]*}}"{{.*}}"tune-cpu"="st40-300"{{.*}} }
// FULL: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-hard-float"}
// NOFPU: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}
// SINGLE: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-hard-float-single"}
// SINGLE-ONLY: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-e-float"}

extern int external(int);

int entry(int value) {
	return external(value) + 1;
}

__attribute__((target("no-st40-300,tune=st40-300"))) int generic_sh4(int value) {
	return value + 2;
}

__attribute__((target("tune=st40-100"))) int tune_st40_100(int value) {
	return value + 3;
}

__attribute__((target("tune=st40-200"))) int tune_st40_200(int value) {
	return value + 4;
}
