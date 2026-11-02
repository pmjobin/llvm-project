; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SH2
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SH4

define i64 @low_pressure_recurrences(ptr %data, ptr %output, i32 %length, i32 %seed) nounwind {
entry:
	%empty = icmp eq i32 %length, 0
	br i1 %empty, label %return, label %outer

outer:
	%index = phi i32 [ %next.index, %exit ], [ 0, %entry ]
	%out = phi i32 [ %next.out, %exit ], [ 0, %entry ]
	%checksum = phi i32 [ %next.checksum, %exit ], [ %seed, %entry ]
	%first.ptr = getelementptr inbounds i8, ptr %data, i32 %index
	%value = load i8, ptr %first.ptr, align 1
	%first.index = add nuw i32 %index, 1
	%one.byte = icmp ult i32 %first.index, %length
	br i1 %one.byte, label %scan, label %exit

scan:
	%scan.index = phi i32 [ %next.index.inner, %match ], [ %first.index, %outer ]
	%run = phi i32 [ %next.run, %match ], [ 1, %outer ]
	%scan.ptr = getelementptr inbounds i8, ptr %data, i32 %scan.index
	%next.value = load i8, ptr %scan.ptr, align 1
	%different = icmp ne i8 %next.value, %value
	br i1 %different, label %exit, label %match

match:
	%next.run = add nuw nsw i32 %run, 1
	%next.index.inner = add i32 %next.run, %index
	%in.bounds = icmp ult i32 %next.index.inner, %length
	%below.limit = icmp ne i32 %next.run, 255
	%continue = and i1 %below.limit, %in.bounds
	br i1 %continue, label %scan, label %exit

exit:
	%final.run = phi i32 [ 1, %outer ], [ %run, %scan ], [ %next.run, %match ]
	%next.index = phi i32 [ %first.index, %outer ], [ %scan.index, %scan ], [ %next.index.inner, %match ]
	%run.byte = trunc i32 %final.run to i8
	%out.ptr = getelementptr inbounds i8, ptr %output, i32 %out
	store i8 %run.byte, ptr %out.ptr, align 1
	%value.ptr = getelementptr inbounds i8, ptr %out.ptr, i32 1
	store i8 %value, ptr %value.ptr, align 1
	%scaled = mul i32 %checksum, 33
	%extended = zext i8 %value to i32
	%with.value = add i32 %scaled, %extended
	%next.checksum = add i32 %with.value, %final.run
	%next.out = add i32 %out, 2
	%done = icmp uge i32 %next.index, %length
	br i1 %done, label %return, label %outer

return:
	%final.checksum = phi i32 [ %seed, %entry ], [ %next.checksum, %exit ]
	%final.out = phi i32 [ 0, %entry ], [ %next.out, %exit ]
	%wide.out = zext i32 %final.out to i64
	%high = shl nuw i64 %wide.out, 32
	%low = zext i32 %final.checksum to i64
	%result = or disjoint i64 %high, %low
	ret i64 %result
}

; CHECK-LABEL: low_pressure_recurrences:
; CHECK: mov.l r8,@-r15
; CHECK-NEXT: mov.l r9,@-r15
; CHECK-NEXT: mov.l r10,@-r15
; CHECK-NEXT: mov.l r11,@-r15
; SH2-NOT: mov.l r12
; The SH4 speed-mode multiply expansion needs one additional saved GPR.
; Both loop recurrences and the balanced save/restore sequence remain.
; SH4-NEXT: mov.l r12,@-r15
; SH4-NOT: mov.l r13
; CHECK-COUNT-2: add #1,{{r[0-9]+}}
; SH4: mov.l @r15+,r12
; CHECK: mov.l @r15+,r11
; CHECK-NEXT: mov.l @r15+,r10
; CHECK-NEXT: mov.l @r15+,r9
; CHECK-NEXT: rts
; CHECK-NEXT: mov.l @r15+,r8
