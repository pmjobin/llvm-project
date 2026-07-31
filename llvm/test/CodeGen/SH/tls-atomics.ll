; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -stop-after=atomic-expand < %s | FileCheck %s --check-prefix=EXPAND
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM

@tls_gd = external thread_local global i32, align 4
@tls_ld = internal thread_local(localdynamic) global i32 0, align 4
@tls_ie = external thread_local(initialexec) global i32, align 4
@tls_le = internal thread_local(localexec) global i32 0, align 4

define i32 @atomic_load_gd() {
	%value = load atomic i32, ptr @tls_gd acquire, align 4
	ret i32 %value
}

define void @atomic_store_ld(i32 %value) {
	store atomic i32 %value, ptr @tls_ld release, align 4
	ret void
}

define i32 @atomic_cmpxchg_ie(i32 %expected, i32 %desired) {
	%pair = cmpxchg ptr @tls_ie, i32 %expected, i32 %desired acq_rel acquire, align 4
	%old = extractvalue { i32, i1 } %pair, 0
	ret i32 %old
}

define i32 @atomic_fetch_add_le(i32 %value) {
	%old = atomicrmw add ptr @tls_le, i32 %value seq_cst, align 4
	ret i32 %old
}

; EXPAND-LABEL: define i32 @atomic_load_gd
; EXPAND: call i32 @__atomic_load_4(ptr @tls_gd, i32 2)
; EXPAND-LABEL: define void @atomic_store_ld
; EXPAND: call void @__atomic_store_4(ptr @tls_ld, i32 %value, i32 3)
; EXPAND-LABEL: define i32 @atomic_cmpxchg_ie
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4(ptr @tls_ie,
; EXPAND-LABEL: define i32 @atomic_fetch_add_le
; EXPAND: call i32 @__atomic_fetch_add_4(ptr @tls_le, i32 %value, i32 5)

; ASM-LABEL: atomic_load_gd:
; ASM: .long	tls_gd@TLSGD
; ASM: .long	__atomic_load_4@PLT
; ASM-LABEL: atomic_store_ld:
; ASM: .long	tls_ld@TLSLDM
; ASM: .long	tls_ld@DTPOFF
; ASM: .long	__atomic_store_4@PLT
; ASM-LABEL: atomic_cmpxchg_ie:
; ASM: .long	tls_ie@GOTTPOFF
; ASM: .long	__atomic_compare_exchange_4@PLT
; ASM-LABEL: atomic_fetch_add_le:
; ASM: .long	tls_le@TPOFF
; ASM: .long	__atomic_fetch_add_4@PLT
