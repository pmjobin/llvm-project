; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=prolog-epilog %s -o - | FileCheck %s --check-prefix=PEI

%S5 = type { i32, i8 }
%S8 = type { i32, i32 }

declare i32 @consume(i32, i32, i32, ptr byval(%S8) align 4, i32)

; ISEL-LABEL: name: caller
; ISEL: ADJCALLSTACKDOWN 8, 0
; ISEL: MOVL_store_disp {{.*}}, $r15, 4
; ISEL: MOVL_store_reg {{.*}}, $r15
; ISEL: $r4 = COPY
; ISEL: $r5 = COPY
; ISEL: $r6 = COPY
; ISEL: $r7 = COPY
; ISEL: JSR {{.*}}, csr_sh
; ISEL: ADJCALLSTACKUP 8, 0
;
; PEI-LABEL: name: caller
; PEI-NOT: ADJCALLSTACK
; PEI: ADDri $r15, -8
; PEI: MOVL_store_disp {{.*}}, $r15, 4
; PEI: MOVL_store_reg {{.*}}, $r15
; PEI: JSR
; PEI-NEXT: ADDri $r15, 8
define i32 @caller(ptr %source, %S5 %value) {
	%result = call i32 @consume(i32 1, i32 2, i32 3, ptr byval(%S8) align 4 %source, i32 4)
	ret i32 %result
}

; ISEL-LABEL: name: callee
; ISEL: fixedStack:
; ISEL: offset: -4, size: 8
; ISEL: isImmutable: false
; ISEL: MOVL_store_disp {{.*}} %fixed-stack
; ISEL: LEA_FI
;
; PEI-LABEL: name: callee
; PEI-NOT: LEA_FI
; PEI: JSR
define i32 @callee(i32 %a, i32 %b, i32 %c, ptr byval(%S8) align 4 %value) {
	call void @escape(ptr %value)
	%second = getelementptr i8, ptr %value, i32 4
	%result = load i32, ptr %second, align 4
	ret i32 %result
}

declare void @escape(ptr)
