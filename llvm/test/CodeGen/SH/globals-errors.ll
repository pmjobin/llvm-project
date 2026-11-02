; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -filetype=obj %t/object-alias.ll -o %t/object-alias.o
; RUN: not llc -mtriple=sh-unknown-elf -relocation-model=static %t/ifunc.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=IFUNC
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static %t/comdat-exact.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=COMDAT
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -filetype=obj %t/common.ll -o %t/common.o
; RUN: not llc -mtriple=sh-unknown-elf -relocation-model=static %t/appending.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=APPENDING
; RUN: not llc -mtriple=sh-unknown-elf -relocation-model=static %t/address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -filetype=obj %t/extern-init.ll -o %t/extern-init.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -filetype=obj %t/comdat-nodeduplicate.ll -o %t/comdat-nodeduplicate.o
; RUN: llvm-readobj --sections --symbols %t/object-alias.o %t/common.o %t/extern-init.o %t/comdat-nodeduplicate.o | FileCheck %s --check-prefix=OBJECT
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static %t/pointer-select.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static %t/pointer-compare.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static %t/pointer-ordered.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -verify-machineinstrs %t/inttoptr-i64.ll -o - | FileCheck %s --check-prefix=INTTOPTR
; RUN: not llc -mtriple=sh-unknown-elf -relocation-model=pic %t/ifunc.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=IFUNC
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=pic %t/comdat-exact.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=COMDAT
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -filetype=obj %t/common.ll -o %t/common-pic.o

; IFUNC: LLVM ERROR: SH indirect functions are not supported
; COMDAT: LLVM ERROR: ELF COMDATs only support SelectionKind::Any and SelectionKind::NoDeduplicate
; APPENDING: LLVM ERROR: SH appending linkage is not supported for this global
; ADDRESS-SPACE: LLVM ERROR: SH nonzero address spaces are not supported
; INTTOPTR-LABEL: inttoptr_i64:
; INTTOPTR: mov	r5,r0
; OBJECT: Name: alias
; OBJECT: Name: object
; OBJECT: Binding: Global
; OBJECT: Section: Common
; OBJECT: Name: .data.object
; OBJECT: SHF_GROUP

;--- object-alias.ll
@object = global i32 0, align 4
@alias = alias i32, ptr @object

;--- ifunc.ll
define internal ptr @resolver() {
	ret ptr null
}
@indirect = ifunc void (), ptr @resolver

;--- comdat-exact.ll
$group = comdat exactmatch
@object = global i32 0, comdat($group), align 4

;--- comdat-nodeduplicate.ll
$group = comdat nodeduplicate
@object = global i32 1, comdat($group), align 4

;--- common.ll
@object = common global i32 0, align 4

;--- appending.ll
@object = appending global [1 x i32] [i32 0], align 4

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
