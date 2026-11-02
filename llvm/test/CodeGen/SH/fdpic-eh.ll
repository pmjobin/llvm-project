; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -exception-model=dwarf -relocation-model=pic -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -exception-model=dwarf -relocation-model=pic -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -exception-model=dwarf -relocation-model=pic -filetype=obj -verify-machineinstrs < %s -o %t.le.o
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -exception-model=dwarf -relocation-model=pic -filetype=obj -verify-machineinstrs < %s -o %t.be.o
; RUN: llvm-readobj --file-headers --sections --relocations %t.le.o %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-dwarfdump --verify %t.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --verify %t.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY

@_ZTIi = external constant ptr

declare void @may_throw()
declare void @cleanup(ptr) nounwind
declare ptr @__cxa_begin_catch(ptr) nounwind
declare void @__cxa_end_catch() nounwind
declare i32 @__gxx_personality_v0(...)
declare i32 @llvm.eh.typeid.for.p0(ptr)
declare void @llvm.eh.return.i32(i32, ptr)

define i32 @catch_and_cleanup(ptr %state) personality ptr @__gxx_personality_v0 {
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

define void @fdpic_eh_return(i32 %offset, ptr %handler) nounwind uwtable {
entry:
	call void @llvm.eh.return.i32(i32 %offset, ptr %handler)
	unreachable
}

; ASM-LABEL: catch_and_cleanup:
; ASM: .cfi_personality 176, __gxx_personality_v0
; ASM-NEXT: .cfi_lsda 27, .Lexception0
; ASM: mov	r12,[[GOT:r[0-9]+]]
; ASM: mov	[[GOT]],r12
; ASM: .long	_Unwind_Resume@PLT
; ASM: .section	.gcc_except_table
; ASM: .byte	187
; ASM-SAME: TType Encoding = indirect datarel sdata4
; ASM: .long	_ZTIi@GOT

; ASM-LABEL: fdpic_eh_return:
; ASM: mov	r5,r2
; ASM: mov	r4,r3
; ASM: add	r3,r15
; ASM-NEXT: lds	r2,pr
; ASM-NEXT: rts

; OBJECT-COUNT-2: Flags [ (0x8002)
; OBJECT: Name: .gcc_except_table
; OBJECT: Name: .eh_frame
; OBJECT: R_SH_GOT32 _ZTIi
; OBJECT: R_SH_GOTFUNCDESC __gxx_personality_v0
; OBJECT: R_SH_REL32 .L0

; VERIFY-NOT: error:
; VERIFY: No errors.
