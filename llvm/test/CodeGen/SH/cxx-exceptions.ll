; RUN: llc -mtriple=sh-unknown-linux-gnu -exception-model=dwarf -relocation-model=static -verify-machineinstrs < %s | FileCheck %s --check-prefix=STATIC
; RUN: llc -mtriple=shle-unknown-linux-gnu -exception-model=dwarf -relocation-model=static -verify-machineinstrs < %s | FileCheck %s --check-prefix=STATIC
; RUN: llc -mtriple=sh-unknown-linux-gnu -exception-model=dwarf -relocation-model=pic -verify-machineinstrs < %s | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=shle-unknown-linux-gnu -exception-model=dwarf -relocation-model=pic -verify-machineinstrs < %s | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=sh-unknown-linux-gnu -exception-model=dwarf -relocation-model=static -filetype=obj < %s -o %t.static.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -exception-model=dwarf -relocation-model=static -filetype=obj < %s -o %t.static.le.o
; RUN: llvm-readobj --sections --relocations %t.static.be.o | FileCheck %s --check-prefix=STATIC-OBJ
; RUN: llvm-readobj --sections --relocations %t.static.le.o | FileCheck %s --check-prefix=STATIC-OBJ
; RUN: llvm-dwarfdump --eh-frame %t.static.be.o | FileCheck %s --check-prefix=STATIC-FRAME
; RUN: llvm-dwarfdump --eh-frame %t.static.le.o | FileCheck %s --check-prefix=STATIC-FRAME
; RUN: llc -mtriple=sh-unknown-linux-gnu -exception-model=dwarf -relocation-model=pic -filetype=obj < %s -o %t.pic.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -exception-model=dwarf -relocation-model=pic -filetype=obj < %s -o %t.pic.le.o
; RUN: llvm-readobj --sections --relocations %t.pic.be.o | FileCheck %s --check-prefix=PIC-OBJ
; RUN: llvm-readobj --sections --relocations %t.pic.le.o | FileCheck %s --check-prefix=PIC-OBJ
; RUN: llvm-dwarfdump --eh-frame %t.pic.be.o | FileCheck %s --check-prefix=PIC-FRAME
; RUN: llvm-dwarfdump --eh-frame %t.pic.le.o | FileCheck %s --check-prefix=PIC-FRAME
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh3 -exception-model=dwarf -relocation-model=static -verify-machineinstrs -filetype=obj < %s -o %t.sh3.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh3 -exception-model=dwarf -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.sh3.le.o
; RUN: llvm-dwarfdump --verify %t.sh3.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --verify %t.sh3.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -exception-model=dwarf -relocation-model=static -verify-machineinstrs -filetype=obj < %s -o %t.sh4.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -exception-model=dwarf -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.sh4.le.o
; RUN: llvm-dwarfdump --verify %t.sh4.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --verify %t.sh4.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY

; VERIFY-NOT: error:
; VERIFY-NOT: warning:
; VERIFY: No errors.

@_ZTIi = external constant ptr

declare void @may_throw()
declare void @cleanup(ptr) nounwind
declare ptr @__cxa_begin_catch(ptr) nounwind
declare void @__cxa_end_catch() nounwind
declare i32 @__gxx_personality_v0(...)
declare i32 @llvm.eh.typeid.for.p0(ptr)

