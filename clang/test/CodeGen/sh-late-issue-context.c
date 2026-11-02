// The original C7 witness selected an experimental late-issue analysis.
// Keep its handler/ordinary-function inputs under the conventional pipeline.
// RUN: %clang --no-default-config --target=shle-unknown-none-elf -m3 -O3 -ffreestanding -mllvm -verify-machineinstrs -S %s -o - | FileCheck %s
// RUN: %clang --no-default-config --target=sh-unknown-none-elf -m3 -O3 -ffreestanding -mllvm -verify-machineinstrs -S %s -o - | FileCheck %s
// RUN: %clang --no-default-config --target=shle-unknown-none-elf -m3 -O3 -ffreestanding -mllvm -force-new-pm-codegen=true -mllvm -verify-machineinstrs -S %s -o - | FileCheck %s
// CHECK-LABEL: hardware_handler:
// CHECK: rte
// CHECK-LABEL: ordinary:
// CHECK: rts
// CHECK-LABEL: scalar_positive:
// CHECK: rts

extern unsigned input_words[96];
extern unsigned output_word;

__attribute__((interrupt_handler)) void hardware_handler(void) {
	unsigned sum = 37;
	for (unsigned i = 0; i != 96; ++i)
		sum += input_words[i];
	output_word = sum;
}

void ordinary(void) {
	unsigned sum = 37;
	for (unsigned i = 0; i != 96; ++i)
		sum += input_words[i];
	output_word = sum;
}

unsigned scalar_positive(unsigned x, unsigned y) {
	return (x + y) ^ y;
}
