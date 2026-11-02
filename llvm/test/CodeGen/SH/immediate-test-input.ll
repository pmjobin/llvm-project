; RUN: llc -O3 -mtriple=shle -mcpu=sh2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O3 -mtriple=sh -mcpu=sh2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O3 -mtriple=shle -mcpu=sh3 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O3 -mtriple=sh -mcpu=sh3 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O3 -mtriple=shle -mcpu=sh4 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O3 -mtriple=sh -mcpu=sh4 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s
; RUN: llc -O0 -mtriple=shle -mcpu=sh2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=LEGACY
; RUN: sed 's/attributes #0 = { nounwind }/attributes #0 = { optsize }/' %s | llc -O3 -mtriple=shle -mcpu=sh2 -verify-machineinstrs -stop-after=finalize-isel | FileCheck %s --check-prefix=LEGACY
; RUN: sed 's/attributes #0 = { nounwind }/attributes #0 = { minsize }/' %s | llc -O3 -mtriple=shle -mcpu=sh2 -verify-machineinstrs -stop-after=finalize-isel | FileCheck %s --check-prefix=LEGACY
; RUN: sed 's/attributes #0 = { nounwind }/attributes #0 = { noinline optnone }/' %s | llc -O3 -mtriple=shle -mcpu=sh2 -verify-machineinstrs -stop-after=finalize-isel | FileCheck %s --check-prefix=LEGACY
; RUN: llc -O3 -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -O3 -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -O3 -mtriple=shle -mcpu=sh4 -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -O3 -mtriple=sh -mcpu=sh4 -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -O3 -mtriple=shle -mcpu=sh4 -target-abi=gnu -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -O3 -mtriple=sh -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs -filetype=obj < %s -o /dev/null

; R0 is an input, not a hidden post-allocation scratch. The reaching T is
; consumed by the branch; side-effecting memory stays on its original edge.
define i32 @test_loaded_value(ptr %p, ptr %q) #0 {
; CHECK-LABEL: name: test_loaded_value
; CHECK: [[VALUE:%[0-9]+]]:gpr = MOVL_load_reg
; CHECK: $r0 = COPY [[VALUE]]
; CHECK-NEXT: TSTri 3, implicit-def $tbit, implicit $r0
; CHECK-NEXT: BF {{.*}}, implicit $tbit
; LEGACY-LABEL: name: test_loaded_value
; LEGACY: TSTri_PSEUDO {{%[0-9]+}}, 3, implicit-def $tbit, implicit-def dead $r0
entry:
	%x = load volatile i32, ptr %p, align 4
	%masked = and i32 %x, 3
	%c = icmp eq i32 %masked, 0
	br i1 %c, label %yes, label %no
yes:
	%a = load volatile i32, ptr %q, align 4
	%r = add i32 %a, %x
	ret i32 %r
no:
	store volatile i32 %x, ptr %q, align 4
	ret i32 %x
}

define i64 @test_carry_and_multiple_use(i32 %x, i64 %acc, ptr %p) #0 {
; CHECK-LABEL: name: test_carry_and_multiple_use
; Multiple condition users keep the existing register-test representation.
; CHECK: TST {{.*}}, implicit-def $tbit
; CHECK: CLRT implicit-def $tbit
; CHECK: ADDC {{.*}}implicit $tbit
; CHECK: TST {{.*}}, implicit-def $tbit
; CHECK-NEXT: {{%[0-9]+}}:gpr = MOVT implicit $tbit
; LEGACY-LABEL: name: test_carry_and_multiple_use
; LEGACY: TST {{.*}}, implicit-def $tbit
entry:
	%masked = and i32 %x, 255
	%c = icmp eq i32 %masked, 0
	%v = select i1 %c, i32 17, i32 %x
	%boolean = zext i1 %c to i32
	store volatile i32 %boolean, ptr %p, align 4
	%wide = zext i32 %v to i64
	%r = add i64 %acc, %wide
	ret i64 %r
}

attributes #0 = { nounwind }
