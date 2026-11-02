; RUN: split-file %s %t
; RUN: opt %t/input.ll -o %t/input.bc
; RUN: llvm-dis %t/input.bc -o - | FileCheck %s --check-prefix=IDENTITY
; RUN: opt -passes='default<O2>' %t/input.bc -S -o - | FileCheck %s --check-prefix=OPT
; RUN: llvm-lto -O2 -j1 -exported-symbol=interrupt_entry -exported-symbol=trapa_entry -o %t/full.o %t/input.bc
; RUN: llvm-objdump -d %t/full.o | FileCheck %s --check-prefix=OBJECT
; RUN: opt -module-summary %t/input.ll -o %t/input-thin.bc
; RUN: llvm-dis %t/input-thin.bc -o - | FileCheck %s --check-prefix=IDENTITY
; RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=interrupt_entry -exported-symbol=trapa_entry %t/input-thin.bc
; RUN: llvm-objdump -d %t/input-thin.bc.thinlto.o | FileCheck %s --check-prefix=OBJECT

; IDENTITY: define void @interrupt_entry() #[[INT:[0-9]+]]
; IDENTITY: define void @trapa_entry() #[[TRAPA:[0-9]+]]
; IDENTITY: attributes #[[INT]] = { {{.*}}"sh-interrupt-handler"="interrupt"
; IDENTITY: attributes #[[TRAPA]] = { {{.*}}"sh-interrupt-handler"="trapa"

; OPT-LABEL: define void @interrupt_entry()
; OPT-SAME: #[[OPT_INT:[0-9]+]]
; OPT: add i32
; OPT-LABEL: define void @trapa_entry()
; OPT-SAME: #[[OPT_TRAPA:[0-9]+]]
; OPT-LABEL: define void @ordinary_empty()
; OPT-DAG: attributes #[[OPT_INT]] = { {{.*}}"sh-interrupt-handler"="interrupt"
; OPT-DAG: attributes #[[OPT_TRAPA]] = { {{.*}}"sh-interrupt-handler"="trapa"

; OBJECT-LABEL: <interrupt_entry>:
; OBJECT: rte
; OBJECT-NEXT: nop
; OBJECT-LABEL: <trapa_entry>:
; OBJECT: rte
; OBJECT-NEXT: nop

;--- input.ll
target triple = "sh-unknown-none-elf"

@value = global i32 0, align 4

define internal i32 @inline_helper(i32 %arg) #0 {
	%result = add i32 %arg, 1
	ret i32 %result
}

define void @interrupt_entry() #1 {
	%old = load volatile i32, ptr @value, align 4
	%new = call i32 @inline_helper(i32 %old)
	store volatile i32 %new, ptr @value, align 4
	ret void
}

define void @trapa_entry() #2 {
	ret void
}

define void @ordinary_empty() #3 {
	ret void
}

attributes #0 = { alwaysinline nounwind }
attributes #1 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "target-abi"="gnu" "target-cpu"="sh2" "target-features"="+sh2,-single-fpu,-double-fpu" }
attributes #2 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="trapa" "target-abi"="gnu" "target-cpu"="sh2" "target-features"="+sh2,-single-fpu,-double-fpu" }
attributes #3 = { nounwind }
