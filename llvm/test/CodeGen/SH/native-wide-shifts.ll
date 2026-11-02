; RUN: split-file %s %t
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %t/variable.ll -o - | FileCheck %s --check-prefix=NATIVE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs %t/variable.ll -o - | FileCheck %s --check-prefix=NATIVE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs -stop-after=finalize-isel %t/variable.ll -o - | FileCheck %s --check-prefix=SH3-BOUNDARY
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %t/variable.ll -o - | FileCheck %s --check-prefix=LOOP
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs %t/variable.ll -o %t.sh2.default
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2a -O3 -verify-machineinstrs %t/variable.ll -o %t.sh2a.default
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %t/variable.ll -o %t.o0.default
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %t/guarded.ll -o %t.guarded.default

; NATIVE-LABEL: shift_left:
; NATIVE-NOT: dt
; NATIVE: shld
; NATIVE-NOT: dt
; NATIVE-LABEL: shift_right:
; NATIVE-NOT: dt
; NATIVE: shld
; NATIVE-NOT: dt
; NATIVE-LABEL: shift_signed:
; NATIVE-NOT: dt
; NATIVE: shad
; NATIVE-NOT: dt
; NATIVE: rts
; NATIVE-NOT: dt

; SH3-BOUNDARY-LABEL: name: shift_left
; SH3-BOUNDARY: [[HALF:%[0-9]+]]:gpr = SHLR
; SH3-BOUNDARY: [[CROSSCOUNT:%[0-9]+]]:gpr = ADDri {{%[0-9]+}}, -31
; SH3-BOUNDARY: {{%[0-9]+}}:gpr = SHLD [[HALF]], [[CROSSCOUNT]]
; SH3-BOUNDARY-LABEL: name: shift_right
; SH3-BOUNDARY-LABEL: name: shift_signed
; SH3-BOUNDARY: [[SIGNCOUNT:%[0-9]+]]:gpr = MOVri -31
; SH3-BOUNDARY: {{%[0-9]+}}:gpr = SHAD {{%[0-9]+}}, [[SIGNCOUNT]]

; LOOP-LABEL: shift_left:
; LOOP: rotcl
; LOOP: dt
; LOOP-LABEL: shift_right:
; LOOP: rotcr
; LOOP: dt
; LOOP-LABEL: shift_signed:
; LOOP: shar
; LOOP: rotcr
; LOOP: dt

;--- variable.ll
define i64 @shift_left(i64 %value, i32 %count) {
  %masked = and i32 %count, 63
  %wide = zext i32 %masked to i64
  %result = shl i64 %value, %wide
  ret i64 %result
}
define i64 @shift_right(i64 %value, i32 %count) {
  %masked = and i32 %count, 63
  %wide = zext i32 %masked to i64
  %result = lshr i64 %value, %wide
  ret i64 %result
}
define i64 @shift_signed(i64 %value, i32 %count) {
  %masked = and i32 %count, 63
  %wide = zext i32 %masked to i64
  %result = ashr i64 %value, %wide
  ret i64 %result
}

;--- guarded.ll
define i64 @small(i64 %value, i64 %count) optsize {
  %result = shl i64 %value, %count
  ret i64 %result
}
define i64 @minimum(i64 %value, i64 %count) minsize {
  %result = lshr i64 %value, %count
  ret i64 %result
}
define i64 @unoptimized(i64 %value, i64 %count) noinline optnone {
  %result = ashr i64 %value, %count
  ret i64 %result
}
