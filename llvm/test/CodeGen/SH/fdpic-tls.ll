; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -filetype=obj -verify-machineinstrs < %s -o %t.le.o
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -filetype=obj -verify-machineinstrs < %s -o %t.be.o
; RUN: llvm-readobj --file-headers --relocations %t.le.o %t.be.o | FileCheck %s --check-prefix=OBJECT

@tls_le = internal thread_local(localexec) global i32 1, align 4
@tls_ie = external thread_local(initialexec) global i32, align 4
@tls_gd = external thread_local global i32, align 4
@tls_ld = hidden thread_local(localdynamic) global i32 2, align 4

declare void @callee()

define ptr @le_address() {
	ret ptr @tls_le
}

define ptr @ie_address() {
	ret ptr @tls_ie
}

define ptr @gd_address() {
	ret ptr @tls_gd
}

define ptr @ld_address() {
	ret ptr @tls_ld
}

define i32 @tls_across_call() {
entry:
	%before = load i32, ptr @tls_ie, align 4
	call void @callee()
	%after = load i32, ptr @tls_ie, align 4
	%result = add i32 %before, %after
	ret i32 %result
}

; ASM-LABEL: le_address:
; ASM: stc	gbr,
; ASM: .long	tls_le@TPOFF

; ASM-LABEL: ie_address:
; ASM-NOT: _GLOBAL_OFFSET_TABLE_
; ASM-DAG: mov	r12,[[IEGOT:r[0-9]+]]
; ASM-DAG: mov.l	[[IELITERAL:.LCPI[0-9_]+]],[[IEOFFSET:r[0-9]+]]
; ASM: mov.l	@([[IEGOT]],[[IEOFFSET]]),[[TPOFF:r[0-9]+]]
; ASM: stc	gbr,r0
; ASM: rts
; ASM-NEXT: add	[[TPOFF]],r0
; ASM: [[IELITERAL]]:
; ASM-NEXT: .long	tls_ie@GOTTPOFF

; ASM-LABEL: gd_address:
; ASM: mov.l	[[GDDESC:.LCPI[0-9_]+]],r4
; ASM: jsr	@r1
; ASM-NEXT: add	r12,r4
; ASM: [[GDDESC]]:
; ASM-NEXT: .long	tls_gd@TLSGD
; ASM: .long	__tls_get_addr@PLT

; ASM-LABEL: ld_address:
; ASM: .long	tls_ld@TLSLDM
; ASM: .long	__tls_get_addr@PLT
; ASM: .long	tls_ld@DTPOFF

; ASM-LABEL: tls_across_call:
; ASM-DAG: mov	r12,[[BASE:r[0-9]+]]
; ASM-DAG: mov.l	[[OFFSET_LITERAL:.LCPI[0-9_]+]],[[OFFSET:r[0-9]+]]
; ASM-DAG: stc	gbr,[[THREADBASE:r[0-9]+]]
; ASM: mov.l	@([[OFFSET]],[[BASE]]),[[TLSPTR:r[89]|r1[013]]]
; ASM: mov	[[THREADBASE]],[[TLSADDR:r[0-9]+]]
; ASM: mov.l	@([[TLSADDR]],[[TLSPTR]]),[[BEFORE:r[89]|r1[013]]]
; ASM: bsrf	{{r[0-9]+}}
; ASM-NEXT: nop
; ASM: .Lsh_fdpic_call{{[0-9]+}}:
; ASM: mov	[[THREADBASE]],[[AFTERADDR:r[0-9]+]]
; ASM-NEXT: mov.l	@([[AFTERADDR]],[[TLSPTR]]),[[AFTER:r[0-9]+]]
; ASM: add	[[BEFORE]],[[AFTER]]
; ASM: .long	callee@PLT

; OBJECT-COUNT-2: Flags [ (0x8002)
; OBJECT-DAG: R_SH_TLS_LE_32 tls_le
; OBJECT-DAG: R_SH_TLS_IE_32 tls_ie
; OBJECT-DAG: R_SH_TLS_GD_32 tls_gd
; OBJECT-DAG: R_SH_TLS_LD_32 tls_ld
; OBJECT-DAG: R_SH_TLS_LDO_32 tls_ld
; OBJECT-DAG: R_SH_PLT32 __tls_get_addr
; OBJECT-NOT: R_SH_GOTPC
