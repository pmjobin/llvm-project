; RUN: split-file %s %t
; RUN: opt %t/fdpic-le.ll -o %t/fdpic-le.bc
; RUN: opt %t/fdpic-be.ll -o %t/fdpic-be.bc
; RUN: llvm-dis %t/fdpic-le.bc -o - | FileCheck %s --check-prefix=IDENTITY
; RUN: llvm-lto -O2 -j1 -exported-symbol=fdpic_entry -o %t/fdpic-le.o %t/fdpic-le.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=fdpic_entry -o %t/fdpic-be.o %t/fdpic-be.bc
; RUN: llvm-readobj --file-headers --relocations %t/fdpic-le.o %t/fdpic-be.o | FileCheck %s --check-prefix=FDPIC-OBJECT
; RUN: opt -module-summary %t/fdpic-le.ll -o %t/fdpic-le-thin.bc
; RUN: opt -module-summary %t/fdpic-be.ll -o %t/fdpic-be-thin.bc
; RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=fdpic_entry %t/fdpic-le-thin.bc
; RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=fdpic_entry %t/fdpic-be-thin.bc
; RUN: llvm-readobj --file-headers --relocations %t/fdpic-le-thin.bc.thinlto.o %t/fdpic-be-thin.bc.thinlto.o | FileCheck %s --check-prefix=FDPIC-OBJECT
; RUN: opt %t/ordinary.ll -o %t/ordinary.bc
; RUN: not llvm-link %t/fdpic-le.bc %t/ordinary.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=ABI-CONFLICT
; RUN: not llvm-link %t/ordinary.bc %t/fdpic-le.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=ABI-CONFLICT
; RUN: opt -module-summary %t/ordinary.ll -o %t/ordinary-thin.bc
; RUN: not llvm-link %t/fdpic-le-thin.bc %t/ordinary-thin.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=ABI-CONFLICT
; RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=ordinary_entry %t/ordinary-thin.bc
; RUN: llvm-readobj --file-headers %t/ordinary-thin.bc.thinlto.o | FileCheck %s --check-prefix=ORDINARY-OBJECT

; IDENTITY: target triple = "sh2-unknown-linux-muslfdpic"
; IDENTITY: declare i32 @external(i32)
; IDENTITY: attributes {{.*}} "target-abi"="gnu-fdpic" "target-cpu"="sh2"
; IDENTITY: !{i32 1, !"target-abi", !"gnu-fdpic"}

; FDPIC-OBJECT: Flags [ (0x8002)
; FDPIC-OBJECT: R_SH_PLT32 external 0x0
; FDPIC-OBJECT: Flags [ (0x8002)
; FDPIC-OBJECT: R_SH_PLT32 external 0x0
; ORDINARY-OBJECT: Flags [ (0x2)
; ABI-CONFLICT: linking module flags 'target-abi': IDs have conflicting values

;--- fdpic-le.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "sh2-unknown-linux-muslfdpic"

declare i32 @external(i32)

define i32 @fdpic_entry(i32 %arg) #0 {
entry:
	%result = call i32 @external(i32 %arg)
	ret i32 %result
}

attributes #0 = { noinline "target-abi"="gnu-fdpic" "target-cpu"="sh2" "target-features"="+sh2,-single-fpu,-double-fpu" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-fdpic"}

;--- fdpic-be.ll
target datalayout = "E-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "sh2eb-unknown-linux-muslfdpic"

declare i32 @external(i32)

define i32 @fdpic_entry(i32 %arg) #0 {
entry:
	%result = call i32 @external(i32 %arg)
	ret i32 %result
}

attributes #0 = { noinline "target-abi"="gnu-fdpic" "target-cpu"="sh2" "target-features"="+sh2,-single-fpu,-double-fpu" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-fdpic"}

;--- ordinary.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-linux-musl"

define i32 @ordinary_entry(i32 %arg) #0 {
entry:
	ret i32 %arg
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh2" "target-features"="+sh2,-single-fpu,-double-fpu" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}
