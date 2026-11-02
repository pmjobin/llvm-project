; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -o /dev/null %s 2>&1 | FileCheck %s

define void @unsupported_cpu() #0 {
	ret void
}

attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-nosave-low-regs" }

; CHECK: error: {{.*}}SH nosave_low_regs requires hardware-banked r0-r7 registers
