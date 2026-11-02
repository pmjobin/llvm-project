// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O1 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -S -pic-level 2 -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O1 -S -pic-level 2 -mllvm -verify-machineinstrs -o - %s | FileCheck %s

int r0_constraint(int value) {
	int result;
	__asm__("mov %1,%0" : "=z"(result) : "r"(value));
	return result;
}

// CHECK-LABEL: r0_constraint:
// CHECK: mov r4,r0

int non_sp_constraint(int value) {
	int result;
	__asm__("mov %1,%0" : "=u"(result) : "u"(value));
	return result;
}

// CHECK-LABEL: non_sp_constraint:
// CHECK: mov r4,r0

int pr_constraint(int value) {
	int result;
	__asm__("lds %1,pr" : "=l"(result) : "r"(value));
	return result;
}

// CHECK-LABEL: pr_constraint:
// CHECK: lds r4,pr
// CHECK: sts pr,r0

int mac_constraint(int value) {
	int result;
	__asm__("lds %1,macl" : "=x"(result) : "r"(value));
	return result;
}

// CHECK-LABEL: mac_constraint:
// CHECK: lds r4,macl
// CHECK: sts macl,r0

int pr_input_constraint(int value) {
	int result;
	__asm__ volatile("sts pr,%0" : "=r"(result) : "l"(value));
	return result;
}

// CHECK-LABEL: pr_input_constraint:
// CHECK: sts.l pr,@-r15
// CHECK: lds r4,pr
// CHECK: sts pr,r0
// CHECK: lds.l @r15+,pr

int mac_input_constraint(int value) {
	int result;
	__asm__ volatile("sts macl,%0" : "=r"(result) : "x"(value));
	return result;
}

// CHECK-LABEL: mac_input_constraint:
// CHECK: lds r4,macl
// CHECK: sts macl,r0

int t_constraint(void) {
	int result;
	__asm__("sett" : "=t"(result));
	return result;
}

// CHECK-LABEL: t_constraint:
// CHECK: sett
// CHECK-NEXT: !NO_APP
// CHECK-NEXT: rts
// CHECK-NEXT: movt r0

int memory_constraint(int *address) {
	int result;
	__asm__("mov.l %1,%0" : "=r"(result) : "m"(*address));
	return result;
}

// CHECK-LABEL: memory_constraint:
// CHECK: mov.l @r4,r0

int global;

int global_memory_constraint(void) {
	int result;
	__asm__("mov.l %1,%0" : "=r"(result) : "m"(global));
	return result;
}

// CHECK-LABEL: global_memory_constraint:
// CHECK: mov.l @r{{[0-9]+}},r0

int stack_memory_constraint(int value) {
	int result;
	__asm__("mov.l %1,%0" : "=r"(result) : "m"(value));
	return result;
}

// CHECK-LABEL: stack_memory_constraint:
// CHECK: mov.l @r{{[0-9]+}},r0

int indexed_memory_constraint(int *address, int index) {
	int result;
	__asm__("mov.l %1,%0" : "=r"(result) : "m"(address[index]));
	return result;
}

// CHECK-LABEL: indexed_memory_constraint:
// CHECK: mov.l @r{{[0-9]+}},r0

int offsettable_constraint(int *address) {
	int result;
	__asm__("mov.l %1,%0" : "=r"(result) : "o"(*address));
	return result;
}

// CHECK-LABEL: offsettable_constraint:
// CHECK: mov.l @r4,r0

int nonoffsettable_constraint(int *address) {
	int result;
	__asm__("mov.l %1,%0" : "=r"(result) : "V"(*address));
	return result;
}

// CHECK-LABEL: nonoffsettable_constraint:
// CHECK: mov.l @r4,r0

int address_constraint(int *address) {
	int result;
	__asm__("mov %1,%0" : "=r"(result) : "p"(address));
	return result;
}

// CHECK-LABEL: address_constraint:
// CHECK: mov r4,r0

int immediate_constraints(void) {
	int result;
	__asm__("mov #%O1,%0" : "=r"(result) : "I08"(127));
	__asm__("and #%O1,r0" : "+z"(result) : "K08"(255));
	__asm__("shll8 %0 ! %O1" : "+r"(result) : "P27"(8));
	__asm__ volatile("! %O0 %O1 %O2 %O3" : : "P27"(1), "P27"(2), "P27"(8), "P27"(16));
	__asm__ volatile("! %O0" : : "K04"(15));
	__asm__ volatile("! %O0" : : "K05"(31));
	__asm__ volatile("! %O0" : : "M"(1));
	__asm__ volatile("! %O0" : : "N"(0));
	__asm__ volatile("! %O0" : : "Jmb"(0xff));
	__asm__ volatile("! %O0" : : "Jmw"(0xffff));
	__asm__ volatile("! %O0" : : "Jhb"(0x80000000U));
	return result;
}

// CHECK-LABEL: immediate_constraints:
// CHECK-DAG: mov #127,r0
// CHECK-DAG: and #255,r0
// CHECK-DAG: shll8 r0 ! 8
// CHECK-DAG: ! 1 2 8 16
// CHECK-DAG: ! 15
// CHECK-DAG: ! 31
// CHECK-DAG: ! 1
// CHECK-DAG: ! 0
// CHECK-DAG: ! 255
// CHECK-DAG: ! 65535
// CHECK-DAG: ! -2147483648

unsigned char tied_narrow(unsigned char value) {
	__asm__("add #1,%0" : "+r"(value));
	return value;
}

// CHECK-LABEL: tied_narrow:
// CHECK: add #1,r{{[0-9]+}}

int early_clobber_alternative(int lhs, int rhs) {
	int result;
	__asm__("mov %1,%0\n\tadd %2,%0" : "=&r,&r"(result) : "r,0"(lhs), "r,r"(rhs));
	return result;
}

// CHECK-LABEL: early_clobber_alternative:
// CHECK: mov r4,r0
// CHECK-NEXT: add r5,r0

void special_clobbers(void) {
	__asm__ volatile("" ::: "pr", "mach", "macl", "gbr", "sr", "t");
}

// CHECK-LABEL: special_clobbers:
// CHECK: sts.l pr,@-r15
// CHECK: lds.l @r15+,pr

int asm_goto(int value) {
	__asm__ goto("tst %0,%0\n\tbt %l1" : : "r"(value) : "t" : zero);
	return 1;
zero:
	return 0;
}

// CHECK-LABEL: asm_goto:
// CHECK: tst r4,r4
// CHECK-NEXT: bt .LBB{{[0-9]+}}_{{[0-9]+}}
