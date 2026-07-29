# RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
# RUN: llvm-dwarfdump --debug-frame --eh-frame %t.be.o 2>&1 | FileCheck %s --check-prefix=FRAME
# RUN: llvm-readobj --relocations %t.be.o 2>&1 | FileCheck %s --check-prefix=RELOC
# RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
# RUN: llvm-dwarfdump --debug-frame --eh-frame %t.le.o 2>&1 | FileCheck %s --check-prefix=FRAME
# RUN: llvm-readobj --relocations %t.le.o 2>&1 | FileCheck %s --check-prefix=RELOC

	.text
	.cfi_sections .debug_frame, .eh_frame
	.globl	f
	.type	f,@function
f:
	.cfi_startproc
	sts.l	pr,@-r15
	.cfi_def_cfa_offset 4
	.cfi_offset	pr,-4
	add	#-8,r15
	.cfi_adjust_cfa_offset 8
	add	#8,r15
	.cfi_adjust_cfa_offset -8
	lds.l	@r15+,pr
	.cfi_restore	pr
	.cfi_def_cfa_offset 0
	rts
	nop
	.cfi_endproc
	.size	f,.-f

# FRAME: .debug_frame contents:
# FRAME-NOT: warning:
# FRAME: Version: 4
# FRAME: Address size: 4
# FRAME: Code alignment factor: 2
# FRAME: Data alignment factor: -4
# FRAME: Return address column: 17
# FRAME: DW_CFA_def_cfa: R15 +0
# FRAME: FDE cie=
# FRAME: 0x0: CFA=R15
# FRAME: 0x2: CFA=R15+4: PR=[CFA-4]
# FRAME: 0x4: CFA=R15+12: PR=[CFA-4]
# FRAME: 0x6: CFA=R15+4: PR=[CFA-4]
# FRAME: 0x8: CFA=R15
# FRAME: .eh_frame contents:
# FRAME: Version: 1
# FRAME: Augmentation: "zR"
# FRAME: Code alignment factor: 2
# FRAME: Data alignment factor: -4
# FRAME: Return address column: 17
# FRAME: Augmentation data: 00
# FRAME: DW_CFA_def_cfa: R15 +0
# FRAME: FDE cie=
# FRAME: 0x0: CFA=R15
# FRAME: 0x2: CFA=R15+4: PR=[CFA-4]
# FRAME: 0x4: CFA=R15+12: PR=[CFA-4]
# FRAME: 0x6: CFA=R15+4: PR=[CFA-4]
# FRAME: 0x8: CFA=R15

# RELOC-COUNT-3: R_SH_DIR32
# RELOC-NOT: R_SH_REL32
# RELOC-NOT: warning:
