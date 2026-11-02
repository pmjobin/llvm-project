; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2a < %s | FileCheck %s --check-prefix=SH2A
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2a < %s | FileCheck %s --check-prefix=SH2A
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -mtune=sh2a < %s | FileCheck %s --check-prefix=TUNE-ONLY

define i32 @signed_20_max() {
; SH2A-LABEL: signed_20_max:
; SH2A: movi20 #524287,r0
; TUNE-ONLY-LABEL: signed_20_max:
; TUNE-ONLY-NOT: movi20
; TUNE-ONLY: rts
	ret i32 524287
}

define i32 @signed_20_min() {
; SH2A-LABEL: signed_20_min:
; SH2A: movi20 #-524288,r0
	ret i32 -524288
}

define i32 @shifted_20_max() {
; SH2A-LABEL: shifted_20_max:
; SH2A: movi20s #134217472,r0
	ret i32 134217472
}

define i32 @shifted_20_min() {
; SH2A-LABEL: shifted_20_min:
; SH2A: movi20s #-134217728,r0
	ret i32 -134217728
}

define i32 @outside_ranges() {
; SH2A-LABEL: outside_ranges:
; SH2A-NOT: movi20
; SH2A: rts
	ret i32 134217473
}
