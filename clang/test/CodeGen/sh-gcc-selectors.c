// RUN: %clang --target=shle-unknown-linux-gnu -m1 -O1 -S -emit-llvm -o %t.m1.ll %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh1 -O1 -S -emit-llvm -o %t.sh1.ll %s
// RUN: diff %t.m1.ll %t.sh1.ll
// RUN: FileCheck %s --check-prefix=IR-SH1 < %t.m1.ll
// RUN: %clang --target=shle-unknown-linux-gnu -m2 -O1 -S -emit-llvm -o %t.m2.ll %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -O1 -S -emit-llvm -o %t.sh2.ll %s
// RUN: diff %t.m2.ll %t.sh2.ll
// RUN: FileCheck %s --check-prefix=IR-SH2 < %t.m2.ll
// RUN: %clang --target=shle-unknown-linux-gnu -m3 -O1 -S -emit-llvm -o %t.m3.ll %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh3 -O1 -S -emit-llvm -o %t.sh3.ll %s
// RUN: diff %t.m3.ll %t.sh3.ll
// RUN: FileCheck %s --check-prefix=IR-SH3 < %t.m3.ll
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -O1 -S -emit-llvm -o %t.m4.ll %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh4 -mfloat-abi=hard -O1 -S -emit-llvm -o %t.sh4-hard.ll %s
// RUN: diff %t.m4.ll %t.sh4-hard.ll
// RUN: FileCheck %s --check-prefix=IR-SH4 < %t.m4.ll
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -O2 -S -o - %s | FileCheck %s --check-prefix=ASM-SH4

// RUN: %clang --target=shle-unknown-linux-gnu -m1 -O1 -flto -c -o %t.m1.full.bc %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh1 -O1 -flto -c -o %t.sh1.full.bc %s
// RUN: cmp %t.m1.full.bc %t.sh1.full.bc
// RUN: llvm-dis %t.m1.full.bc -o - | FileCheck %s --check-prefix=IR-SH1
// RUN: %clang --target=shle-unknown-linux-gnu -m1 -O1 -flto=thin -c -o %t.m1.thin.bc %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh1 -O1 -flto=thin -c -o %t.sh1.thin.bc %s
// RUN: cmp %t.m1.thin.bc %t.sh1.thin.bc
// RUN: llvm-dis %t.m1.thin.bc -o - | FileCheck %s --check-prefix=IR-SH1
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -O1 -flto -c -o %t.m4.full.bc %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh4 -mfloat-abi=hard -O1 -flto -c -o %t.sh4-hard.full.bc %s
// RUN: cmp %t.m4.full.bc %t.sh4-hard.full.bc
// RUN: llvm-dis %t.m4.full.bc -o - | FileCheck %s --check-prefix=IR-SH4
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -O1 -flto=thin -c -o %t.m4.thin.bc %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh4 -mfloat-abi=hard -O1 -flto=thin -c -o %t.sh4-hard.thin.bc %s
// RUN: cmp %t.m4.thin.bc %t.sh4-hard.thin.bc
// RUN: llvm-dis %t.m4.thin.bc -o - | FileCheck %s --check-prefix=IR-SH4

// IR-SH1: attributes #[[ATTR:[0-9]+]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh1" "use-soft-float"="true" }
// IR-SH1: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}
// IR-SH2: attributes #[[ATTR:[0-9]+]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh2" "target-features"="+sh2" "use-soft-float"="true" }
// IR-SH2: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}
// IR-SH3: attributes #[[ATTR:[0-9]+]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh3" "target-features"="+banked-gprs,+dynamic-shift,+exception-state,+mmu,+prefetch,+s-bit-control,+sh2" "use-soft-float"="true" }
// IR-SH3: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}
// IR-SH4: attributes #[[ATTR:[0-9]+]] = { {{.*}}"target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="{{[^"]*}}+double-fpu{{[^"]*}}+single-fpu" }
// IR-SH4: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-hard-float"}

// ASM-SH4-LABEL: add_f32:
// ASM-SH4: fmov fr5,fr0
// ASM-SH4: fadd fr4,fr0
// ASM-SH4-LABEL: add_f64:
// ASM-SH4: fadd dr6,dr0

float add_f32(float a, float b) {
	return a + b;
}

double add_f64(double a, double b) {
	return a + b;
}
