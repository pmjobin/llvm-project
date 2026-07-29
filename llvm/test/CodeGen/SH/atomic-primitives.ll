; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=machine-scheduler < %s | FileCheck %s --check-prefix=SCHED
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=machine-scheduler < %s | FileCheck %s --check-prefix=SCHED
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

@byte = global i8 0, align 1

declare i32 @llvm.sh.tas.b(ptr)
declare i32 @callee(i32)

; ISEL-LABEL: name: tas_result
; ISEL: TAS_B %{{[0-9]+}}, implicit-def $tbit :: (load store monotonic (s8) on %ir.p)
; ISEL-NEXT: %{{[0-9]+}}:gpr = MOVT implicit $tbit
; SCHED-LABEL: name: tas_result
; SCHED: TAS_B %{{[0-9]+}}, implicit-def $tbit :: (load store monotonic (s8) on %ir.p)
; SCHED-NEXT: %{{[0-9]+}}:gpr = MOVT implicit $tbit
; ASM-LABEL: tas_result:
; ASM: tas.b	@r4
; ASM-NEXT: movt	r0
define i32 @tas_result(ptr %p) {
  %result = call i32 @llvm.sh.tas.b(ptr %p)
  ret i32 %result
}

; ASM-LABEL: tas_branch:
; ASM: tas.b	@r4
; ASM-NEXT: movt	[[RESULT:r[0-9]+]]
; ASM-NEXT: tst	[[RESULT]],[[RESULT]]
define i32 @tas_branch(ptr %p) {
  %result = call i32 @llvm.sh.tas.b(ptr %p)
  %set = icmp ne i32 %result, 0
  br i1 %set, label %yes, label %no

yes:
  ret i32 7

no:
  ret i32 9
}

; ASM-LABEL: tas_store:
; ASM: tas.b	@r4
; ASM-NEXT: movt	[[STORED:r[0-9]+]]
; ASM: mov.l	[[STORED]],@r5
define void @tas_store(ptr %p, ptr %out) {
  %result = call i32 @llvm.sh.tas.b(ptr %p)
  store i32 %result, ptr %out, align 4
  ret void
}

; ASM-LABEL: tas_pair:
; ASM: tas.b	@r4
; ASM-NEXT: movt	{{r[0-9]+}}
; ASM: tas.b	@r5
; ASM-NEXT: movt	{{r[0-9]+}}
define i32 @tas_pair(ptr %a, ptr %b) {
  %x = call i32 @llvm.sh.tas.b(ptr %a)
  %y = call i32 @llvm.sh.tas.b(ptr %b)
  %sum = add i32 %x, %y
  ret i32 %sum
}

; ISEL-LABEL: name: tas_global
; ISEL: TAS_B killed %{{[0-9]+}}, implicit-def $tbit :: (load store monotonic (s8) on @byte)
; ISEL-NEXT: %{{[0-9]+}}:gpr = MOVT implicit $tbit
; ASM-LABEL: tas_global:
; ASM: tas.b	@{{r[0-9]+}}
; ASM-NEXT: movt	r0
; ASM-NOT: __atomic
define i32 @tas_global() {
  %result = call i32 @llvm.sh.tas.b(ptr @byte)
  ret i32 %result
}

; ASM-LABEL: tas_around_call:
; ASM: tas.b	@r4
; ASM-NEXT: movt	{{r[0-9]+}}
; ASM: jsr	@{{r[0-9]+}}
; ASM-NEXT: nop
; ASM: tas.b	@{{r[0-9]+}}
; ASM-NEXT: movt	{{r[0-9]+}}
define i32 @tas_around_call(ptr %a, ptr %b, i32 %v) {
  %x = call i32 @llvm.sh.tas.b(ptr %a)
  %called = call i32 @callee(i32 %v)
  %y = call i32 @llvm.sh.tas.b(ptr %b)
  %sum0 = add i32 %x, %called
  %sum1 = add i32 %sum0, %y
  ret i32 %sum1
}

; ASM-LABEL: tas_with_i64_carry:
; ASM: addc
; ASM: tas.b	@r4
; ASM-NEXT: movt	{{r[0-9]+}}
define i64 @tas_with_i64_carry(ptr %p, i64 %a, i64 %b, ptr %out) {
  %result = call i32 @llvm.sh.tas.b(ptr %p)
  store i32 %result, ptr %out, align 4
  %sum = add i64 %a, %b
  ret i64 %sum
}

; ASM-LABEL: tas_with_division:
; ASM: div0u
; ASM: tas.b	@r4
; ASM-NEXT: movt	{{r[0-9]+}}
define i32 @tas_with_division(ptr %p, i32 %a, i32 %b) {
  %result = call i32 @llvm.sh.tas.b(ptr %p)
  %quotient = udiv i32 %a, %b
  %sum = add i32 %result, %quotient
  ret i32 %sum
}

; ASM-LABEL: tas_pressure:
; ASM: tas.b	@r4
; ASM-NEXT: movt	{{r[0-9]+}}
define i32 @tas_pressure(ptr %p, i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f, i32 %g, i32 %h) {
  %result = call i32 @llvm.sh.tas.b(ptr %p)
  %s0 = add i32 %a, %b
  %s1 = add i32 %c, %d
  %s2 = add i32 %e, %f
  %s3 = add i32 %g, %h
  %s4 = add i32 %s0, %s1
  %s5 = add i32 %s2, %s3
  %s6 = add i32 %s4, %s5
  %sum = add i32 %s6, %result
  ret i32 %sum
}

; ASM-NOT: __atomic
