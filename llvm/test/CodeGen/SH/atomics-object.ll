; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --file-headers --symbols --relocations --sections %t.be.o | FileCheck %s --check-prefix=OBJ
; RUN: llvm-readobj --file-headers --symbols --relocations --sections %t.le.o | FileCheck %s --check-prefix=OBJ
; RUN: llvm-readelf -S %t.be.o | FileCheck %s --check-prefix=SECTIONS
; RUN: llvm-readelf -S %t.le.o | FileCheck %s --check-prefix=SECTIONS
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DIS
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DIS

@byte = global i8 0, align 1

declare i32 @llvm.sh.tas.b(ptr)

define i32 @atomic_object(ptr %p) {
  fence seq_cst
  %value = load atomic i32, ptr %p seq_cst, align 4
  ret i32 %value
}

define i32 @tas_object() {
  %value = call i32 @llvm.sh.tas.b(ptr @byte)
  ret i32 %value
}

; OBJ: Format: elf32-sh
; OBJ: Arch: sh
; OBJ: AddressSize: 32bit
; OBJ: Flags [ (0x2)
; OBJ-NEXT: {{^ *}}0x2
; OBJ: R_SH_DIR32
; OBJ: Name: __sync_synchronize
; OBJ: Binding: Global
; OBJ: Section: Undefined
; OBJ: Name: __atomic_load_4
; OBJ: Binding: Global
; OBJ: Section: Undefined
; SECTIONS: Section Headers:
; SECTIONS-NOT: .got
; SECTIONS-NOT: .plt
; SECTIONS-NOT: .tdata
; SECTIONS-NOT: .tbss
; SECTIONS-NOT: .dynamic

; DIS-LABEL: <tas_object>:
; DIS: tas.b	@{{r[0-9]+}}
; DIS-NEXT: movt	r0
