; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-elf -O0 %t/floating.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=FLOATING
; RUN: not llc -mtriple=sh-unknown-elf -O0 %t/special.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SPECIAL
; RUN: not llc -mtriple=sh-unknown-elf -O0 %t/register.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=REGISTER
; RUN: not llc -mtriple=sh-unknown-elf -O0 %t/register-type.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=REGISTER-TYPE
; RUN: not llc -mtriple=sh-unknown-elf -O0 %t/modifier.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MODIFIER
; RUN: not llc -mtriple=sh-unknown-elf -O0 %t/tied.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TIED

; FLOATING: error: unknown asm constraint 'f'
; SPECIAL: error: unknown asm constraint 'w'
; REGISTER: error: could not allocate input reg for constraint '{r16}'
; REGISTER-TYPE: error: could not allocate output register for constraint 'z'
; MODIFIER: error: invalid operand in inline asm: 'mov ${1:M},$0'
; TIED: error: {{.*}}SH tied inline assembly operands require compatible integer GPR values

;--- floating.ll
define i32 @floating(i32 %input) {
	%value = call i32 asm "fmov $1,$0", "=r,f"(i32 %input)
	ret i32 %value
}

;--- special.ll
define i32 @special(i32 %input) {
	%value = call i32 asm "mov $1,$0", "=r,w"(i32 %input)
	ret i32 %value
}

;--- register.ll
define void @register(i32 %input) {
	call void asm sideeffect "", "{r16}"(i32 %input)
	ret void
}

;--- register-type.ll
define i64 @register_type() {
	%value = call i64 asm "", "=z"()
	ret i64 %value
}

;--- modifier.ll
define i32 @modifier(i32 %input) {
	%value = call i32 asm "mov ${1:M},$0", "=r,r"(i32 %input)
	ret i32 %value
}

;--- tied.ll
define i32 @tied(i64 %input) {
	%value = call i32 asm "", "=r,0"(i64 %input)
	ret i32 %value
}
