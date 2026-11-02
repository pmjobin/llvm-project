; RUN: llc -mtriple=shle -mcpu=sh4 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh -mcpu=sh4 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh4 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh4 -enable-new-pm=1 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh4 -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle -mcpu=sh4 -stop-after=sh-prepare-ir %s -o - | FileCheck %s --check-prefix=IR
; RUN: llc -mtriple=sh -mcpu=sh4 -enable-new-pm=1 -stop-after=sh-prepare-ir-module %s -o - | FileCheck %s --check-prefix=IR

declare void @llvm.sh.ftrv4(ptr, ptr, ptr)
declare void @llvm.sh.ftrv.array(ptr, ptr, i32, ptr)
declare void @llvm.sh.ftrv.compose(ptr, ptr, i32, ptr, ptr)
declare void @observe(ptr)

; No pointer access, matrix setup or FP status operation for constant zero.
; ASM-LABEL: zero:
; ASM-NOT: fmov
; ASM-NOT: fpscr
; ASM-NOT: ftrv
; ASM: rts
; ASM: .size zero
define void @zero() {
  call void @llvm.sh.ftrv.array(ptr null, ptr null, i32 0, ptr null)
  call void @llvm.sh.ftrv.compose(ptr null, ptr null, i32 0, ptr null, ptr null)
  ret void
}

; The loop's advancing addresses must not inherit align64 at every iteration.
; MIR-LABEL: name: array
; MIR: size: 16, alignment: 8
; MIR: TST
; MIR: GRAPHICS_BEGIN
; MIR: FMOV_V_LOAD_PAIR {{.*}} :: (load (s128)){{$}}
; MIR: FTRV
; MIR: GRAPHICS_END
; ASM-LABEL: array:
; ASM: tst
; ASM: bt
; ASM: fmov.d {{.*}},xd0
; ASM: ftrv xmtrx,
; ASM-NOT: fmov.d {{.*}},xd0
; ASM: .size array
define void @array(ptr %d, ptr %s, i32 %n, ptr %m) {
  call void @llvm.sh.ftrv.array(ptr align 64 %d, ptr align 64 %s, i32 %n, ptr align 8 %m)
  ret void
}

; Align4 paths retain the safe aligned matrix scratch and scalar loop mode.
; MIR-LABEL: name: unaligned
; MIR: size: 80, alignment: 8
; MIR: GRAPHICS_BEGIN
; MIR: GRAPHICS_SCALAR
; MIR: FMOV_V_LOAD
; MIR: GRAPHICS_END
define void @unaligned(ptr %d, ptr %s, i32 %n, ptr %m) {
  call void @llvm.sh.ftrv.array(ptr align 4 %d, ptr align 4 %s, i32 %n, ptr align 4 %m)
  ret void
}

; All four results remain live until the product is installed in XMTRX.
; MIR-LABEL: name: compose
; MIR: GRAPHICS_BEGIN
; MIR-COUNT-4: = FTRV
; MIR: XMTRX_FROM_FVS
; MIR: GRAPHICS_END
; ASM-LABEL: compose:
; ASM-COUNT-4: ftrv xmtrx,
; ASM-COUNT-8: fmov dr{{[0-9]+}},xd
; ASM: ftrv xmtrx,
; ASM-NOT: frchg
; ASM: .size compose
define void @compose(ptr %d, ptr %s, i32 %n, ptr %a, ptr %b) {
  call void @llvm.sh.ftrv.compose(ptr align 8 %d, ptr align 8 %s, i32 %n, ptr align 8 %a, ptr align 4 %b)
  ret void
}

; IR-LABEL: define void @private_product(
; IR-NOT: alloca
; IR-NOT: call void @llvm.sh.ftrv4
; IR: call void @llvm.sh.ftrv.compose(ptr align 16 %d, ptr align 8 %s, i32 4, ptr align 8 %a, ptr align 4 %b)
; IR: ret void
; ASM-LABEL: private_product:
; ASM-NOT: tst
; ASM-COUNT-8: ftrv xmtrx,
; ASM-NOT: ftrv xmtrx,
; ASM: .size private_product
define void @private_product(ptr %d, ptr %s, ptr %a, ptr %b) {
  %tmp = alloca [4 x <4 x float>], align 16
  call void @llvm.sh.ftrv4(ptr align 16 %tmp, ptr align 4 %b, ptr align 8 %a)
  call void @llvm.sh.ftrv4(ptr align 16 %d, ptr align 8 %s, ptr align 16 %tmp)
  ret void
}

; Observable intermediates and possible zero counts retain their original
; stores and arithmetic effects; no algebraic reassociation is allowed.
; IR-LABEL: define void @observed_product(
; IR: call void @llvm.sh.ftrv4
; IR: call void @observe
; IR: call void @llvm.sh.ftrv4
define void @observed_product(ptr %d, ptr %s, ptr %a, ptr %b) {
  %tmp = alloca [4 x <4 x float>], align 16
  call void @llvm.sh.ftrv4(ptr %tmp, ptr %b, ptr %a)
  call void @observe(ptr %tmp)
  call void @llvm.sh.ftrv4(ptr %d, ptr %s, ptr %tmp)
  ret void
}

; IR-LABEL: define void @possible_zero(
; IR: call void @llvm.sh.ftrv4
; IR: call void @llvm.sh.ftrv.array
define void @possible_zero(ptr %d, ptr %s, i32 %n, ptr %a, ptr %b) {
  %tmp = alloca [4 x <4 x float>], align 16
  call void @llvm.sh.ftrv4(ptr %tmp, ptr %b, ptr %a)
  call void @llvm.sh.ftrv.array(ptr %d, ptr %s, i32 %n, ptr %tmp)
  ret void
}

; IR-LABEL: define void @escaped_product(
; IR: call void @llvm.sh.ftrv4
; IR: call void @llvm.sh.ftrv4
; IR: store ptr
define void @escaped_product(ptr %d, ptr %s, ptr %a, ptr %b, ptr %out) {
  %tmp = alloca [4 x <4 x float>], align 16
  call void @llvm.sh.ftrv4(ptr %tmp, ptr %b, ptr %a)
  call void @llvm.sh.ftrv4(ptr %d, ptr %s, ptr %tmp)
  store ptr %tmp, ptr %out
  ret void
}
