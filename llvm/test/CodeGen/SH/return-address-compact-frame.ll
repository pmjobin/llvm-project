; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O2 -enable-new-pm -verify-machineinstrs %s -o - | FileCheck %s

declare ptr @llvm.returnaddress(i32 immarg)
declare void @callee()

; The compact no-CFI layout must use the same frame offsets as ordinary
; spills. PR is at entry-SP-4, before the r8 push, and is restored after r8.
; CHECK-LABEL: compact_return_address:
; CHECK: sts.l pr,@-r15
; CHECK-NEXT: mov.l r8,@-r15
; CHECK: mov.l @(4,r15),
; CHECK: mov.l @r15+,r8
; CHECK: lds.l @r15+,pr
define ptr @compact_return_address() nounwind {
  call void asm sideeffect "", "~{r8}"()
  %ra = call ptr @llvm.returnaddress(i32 0)
  ret ptr %ra
}

; CHECK-LABEL: compact_nonleaf:
; CHECK: sts.l pr,@-r15
; CHECK-NEXT: mov.l r8,@-r15
; CHECK-NEXT: mov.l r9,@-r15
; CHECK: jsr
; CHECK: mov.l @(8,r15),
; CHECK: mov.l @r15+,r9
; CHECK: mov.l @r15+,r8
; CHECK: lds.l @r15+,pr
define ptr @compact_nonleaf() nounwind optsize {
  call void asm sideeffect "", "~{r8},~{r9}"()
  call void @callee()
  %ra = call ptr @llvm.returnaddress(i32 0)
  ret ptr %ra
}

; CHECK-LABEL: cfi_return_address:
; CHECK: sts.l pr,@-r15
; CHECK: .cfi_offset pr, -4
; CHECK: mov.l @(4,r15),
; CHECK: lds.l @r15+,pr
define ptr @cfi_return_address() nounwind uwtable {
  call void asm sideeffect "", "~{r8}"()
  %ra = call ptr @llvm.returnaddress(i32 0)
  ret ptr %ra
}
