; RUN: split-file %s %t
; RUN: not llvm-as %t/invalid-operand-type.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=OPERAND-TYPE
; RUN: not llc -mtriple=sh-unknown-elf %t/address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE
; RUN: not llc -mtriple=sh-unknown-elf %t/non-tls-global.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NON-TLS
; RUN: not llc -mtriple=sh-unknown-elf -emulated-tls %t/valid.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=EMULATED
; RUN: not llc -mtriple=sh-unknown-elf -enable-tlsdesc %t/valid.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TLSDESC
; RUN: not llc -mtriple=sh-unknown-freebsd -relocation-model=pic %t/valid.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TRIPLE

; OPERAND-TYPE: intrinsic argument 0 type (matching overload type 0) expected ptr, but got i32
; ADDRESS-SPACE: LLVM ERROR: SH nonzero address spaces are not supported
; NON-TLS: llvm.threadlocal.address operand isThreadLocal() must be true
; EMULATED: LLVM ERROR: SH emulated TLS is not supported
; TLSDESC: LLVM ERROR: SH TLSDESC is not supported
; TRIPLE: LLVM ERROR: SH native ELF TLS supports only unknown-elf and linux-gnu triples

;--- invalid-operand-type.ll
declare ptr @llvm.threadlocal.address.p0(i32)

define ptr @invalid_operand_type(i32 %value) {
	%address = call ptr @llvm.threadlocal.address.p0(i32 %value)
	ret ptr %address
}

;--- address-space.ll
@tls = thread_local addrspace(1) global i32 0

declare ptr addrspace(1) @llvm.threadlocal.address.p1(ptr addrspace(1))

define ptr addrspace(1) @invalid_address_space() {
	%address = call ptr addrspace(1) @llvm.threadlocal.address.p1(ptr addrspace(1) @tls)
	ret ptr addrspace(1) %address
}

;--- non-tls-global.ll
@ordinary = global i32 0

declare ptr @llvm.threadlocal.address.p0(ptr)

define ptr @non_tls_global() {
	%address = call ptr @llvm.threadlocal.address.p0(ptr @ordinary)
	ret ptr %address
}

;--- valid.ll
@tls = thread_local(localexec) global i32 0

declare ptr @llvm.threadlocal.address.p0(ptr)

define ptr @valid_intrinsic() {
	%address = call ptr @llvm.threadlocal.address.p0(ptr @tls)
	ret ptr %address
}
