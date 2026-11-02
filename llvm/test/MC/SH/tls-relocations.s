! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s --check-prefix=PRINT
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj -r --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=RELOC,BE
! RUN: llvm-readobj -r --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=RELOC,LE
! RUN: llvm-readelf -r %t.be.o | FileCheck %s --check-prefix=READELF
! RUN: llvm-readelf -r %t.le.o | FileCheck %s --check-prefix=READELF

.type tls_gd,@tls_object
.type tls_ld,@tls_object
.type tls_ldo,@tls_object
.type tls_ie,@tls_object
.type tls_le,@tls_object

.text
.long tls_gd@TLSGD
.long tls_ld@TLSLDM
.long tls_ldo@DTPOFF+20
.long tls_ldo@DTPOFF-20
.long tls_ie@GOTTPOFF
.long tls_le@TPOFF+36
.long tls_le@TPOFF-36

! PRINT: .long	tls_gd@TLSGD
! PRINT: .long	tls_ld@TLSLDM
! PRINT: .long	tls_ldo@DTPOFF+20
! PRINT: .long	tls_ldo@DTPOFF-20
! PRINT: .long	tls_ie@GOTTPOFF
! PRINT: .long	tls_le@TPOFF+36
! PRINT: .long	tls_le@TPOFF-36

! RELOC: Section {{.*}} .rela.text {
! RELOC-NEXT: 0x0 R_SH_TLS_GD_32 tls_gd 0x0
! RELOC-NEXT: 0x4 R_SH_TLS_LD_32 tls_ld 0x0
! RELOC-NEXT: 0x8 R_SH_TLS_LDO_32 tls_ldo 0x0
! RELOC-NEXT: 0xC R_SH_TLS_LDO_32 tls_ldo 0x0
! RELOC-NEXT: 0x10 R_SH_TLS_IE_32 tls_ie 0x0
! RELOC-NEXT: 0x14 R_SH_TLS_LE_32 tls_le 0x0
! RELOC-NEXT: 0x18 R_SH_TLS_LE_32 tls_le 0x0
! RELOC-NEXT: }

! BE: Hex dump of section '.text':
! BE-NEXT: 0x00000000 00000000 00000000 00000014 ffffffec
! BE-NEXT: 0x00000010 00000000 00000024 ffffffdc
! LE: Hex dump of section '.text':
! LE-NEXT: 0x00000000 00000000 00000000 14000000 ecffffff
! LE-NEXT: 0x00000010 00000000 24000000 dcffffff

! READELF: R_SH_TLS_GD_32
! READELF: R_SH_TLS_LD_32
! READELF: R_SH_TLS_LDO_32
! READELF: R_SH_TLS_IE_32
! READELF: R_SH_TLS_LE_32
