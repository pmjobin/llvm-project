; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=machine-sink -o - %s | FileCheck --check-prefix=SINK %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=machine-sink -o - %s | FileCheck --check-prefixes=SINK,FPU %s

declare void @use(ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr)
declare void @use_float(float)

define void @sink(ptr %base, i32 %n) optsize {
entry:
	%p1 = getelementptr i8, ptr %base, i32 4
	%p2 = getelementptr i8, ptr %base, i32 8
	%p3 = getelementptr i8, ptr %base, i32 12
	%p4 = getelementptr i8, ptr %base, i32 16
	%p5 = getelementptr i8, ptr %base, i32 20
	%p6 = getelementptr i8, ptr %base, i32 24
	%p7 = getelementptr i8, ptr %base, i32 28
	%p8 = getelementptr i8, ptr %base, i32 32
	%p9 = getelementptr i8, ptr %base, i32 36
	%p10 = getelementptr i8, ptr %base, i32 40
	%p11 = getelementptr i8, ptr %base, i32 44
	%p12 = getelementptr i8, ptr %base, i32 48
	%p13 = getelementptr i8, ptr %base, i32 52
	%p14 = getelementptr i8, ptr %base, i32 56
	%p15 = getelementptr i8, ptr %base, i32 60
	%p16 = getelementptr i8, ptr %base, i32 64
	br label %loop

loop:
	%i = phi i32 [ %n, %entry ], [ %next, %loop ]
	call void @use(ptr %p1, ptr %p2, ptr %p3, ptr %p4, ptr %p5, ptr %p6, ptr %p7, ptr %p8, ptr %p9, ptr %p10, ptr %p11, ptr %p12, ptr %p13, ptr %p14, ptr %p15, ptr %p16)
	%next = add i32 %i, -1
	%again = icmp ne i32 %next, 0
	br i1 %again, label %loop, label %exit

exit:
	ret void
}

; SINK-LABEL: name: sink
; SINK: bb.0.entry:
; SINK: ADDri {{.*}}, 64
; SINK: bb.1.loop:
; SINK-NOT: ADDri {{.*}}, 64
; SINK: RTS

define void @dont_sink_fpu(ptr %base, i32 %n, float %x) optsize {
entry:
	%p1 = getelementptr i8, ptr %base, i32 4
	%p2 = getelementptr i8, ptr %base, i32 8
	%p3 = getelementptr i8, ptr %base, i32 12
	%p4 = getelementptr i8, ptr %base, i32 16
	%p5 = getelementptr i8, ptr %base, i32 20
	%p6 = getelementptr i8, ptr %base, i32 24
	%p7 = getelementptr i8, ptr %base, i32 28
	%p8 = getelementptr i8, ptr %base, i32 32
	%p9 = getelementptr i8, ptr %base, i32 36
	%p10 = getelementptr i8, ptr %base, i32 40
	%p11 = getelementptr i8, ptr %base, i32 44
	%p12 = getelementptr i8, ptr %base, i32 48
	%p13 = getelementptr i8, ptr %base, i32 52
	%p14 = getelementptr i8, ptr %base, i32 56
	%p15 = getelementptr i8, ptr %base, i32 60
	%p16 = getelementptr i8, ptr %base, i32 64
	br label %loop

loop:
	%i = phi i32 [ %n, %entry ], [ %next, %loop ]
	%f = fadd float %x, 1.0
	call void @use_float(float %f)
	call void @use(ptr %p1, ptr %p2, ptr %p3, ptr %p4, ptr %p5, ptr %p6, ptr %p7, ptr %p8, ptr %p9, ptr %p10, ptr %p11, ptr %p12, ptr %p13, ptr %p14, ptr %p15, ptr %p16)
	%next = add i32 %i, -1
	%again = icmp ne i32 %next, 0
	br i1 %again, label %loop, label %exit

exit:
	ret void
}

; FPU-LABEL: name: dont_sink_fpu
; FPU: bb.0.entry:
; FPU: ADDri {{.*}}, 64
; FPU-NOT: FADD_FR
; FPU: bb.1.loop:
; FPU-NOT: ADDri {{.*}}, 64
; FPU: FADD_FR
