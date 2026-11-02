; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O0 -enable-new-pm -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O2 -enable-new-pm -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s

; Prologue scratch selection must preserve the incoming static chain in r3.
; These are direct IR ABI tests, not a claim of frontend nested-function support.
; CHECK-LABEL: nested_with_realign:
; CHECK: mov #-16,r1
; CHECK: and r1,r15
; CHECK: mov.l @r3,r0
define i32 @nested_with_realign(ptr nest %environment) nounwind {
  %local = alloca [16 x i8], align 16
  store volatile i8 0, ptr %local, align 16
  %value = load i32, ptr %environment, align 4
  ret i32 %value
}

; CHECK-LABEL: nested_with_large_frame:
; CHECK: sub r1,r15
; CHECK: mov.l @r3,r0
define i32 @nested_with_large_frame(ptr nest %environment) nounwind optsize {
  %local = alloca [512 x i8], align 4
  call void asm sideeffect "", "*m"(ptr elementtype([512 x i8]) %local)
  %value = load i32, ptr %environment, align 4
  ret i32 %value
}

; Check sret and all ordinary integer argument live-ins as well as nest.
define void @nested_with_arguments(ptr sret({ i32 }) %out, ptr nest %environment,
                                  i32 %a, i32 %b, i32 %c, i32 %d) nounwind {
  %local = alloca [512 x i8], align 16
  call void asm sideeffect "", "*m"(ptr elementtype([512 x i8]) %local)
  %value = load i32, ptr %environment, align 4
  %ab = add i32 %a, %b
  %cd = add i32 %c, %d
  %abcd = add i32 %ab, %cd
  %sum = add i32 %value, %abcd
  store i32 %sum, ptr %out, align 4
  ret void
}
