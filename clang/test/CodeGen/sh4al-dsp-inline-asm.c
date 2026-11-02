// RUN: %clang --target=shle-unknown-none-elf -m4al -O1 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang --target=sh-unknown-none-elf -m4al -O2 -c -mllvm -verify-machineinstrs -o %t.be.o %s
// RUN: %clang --target=shle-unknown-none-elf -m4al -O2 -c -mllvm -verify-machineinstrs -o %t.le.o %s
// RUN: llvm-readobj --file-headers %t.be.o %t.le.o | FileCheck %s --check-prefix=ELF
// RUN: llvm-objdump -d --mcpu=sh4al-dsp %t.be.o | FileCheck %s --check-prefix=ASM
// RUN: llvm-objdump -d --mcpu=sh4al-dsp %t.le.o | FileCheck %s --check-prefix=ASM

// IR-LABEL: define{{.*}} void @clobber_dsp_data()
// IR: call void asm sideeffect "", "~{a0},~{a1},~{x0},~{x1},~{y0},~{y1},~{m0},~{m1}"()
// IR-LABEL: define{{.*}} void @clobber_guards()
// IR: call void asm sideeffect "pclr a0", "~{a0},~{a0g},~{a1g},~{dsr}"()
// IR-LABEL: define{{.*}} void @clobber_control()
// IR: call void asm sideeffect "setdmx", "~{sr},~{dsr},~{rs},~{re},~{mod}"()
// IR: attributes #{{[0-9]+}} = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4al-dsp{{[^"]*}}"{{.*}} }
// ELF-COUNT-2: Flags [ (0x6)
// ASM: pclr	a0
// ASM: setdmx

void clobber_dsp_data(void) {
	__asm__ volatile("" ::: "a0", "a1", "x0", "x1", "y0", "y1", "m0", "m1");
}

void clobber_guards(void) {
	__asm__ volatile("pclr a0" ::: "a0", "a0g", "a1g", "dsr");
}

void clobber_control(void) {
	__asm__ volatile("setdmx" ::: "sr", "dsr", "rs", "re", "mod");
}
