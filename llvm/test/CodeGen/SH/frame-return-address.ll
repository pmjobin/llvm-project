; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-dwarfdump --verify %t.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --verify %t.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --eh-frame %t.be.o | FileCheck %s --check-prefix=FRAME
; RUN: llvm-dwarfdump --eh-frame %t.le.o | FileCheck %s --check-prefix=FRAME

declare ptr @llvm.frameaddress.p0(i32 immarg)
declare ptr @llvm.returnaddress(i32 immarg)
declare void @consume(ptr)

; ASM-LABEL: frame_address_zero:
; ASM: mov.l	r14,@r15
; ASM: mov	r15,r14
; ASM-NEXT: .cfi_def_cfa r14, 4
; ASM: mov	r14,r0
; ASM: mov	r14,r15
; ASM: mov.l	@r15,r14
define ptr @frame_address_zero() uwtable {
	%address = call ptr @llvm.frameaddress.p0(i32 0)
	ret ptr %address
}

; ASM-LABEL: return_address_leaf:
; ASM: sts.l	pr,@-r15
; ASM-NEXT: .cfi_def_cfa_offset 4
; ASM-NEXT: .cfi_offset pr, -4
; ASM: mov.l	@r15,r0
; ASM: lds.l	@r15+,pr
define ptr @return_address_leaf() uwtable {
	%address = call ptr @llvm.returnaddress(i32 0)
	ret ptr %address
}

; ASM-LABEL: return_address_call:
; ASM: sts.l	pr,@-r15
; ASM: jsr
; ASM: mov.l	@r15,r0
; ASM: lds.l	@r15+,pr
define ptr @return_address_call() uwtable {
	call void @consume(ptr null)
	%address = call ptr @llvm.returnaddress(i32 0)
	ret ptr %address
}

; ASM-LABEL: return_address_realign:
; ASM: sts.l	pr,@-r15
; ASM: mov	r15,r14
; ASM: and	{{r[0-9]+}},r15
; ASM-DAG: mov	r15,r13
; ASM-DAG: mov.l	r4,@(32,r13)
; ASM-DAG: mov.l	@(8,r14),r0
; ASM: mov	r14,r15
; ASM: lds.l	@r15+,pr
define ptr @return_address_realign(i32 %value) uwtable {
	%object = alloca i32, align 32
	store volatile i32 %value, ptr %object, align 32
	%address = call ptr @llvm.returnaddress(i32 0)
	ret ptr %address
}

; VERIFY-NOT: error:
; VERIFY: No errors.

; FRAME: DW_CFA_def_cfa: R14
; FRAME: DW_CFA_offset: PR
; FRAME: DW_CFA_restore: PR