define i32 @catch_and_cleanup(ptr %state) personality ptr @__gxx_personality_v0 {
; STATIC-LABEL: catch_and_cleanup:
; STATIC: .cfi_personality 0, __gxx_personality_v0
; STATIC-NEXT: .cfi_lsda 11, .Lexception0
; STATIC: mov	r4,{{r[0-9]+}}
; STATIC: cmp/eq	{{r[0-9]+}},r5
; STATIC: .long	_Unwind_Resume
; STATIC: .section	.gcc_except_table
; STATIC: .byte	11
; STATIC-SAME: TType Encoding = sdata4
; STATIC: Call Site 1
; STATIC: Action Record 1
; STATIC: Catch TypeInfos
; STATIC: .long	_ZTIi
;
; PIC-LABEL: catch_and_cleanup:
; PIC: .cfi_personality 144, DW.ref.__gxx_personality_v0
; PIC-NEXT: .cfi_lsda 27, .Lexception0
; PIC: mov.l	r12,
; PIC: mov	r4,{{r[0-9]+}}
; PIC: cmp/eq	{{r[0-9]+}},r5
; PIC: .long	_Unwind_Resume@PLT
; PIC: .section	.gcc_except_table
; PIC: .byte	155
; PIC-SAME: TType Encoding = indirect pcrel sdata4
; PIC: .long	.L_ZTIi.DW.stub-
; PIC: .section	.data.DW.ref.__gxx_personality_v0
entry:
	invoke void @may_throw() to label %normal unwind label %lpad

normal:
	call void @cleanup(ptr %state)
	ret i32 0

lpad:
	%landing = landingpad { ptr, i32 }
		cleanup
		catch ptr @_ZTIi
	%exception = extractvalue { ptr, i32 } %landing, 0
	%selector = extractvalue { ptr, i32 } %landing, 1
	%typeid = call i32 @llvm.eh.typeid.for.p0(ptr @_ZTIi)
	%matches = icmp eq i32 %selector, %typeid
	br i1 %matches, label %catch, label %resume

catch:
	%object = call ptr @__cxa_begin_catch(ptr %exception)
	call void @__cxa_end_catch()
	call void @cleanup(ptr %state)
	ret i32 1

resume:
	call void @cleanup(ptr %state)
	resume { ptr, i32 } %landing
}

define i32 @far_landing_pad() personality ptr @__gxx_personality_v0 {
entry:
	invoke void @may_throw() to label %normal unwind label %lpad

normal:
	call void asm sideeffect ".rept 2500\0A\09nop\0A\09.endr", ""()
	ret i32 0

lpad:
	%landing = landingpad { ptr, i32 }
		cleanup
		catch ptr @_ZTIi
	%exception = extractvalue { ptr, i32 } %landing, 0
	%selector = extractvalue { ptr, i32 } %landing, 1
	%typeid = call i32 @llvm.eh.typeid.for.p0(ptr @_ZTIi)
	%matches = icmp eq i32 %selector, %typeid
	br i1 %matches, label %catch, label %resume

catch:
	%object = call ptr @__cxa_begin_catch(ptr %exception)
	call void asm sideeffect ".rept 2500\0A\09nop\0A\09.endr", ""()
	call void @__cxa_end_catch()
	ret i32 1

resume:
	resume { ptr, i32 } %landing
}

; STATIC-OBJ: Name: .gcc_except_table
; STATIC-OBJ: Name: .eh_frame
; STATIC-OBJ: Section {{.*}} .rela.gcc_except_table {
; STATIC-OBJ: R_SH_DIR32 _ZTIi
; STATIC-OBJ: Section {{.*}} .rela.eh_frame {
; STATIC-OBJ-DAG: R_SH_DIR32 __gxx_personality_v0
; STATIC-OBJ-DAG: R_SH_REL32 .L0
; STATIC-OBJ-DAG: R_SH_DIR32 .L0

; PIC-OBJ: Name: .gcc_except_table
; PIC-OBJ: Name: .data.DW.ref.__gxx_personality_v0
; PIC-OBJ: Name: .eh_frame
; PIC-OBJ: Section {{.*}} .rela.gcc_except_table {
; PIC-OBJ: R_SH_REL32 .L_ZTIi.DW.stub
; PIC-OBJ: Section {{.*}} .rela.data.DW.ref.__gxx_personality_v0 {
; PIC-OBJ: R_SH_DIR32 __gxx_personality_v0
; PIC-OBJ: Section {{.*}} .rela.eh_frame {
; PIC-OBJ-DAG: R_SH_REL32 DW.ref.__gxx_personality_v0
; PIC-OBJ-DAG: R_SH_REL32 .L0

; STATIC-FRAME: Augmentation: "zPLR"
; STATIC-FRAME: Return address column: 17
; STATIC-FRAME: Augmentation data: 00 00 00 00 00 0B 1B
; STATIC-FRAME: DW_CFA_def_cfa: R15 +0
; STATIC-FRAME: FDE cie=

; PIC-FRAME: Augmentation: "zPLR"
; PIC-FRAME: Return address column: 17
; PIC-FRAME: Augmentation data: 90 00 00 00 00 1B 1B
; PIC-FRAME: DW_CFA_def_cfa: R15 +0
; PIC-FRAME: FDE cie=
