; RUN: llc -mtriple=shle -target-abi=gnu-hard-float -mcpu=sh4a -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle -target-abi=gnu-hard-float -mcpu=sh4a -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh -target-abi=gnu-hard-float -mcpu=sh4a -force-new-pm-codegen -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle -target-abi=gnu-hard-float -mcpu=sh4 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=SH4
; RUN: llc -mtriple=shle -target-abi=gnu-hard-float -mcpu=sh4a -O0 -verify-machineinstrs %s -o /dev/null

declare float @llvm.sh.fipr(<4 x float>, <4 x float>)

; The tied FV output ends at the pseudo, while the FR result is independent.
; MIR-LABEL: name: dot
; MIR: = FIPR 
; CHECK-LABEL: dot:
; CHECK: fpchg
; CHECK: fschg
; CHECK: fipr
; CHECK: fschg
; CHECK: fpchg
; CHECK: rts
; CHECK: fmov
; SH4-LABEL: dot:
; SH4-NOT: fpchg
; SH4: fipr
; SH4-NOT: fpchg
; SH4: rts
define float @dot(ptr %p, ptr %q) {
  %a = load <4 x float>, ptr %p, align 16
  %b = load <4 x float>, ptr %q, align 16
  %d = call float @llvm.sh.fipr(<4 x float> %a, <4 x float> %b)
  ret float %d
}

; Identical operands must remain legal, and original lanes remain live.
; MIR-LABEL: name: identical_live
; MIR: = FIPR 
; CHECK-LABEL: identical_live:
; CHECK: fipr
; CHECK: fadd
; CHECK: fadd
define float @identical_live(ptr %p) {
  %v = load <4 x float>, ptr %p, align 16
  %d = call float @llvm.sh.fipr(<4 x float> %v, <4 x float> %v)
  %x = extractelement <4 x float> %v, i32 0
  %y = extractelement <4 x float> %v, i32 3
  %a = fadd float %d, %x
  %b = fadd float %a, %y
  ret float %b
}

; Packing independent scalar results must not keep four FV allocations live.
; MIR-LABEL: name: pack
; MIR-COUNT-4: :fr, {{%[0-9]+}}:fv = FIPR_VALUE
; CHECK-LABEL: pack:
; CHECK: fpchg
; CHECK: fschg
; CHECK-COUNT-4: fipr
; CHECK: fmov.d
; CHECK: fschg
; CHECK: fpchg
; CHECK: rts
define void @pack(ptr %out, ptr %a, ptr %b) {
  %v = load <4 x float>, ptr %b, align 16
  %a0 = load <4 x float>, ptr %a, align 16
  %p1 = getelementptr <4 x float>, ptr %a, i32 1
  %a1 = load <4 x float>, ptr %p1, align 16
  %p2 = getelementptr <4 x float>, ptr %a, i32 2
  %a2 = load <4 x float>, ptr %p2, align 16
  %p3 = getelementptr <4 x float>, ptr %a, i32 3
  %a3 = load <4 x float>, ptr %p3, align 16
  %d0 = call float @llvm.sh.fipr(<4 x float> %a0, <4 x float> %v)
  %d1 = call float @llvm.sh.fipr(<4 x float> %a1, <4 x float> %v)
  %d2 = call float @llvm.sh.fipr(<4 x float> %a2, <4 x float> %v)
  %d3 = call float @llvm.sh.fipr(<4 x float> %a3, <4 x float> %v)
  %i0 = insertelement <4 x float> poison, float %d0, i32 0
  %i1 = insertelement <4 x float> %i0, float %d1, i32 1
  %i2 = insertelement <4 x float> %i1, float %d2, i32 2
  %i3 = insertelement <4 x float> %i2, float %d3, i32 3
  store <4 x float> %i3, ptr %out, align 16
  ret void
}
