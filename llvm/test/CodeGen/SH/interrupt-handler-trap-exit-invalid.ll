; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -o /dev/null %t/missing.ll 2>&1 | FileCheck %s --check-prefix=MISSING
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -o /dev/null %t/low.ll 2>&1 | FileCheck %s --check-prefix=NUMBER
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -o /dev/null %t/high.ll 2>&1 | FileCheck %s --check-prefix=NUMBER
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -o /dev/null %t/malformed.ll 2>&1 | FileCheck %s --check-prefix=NUMBER
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -o /dev/null %t/trapa.ll 2>&1 | FileCheck %s --check-prefix=TRAPA
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2a -mattr=-single-fpu,-double-fpu -o /dev/null %t/resbank.ll 2>&1 | FileCheck %s --check-prefix=RESBANK
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -o /dev/null %t/switch.ll 2>&1 | FileCheck %s --check-prefix=SWITCH

; MISSING: error: {{.*}}SH trap_exit requires an interrupt hardware handler
; NUMBER: error: {{.*}}SH trap_exit attribute has an invalid number
; TRAPA: error: {{.*}}SH trap_exit requires an interrupt hardware handler
; RESBANK: error: {{.*}}SH trap_exit and resbank attributes are incompatible
; SWITCH: error: {{.*}}SH trap_exit and sp_switch attributes are incompatible

;--- missing.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-trap-exit"="1" }

;--- low.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-trap-exit"="-1" }

;--- high.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-trap-exit"="256" }

;--- malformed.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-trap-exit"="trap" }

;--- trapa.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-interrupt-handler"="trapa" "sh-trap-exit"="1" }

;--- resbank.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-resbank" "sh-trap-exit"="1" "target-abi"="gnu" }

;--- switch.ll
define void @f() #0 { ret void }
attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-sp-switch"="alternate_stack" "sh-trap-exit"="1" }
