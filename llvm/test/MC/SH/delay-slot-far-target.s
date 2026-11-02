# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2 -filetype=obj %s -o %t.be.o
# RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=BE
# RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh2 -filetype=obj %s -o %t.le.o
# RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=LE

# The conditional branch at 2 reaches near at 8: 8 - (2 + 4) = 2 bytes.
# The first BRA at 4 reaches far at 272: 272 - (4 + 4) = 264 bytes.
# A conditional branch from 2 to far would need 266 bytes, or 133 scaled
# units, outside the signed eight-bit conditional branch range.
	.text
	.globl far_false_target
far_false_target:
	cmp/eq r5,r4
	bt near
	bra far
	nop
near:
	.rept 130
	nop
	.endr
	bra far
	nop
far:
	rts
	nop

# BE-LABEL: <far_false_target>:
# BE: 2: 89 01 {{.*}}bt{{.*}}0x8 <near>
# BE-NEXT: 4: a0 84 {{.*}}bra{{.*}}0x110 <far>
# BE-NEXT: 6: 00 09 {{.*}}nop
# BE: 10c: a0 00 {{.*}}bra{{.*}}0x110 <far>
# BE-NEXT: 10e: 00 09 {{.*}}nop
# BE: 110: 00 0b {{.*}}rts
# BE-NEXT: 112: 00 09 {{.*}}nop

# LE-LABEL: <far_false_target>:
# LE: 2: 01 89 {{.*}}bt{{.*}}0x8 <near>
# LE-NEXT: 4: 84 a0 {{.*}}bra{{.*}}0x110 <far>
# LE-NEXT: 6: 09 00 {{.*}}nop
# LE: 10c: 00 a0 {{.*}}bra{{.*}}0x110 <far>
# LE-NEXT: 10e: 09 00 {{.*}}nop
# LE: 110: 0b 00 {{.*}}rts
# LE-NEXT: 112: 09 00 {{.*}}nop
