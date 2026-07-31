; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t-be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t-le.o
; RUN: llvm-readobj --file-headers --sections --symbols --relocations %t-be.o | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llvm-readobj --file-headers --sections --symbols --relocations %t-le.o | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llvm-readelf -h -s -r %t-be.o | FileCheck %s --check-prefixes=ELF,ELF-BE
; RUN: llvm-readelf -h -s -r %t-le.o | FileCheck %s --check-prefixes=ELF,ELF-LE
; RUN: llvm-objdump -d %t-be.o | FileCheck %s --check-prefix=DIS-BE
; RUN: llvm-objdump -d %t-le.o | FileCheck %s --check-prefix=DIS-LE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj %s -o %t-pic-be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj %s -o %t-pic-le.o
; RUN: llvm-readobj --relocations %t-pic-be.o | FileCheck %s --check-prefix=PIC
; RUN: llvm-readobj --relocations %t-pic-le.o | FileCheck %s --check-prefix=PIC

%S8 = type { i32, i32 }
%S12 = type { i32, i32, i32 }

declare %S8 @external_direct(%S8)
declare void @external_sret(ptr sret(%S12) align 4, i32)
declare void @external_byval(ptr byval(%S12) align 4)
declare void @llvm.memcpy.p0.p0.i32(ptr, ptr, i32, i1 immarg)

define %S8 @call_external_direct(%S8 %value) {
	%result = call %S8 @external_direct(%S8 %value)
	ret %S8 %result
}

define void @call_external_sret(ptr %result, i32 %value) {
	call void @external_sret(ptr sret(%S12) align 4 %result, i32 %value)
	ret void
}

define void @call_external_byval(ptr %source) {
	call void @external_byval(ptr byval(%S12) align 4 %source)
	ret void
}

define void @inline_copy(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i32(ptr align 4 %dst, ptr align 4 %src, i32 12, i1 false)
	ret void
}

define void @helper_copy(ptr %dst, ptr %src) {
	call void @llvm.memcpy.p0.p0.i32(ptr align 4 %dst, ptr align 4 %src, i32 61, i1 false)
	ret void
}

; BE: Format: elf32-sh
; BE-NEXT: Arch: sh
; LE: Format: elf32-shl
; LE-NEXT: Arch: shle
; COMMON: AddressSize: 32bit
; COMMON: Class: 32-bit
; BE: DataEncoding: BigEndian
; LE: DataEncoding: LittleEndian
; COMMON: Machine: EM_SH
; COMMON: Flags [ (0x2)
; COMMON: 0x2
; COMMON: Name: .text
; COMMON-NOT: Name: .got
; COMMON-NOT: Name: .plt
; COMMON-NOT: Name: .dynamic
; COMMON: R_SH_DIR32 external_direct
; COMMON: R_SH_DIR32 external_sret
; COMMON: R_SH_DIR32 external_byval
; COMMON: R_SH_DIR32 memcpy
; COMMON: Name: external_direct
; COMMON: Section: Undefined
; COMMON: Name: external_sret
; COMMON: Section: Undefined
; COMMON: Name: external_byval
; COMMON: Section: Undefined
; COMMON: Name: memcpy
; COMMON: Section: Undefined

; ELF: Class:                             ELF32
; ELF-BE: Data:                              2's complement, big endian
; ELF-LE: Data:                              2's complement, little endian
; ELF: Machine:                           Hitachi SH
; ELF: Flags:                             0x2
; ELF: UND external_direct
; ELF: UND external_sret
; ELF: UND external_byval
; ELF: UND memcpy

; DIS-BE-COUNT-4: jsr	@r0
; DIS-LE-COUNT-4: jsr	@r0

; PIC-DAG: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; PIC-DAG: R_SH_PLT32 external_direct
; PIC-DAG: R_SH_PLT32 external_sret
; PIC-DAG: R_SH_PLT32 external_byval
; PIC-DAG: R_SH_PLT32 memcpy
; PIC-NOT: R_SH_DIR32
