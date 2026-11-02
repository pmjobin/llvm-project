; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs -stop-before=finalize-isel %s -o - | FileCheck %s
; RUN: llc -O3 -mtriple=sh-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs %s -o /dev/null

@observe = alias i32 (i32), ptr @fetestexcept

define i32 @fetestexcept(i32 %mask) noinline {
	%status = call i32 asm sideeffect "sts fpscr,$0", "=r"()
	%flags = and i32 %status, %mask
	ret i32 %flags
}

; Aliases do not hide explicit environment observations from the gate.
; CHECK-LABEL: name: alias_observation
; CHECK: SELECT_F32_PSEUDO
define i32 @alias_observation(float %x) #0 {
	%r = fptoui float %x to i32
	%flags = call i32 @observe(i32 31)
	%value = xor i32 %r, %flags
	ret i32 %value
}

; Unresolved callees cannot establish this optional policy's environment
; contract. Ordinary conversion correctness still uses the baseline path.
; CHECK-LABEL: name: indirect_observation
; CHECK: SELECT_F64_PSEUDO
define i32 @indirect_observation(double %x, ptr %observe) #0 {
	%r = fptoui double %x to i32
	%flags = call i32 %observe(i32 31)
	%value = xor i32 %r, %flags
	ret i32 %value
}

; CHECK-LABEL: name: trapping_environment
; CHECK: SELECT_F32_PSEUDO
define i32 @trapping_environment(float %x) #0 {
	%old = call i32 @feenableexcept(i32 31)
	%r = fptoui float %x to i32
	ret i32 %r
}
declare i32 @feenableexcept(i32)

attributes #0 = { "no-trapping-math"="true" }
