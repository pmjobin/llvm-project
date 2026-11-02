; RUN: llc -mtriple=sh-unknown-elf -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -O0 -verify-machineinstrs %s -o - | FileCheck %s

declare void @llvm.va_copy.p0(ptr, ptr)

define void @soft_copy(ptr %destination, ptr %source) #0 {
; CHECK-LABEL: soft_copy:
; CHECK: mov.l	@r5,r0
; CHECK: rts
; CHECK-NEXT: mov.l	r0,@r4
	call void @llvm.va_copy.p0(ptr %destination, ptr %source)
	ret void
}

define void @hard_copy(ptr %destination, ptr %source) #1 {
; CHECK-LABEL: hard_copy:
; CHECK-COUNT-9: mov.l
; CHECK: rts
; CHECK-NEXT: mov.l
	call void @llvm.va_copy.p0(ptr %destination, ptr %source)
	ret void
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh2" }
attributes #1 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" }
