; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh3 -o /dev/null %t/nosave-missing.ll 2>&1 | FileCheck %s --check-prefix=NOSAVE-MISSING
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh3 -o /dev/null %t/nosave-value.ll 2>&1 | FileCheck %s --check-prefix=NOSAVE-VALUE
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2a -o /dev/null %t/resbank-missing.ll 2>&1 | FileCheck %s --check-prefix=RESBANK-MISSING
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2a -o /dev/null %t/resbank-value.ll 2>&1 | FileCheck %s --check-prefix=RESBANK-VALUE
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2a -o /dev/null %t/resbank-trapa.ll 2>&1 | FileCheck %s --check-prefix=RESBANK-TRAPA
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2a -o /dev/null %t/resbank-nosave.ll 2>&1 | FileCheck %s --check-prefix=RESBANK-NOSAVE
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -o /dev/null %t/sp-null.ll 2>&1 | FileCheck %s --check-prefix=SP-NULL

; NOSAVE-MISSING: error: {{.*}}SH nosave_low_regs requires an interrupt hardware handler
; NOSAVE-VALUE: error: {{.*}}SH nosave_low_regs attribute has an invalid value
; RESBANK-MISSING: error: {{.*}}SH resbank requires an interrupt hardware handler
; RESBANK-VALUE: error: {{.*}}SH resbank attribute has an invalid value
; RESBANK-TRAPA: error: {{.*}}SH resbank requires an interrupt hardware handler
; RESBANK-NOSAVE: error: {{.*}}SH resbank and nosave_low_regs attributes are incompatible
; SP-NULL: error: {{.*}}SH sp_switch attribute has an invalid symbol

;--- nosave-missing.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-nosave-low-regs" }

;--- nosave-value.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-nosave-low-regs"="yes" }

;--- resbank-missing.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-resbank" }

;--- resbank-value.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-resbank"="yes" }

;--- resbank-trapa.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-interrupt-handler"="trapa" "sh-resbank" }

;--- resbank-nosave.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-nosave-low-regs" "sh-resbank" }

;--- sp-null.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-sp-switch"="alternate\00stack" }
