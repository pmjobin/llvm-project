; RUN: llc -mtriple=x86_64-unknown-unknown -mattr=+sse2 -O2 < %s | FileCheck %s

; Keep vector div/rem folds independent of scalar select support. The three
; operations below must lower to their vector compare/select forms rather than
; scalarized division or remainder operations.

define <4 x i32> @sdiv_min(<4 x i32> %x) {
; CHECK-LABEL: sdiv_min:
; CHECK:       # %bb.0:
; CHECK-NEXT:    pcmpeqd {{\.?LCPI[0-9]+_[0-9]+}}(%rip), %xmm0
; CHECK-NEXT:    psrld $31, %xmm0
; CHECK-NEXT:    retq
  %result = sdiv <4 x i32> %x, <i32 -2147483648, i32 -2147483648, i32 -2147483648, i32 -2147483648>
  ret <4 x i32> %result
}

define <4 x i32> @udiv_max(<4 x i32> %x) {
; CHECK-LABEL: udiv_max:
; CHECK:       # %bb.0:
; CHECK-NEXT:    pcmpeqd %xmm1, %xmm1
; CHECK-NEXT:    pcmpeqd %xmm1, %xmm0
; CHECK-NEXT:    psrld $31, %xmm0
; CHECK-NEXT:    retq
  %result = udiv <4 x i32> %x, <i32 -1, i32 -1, i32 -1, i32 -1>
  ret <4 x i32> %result
}

define <4 x i32> @urem_max(<4 x i32> %x) {
; CHECK-LABEL: urem_max:
; CHECK:       # %bb.0:
; CHECK-NEXT:    pcmpeqd %xmm1, %xmm1
; CHECK-NEXT:    pcmpeqd %xmm0, %xmm1
; CHECK-NEXT:    pandn %xmm0, %xmm1
; CHECK-NEXT:    movdqa %xmm1, %xmm0
; CHECK-NEXT:    retq
  %result = urem <4 x i32> %x, <i32 -1, i32 -1, i32 -1, i32 -1>
  ret <4 x i32> %result
}
