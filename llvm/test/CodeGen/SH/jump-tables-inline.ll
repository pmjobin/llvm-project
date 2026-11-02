; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=SH2,NO-GOT
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=SH2,NO-GOT
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=SH2,NO-GOT
; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=SH2,NO-GOT
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh1 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=SH1
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-inline-jump-tables %s -o - | FileCheck %s --check-prefix=INLINE32
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-compress-jump-tables %s -o - | FileCheck %s --check-prefixes=COMPRESS16,WIDE32
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-inline-jump-tables %s -o %t.inline.mir
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -verify-machineinstrs -start-after=sh-inline-jump-tables -stop-after=sh-compress-jump-tables %t.inline.mir -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llvm-readobj --file-headers --relocations --hex-dump=.text %t.le.o | FileCheck %s --check-prefix=LEOBJ
; RUN: llvm-readobj --file-headers --relocations --hex-dump=.text %t.be.o | FileCheck %s --check-prefix=BEOBJ

define i32 @dense(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 0, label %c0
		i32 1, label %c1
		i32 2, label %c2
		i32 3, label %c3
		i32 4, label %c4
		i32 5, label %c5
	]
c0:
	ret i32 11
c1:
	ret i32 23
c2:
	ret i32 37
c3:
	ret i32 53
c4:
	ret i32 71
c5:
	ret i32 89
default:
	ret i32 -1
}

; The conservative pre-delay layout exceeds signed 16-bit range, so this
; table retains 32-bit entries while still using the PIC-safe inline form.
define i32 @wide(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 0, label %c0
		i32 1, label %c1
		i32 2, label %c2
		i32 3, label %c3
	]
c0:
	call void asm sideeffect ".space 32768", ""()
	ret i32 0
c1:
	ret i32 1
c2:
	ret i32 2
c3:
	ret i32 3
default:
	ret i32 -1
}

; SH2-LABEL: dense:
; SH2: mova	[[TABLE:.Lsh_jumptable[0-9]+]],r0
; SH2-NEXT: shll	[[INDEX:r[0-9]+]]
; SH2-NEXT: mov.w	@(r0,[[INDEX]]),[[OFFSET:r[0-9]+]]
; SH2-NEXT: [[BASE:.Lsh_jumptable_base[0-9]+]]:
; SH2-NEXT: braf	[[OFFSET]]
; SH2-NEXT: nop
; SH2: [[TABLE]]:
; SH2-COUNT-6: .short	.LBB{{[0-9_]+}}-[[BASE]]-4
; SH2-NOT: .LJTI

; NO-GOT-NOT: _GLOBAL_OFFSET_TABLE_
; NO-GOT-NOT: @GOTOFF

; SH1-LABEL: dense:
; SH1: mov.l	[[POINTER:.LCPI[0-9_]+]],[[BASE:r[0-9]+]]
; SH1: shll2
; SH1: add	[[BASE]],
; SH1: mov.l	@
; SH1: jmp	@
; SH1: [[POINTER]]:
; SH1-NEXT: .long	[[TABLE:.LJTI[0-9_]+]]
; SH1: [[TABLE]]:
; SH1-COUNT-6: .long	.LBB

; INLINE32-LABEL: name:            dense
; INLINE32: kind:            inline
; INLINE32: $r0 = MOVA <mcsymbol
; INLINE32-NEXT: ${{r[0-9]+}} = SHLL2
; INLINE32-NEXT: ${{r[0-9]+}} = MOVL_load_indexed
; INLINE32-NEXT: BRAF {{.*}}, %jump-table.0, <mcsymbol
; INLINE32: SH_JUMPTABLE %jump-table.0, <mcsymbol {{.*}}>, <mcsymbol {{.*}}>, 4

; COMPRESS16-LABEL: name:            dense
; COMPRESS16: $r0 = MOVA <mcsymbol
; COMPRESS16-NEXT: ${{r[0-9]+}} = SHLL
; COMPRESS16-NEXT: ${{r[0-9]+}} = MOVW_load_indexed
; COMPRESS16-NEXT: BRAF {{.*}}, %jump-table.0, <mcsymbol
; COMPRESS16: SH_JUMPTABLE %jump-table.0, <mcsymbol {{.*}}>, <mcsymbol {{.*}}>, 2

; WIDE32-LABEL: name:            wide
; WIDE32: $r0 = MOVA <mcsymbol
; WIDE32-NEXT: ${{r[0-9]+}} = SHLL2
; WIDE32-NEXT: ${{r[0-9]+}} = MOVL_load_indexed
; WIDE32-NEXT: BRAF {{.*}}, %jump-table.0, <mcsymbol
; WIDE32: SH_JUMPTABLE %jump-table.0, <mcsymbol {{.*}}>, <mcsymbol {{.*}}>, 4

; LEOBJ: DataEncoding: LittleEndian
; LEOBJ-NOT: .rela.text
; LEOBJ: 0x00000000 05e00634 168902c7 00444d00 23000900

; BEOBJ: DataEncoding: BigEndian
; BEOBJ-NOT: .rela.text
; BEOBJ: 0x00000000 e0053406 8916c702 4400004d 00230009
