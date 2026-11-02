; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
;
; A 4116-byte direct aggregate uses r4-r7 for its first four ABI words and
; 4100 bytes of incoming and outgoing stack argument area. This is beyond the
; former artificial limit and requires a materialized call-frame adjustment.

%Large = type [4116 x i8]

declare i32 @large_call_area_callee(%Large)

define i32 @large_call_area_caller(%Large %value) {
	%result = call i32 @large_call_area_callee(%Large %value)
	ret i32 %result
}
