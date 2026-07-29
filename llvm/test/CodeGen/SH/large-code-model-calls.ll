; RUN: llc -mtriple=sh-unknown-elf -code-model=large -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -code-model=large -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -code-model=large -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -code-model=large -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj -r %t.be.o | FileCheck %s --check-prefix=OBJ
; RUN: llvm-readobj -r %t.le.o | FileCheck %s --check-prefix=OBJ
; RUN: llc -mtriple=sh-unknown-elf -code-model=large -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -code-model=large -O0 -verify-machineinstrs -stop-after=sh-literal-islands < %s | FileCheck %s --check-prefix=ISLAND

declare i64 @external_i64(i64, i64)

define i64 @large_external(i64 %a, i64 %b) {
; CHECK-LABEL: large_external:
; CHECK: sts.l	pr,@-r15
; CHECK: mov.l	[[EXT:.LCPI[0-9]+_0_0]],[[CALLEE:r[0-9]+]]
; CHECK: jsr	@[[CALLEE]]
; CHECK-NEXT: nop
; CHECK: lds.l	@r15+,pr
; CHECK: rts
; CHECK-NEXT: nop
; CHECK: [[EXT]]:
; CHECK-NEXT: .long	external_i64
; CHECK-NOT: bsr
	%result = call i64 @external_i64(i64 %a, i64 %b)
	ret i64 %result
}

define i32 @large_recursive(i32 %n) {
; CHECK-LABEL: large_recursive:
; CHECK: mov.l	[[SELF:.LCPI[0-9]+_0_0]],[[SELFREG:r[0-9]+]]
; CHECK: jsr	@[[SELFREG]]
; CHECK-NEXT: nop
; CHECK: [[SELF]]:
; CHECK-NEXT: .long	large_recursive
; CHECK-NOT: bsr
	%next = sub i32 %n, 1
	%call = call i32 @large_recursive(i32 %next)
	%result = add i32 %call, %n
	ret i32 %result
}

; ISEL: MOVL_load_pc_island %const.0, -1
; ISEL: JSR {{.*}}, csr_sh
; ISEL: MOVL_load_pc_island %const.0, -1
; ISEL: JSR {{.*}}, csr_sh

; ISLAND: MOVL_load_pc_island %const.0, 0
; ISLAND: SH_CONSTPOOL_ENTRY %const.0, 0, 4, 4

; OBJ: R_SH_DIR32 external_i64
; OBJ: R_SH_DIR32 large_recursive
; OBJ-NOT: R_SH_IND12W
