; RUN: llc --version | FileCheck %s --check-prefix=TARGETS
; RUN: llvm-config --components | FileCheck %s --check-prefix=COMPONENTS
; RUN: llc -mtriple=sh-unknown-elf -O2 -verify-machineinstrs -filetype=null < %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=LE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=not-a-cpu -O2 -verify-machineinstrs -filetype=null < %s 2>&1 | FileCheck %s --check-prefix=CPU
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -relocation-model=pic < %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=PIC
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -relocation-model=ropi < %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=PIC
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -relocation-model=rwpi < %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=PIC
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -relocation-model=ropi-rwpi < %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=PIC
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -code-model=medium < %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=CODEMODEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -code-model=large -filetype=null < %s

define i32 @zero() {
	ret i32 0
}

; TARGETS: Registered Targets:
; TARGETS: sh     - SuperH (32-bit big endian)
; TARGETS-NEXT: shle   - SuperH (32-bit little endian)

; COMPONENTS: sh
; COMPONENTS-SAME: shasmparser
; COMPONENTS-SAME: shcodegen
; COMPONENTS-SAME: shdesc
; COMPONENTS-SAME: shdisassembler
; COMPONENTS-SAME: shinfo

; BE: target datalayout = "E-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
; LE: target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"

; CPU: 'not-a-cpu' is not a recognized processor for this target
; PIC: LLVM ERROR: SH only supports static relocation
; CODEMODEL: LLVM ERROR: SH only supports the small and large code models
