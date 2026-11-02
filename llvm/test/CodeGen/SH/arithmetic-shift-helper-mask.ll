; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O1 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2e -O3 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2e -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2e -relocation-model=pic -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=NATIVE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3e -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=NATIVE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=NATIVE

; The arithmetic helper destroys r5. A live count must therefore interfere
; with its call mask, while left/logical shift helpers still preserve r5.
; Check the selected MIR contract independently of register allocation.
define i32 @ashr_count_live(i32 %value, i32 %count) {
; MIR-LABEL: name:            ashr_count_live
; MIR: value:           {{'?}}__ashrsi3
; MIR: {{JSR|BSRF}} {{.*}}, csr_sh_ashr, {{.*}}$r4, {{(implicit )?}}$r5,
; NATIVE-LABEL: name:            ashr_count_live
; NATIVE-NOT: csr_sh_
; NATIVE: SHAD
	%shifted = ashr i32 %value, %count
	%result = xor i32 %shifted, %count
	ret i32 %result
}

; The second incoming argument starts in r5 but is unrelated to the count.
define i32 @ashr_unrelated_r5(i32 %value, i32 %other, i32 %count) {
; MIR-LABEL: name:            ashr_unrelated_r5
; MIR: COPY $r5
; MIR: {{JSR|BSRF}} {{.*}}, csr_sh_ashr,
	%shifted = ashr i32 %value, %count
	%result = xor i32 %shifted, %other
	ret i32 %result
}

define i32 @shl_count_live(i32 %value, i32 %count) {
; MIR-LABEL: name:            shl_count_live
; MIR: value:           {{'?}}__ashlsi3
; MIR: {{JSR|BSRF}} {{.*}}, csr_sh_shift, {{.*}}$r4, {{(implicit )?}}$r5,
; NATIVE-LABEL: name:            shl_count_live
; NATIVE-NOT: csr_sh_
; NATIVE: SHLD
	%shifted = shl i32 %value, %count
	%result = xor i32 %shifted, %count
	ret i32 %result
}

define i32 @lshr_count_live(i32 %value, i32 %count) {
; MIR-LABEL: name:            lshr_count_live
; MIR: value:           {{'?}}__lshrsi3
; MIR: {{JSR|BSRF}} {{.*}}, csr_sh_shift, {{.*}}$r4, {{(implicit )?}}$r5,
; NATIVE-LABEL: name:            lshr_count_live
; NATIVE-NOT: csr_sh_
; NATIVE: SHLD
	%shifted = lshr i32 %value, %count
	%result = xor i32 %shifted, %count
	ret i32 %result
}

define i32 @ashr_optsize(i32 %value, i32 %count) optsize {
; MIR-LABEL: name:            ashr_optsize
; MIR: {{JSR|BSRF}} {{.*}}, csr_sh_ashr,
	%shifted = ashr i32 %value, %count
	%result = xor i32 %shifted, %count
	ret i32 %result
}

define i32 @ashr_minsize(i32 %value, i32 %count) minsize {
; MIR-LABEL: name:            ashr_minsize
; MIR-NOT: csr_sh_
; MIR: SHAR
; MIR-NOT: csr_sh_
	%shifted = ashr i32 %value, %count
	%result = xor i32 %shifted, %count
	ret i32 %result
}

define i32 @ashr_optnone(i32 %value, i32 %count) noinline optnone {
; MIR-LABEL: name:            ashr_optnone
; MIR-NOT: csr_sh_
; MIR: SHAR
; MIR-NOT: csr_sh_
	%shifted = ashr i32 %value, %count
	%result = xor i32 %shifted, %count
	ret i32 %result
}

; Explicit IR calls retain the ordinary C ABI mask.
declare i32 @__ashrsi3(i32, i32)
define i32 @explicit_ashr(i32 %value, i32 %count) {
; MIR-LABEL: name:            explicit_ashr
; MIR: {{JSR|BSRF}} {{.*}}, csr_sh{{(_fdpic)?(_hard)?}},
	%shifted = call i32 @__ashrsi3(i32 %value, i32 %count)
	%result = xor i32 %shifted, %count
	ret i32 %result
}
