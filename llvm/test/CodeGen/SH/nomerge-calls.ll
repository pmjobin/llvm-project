; RUN: llc < %s -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O2 | FileCheck %s

declare void @report(i32)

define void @nomerge_calls(i1 %condition, i32 %address) {
; CHECK-LABEL: nomerge_calls:
; CHECK: jsr
; CHECK: jsr
entry:
	br i1 %condition, label %first, label %second

first:
	call void @report(i32 %address) #0
	unreachable

second:
	call void @report(i32 %address) #0
	unreachable
}

attributes #0 = { nomerge }
