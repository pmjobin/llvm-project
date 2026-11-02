; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4al-dsp -target-abi=gnu -relocation-model=pic -O2 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4al-dsp -target-abi=gnu -relocation-model=pic -O2 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4al-dsp -target-abi=gnu -relocation-model=pic -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llvm-readobj --file-headers --sections --relocations %t.be.o %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-dwarfdump --verify %t.be.o %t.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --eh-frame %t.be.o %t.le.o | FileCheck %s --check-prefix=FRAME

@external_global = external global i32
@tls_gd = external thread_local global i32
@tls_ld = internal thread_local(localdynamic) global i32 1
@tls_ie = external thread_local(initialexec) global i32
@tls_le = internal thread_local(localexec) global i32 2

declare i32 @external_call(i32)
declare void @may_throw()
declare i32 @__gxx_personality_v0(...)

define i32 @dsp_pic_tls(i32 %input) #0 uwtable {
entry:
	call void asm sideeffect "padd x0,y0,a0", "~{a0},~{x0},~{y0},~{dsr}"()
	%global = load i32, ptr @external_global, align 4
	%gd = load i32, ptr @tls_gd, align 4
	%ld = load i32, ptr @tls_ld, align 4
	%ie = load i32, ptr @tls_ie, align 4
	%le = load i32, ptr @tls_le, align 4
	%sum0 = add i32 %input, %global
	%sum1 = add i32 %sum0, %gd
	%sum2 = add i32 %sum1, %ld
	%sum3 = add i32 %sum2, %ie
	%sum4 = add i32 %sum3, %le
	%result = call i32 @external_call(i32 %sum4)
	ret i32 %result
}

define void @dsp_eh() #0 uwtable personality ptr @__gxx_personality_v0 {
entry:
	call void asm sideeffect "movx.w @r4,x0 nopy", "~{x0},~{r4},~{memory}"()
	invoke void @may_throw()
		to label %done unwind label %landing

done:
	ret void

landing:
	%exception = landingpad { ptr, i32 }
		cleanup
	call void asm sideeffect "setdmx", "~{sr}"()
	resume { ptr, i32 } %exception
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="+dsp,+sh4al-dsp" "tune-cpu"="sh4al-dsp" "use-soft-float"="true" }

; ASM-LABEL: dsp_pic_tls:
; ASM-DAG: .cfi_offset pr,
; ASM-DAG: .cfi_offset r12,
; ASM-DAG: padd	x0,y0,a0
; ASM-DAG: .long	_GLOBAL_OFFSET_TABLE_
; ASM-DAG: .long	external_global@GOT
; ASM-DAG: .long	tls_gd@TLSGD
; ASM-DAG: .long	tls_ld@TLSLDM
; ASM-DAG: .long	tls_ld@DTPOFF
; ASM-DAG: .long	tls_ie@GOTTPOFF
; ASM-DAG: .long	tls_le@TPOFF
; ASM-DAG: .long	external_call@PLT
; ASM-LABEL: dsp_eh:
; ASM-DAG: movx.w	@r4,x0
; ASM-DAG: .long	may_throw@PLT
; ASM-DAG: setdmx
; ASM: .section	.gcc_except_table

; OBJECT-COUNT-2: Flags [ (0x6)
; OBJECT-DAG: Name: .eh_frame
; OBJECT-DAG: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; OBJECT-DAG: R_SH_GOT32 external_global
; OBJECT-DAG: R_SH_TLS_GD_32 tls_gd
; OBJECT-DAG: R_SH_TLS_LD_32 tls_ld
; OBJECT-DAG: R_SH_TLS_LDO_32 tls_ld
; OBJECT-DAG: R_SH_TLS_IE_32 tls_ie
; OBJECT-DAG: R_SH_TLS_LE_32 tls_le
; OBJECT-DAG: R_SH_PLT32 external_call
; OBJECT-DAG: R_SH_PLT32 may_throw
; OBJECT-DAG: R_SH_REL32
; VERIFY: No errors.
; FRAME: DW_CFA_offset: PR
; FRAME: DW_CFA_offset: R12
