; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=STATIC
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=STATIC

@default_external = external thread_local global i32
@default_internal = internal thread_local global i32 1
@default_hidden = hidden thread_local global i32 2
@default_protected = protected thread_local global i32 3
@default_dso_local = dso_local thread_local global i32 4
@explicit_ld = internal thread_local(localdynamic) global i32 4
@explicit_ie = external thread_local(initialexec) global i32
@explicit_le = internal thread_local(localexec) global i32 5

define ptr @address_default_external() {
	ret ptr @default_external
}

define ptr @address_default_internal() {
	ret ptr @default_internal
}

define ptr @address_default_hidden() {
	ret ptr @default_hidden
}

define ptr @address_default_protected() {
	ret ptr @default_protected
}

define ptr @address_default_dso_local() {
	ret ptr @default_dso_local
}

define ptr @address_explicit_ld() {
	ret ptr @explicit_ld
}

define ptr @address_explicit_ie() {
	ret ptr @explicit_ie
}

define ptr @address_explicit_le() {
	ret ptr @explicit_le
}

; PIC-LABEL: address_default_external:
; PIC: .long	default_external@TLSGD
; PIC-LABEL: address_default_internal:
; PIC: .long	default_internal@TLSLDM
; PIC: .long	default_internal@DTPOFF
; PIC-LABEL: address_default_hidden:
; PIC: .long	default_hidden@TLSLDM
; PIC: .long	default_hidden@DTPOFF
; PIC-LABEL: address_default_protected:
; PIC: .long	default_protected@TLSLDM
; PIC: .long	default_protected@DTPOFF
; PIC-LABEL: address_default_dso_local:
; PIC: .long	default_dso_local@TLSLDM
; PIC: .long	default_dso_local@DTPOFF
; PIC-LABEL: address_explicit_ld:
; PIC: .long	explicit_ld@TLSLDM
; PIC: .long	explicit_ld@DTPOFF
; PIC-LABEL: address_explicit_ie:
; PIC: .long	explicit_ie@GOTTPOFF
; PIC-LABEL: address_explicit_le:
; PIC: .long	explicit_le@TPOFF

; STATIC-LABEL: address_default_external:
; STATIC: .long	default_external@GOTTPOFF
; STATIC-LABEL: address_default_internal:
; STATIC: .long	default_internal@TPOFF
; STATIC-LABEL: address_default_hidden:
; STATIC: .long	default_hidden@TPOFF
; STATIC-LABEL: address_default_protected:
; STATIC: .long	default_protected@TPOFF
; STATIC-LABEL: address_default_dso_local:
; STATIC: .long	default_dso_local@TPOFF
; STATIC-LABEL: address_explicit_ld:
; STATIC: .long	explicit_ld@TPOFF
; STATIC-LABEL: address_explicit_ie:
; STATIC: .long	explicit_ie@GOTTPOFF
; STATIC-LABEL: address_explicit_le:
; STATIC: .long	explicit_le@TPOFF
