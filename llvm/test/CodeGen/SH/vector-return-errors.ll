; RUN: split-file %s %t
; RUN: not llc -mtriple=shle -mcpu=sh4 %t/return.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=RETURN
; RUN: not llc -mtriple=shle -mcpu=sh4 %t/call.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALL
; RUN: not llc -mtriple=shle -mcpu=sh4a %t/strict.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=STRICT
; RETURN: SH vector returns require an explicit sret parameter
; CALL: SH vector call results require an explicit sret parameter
; STRICT: SH graphics intrinsics are unsupported in handlers or strict FP functions
; Clang supplies the GNU hidden R2 result pointer for both native vector sizes.
; Reject unlowered direct-result IR rather than inventing a register ABI.
;--- return.ll
define <2 x float> @bad(<2 x float> %v) { ret <2 x float> %v }
;--- call.ll
declare <4 x float> @callee()
define void @bad(ptr %out) {
  %v = call <4 x float> @callee()
  store <4 x float> %v, ptr %out
  ret void
}
;--- strict.ll
declare float @llvm.sh.fsrra(float)
define float @strict(float %x) strictfp {
  %r = call float @llvm.sh.fsrra(float %x)
  ret float %r
}
