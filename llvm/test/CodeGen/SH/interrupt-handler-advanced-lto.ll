; RUN: split-file %s %t
; RUN: opt %t/input-sh4.ll -o %t/sh4.bc
; RUN: llvm-dis %t/sh4.bc -o - | FileCheck %s --check-prefix=SH4-IDENTITY
; RUN: opt -passes='default<O2>' %t/sh4.bc -S -o - | FileCheck %s --check-prefix=SH4-OPT
; RUN: llvm-lto -O2 -j1 -exported-symbol=nosave_entry -exported-symbol=switched_entry -exported-symbol=trap_entry -exported-symbol=fpu_entry -o %t/sh4-full.o %t/sh4.bc
; RUN: llvm-objdump -d --mcpu=sh4 --mattr=+single-fpu,+double-fpu,+fpu-size-mode,+fpu-register-bank %t/sh4-full.o | FileCheck %s --check-prefix=SH4-OBJECT
; RUN: opt -module-summary %t/input-sh4.ll -o %t/sh4-thin.bc
; RUN: llvm-dis %t/sh4-thin.bc -o - | FileCheck %s --check-prefix=SH4-IDENTITY
; RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=nosave_entry -exported-symbol=switched_entry -exported-symbol=trap_entry -exported-symbol=fpu_entry %t/sh4-thin.bc
; RUN: llvm-objdump -d --mcpu=sh4 --mattr=+single-fpu,+double-fpu,+fpu-size-mode,+fpu-register-bank %t/sh4-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4-OBJECT
; RUN: opt %t/input-sh2a.ll -o %t/sh2a.bc
; RUN: llvm-dis %t/sh2a.bc -o - | FileCheck %s --check-prefix=SH2A-IDENTITY
; RUN: opt -passes='default<O2>' %t/sh2a.bc -S -o - | FileCheck %s --check-prefix=SH2A-OPT
; RUN: llvm-lto -O2 -j1 -exported-symbol=resbank_entry -o %t/sh2a-full.o %t/sh2a.bc
; RUN: llvm-objdump -d --mcpu=sh2a %t/sh2a-full.o | FileCheck %s --check-prefix=SH2A-OBJECT
; RUN: opt -module-summary %t/input-sh2a.ll -o %t/sh2a-thin.bc
; RUN: llvm-dis %t/sh2a-thin.bc -o - | FileCheck %s --check-prefix=SH2A-IDENTITY
; RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=resbank_entry %t/sh2a-thin.bc
; RUN: llvm-objdump -d --mcpu=sh2a %t/sh2a-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH2A-OBJECT

; SH4-IDENTITY: define void @nosave_entry() #[[NOSAVE:[0-9]+]]
; SH4-IDENTITY: define void @switched_entry() #[[SWITCH:[0-9]+]]
; SH4-IDENTITY: define void @trap_entry() #[[TRAP:[0-9]+]]
; SH4-IDENTITY: define void @fpu_entry() #[[FPU:[0-9]+]]
; SH4-IDENTITY-DAG: attributes #[[NOSAVE]] = { {{.*}}"sh-interrupt-handler"="interrupt"{{.*}}"sh-nosave-low-regs"
; SH4-IDENTITY-DAG: attributes #[[SWITCH]] = { {{.*}}"sh-interrupt-handler"="interrupt"{{.*}}"sh-sp-switch"="alternate_stack"
; SH4-IDENTITY-DAG: attributes #[[TRAP]] = { {{.*}}"sh-interrupt-handler"="interrupt"{{.*}}"sh-trap-exit"="255"
; SH4-IDENTITY-DAG: attributes #[[FPU]] = { {{.*}}"sh-interrupt-handler"="interrupt"{{.*}}"target-abi"="gnu-hard-float"

; SH2A-IDENTITY: define void @resbank_entry() #[[RESBANK:[0-9]+]]
; SH2A-IDENTITY: attributes #[[RESBANK]] = { {{.*}}"sh-interrupt-handler"="interrupt"{{.*}}"sh-resbank"

; SH4-OPT-LABEL: define void @nosave_entry()
; SH4-OPT-SAME: #[[OPT_NOSAVE:[0-9]+]]
; SH4-OPT-LABEL: define void @switched_entry()
; SH4-OPT-SAME: #[[OPT_SWITCH:[0-9]+]]
; SH4-OPT-LABEL: define void @trap_entry()
; SH4-OPT-SAME: #[[OPT_TRAP:[0-9]+]]
; SH4-OPT-DAG: attributes #[[OPT_NOSAVE]] = { {{.*}}"sh-nosave-low-regs"
; SH4-OPT-DAG: attributes #[[OPT_SWITCH]] = { {{.*}}"sh-sp-switch"="alternate_stack"
; SH4-OPT-DAG: attributes #[[OPT_TRAP]] = { {{.*}}"sh-trap-exit"="255"
; SH2A-OPT: attributes #[[OPT_RESBANK:[0-9]+]] = { {{.*}}"sh-resbank"

; SH4-OBJECT-LABEL: <nosave_entry>:
; SH4-OBJECT: rte
; SH4-OBJECT-NEXT: nop
; SH4-OBJECT-LABEL: <switched_entry>:
; SH4-OBJECT: mov.l r0,@-r15
; SH4-OBJECT: mov.l @r15,r15
; SH4-OBJECT-NEXT: mov.l @r15+,r0
; SH4-OBJECT-NEXT: rte
; SH4-OBJECT-NEXT: nop
; SH4-OBJECT-LABEL: <trap_entry>:
; SH4-OBJECT: trapa #255
; SH4-OBJECT-NOT: rte
; SH4-OBJECT-LABEL: <fpu_entry>:
; SH4-OBJECT: sts.l fpscr,@-r15
; SH4-OBJECT: sts.l fpul,@-r15
; SH4-OBJECT: lds.l @r0+,fpscr
; SH4-OBJECT: rte
; SH4-OBJECT-NEXT: nop
; SH2A-OBJECT-LABEL: <resbank_entry>:
; SH2A-OBJECT: resbank
; SH2A-OBJECT-NEXT: rte
; SH2A-OBJECT-NEXT: nop

;--- input-sh4.ll
target triple = "sh-unknown-none-elf"

@alternate_stack = external global ptr
@float_value = global float 1.000000e+00, align 4

define void @nosave_entry() #0 {
	ret void
}

define void @switched_entry() #1 {
	ret void
}

define void @trap_entry() #2 {
	ret void
}

define void @fpu_entry() #3 {
	%value = load volatile float, ptr @float_value, align 4
	%result = fadd float %value, 1.000000e+00
	store volatile float %result, ptr @float_value, align 4
	ret void
}

attributes #0 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-nosave-low-regs" "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+banked-gprs,+double-fpu,+fpu-register-bank,+fpu-size-mode,+single-fpu" }
attributes #1 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-sp-switch"="alternate_stack" "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+banked-gprs,+double-fpu,+fpu-register-bank,+fpu-size-mode,+single-fpu" }
attributes #2 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-trap-exit"="255" "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+banked-gprs,+double-fpu,+fpu-register-bank,+fpu-size-mode,+single-fpu" }
attributes #3 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+banked-gprs,+double-fpu,+fpu-register-bank,+fpu-size-mode,+single-fpu" }

;--- input-sh2a.ll
target triple = "sh-unknown-none-elf"

define void @resbank_entry() #0 {
	ret void
}

attributes #0 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-resbank" "target-abi"="gnu" "target-cpu"="sh2a" "target-features"="+sh2,+sh2a,-double-fpu,-fpu-register-bank,-fpu-size-mode,-single-fpu" }
