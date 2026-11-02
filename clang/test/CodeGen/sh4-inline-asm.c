// RUN: %clang -target sh-unknown-none-elf -mcpu=sh4 -O2 -c %s -o %t.be.o
// RUN: %clang -target shle-unknown-none-elf -mcpu=sh4 -O2 -c %s -o %t.le.o
// RUN: llvm-objdump -d --mcpu=sh4 %t.be.o | FileCheck %s
// RUN: llvm-objdump -d --mcpu=sh4 %t.le.o | FileCheck %s
// RUN: not %clang -target sh-unknown-none-elf -mcpu=sh3 -O2 -c %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SH3

void sh4_inline_asm(void) {
	__asm__ volatile(
		"stc dbr,r4\n\t"
		"movca.l r0,@r4\n\t"
		"fmov.s @r4,fr0\n\t"
		"fadd fr0,fr2\n\t"
		"fmov dr2,xd4\n\t"
		"fipr fv0,fv4\n\t"
		"ftrv xmtrx,fv8\n\t"
		"frchg\n\t"
		"fschg");
}

// CHECK: stc	dbr,r4
// CHECK-NEXT: movca.l	r0,@r4
// CHECK-NEXT: fmov	@r4,fr0
// CHECK-NEXT: fadd	fr0,fr2
// CHECK-NEXT: fmov	fr2,fr5
// CHECK-NEXT: fipr	fv0,fv4
// CHECK-NEXT: ftrv	xmtrx,fv8
// CHECK-NEXT: frchg
// CHECK-NEXT: fschg

// SH3-COUNT-9: error: instruction requires an unavailable feature
