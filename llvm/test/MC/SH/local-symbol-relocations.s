! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj --symbols --relocations %t.be.o | FileCheck %s --check-prefix=OBJ
! RUN: llvm-readobj --symbols --relocations %t.le.o | FileCheck %s --check-prefix=OBJ
! RUN: llvm-symbolizer --obj=%t.be.o 2 | FileCheck %s --check-prefix=SYMBOLIZE
! RUN: llvm-symbolizer --obj=%t.le.o 2 | FileCheck %s --check-prefix=SYMBOLIZE

	.text
	.globl symbolization_target
	.type symbolization_target,@function
symbolization_target:
	nop
.Linside:
	rts
	nop
	.size symbolization_target,.-symbolization_target

	.data
	.long .Linside

! OBJ: Section {{.*}} .rela.data {
! OBJ-NEXT: 0x0 R_SH_DIR32 .Linside 0x0
! OBJ-NEXT: }
! OBJ: Name: .Linside
! OBJ: Section: .text
! OBJ: Name: symbolization_target
! OBJ: Section: .text

! SYMBOLIZE: symbolization_target
! SYMBOLIZE-NEXT: ??:0:0
