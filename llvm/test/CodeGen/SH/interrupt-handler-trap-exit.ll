; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -relocation-model=static -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,STATIC
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -relocation-model=pic -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,PIC

@value = external global i32

declare void @helper(i32)

define void @minimum() #0 {
	ret void
}

define void @maximum() #1 {
	ret void
}

define void @used() #2 {
	call void asm sideeffect "", "~{r8},~{mach},~{macl}"()
	ret void
}

define void @nonleaf() #2 {
	call void @helper(i32 1)
	ret void
}

define void @nosave() #3 {
	call void @helper(i32 2)
	ret void
}

define void @global_access() #2 {
	%loaded = load volatile i32, ptr @value
	call void @helper(i32 %loaded)
	ret void
}

attributes #0 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-trap-exit"="0" "target-abi"="gnu" }
attributes #1 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-trap-exit"="255" "target-abi"="gnu" }
attributes #2 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-trap-exit"="33" "target-abi"="gnu" }
attributes #3 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-nosave-low-regs" "sh-trap-exit"="34" "target-abi"="gnu" }

; COMMON-LABEL: minimum:
; COMMON-NEXT: ! %bb.0:
; COMMON-NEXT: trapa #0
; COMMON-NOT: rte

; COMMON-LABEL: maximum:
; COMMON-NEXT: ! %bb.0:
; COMMON-NEXT: trapa #255
; COMMON-NOT: rte

; COMMON-LABEL: used:
; COMMON: sts.l mach,{{.*}}Folded Spill
; COMMON: sts.l macl,{{.*}}Folded Spill
; COMMON: lds.l {{.*}},macl
; COMMON: lds.l {{.*}},mach
; COMMON-NEXT: trapa #33
; COMMON-NOT: rte

; COMMON-LABEL: nonleaf:
; COMMON: jsr
; COMMON: lds.l {{.*}},pr
; COMMON-NEXT: trapa #33
; COMMON-NOT: rte

; COMMON-LABEL: nosave:
; COMMON-NOT: mov.l r{{[0-7]}},{{.*}}Folded Spill
; COMMON: jsr
; COMMON: trapa #34
; COMMON-NOT: rte

; STATIC-LABEL: global_access:
; STATIC-NOT: mova
; STATIC: jsr
; STATIC: trapa #33
; STATIC-NOT: rte

; PIC-LABEL: global_access:
; PIC: mov.l r12,{{.*}}Folded Spill
; PIC: mova
; PIC: jsr
; PIC: mov.l {{.*}},r12{{.*}}Folded Reload
; PIC: trapa #33
; PIC-NOT: rte
