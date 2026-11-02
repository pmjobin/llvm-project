// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -fsyntax-only -verify %s

void invalid_register(void) {
	__asm__ volatile("" ::: "r16"); // expected-error {{unknown register name 'r16' in asm}}
}

void invalid_constraint(int value) {
	__asm__ volatile("" : : "f"(value)); // expected-error {{invalid input constraint 'f' in asm}}
}

void valid_register_clobbers(void) {
	__asm__ volatile("" ::: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
					 "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
					 "sp", "fp", "pr", "mach", "macl", "gbr", "sr", "t");
}

void immediate_boundaries(int value) {
	__asm__ volatile("" : : "I08"(-128));
	__asm__ volatile("" : : "I08"(127));
	__asm__ volatile("" : : "K04"(0));
	__asm__ volatile("" : : "K04"(15));
	__asm__ volatile("" : : "K05"(31));
	__asm__ volatile("" : : "K08"(255));
	__asm__ volatile("" : : "M"(1));
	__asm__ volatile("" : : "N"(0));
	__asm__ volatile("" : : "I08"(-129)); // expected-error {{value '-129' out of range for constraint 'I08'}}
	__asm__ volatile("" : : "I08"(128)); // expected-error {{value '128' out of range for constraint 'I08'}}
	__asm__ volatile("" : : "K04"(16)); // expected-error {{value '16' out of range for constraint 'K04'}}
	__asm__ volatile("" : : "K05"(32)); // expected-error {{value '32' out of range for constraint 'K05'}}
	__asm__ volatile("" : : "K08"(256)); // expected-error {{value '256' out of range for constraint 'K08'}}
	__asm__ volatile("" : : "M"(2)); // expected-error {{value '2' out of range for constraint 'M'}}
	__asm__ volatile("" : : "N"(1)); // expected-error {{value '1' out of range for constraint 'N'}}
	__asm__ volatile("" : : "Pso"(1)); // expected-error {{invalid input constraint 'Pso' in asm}}
	__asm__ volatile("" : : "I"(1)); // expected-error {{invalid input constraint 'I' in asm}}
	__asm__ volatile("" : : "t"(value)); // expected-error {{invalid input size for constraint 't'}}
	__asm__ volatile("" : "+t"(value)); // expected-error {{invalid output size for constraint '+t'}}
}
