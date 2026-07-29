; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/tls.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TLS
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/alias.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ALIAS
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/ifunc.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=IFUNC
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/comdat.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=COMDAT
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/weak.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=WEAK
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/extern-init.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=EXTERN-INIT
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/pointer-select.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SELECT
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/pointer-compare.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MATERIALIZED
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/pointer-ordered.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ORDERED
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/inttoptr-i64.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTTOPTR

; TLS: LLVM ERROR: SH thread-local storage is not supported
; ALIAS: LLVM ERROR: SH global aliases are not supported
; IFUNC: LLVM ERROR: SH indirect functions are not supported
; COMDAT: LLVM ERROR: SH COMDAT is not supported
; WEAK: LLVM ERROR: SH weak, linkonce, common, and appending linkage are not supported
; ADDRESS-SPACE: LLVM ERROR: SH nonzero address spaces are not supported
; EXTERN-INIT: LLVM ERROR: SH externally initialized globals are not supported
; SELECT: LLVM ERROR: SH select is not supported
; MATERIALIZED: LLVM ERROR: SH comparison results may only be used by conditional branches
; ORDERED: LLVM ERROR: SH only supports pointer equality and inequality comparisons
; INTTOPTR: LLVM ERROR: SH inttoptr requires an i32 source and an address-space-zero result

;--- tls.ll
@tls = thread_local global i32 0, align 4
define ptr @tls_address() {
	ret ptr @tls
}

;--- alias.ll
@object = global i32 0, align 4
@alias = alias i32, ptr @object

;--- ifunc.ll
define internal ptr @resolver() {
	ret ptr null
}
@indirect = ifunc void (), ptr @resolver

;--- comdat.ll
$group = comdat any
@object = global i32 0, comdat($group), align 4

;--- weak.ll
@object = weak global i32 0, align 4

;--- address-space.ll
@object = addrspace(1) global i32 0, align 4

;--- extern-init.ll
@object = externally_initialized global i32 0, align 4

;--- pointer-select.ll
define ptr @pointer_select(ptr %a, ptr %b) {
	%result = select i1 true, ptr %a, ptr %b
	ret ptr %result
}

;--- pointer-compare.ll
define i32 @materialized_pointer_compare(ptr %a, ptr %b) {
	%condition = icmp eq ptr %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

;--- pointer-ordered.ll
define i32 @ordered_pointer_compare(ptr %a, ptr %b) {
	%condition = icmp ult ptr %a, %b
	br i1 %condition, label %yes, label %no
yes:
	ret i32 1
no:
	ret i32 0
}

;--- inttoptr-i64.ll
define ptr @inttoptr_i64(i64 %value) {
	%address = inttoptr i64 %value to ptr
	ret ptr %address
}
