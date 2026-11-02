; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O2 -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O2 -verify-machineinstrs -filetype=obj -o %t.be.o %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O2 -verify-machineinstrs -filetype=obj -o %t.le.o %s
; RUN: llvm-readobj --relocations %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --relocations %t.le.o | FileCheck %s --check-prefix=OBJECT

@default_global = external global i32
@hidden_global = external hidden global i32
@tls_gd = external thread_local global i32
@tls_ld = internal thread_local(localdynamic) global i32 1
@tls_ie = external thread_local(initialexec) global i32
@tls_le = internal thread_local(localexec) global i32 2

declare i32 @external_callee(i32)

define i32 @st40_340_pic_tls() #0 {
	%default = load i32, ptr @default_global, align 4
	%hidden = load i32, ptr @hidden_global, align 4
	%gd = load i32, ptr @tls_gd, align 4
	%ld = load i32, ptr @tls_ld, align 4
	%ie = load i32, ptr @tls_ie, align 4
	%le = load i32, ptr @tls_le, align 4
	%sum0 = add i32 %default, %hidden
	%sum1 = add i32 %sum0, %gd
	%sum2 = add i32 %sum1, %ld
	%sum3 = add i32 %sum2, %ie
	%sum4 = add i32 %sum3, %le
	%result = call i32 @external_callee(i32 %sum4)
	ret i32 %result
}

define i32 @st40_400_pic_tls() #1 {
	%default = load i32, ptr @default_global, align 4
	%hidden = load i32, ptr @hidden_global, align 4
	%gd = load i32, ptr @tls_gd, align 4
	%ld = load i32, ptr @tls_ld, align 4
	%ie = load i32, ptr @tls_ie, align 4
	%le = load i32, ptr @tls_le, align 4
	%sum0 = add i32 %default, %hidden
	%sum1 = add i32 %sum0, %gd
	%sum2 = add i32 %sum1, %ld
	%sum3 = add i32 %sum2, %ie
	%sum4 = add i32 %sum3, %le
	%result = call i32 @external_callee(i32 %sum4)
	ret i32 %result
}

define i32 @st40_500_pic_tls() #2 {
	%default = load i32, ptr @default_global, align 4
	%hidden = load i32, ptr @hidden_global, align 4
	%gd = load i32, ptr @tls_gd, align 4
	%ld = load i32, ptr @tls_ld, align 4
	%ie = load i32, ptr @tls_ie, align 4
	%le = load i32, ptr @tls_le, align 4
	%sum0 = add i32 %default, %hidden
	%sum1 = add i32 %sum0, %gd
	%sum2 = add i32 %sum1, %ld
	%sum3 = add i32 %sum2, %ie
	%sum4 = add i32 %sum3, %le
	%result = call i32 @external_callee(i32 %sum4)
	ret i32 %result
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-340" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-400" "use-soft-float"="true" }
attributes #2 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-500" "use-soft-float"="true" }

; ASM-LABEL: st40_340_pic_tls:
; ASM-DAG: .long	default_global@GOT
; ASM-DAG: .long	hidden_global@GOTOFF
; ASM-DAG: .long	tls_gd@TLSGD
; ASM-DAG: .long	tls_ld@TLSLDM
; ASM-DAG: .long	tls_ld@DTPOFF
; ASM-DAG: .long	tls_ie@GOTTPOFF
; ASM-DAG: .long	tls_le@TPOFF
; ASM-DAG: .long	external_callee@PLT
; ASM-LABEL: st40_400_pic_tls:
; ASM-DAG: .long	default_global@GOT
; ASM-DAG: .long	hidden_global@GOTOFF
; ASM-DAG: .long	tls_gd@TLSGD
; ASM-DAG: .long	tls_ld@TLSLDM
; ASM-DAG: .long	tls_ld@DTPOFF
; ASM-DAG: .long	tls_ie@GOTTPOFF
; ASM-DAG: .long	tls_le@TPOFF
; ASM-DAG: .long	external_callee@PLT
; ASM-LABEL: st40_500_pic_tls:
; ASM-DAG: .long	default_global@GOT
; ASM-DAG: .long	hidden_global@GOTOFF
; ASM-DAG: .long	tls_gd@TLSGD
; ASM-DAG: .long	tls_ld@TLSLDM
; ASM-DAG: .long	tls_ld@DTPOFF
; ASM-DAG: .long	tls_ie@GOTTPOFF
; ASM-DAG: .long	tls_le@TPOFF
; ASM-DAG: .long	external_callee@PLT

; OBJECT-DAG: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; OBJECT-DAG: R_SH_GOT32 default_global
; OBJECT-DAG: R_SH_GOTOFF hidden_global
; OBJECT-DAG: R_SH_TLS_GD_32 tls_gd
; OBJECT-DAG: R_SH_TLS_LD_32 tls_ld
; OBJECT-DAG: R_SH_TLS_LDO_32 tls_ld
; OBJECT-DAG: R_SH_TLS_IE_32 tls_ie
; OBJECT-DAG: R_SH_TLS_LE_32 tls_le
; OBJECT-DAG: R_SH_PLT32 external_callee
