; RUN: llc -mtriple=sh-unknown-elf -verify-machineinstrs < %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -verify-machineinstrs < %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -verify-machineinstrs < %s -o - | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=shle-unknown-elf -filetype=obj < %s -o %t.o
; RUN: llvm-readobj --sections --symbols --section-groups --relocations %t.o | FileCheck %s --check-prefix=OBJ

$odr_guard = comdat any
$odr_object = comdat any

@ordered = thread_local(localexec) global i32 0, align 4
@__tls_guard = internal thread_local(localexec) global i8 0, align 1
@local_guard = internal thread_local(localexec) global i1 false, align 1
@odr_guard = linkonce_odr thread_local(localexec) global i64 0, comdat, align 4
@odr_object = weak_odr thread_local(localexec) global i32 0, comdat, align 4
@pic_guard = internal thread_local global i8 0, align 1
@__dso_handle = external hidden global i8

declare ptr @llvm.invariant.start.p0(i64 immarg, ptr captures(none))
declare void @llvm.invariant.end.p0(ptr, i64 immarg, ptr captures(none))
declare i32 @runtime_value()
declare i32 @__cxa_thread_atexit(ptr, ptr, ptr)
declare void @destroy(ptr)

define void @_ZTH7ordered() {
entry:
	%guard = load i8, ptr @__tls_guard, align 1
	%uninitialized = icmp eq i8 %guard, 0
	br i1 %uninitialized, label %init, label %exit

init:
	store i8 1, ptr @__tls_guard, align 1
	%marker = call ptr @llvm.invariant.start.p0(i64 1, ptr @__tls_guard)
	%value = call i32 @runtime_value()
	store i32 %value, ptr @ordered, align 4
	%registered = call i32 @__cxa_thread_atexit(ptr @destroy, ptr @ordered, ptr @__dso_handle)
	call void @llvm.invariant.end.p0(ptr %marker, i64 1, ptr @__tls_guard)
	br label %exit

exit:
	ret void
}

define ptr @local() {
entry:
	%initialized = load i1, ptr @local_guard, align 1
	br i1 %initialized, label %exit, label %init

init:
	store i1 true, ptr @local_guard, align 1
	br label %exit

exit:
	ret ptr @odr_object
}

define ptr @pic_access() {
entry:
	%guard = load i8, ptr @pic_guard, align 1
	ret ptr @pic_guard
}

; ASM-LABEL: _ZTH7ordered:
; ASM: .long __tls_guard@TPOFF
; ASM: .long ordered@TPOFF
; ASM: .long __cxa_thread_atexit
; ASM-NOT: llvm.invariant.start
; ASM-LABEL: local:
; ASM: .long local_guard@TPOFF

; PIC-LABEL: pic_access:
; PIC: .long pic_guard@TLSLDM
; PIC: .long __tls_get_addr@PLT

; PIC: .long pic_guard@DTPOFF

; OBJ-DAG: Name: .tbss.odr_guard
; OBJ-DAG: SHF_GROUP
; OBJ-DAG: SHF_TLS
; OBJ-DAG: Name: .tbss.odr_object
; OBJ-DAG: Name: odr_guard
; OBJ-DAG: Type: TLS
; OBJ-DAG: Binding: Weak
; OBJ-DAG: Name: odr_object
