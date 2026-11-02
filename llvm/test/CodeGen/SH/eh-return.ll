; RUN: llc -mtriple=sh-unknown-linux-gnu -exception-model=dwarf -verify-machineinstrs < %s | FileCheck %s --check-prefix=CHECK
; RUN: llc -mtriple=shle-unknown-linux-gnu -exception-model=dwarf -verify-machineinstrs < %s | FileCheck %s --check-prefix=CHECK
; RUN: llc -mtriple=sh-unknown-linux-gnu -exception-model=dwarf -relocation-model=pic -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,PIC
; RUN: llc -mtriple=shle-unknown-linux-gnu -exception-model=dwarf -relocation-model=pic -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,PIC
; RUN: llc -mtriple=sh-unknown-linux-gnu -exception-model=dwarf -relocation-model=pic -filetype=obj -verify-machineinstrs < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -exception-model=dwarf -relocation-model=pic -filetype=obj -verify-machineinstrs < %s -o %t.le.o
; RUN: llvm-dwarfdump --eh-frame %t.be.o | FileCheck %s --check-prefix=FRAME
; RUN: llvm-dwarfdump --eh-frame %t.le.o | FileCheck %s --check-prefix=FRAME

define void @eh_return(i32 %offset, ptr %handler) nounwind uwtable {
; CHECK-LABEL: eh_return:
; CHECK: add	#-16,r15
; CHECK: mov.l	r4,@(12,r15)
; CHECK: mov.l	r5,@(8,r15)
; CHECK: mov.l	r6,@(4,r15)
; CHECK: mov.l	r7,@r15
; CHECK: mov.l	@r15,r7
; CHECK: mov	r5,r2
; CHECK: mov	r4,r3
; CHECK: mov.l	@(4,r15),r6
; CHECK: mov.l	@(8,r15),r5
; CHECK: mov.l	@(12,r15),r4
; CHECK: add	#16,r15
; CHECK: add	r3,r15
; CHECK-NEXT: lds	r2,pr
; CHECK-NEXT: rts
; CHECK-NEXT: nop
entry:
	call void @llvm.eh.return.i32(i32 %offset, ptr %handler)
	unreachable
}

define void @eh_return_large_frame(i32 %offset, ptr %handler) nounwind uwtable {
; CHECK-LABEL: eh_return_large_frame:
; CHECK: mov	#16,r3
; CHECK-NEXT: shll8	r3
; CHECK: sub	r3,r15
; CHECK: mov	r5,r2
; CHECK: mov	r4,r3
; CHECK: mov	#16,r0
; CHECK: add	r0,r15
; CHECK: add	r3,r15
; CHECK-NEXT: lds	r2,pr
; CHECK-NEXT: rts
entry:
	%frame = alloca [1024 x i32], align 4
	%slot = getelementptr inbounds [1024 x i32], ptr %frame, i32 0, i32 1023
	store volatile i32 1, ptr %slot, align 4
	call void @llvm.eh.return.i32(i32 %offset, ptr %handler)
	unreachable
}

define void @eh_return_call(i32 %offset, ptr %handler) nounwind uwtable {
; CHECK-LABEL: eh_return_call:
; CHECK: sts.l	pr,@-r15
; PIC: mov.l	r12,@r15
; CHECK: jsr	@{{r[0-9]+}}
; CHECK-NEXT: {{(nop|mov[[:space:]]+r4,r9)}}
; PIC: mov.l	@r15,r12
; CHECK: lds.l	@r15+,pr
; CHECK: add	r3,r15
; CHECK-NEXT: lds	r2,pr
; CHECK-NEXT: rts
entry:
	call void @callee()
	call void @llvm.eh.return.i32(i32 %offset, ptr %handler)
	unreachable
}

define void @mixed_return(i32 %normal, i32 %offset, ptr %handler) nounwind {
; CHECK-LABEL: mixed_return:
; CHECK-NOT: add	r3,r15
; CHECK: add	r3,r15
; CHECK-NOT: add	r3,r15
; CHECK: .Lfunc_end
entry:
	%condition = icmp ne i32 %normal, 0
	br i1 %condition, label %ordinary, label %exception

ordinary:
	ret void

exception:
	call void @llvm.eh.return.i32(i32 %offset, ptr %handler)
	unreachable
}

define void @eh_return_dynamic(i32 %offset, ptr %handler, i32 %count) nounwind uwtable {
; CHECK-LABEL: eh_return_dynamic:
; CHECK: mov	r15,r14
; CHECK: sub	{{r[0-9]+}},{{r[0-9]+}}
; CHECK: mov	r14,r15
; CHECK: add	r3,r15
; CHECK-NEXT: lds	r2,pr
entry:
	%frame = alloca i32, i32 %count, align 4
	store volatile i32 %count, ptr %frame, align 4
	call void @llvm.eh.return.i32(i32 %offset, ptr %handler)
	unreachable
}

define void @eh_return_realign(i32 %offset, ptr %handler) nounwind uwtable {
; CHECK-LABEL: eh_return_realign:
; CHECK: mov	r15,r14
; CHECK: and	{{r[0-9]+}},r15
; CHECK: mov	r14,r15
; CHECK: add	r3,r15
; CHECK-NEXT: lds	r2,pr
entry:
	%frame = alloca i32, align 32
	store volatile i32 1, ptr %frame, align 32
	call void @llvm.eh.return.i32(i32 %offset, ptr %handler)
	unreachable
}

declare void @llvm.eh.return.i32(i32, ptr)
declare void @callee()

; FRAME: .eh_frame contents:
; FRAME: Return address column: 17
; FRAME: Augmentation data: 1B
; FRAME: CFA=R15+16: R4=[CFA-4], R5=[CFA-8], R6=[CFA-12], R7=[CFA-16]
; FRAME: CFA=R15
; FRAME: CFA=R15+4116: R4=[CFA-4], R5=[CFA-8], R6=[CFA-12], R7=[CFA-16]
; FRAME: CFA=R15
; FRAME: CFA=R15+32: R4=[CFA-8], R5=[CFA-12], R6=[CFA-16], R7=[CFA-20], R8=[CFA-24], R9=[CFA-28], R12=[CFA-32], PR=[CFA-4]
; FRAME: CFA=R15
