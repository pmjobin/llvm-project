; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -relocation-model=pic -O2 %s -o /dev/null 2>&1 | FileCheck %s

define void @switched() #0 {
	ret void
}

attributes #0 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-sp-switch"="alternate_stack" "target-abi"="gnu" }

; CHECK: error: {{.*}}SH sp_switch is unsupported for position-independent code
