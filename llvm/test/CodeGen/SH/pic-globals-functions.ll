; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -code-model=large -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=LARGE
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -function-sections -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SECTIONS
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj -r %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj -r %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: %python %S/Inputs/check-sh-pic-elf.py llvm-readelf %t.be.o %t.le.o

@hidden_data = hidden global i32 1, align 4
@local_data = internal global i32 2, align 4
@dso_local_data = dso_local global i32 3, align 4
@protected_data = protected global i32 4, align 4
@defined_default = global i32 5, align 4
@external_data = external global i32, align 4

declare i32 @external_function(i32)

define hidden i32 @hidden_function(i32 %x) {
entry:
	ret i32 %x
}

define i32 @defined_function(i32 %x) {
entry:
	ret i32 %x
}

define i32 @pic_leaf(i32 %x) {
entry:
	%result = add i32 %x, 1
	ret i32 %result
}

define i32 @load_hidden() {
entry:
	%value = load i32, ptr @hidden_data, align 4
	ret i32 %value
}

define i32 @load_local() {
entry:
	%value = load i32, ptr @local_data, align 4
	ret i32 %value
}

define i32 @load_dso_local() {
entry:
	%value = load i32, ptr @dso_local_data, align 4
	ret i32 %value
}

define i32 @load_protected() {
entry:
	%value = load i32, ptr @protected_data, align 4
	ret i32 %value
}

define i32 @load_default() {
entry:
	%value = load i32, ptr @defined_default, align 4
	ret i32 %value
}

define i32 @load_external() {
entry:
	%value = load i32, ptr @external_data, align 4
	ret i32 %value
}

define ptr @external_data_plus_twelve() {
entry:
	ret ptr getelementptr (i8, ptr @external_data, i32 12)
}

define ptr @hidden_function_address() {
entry:
	ret ptr @hidden_function
}

define ptr @external_function_address() {
entry:
	ret ptr @external_function
}

define i32 @compare_external_function_pointer(ptr %pointer) {
entry:
	%equal = icmp eq ptr %pointer, @external_function
	br i1 %equal, label %yes, label %no

yes:
	ret i32 1

no:
	ret i32 0
}

define i32 @call_function_pointer(ptr %pointer, i32 %value) {
entry:
	%result = call i32 %pointer(i32 %value)
	ret i32 %result
}

define i32 @direct_calls(i32 %x) {
entry:
	%local = call i32 @hidden_function(i32 %x)
	%defined = call i32 @defined_function(i32 %local)
	%external = call i32 @external_function(i32 %defined)
	ret i32 %external
}

; ASM-LABEL: pic_leaf:
; ASM-NOT: mova
; ASM: rts

; ASM-LABEL: load_hidden:
; ASM-COUNT-1: mova	[[GOTPC:.LCPI[0-9_]+]],r0
; ASM: mov.l	[[GOTPC]],r12
; ASM: add	r0,r12
; ASM: mov.l	[[HIDDEN:.LCPI[0-9_]+]],[[ADDR:r[0-9]+]]
; ASM: add	r12,[[ADDR]]
; ASM: mov.l	@[[ADDR]],r0
; ASM: [[HIDDEN]]:
; ASM-NEXT: .long	hidden_data@GOTOFF

; ASM-LABEL: load_local:
; ASM: .long	local_data@GOTOFF

; ASM-LABEL: load_dso_local:
; ASM: .long	.Ldso_local_data$local@GOTOFF

; ASM-LABEL: load_protected:
; ASM: .long	protected_data@GOTOFF

; ASM-LABEL: load_default:
; ASM: mov.l	{{[^,]+}},r0
; ASM-NEXT: add	r12,r0
; ASM-NEXT: mov.l	@r0,r0
; ASM-NEXT: mov.l	@r0,r0
; ASM: .long	defined_default@GOT

; ASM-LABEL: load_external:
; ASM: mov.l	{{[^,]+}},r0
; ASM-NEXT: add	r12,r0
; ASM-NEXT: mov.l	@r0,r0
; ASM-NEXT: mov.l	@r0,r0
; ASM: .long	external_data@GOT

; ASM-LABEL: external_data_plus_twelve:
; ASM: mov.l	@{{r[0-9]+}},[[ADDR:r[0-9]+]]
; ASM: add	#12,[[ADDR]]
; ASM-NOT: external_data@GOT+

; ASM-LABEL: hidden_function_address:
; ASM: .long	hidden_function@GOTOFF

; ASM-LABEL: external_function_address:
; ASM: mov.l	@{{r[0-9]+}},r0
; ASM: .long	external_function@GOT
; ASM-NOT: external_function@PLT

; ASM-LABEL: compare_external_function_pointer:
; ASM: mov.l	@{{r[0-9]+}},{{r[0-9]+}}
; ASM: .long	external_function@GOT
; ASM-NOT: external_function@PLT

; ASM-LABEL: call_function_pointer:
; ASM-NOT: mova
; ASM: jsr	@{{r[0-9]+}}

; ASM-LABEL: direct_calls:
; ASM: bsr	hidden_function
; ASM: mova	[[DEFINED_PLT:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[DEFINED_PLT]],[[CALLEE:r[1-9][0-9]*]]
; ASM-NEXT: add	r0,[[CALLEE]]
; ASM-NEXT: jsr	@[[CALLEE]]
; ASM-NEXT: nop
; ASM: mova	[[EXTERNAL_PLT:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[EXTERNAL_PLT]],{{r[1-9][0-9]*}}
; ASM: [[DEFINED_PLT]]:
; ASM-NEXT: .long	defined_function@PLT
; ASM: [[EXTERNAL_PLT]]:
; ASM-NEXT: .long	external_function@PLT

; LARGE-LABEL: direct_calls:
; LARGE-NOT: bsr
; LARGE: jsr
; LARGE: .long	hidden_function@GOTOFF
; LARGE: .long	defined_function@PLT
; LARGE: .long	external_function@PLT

; SECTIONS-LABEL: direct_calls:
; SECTIONS-NOT: bsr
; SECTIONS: jsr
; SECTIONS: .long	hidden_function@GOTOFF
; SECTIONS: .long	defined_function@PLT
; SECTIONS: .long	external_function@PLT

; OBJECT: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; OBJECT: R_SH_GOTOFF hidden_data
; OBJECT: R_SH_GOTOFF local_data
; OBJECT: R_SH_GOTOFF .Ldso_local_data$local
; OBJECT: R_SH_GOTOFF protected_data
; OBJECT: R_SH_GOT32 defined_default
; OBJECT: R_SH_GOT32 external_data
; OBJECT: R_SH_GOTOFF hidden_function
; OBJECT: R_SH_GOT32 external_function
; OBJECT: R_SH_PLT32 defined_function
; OBJECT: R_SH_PLT32 external_function
; OBJECT-NOT: R_SH_DIR32
