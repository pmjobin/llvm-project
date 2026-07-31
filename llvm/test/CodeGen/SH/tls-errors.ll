; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-elf -emulated-tls %t/valid.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=EMULATED
; RUN: not llc -mtriple=sh-unknown-elf -enable-tlsdesc %t/valid.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TLSDESC
; RUN: not --crash llc -mtriple=sh-unknown-freebsd -relocation-model=pic %t/valid.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TRIPLE
; RUN: not --crash llc -mtriple=sh-unknown-elf %t/weak.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=WEAK
; RUN: not --crash llc -mtriple=sh-unknown-elf %t/weak-declaration.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=WEAK
; RUN: not --crash llc -mtriple=sh-unknown-elf %t/linkonce.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=LINKONCE
; RUN: not --crash llc -mtriple=sh-unknown-elf %t/common.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=COMMON
; RUN: not --crash llc -mtriple=sh-unknown-elf %t/comdat.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=COMDAT
; RUN: not --crash llc -mtriple=sh-unknown-elf %t/address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE
; RUN: not --crash llc -mtriple=sh-unknown-elf %t/vector.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VECTOR

; EMULATED: LLVM ERROR: SH emulated TLS is not supported
; TLSDESC: LLVM ERROR: SH TLSDESC is not supported
; TRIPLE: LLVM ERROR: SH native ELF TLS supports only unknown-elf and linux-gnu triples
; WEAK: LLVM ERROR: SH weak TLS is not supported
; LINKONCE: LLVM ERROR: SH linkonce TLS is not supported
; COMMON: LLVM ERROR: SH TLS common is not supported
; COMDAT: LLVM ERROR: SH COMDAT TLS is not supported
; ADDRESS-SPACE: LLVM ERROR: SH nonzero address spaces are not supported
; VECTOR: LLVM ERROR: SH TLS globals only support i8, i16, i32, i64, float, double, pointers, fixed arrays, and fixed structures

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
