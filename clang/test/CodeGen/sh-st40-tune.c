// RUN: %clang --target=shle-unknown-none-elf -m4-100 -O1 -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,ST100,HARD
// RUN: %clang --target=shle-unknown-none-elf -m4-200 -O1 -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,ST200,HARD
// RUN: %clang --target=shle-unknown-none-elf -m4 -mtune=st40-100 -O1 -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,ST100,HARD

// RUN: %clang --target=shle-unknown-none-elf -m4-100 -O1 -flto -c -o %t.100.full.bc %s
// RUN: llvm-dis %t.100.full.bc -o - | FileCheck %s --check-prefixes=COMMON,ST100,HARD
// RUN: %clang --target=shle-unknown-none-elf -m4-100-nofpu -O1 -flto -c -o %t.100.nofpu.full.bc %s
// RUN: llvm-dis %t.100.nofpu.full.bc -o - | FileCheck %s --check-prefixes=COMMON,ST100,SOFT
// RUN: %clang --target=shle-unknown-none-elf -m4-100-single -O1 -flto -c -o %t.100.single.full.bc %s
// RUN: llvm-dis %t.100.single.full.bc -o - | FileCheck %s --check-prefixes=COMMON,ST100,HARD-SINGLE
// RUN: %clang --target=shle-unknown-none-elf -m4-100-single-only -O1 -flto -c -o %t.100.single-only.full.bc %s
// RUN: llvm-dis %t.100.single-only.full.bc -o - | FileCheck %s --check-prefixes=COMMON,ST100,E
// RUN: %clang --target=shle-unknown-none-elf -m4-200 -O1 -flto -c -o %t.200.full.bc %s
// RUN: llvm-dis %t.200.full.bc -o - | FileCheck %s --check-prefixes=COMMON,ST200,HARD
// RUN: %clang --target=shle-unknown-none-elf -m4-200-nofpu -O1 -flto -c -o %t.200.nofpu.full.bc %s
// RUN: llvm-dis %t.200.nofpu.full.bc -o - | FileCheck %s --check-prefixes=COMMON,ST200,SOFT
// RUN: %clang --target=shle-unknown-none-elf -m4-200-single -O1 -flto -c -o %t.200.single.full.bc %s
// RUN: llvm-dis %t.200.single.full.bc -o - | FileCheck %s --check-prefixes=COMMON,ST200,HARD-SINGLE
// RUN: %clang --target=shle-unknown-none-elf -m4-200-single-only -O1 -flto -c -o %t.200.single-only.full.bc %s
// RUN: llvm-dis %t.200.single-only.full.bc -o - | FileCheck %s --check-prefixes=COMMON,ST200,E

// RUN: %clang --target=shle-unknown-none-elf -m4-100 -O1 -flto=thin -c -o %t.100.thin.bc %s
// RUN: llvm-dis %t.100.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,ST100,HARD
// RUN: %clang --target=shle-unknown-none-elf -m4-100-nofpu -O1 -flto=thin -c -o %t.100.nofpu.thin.bc %s
// RUN: llvm-dis %t.100.nofpu.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,ST100,SOFT
// RUN: %clang --target=shle-unknown-none-elf -m4-100-single -O1 -flto=thin -c -o %t.100.single.thin.bc %s
// RUN: llvm-dis %t.100.single.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,ST100,HARD-SINGLE
// RUN: %clang --target=shle-unknown-none-elf -m4-100-single-only -O1 -flto=thin -c -o %t.100.single-only.thin.bc %s
// RUN: llvm-dis %t.100.single-only.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,ST100,E
// RUN: %clang --target=shle-unknown-none-elf -m4-200 -O1 -flto=thin -c -o %t.200.thin.bc %s
// RUN: llvm-dis %t.200.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,ST200,HARD
// RUN: %clang --target=shle-unknown-none-elf -m4-200-nofpu -O1 -flto=thin -c -o %t.200.nofpu.thin.bc %s
// RUN: llvm-dis %t.200.nofpu.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,ST200,SOFT
// RUN: %clang --target=shle-unknown-none-elf -m4-200-single -O1 -flto=thin -c -o %t.200.single.thin.bc %s
// RUN: llvm-dis %t.200.single.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,ST200,HARD-SINGLE
// RUN: %clang --target=shle-unknown-none-elf -m4-200-single-only -O1 -flto=thin -c -o %t.200.single-only.thin.bc %s
// RUN: llvm-dis %t.200.single-only.thin.bc -o - | FileCheck %s --check-prefixes=COMMON,ST200,E

// COMMON-DAG: define{{.*}} @entry({{.*}}){{.*}}#[[DEF:[0-9]+]]
// COMMON-DAG: define{{.*}} @per_function_tune({{.*}}){{.*}}#[[PER_FUNCTION:[0-9]+]]
// COMMON-DAG: declare{{.*}} @external({{.*}}){{.*}}#[[DECL:[0-9]+]]
// ST100-DAG: attributes #[[DEF]] = { {{.*}}"target-cpu"="sh4"{{.*}}"tune-cpu"="st40-100"{{.*}} }
// ST200-DAG: attributes #[[DEF]] = { {{.*}}"target-cpu"="sh4"{{.*}}"tune-cpu"="st40-200"{{.*}} }
// HARD-DAG: attributes #[[DECL]] = { {{.*}}"target-abi"="gnu-hard-float"{{.*}}"target-cpu"="sh4"
// SOFT-DAG: attributes #[[DECL]] = { {{.*}}"target-abi"="gnu"{{.*}}"target-cpu"="sh4"
// HARD-SINGLE-DAG: attributes #[[DECL]] = { {{.*}}"target-abi"="gnu-hard-float-single"{{.*}}"target-cpu"="sh4"{{.*}}"target-features"="{{[^"]*}}+fpu-single-mode{{[^"]*}}"
// E-DAG: attributes #[[DECL]] = { {{.*}}"target-abi"="gnu-e-float"{{.*}}"target-cpu"="sh4"{{.*}}"target-features"="{{[^"]*}}+fpu-single-mode{{[^"]*}}-double-fpu{{[^"]*}}"
// ST100-DAG: "tune-cpu"="st40-100"
// ST200-DAG: "tune-cpu"="st40-200"
// SOFT-DAG: "use-soft-float"="true"{{.*}} }
// COMMON-DAG: attributes #[[PER_FUNCTION]] = { {{.*}}"target-cpu"="sh4"{{.*}}"tune-cpu"="st40-200"{{.*}} }

extern int external(int);

int entry(int value) {
	return external(value) + 1;
}

__attribute__((target("tune=st40-200"))) int per_function_tune(int value) {
	return value + 2;
}
