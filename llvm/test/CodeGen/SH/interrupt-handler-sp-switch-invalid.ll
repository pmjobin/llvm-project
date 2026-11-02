; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -relocation-model=static -o /dev/null %t/missing.ll 2>&1 | FileCheck %s --check-prefix=MISSING
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -relocation-model=static -o /dev/null %t/empty.ll 2>&1 | FileCheck %s --check-prefix=EMPTY
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -relocation-model=static -o /dev/null %t/trapa.ll 2>&1 | FileCheck %s --check-prefix=TRAPA

; MISSING: error: {{.*}}SH sp_switch requires an interrupt hardware handler
; EMPTY: error: {{.*}}SH sp_switch attribute has an invalid symbol
; TRAPA: error: {{.*}}SH sp_switch requires an interrupt hardware handler

;--- missing.ll
define void @missing() #0 {
	ret void
}
attributes #0 = { "sh-sp-switch"="alternate_stack" }

;--- empty.ll
define void @empty() #0 {
	ret void
}
attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-sp-switch"="" }

;--- trapa.ll
define void @trapa() #0 {
	ret void
}
attributes #0 = { "sh-interrupt-handler"="trapa" "sh-sp-switch"="alternate_stack" }
