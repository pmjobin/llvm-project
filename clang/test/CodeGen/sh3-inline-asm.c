// RUN: %clang -target sh-unknown-none-elf -mcpu=sh3 -O2 -c %s -o %t.o
// RUN: llvm-objdump -d --mcpu=sh3 %t.o | FileCheck %s --check-prefix=SH3
// RUN: not %clang -target sh-unknown-none-elf -mcpu=sh2 -O2 -c %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SH2

void sh3_inline_asm(void) {
	__asm__ volatile(
		"shad r4,r5\n\t"
		"shld r5,r4\n\t"
		"ldc r4,ssr\n\t"
		"stc spc,r5\n\t"
		"ldc r4,r0_bank\n\t"
		"pref @r4\n\t"
		"ldtlb");
}

// SH3: shad	r4,r5
// SH3-NEXT: shld	r5,r4
// SH3-NEXT: ldc	r4,ssr
// SH3-NEXT: stc	spc,r5
// SH3-NEXT: ldc	r4,r0_bank
// SH3-NEXT: pref	@r4
// SH3-NEXT: ldtlb

// SH2-COUNT-7: error: instruction requires an unavailable feature
