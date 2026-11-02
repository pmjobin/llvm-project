; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -o /dev/null %s 2>&1 | FileCheck %s

define void @unsupported_cpu() #0 {
	ret void
}

attributes #0 = { "sh-interrupt-handler"="interrupt" "sh-resbank" }

; CHECK: error: {{.*}}SH resbank requires an SH2A target
