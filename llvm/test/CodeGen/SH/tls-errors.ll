; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-elf -emulated-tls %t/valid.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=EMULATED
; RUN: not llc -mtriple=sh-unknown-elf -enable-tlsdesc %t/valid.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TLSDESC
; RUN: not llc -mtriple=sh-unknown-freebsd -relocation-model=pic %t/valid.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TRIPLE
; RUN: llc -mtriple=sh-unknown-elf -filetype=obj %t/weak.ll -o %t/weak.o
; RUN: llc -mtriple=sh-unknown-elf -filetype=obj %t/weak-declaration.ll -o %t/weak-declaration.o
; RUN: llc -mtriple=sh-unknown-elf -filetype=obj %t/linkonce.ll -o %t/linkonce.o
; RUN: llc -mtriple=sh-unknown-elf -filetype=obj %t/common.ll -o %t/common.o
; RUN: llc -mtriple=sh-unknown-elf -filetype=obj %t/comdat.ll -o %t/comdat.o
; RUN: llvm-readobj --sections --symbols %t/comdat.o | FileCheck %s --check-prefix=COMDAT
; RUN: not llc -mtriple=sh-unknown-elf %t/address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE
; RUN: not llc -mtriple=sh-unknown-elf %t/vector.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VECTOR
; RUN: not llc -mtriple=sh-unknown-elf %t/tls-pointer.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TLS-POINTER
; RUN: llc -mtriple=sh-unknown-elf -filetype=obj %t/inttoptr.ll -o %t/inttoptr.o
; RUN: llc -mtriple=sh-unknown-elf -filetype=obj %t/ptrtoint.ll -o %t/ptrtoint.o
; RUN: llc -mtriple=sh-unknown-elf -filetype=obj %t/blockaddress.ll -o %t/blockaddress.o
; RUN: not llc -mtriple=sh-unknown-elf %t/dso-local-equivalent.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=DSO-LOCAL-EQUIVALENT
; RUN: llc -mtriple=sh-unknown-elf -filetype=obj %t/alias.ll -o %t/alias.o
; RUN: llvm-readobj --sections --symbols --relocations %t/weak.o %t/linkonce.o %t/common.o %t/alias.o | FileCheck %s --check-prefix=SUPPORTED

; EMULATED: LLVM ERROR: SH emulated TLS is not supported
; TLSDESC: LLVM ERROR: SH TLSDESC is not supported
; TRIPLE: LLVM ERROR: SH native ELF TLS supports only unknown-elf and linux-gnu triples
; COMDAT: Name: .tbss.tls
; COMDAT: Type: SHT_NOBITS
; COMDAT: SHF_GROUP
; COMDAT: SHF_TLS
; COMDAT: Name: tls
; COMDAT: Type: TLS
; ADDRESS-SPACE: LLVM ERROR: SH nonzero address spaces are not supported
; VECTOR: LLVM ERROR: SH TLS globals only support i1, i8, i16, i32, i64, float, double, pointers, native float2/float4 vectors, fixed arrays, and fixed structures
; TLS-POINTER: LLVM ERROR: SH global initializer cannot reference a TLS symbol
; DSO-LOCAL-EQUIVALENT: LLVM ERROR: SH dso_local_equivalent initializer is not supported
; SUPPORTED: Name: tls
; SUPPORTED: Type: TLS
; SUPPORTED: Name: tls
; SUPPORTED: Binding: Weak
; SUPPORTED: Name: tls
; SUPPORTED: Type: TLS
; SUPPORTED: Name: alias

;--- valid.ll
@tls = thread_local(localexec) global i32 0

;--- weak.ll
@tls = weak thread_local global i32 0

;--- weak-declaration.ll
@tls = extern_weak thread_local global i32

;--- linkonce.ll
@tls = linkonce thread_local global i32 0

;--- common.ll
@tls = common thread_local global i32 0

;--- comdat.ll
$group = comdat any
@tls = thread_local global i32 0, comdat($group)

;--- address-space.ll
@tls = thread_local addrspace(1) global i32 0

;--- vector.ll
@tls = thread_local global <4 x i32> zeroinitializer

;--- tls-pointer.ll
@referenced = thread_local global i32 0
@pointer = thread_local global ptr @referenced

;--- inttoptr.ll
@pointer = thread_local global ptr inttoptr (i32 305419896 to ptr)

;--- ptrtoint.ll
@ordinary = global i32 0
@integer = thread_local global i32 ptrtoint (ptr @ordinary to i32)

;--- blockaddress.ll
@pointer = thread_local global ptr blockaddress(@owner, %target)

define void @owner() {
entry:
	br label %target

target:
	ret void
}

;--- dso-local-equivalent.ll
@pointer = thread_local global ptr dso_local_equivalent @function

declare void @function()

;--- alias.ll
@ordinary = global i32 0
@alias = alias i32, ptr @ordinary
@pointer = thread_local global ptr @alias
