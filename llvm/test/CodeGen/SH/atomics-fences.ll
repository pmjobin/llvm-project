; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.pic.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs -filetype=obj < %s -o %t.pic.le.o
; RUN: llvm-readobj --relocations %t.pic.be.o | FileCheck %s --check-prefix=PIC
; RUN: llvm-readobj --relocations %t.pic.le.o | FileCheck %s --check-prefix=PIC

; ASM-LABEL: system_acquire:
; ASM: sts.l	pr,@-r15
; ASM: mov.l	{{.*}},r0
; ASM: jsr	@r0
; ASM-NEXT: nop
; ASM: lds.l	@r15+,pr
; ASM: .long	__sync_synchronize
define void @system_acquire() {
  fence acquire
  ret void
}

; ASM-LABEL: system_release:
; ASM: jsr	@r0
; ASM-NEXT: nop
; ASM: .long	__sync_synchronize
define void @system_release() {
  fence release
  ret void
}

; ASM-LABEL: system_acq_rel:
; ASM: jsr	@r0
; ASM-NEXT: nop
; ASM: .long	__sync_synchronize
define void @system_acq_rel() {
  fence acq_rel
  ret void
}

; ASM-LABEL: system_seq_cst:
; ASM: jsr	@r0
; ASM-NEXT: nop
; ASM: .long	__sync_synchronize
define void @system_seq_cst() {
  fence seq_cst
  ret void
}

; ASM-LABEL: singlethread_all:
; ASM-NOT: sts.l
; ASM-NOT: jsr
; ASM-NOT: __sync_synchronize
; ASM: rts
; ASM-NEXT: nop
define void @singlethread_all(ptr %p) {
  store i32 1, ptr %p, align 4
  fence syncscope("singlethread") acquire
  store i32 2, ptr %p, align 4
  fence syncscope("singlethread") release
  store i32 3, ptr %p, align 4
  fence syncscope("singlethread") acq_rel
  store i32 4, ptr %p, align 4
  fence syncscope("singlethread") seq_cst
  ret void
}

; ASM-LABEL: seq_atomic_only:
; ASM: .long	__atomic_load_4
; ASM-NOT: __sync_synchronize
define i32 @seq_atomic_only(ptr %p) {
  %value = load atomic i32, ptr %p seq_cst, align 4
  ret i32 %value
}

; ASM-LABEL: fence_then_atomic:
; ASM: .long	__sync_synchronize
; ASM: .long	__atomic_load_4
define i32 @fence_then_atomic(ptr %p) {
  fence seq_cst
  %value = load atomic i32, ptr %p seq_cst, align 4
  ret i32 %value
}

; ASM-LABEL: mixed:
; ASM-COUNT-4: jsr	@{{r[0-9]+}}
; ASM: .long	__sync_synchronize
; ASM: .long	__atomic_store_4
; ASM: .long	__atomic_load_4
define i32 @mixed(ptr %ordinary, ptr %atomic) {
  store i32 1, ptr %ordinary, align 4
  fence release
  store atomic i32 2, ptr %atomic release, align 4
  %value = load atomic i32, ptr %atomic acquire, align 4
  fence acquire
  %result = load i32, ptr %ordinary, align 4
  %sum = add i32 %value, %result
  ret i32 %sum
}

; PIC: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; PIC-DAG: R_SH_PLT32 __sync_synchronize
; PIC-DAG: R_SH_PLT32 __atomic_load_4
; PIC-DAG: R_SH_PLT32 __atomic_store_4
; PIC-NOT: R_SH_DIR32
