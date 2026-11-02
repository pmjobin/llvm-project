; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR

declare void @llvm.memcpy.p0.p0.i32(ptr, ptr, i32, i1 immarg)
declare void @llvm.memmove.p0.p0.i32(ptr, ptr, i32, i1 immarg)
declare void @llvm.memset.p0.i32(ptr, i8, i32, i1 immarg)

; CHECK-LABEL: large_memcpy:
; CHECK: jsr	@
; CHECK-NEXT: mov	#61,r6
; CHECK: .long	memcpy
; CHECK-NOT: .long	memcpy
define void @large_memcpy(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i32(ptr align 4 %dst, ptr align 4 %src, i32 61, i1 false)
	ret void
}

; MIR-LABEL: name:            large_memcpy
; MIR: value:           memcpy
; MIR: JSR {{.*}}, csr_sh
; MIR-NOT: JSR

; CHECK-LABEL: dynamic_memmove:
; CHECK: jsr	@
; CHECK-NEXT: nop
; CHECK: .long	memmove
; CHECK-NOT: .long	memmove
define void @dynamic_memmove(ptr %dst, ptr %src, i32 %size) {
	call void @llvm.memmove.p0.p0.i32(ptr align 4 %dst, ptr align 4 %src, i32 %size, i1 false)
	ret void
}

; MIR-LABEL: name:            dynamic_memmove
; MIR: value:           memmove
; MIR: JSR {{.*}}, csr_sh
; MIR-NOT: JSR

; CHECK-LABEL: large_memmove:
; CHECK: mov.b	@r0,r0
; CHECK-COUNT-4: mov.l	@
; CHECK-COUNT-4: mov.l	r
; CHECK: mov.b	r0,@r4
; CHECK-NOT: jsr
; CHECK: .size	large_memmove
define void @large_memmove(ptr %dst, ptr %src) {
	call void @llvm.memmove.p0.p0.i32(ptr align 4 %dst, ptr align 4 %src, i32 17, i1 false)
	ret void
}

; MIR-LABEL: name:            large_memmove
; MIR-NOT: JSR
; MIR: RTS

; CHECK-LABEL: large_memset:
; The standard memset libcall passes an int whose low byte is significant.
; CHECK: jsr	@
; CHECK-NEXT: mov	#61,r6
; CHECK: .long	memset
; CHECK-NOT: .long	memset
define void @large_memset(ptr %dst, i32 %input) {
	%value = trunc i32 %input to i8
	call void @llvm.memset.p0.i32(ptr align 4 %dst, i8 %value, i32 61, i1 false)
	ret void
}

; MIR-LABEL: name:            large_memset
; MIR: value:           memset
; MIR: JSR {{.*}}, csr_sh
; MIR-NOT: JSR
