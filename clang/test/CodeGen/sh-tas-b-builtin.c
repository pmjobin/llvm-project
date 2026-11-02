// RUN: %clang_cc1 -triple sh-unknown-elf -O1 -emit-llvm %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple shle-unknown-elf -O1 -emit-llvm %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple sh-unknown-elf -O1 -S %s -o - | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple shle-unknown-elf -O1 -S %s -o - | FileCheck %s --check-prefix=ASM

int tas(unsigned char *address) {
	return __builtin_sh_tas_b(address);
}

int tas_volatile(volatile unsigned char *address) {
	return __builtin_sh_tas_b(address);
}

// IR-LABEL: define{{.*}} i32 @tas(
// IR: call i32 @llvm.sh.tas.b(ptr %address)
// IR: declare i32 @llvm.sh.tas.b(ptr captures(none))
// IR-LABEL: define{{.*}} i32 @tas_volatile(
// IR: call i32 @llvm.sh.tas.b(ptr %address)

// ASM-LABEL: tas:
// ASM: tas.b @r4
// ASM-NEXT: rts
// ASM-NEXT: movt r0
// ASM-LABEL: tas_volatile:
// ASM: tas.b @r4
// ASM-NEXT: rts
// ASM-NEXT: movt r0
