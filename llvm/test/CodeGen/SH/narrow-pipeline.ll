; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-before=prolog-epilog < %s | FileCheck %s --check-prefix=PREPEI
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=prolog-epilog < %s | FileCheck %s --check-prefix=PEI
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-delay-slot-filler < %s | FileCheck %s --check-prefix=FINAL

define i32 @narrow_pipeline(i32 %byte, i32 %word) {
	%byte_slot = alloca i8, align 1
	%word_slot = alloca i16, align 2
	%byte_value = trunc i32 %byte to i8
	%word_value = trunc i32 %word to i16
	store volatile i8 %byte_value, ptr %byte_slot, align 1
	store volatile i16 %word_value, ptr %word_slot, align 2
	%loaded_byte = load volatile i8, ptr %byte_slot, align 1
	%loaded_word = load volatile i16, ptr %word_slot, align 2
	%extended_byte = zext i8 %loaded_byte to i32
	%extended_word = zext i16 %loaded_word to i32
	%result = add i32 %extended_byte, %extended_word
	ret i32 %result
}

; ISEL-LABEL: name:            narrow_pipeline
; ISEL: registers:
; ISEL-DAG: class: gpr
; ISEL-DAG: class: gpr
; ISEL: stack:
; ISEL-DAG: size: 1, alignment: 1
; ISEL-DAG: size: 2, alignment: 2
; ISEL-LABEL: body:
; ISEL: MOVB_store_frame {{.*}}, %stack.{{[0-9]+}}.byte_slot, 0, implicit-def dead early-clobber $r0 :: (volatile store (s8)
; ISEL: MOVW_store_frame {{.*}}, %stack.{{[0-9]+}}.word_slot, 0, implicit-def dead early-clobber $r0 :: (volatile store (s16)
; ISEL: {{.*}}:gpr = MOVB_load_frame %stack.{{[0-9]+}}.byte_slot, 0, implicit-def dead $r0 :: (volatile dereferenceable load (s8)
; ISEL: {{.*}}:gpr = EXTUB
; ISEL: {{.*}}:gpr = MOVW_load_frame %stack.{{[0-9]+}}.word_slot, 0, implicit-def dead $r0 :: (volatile dereferenceable load (s16)
; ISEL: {{.*}}:gpr = EXTUW

; PREPEI-LABEL: name:            narrow_pipeline
; PREPEI: noVRegs:         true
; PREPEI-LABEL: body:
; PREPEI: MOVB_store_frame {{.*}}, %stack.{{[0-9]+}}.byte_slot, 0, implicit-def dead early-clobber $r0 :: (volatile store (s8)
; PREPEI: MOVW_store_frame {{.*}}, %stack.{{[0-9]+}}.word_slot, 0, implicit-def dead early-clobber $r0 :: (volatile store (s16)
; PREPEI: MOVB_load_frame %stack.{{[0-9]+}}.byte_slot, 0, implicit-def dead $r0 :: (volatile dereferenceable load (s8)
; PREPEI: MOVW_load_frame %stack.{{[0-9]+}}.word_slot, 0, implicit-def dead $r0 :: (volatile dereferenceable load (s16)

; PEI-LABEL: name:            narrow_pipeline
; PEI: noVRegs:         true
; PEI-LABEL: body:
; PEI-NOT: %stack.
; PEI: MOVB_store_frame {{.*}}, $r15, {{[0-9]+}}, implicit-def dead early-clobber $r0 :: (volatile store (s8)
; PEI: MOVW_store_frame {{.*}}, $r15, {{[0-9]+}}, implicit-def dead early-clobber $r0 :: (volatile store (s16)
; PEI: MOVB_load_frame $r15, {{[0-9]+}}, implicit-def dead $r0 :: (volatile dereferenceable load (s8)
; PEI: MOVW_load_frame $r15, {{[0-9]+}}, implicit-def dead $r0 :: (volatile dereferenceable load (s16)

; FINAL-LABEL: name:            narrow_pipeline
; FINAL: noVRegs:         true
; FINAL-LABEL: body:
; FINAL-NOT: %stack.
; FINAL-NOT: MOVB_{{.*}}_frame
; FINAL-NOT: MOVW_{{.*}}_frame
; FINAL: MOVB_store_disp killed $r0, $r15, {{[0-9]+}} :: (volatile store (s8)
; FINAL: MOVW_store_{{disp|reg}} {{.*}} :: (volatile store (s16)
; FINAL: $r0 = MOVB_load_disp $r15, {{[0-9]+}} :: (volatile dereferenceable load (s8)
; FINAL: MOVW_load_{{disp|reg}} {{.*}} :: (volatile dereferenceable load (s16)
