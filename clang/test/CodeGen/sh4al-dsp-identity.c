// RUN: %clang --target=shle-unknown-none-elf -m4al -O1 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=ARCH
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh4al-dsp -O1 -flto -c -o %t.full.bc %s
// RUN: llvm-dis %t.full.bc -o - | FileCheck %s --check-prefix=ARCH
// RUN: %clang --target=shle-unknown-none-elf -m4al -O1 -flto=thin -c -o %t.thin.bc %s
// RUN: llvm-dis %t.thin.bc -o - | FileCheck %s --check-prefix=ARCH
// RUN: %clang --target=shle-unknown-none-elf -m3 -mtune=sh4al-dsp -O1 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=TUNE-ONLY
// RUN: %clang --target=sh-unknown-none-elf -m4al -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -m4al -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s

// ARCH-DAG: define{{.*}} @entry({{.*}}){{.*}}#[[DEFAULT:[0-9]+]]
// ARCH-DAG: define{{.*}} @retuned({{.*}}){{.*}}#[[RETUNED:[0-9]+]]
// ARCH-DAG: declare{{.*}} @external({{.*}}){{.*}}#[[DECL:[0-9]+]]
// ARCH-DAG: attributes #[[DEFAULT]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4a,+sh4a-common,+sh4al-dsp{{[^"]*}}"{{.*}} }
// ARCH-DAG: attributes #[[RETUNED]] = { {{.*}}"target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4al-dsp{{[^"]*}}" "tune-cpu"="sh4a"{{.*}} }
// ARCH-DAG: attributes #[[DECL]] = { {{.*}}"target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4al-dsp{{[^"]*}}"{{.*}} }
// ARCH: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}

// TUNE-ONLY-DAG: attributes #[[TUNE:[0-9]+]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh3" "target-features"="{{[^"]*}}+sh2{{[^"]*}}" "tune-cpu"="sh4al-dsp"{{.*}} }
// TUNE-ONLY-NOT: +dsp
// TUNE-ONLY-NOT: +sh4al-dsp

extern int external(int);

int entry(int value) {
	return external(value) + 1;
}

__attribute__((target("tune=sh4a"))) int retuned(int value) {
	return value + 2;
}
