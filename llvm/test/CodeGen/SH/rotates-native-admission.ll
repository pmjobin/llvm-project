; Native-rotate admission is omitted; retain these contexts, poison/freeze
; witnesses and negative cases through the conventional helper lowering.
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s

; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -enable-new-pm=1 -verify-machineinstrs -verify-machine-dom-info -verify-loop-info < %s > /dev/null

; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -verify-machine-dom-info -verify-loop-info < %s > /dev/null

declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)

; CHECK-LABEL: name: static_1
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
define i32 @static_1(ptr %x,ptr %counts) {
	%xp0=getelementptr i32,ptr %x,i32 0
	%cp0=getelementptr i32,ptr %counts,i32 0
	%x0=load i32,ptr %xp0
	%c0=load i32,ptr %cp0
	%r0=call i32 @llvm.fshl.i32(i32 %x0,i32 %x0,i32 %c0)
	ret i32 %r0
}

; CHECK-LABEL: name: static_8
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
define i32 @static_8(ptr %x,ptr %counts) {
	%xp0=getelementptr i32,ptr %x,i32 0
	%cp0=getelementptr i32,ptr %counts,i32 0
	%x0=load i32,ptr %xp0
	%c0=load i32,ptr %cp0
	%r0=call i32 @llvm.fshl.i32(i32 %x0,i32 %x0,i32 %c0)
	%xp1=getelementptr i32,ptr %x,i32 1
	%cp1=getelementptr i32,ptr %counts,i32 1
	%x1=load i32,ptr %xp1
	%c1=load i32,ptr %cp1
	%r1=call i32 @llvm.fshl.i32(i32 %x1,i32 %x1,i32 %c1)
	%sum1=xor i32 %r0,%r1
	%xp2=getelementptr i32,ptr %x,i32 2
	%cp2=getelementptr i32,ptr %counts,i32 2
	%x2=load i32,ptr %xp2
	%c2=load i32,ptr %cp2
	%r2=call i32 @llvm.fshl.i32(i32 %x2,i32 %x2,i32 %c2)
	%sum2=xor i32 %sum1,%r2
	%xp3=getelementptr i32,ptr %x,i32 3
	%cp3=getelementptr i32,ptr %counts,i32 3
	%x3=load i32,ptr %xp3
	%c3=load i32,ptr %cp3
	%r3=call i32 @llvm.fshl.i32(i32 %x3,i32 %x3,i32 %c3)
	%sum3=xor i32 %sum2,%r3
	%xp4=getelementptr i32,ptr %x,i32 4
	%cp4=getelementptr i32,ptr %counts,i32 4
	%x4=load i32,ptr %xp4
	%c4=load i32,ptr %cp4
	%r4=call i32 @llvm.fshl.i32(i32 %x4,i32 %x4,i32 %c4)
	%sum4=xor i32 %sum3,%r4
	%xp5=getelementptr i32,ptr %x,i32 5
	%cp5=getelementptr i32,ptr %counts,i32 5
	%x5=load i32,ptr %xp5
	%c5=load i32,ptr %cp5
	%r5=call i32 @llvm.fshl.i32(i32 %x5,i32 %x5,i32 %c5)
	%sum5=xor i32 %sum4,%r5
	%xp6=getelementptr i32,ptr %x,i32 6
	%cp6=getelementptr i32,ptr %counts,i32 6
	%x6=load i32,ptr %xp6
	%c6=load i32,ptr %cp6
	%r6=call i32 @llvm.fshl.i32(i32 %x6,i32 %x6,i32 %c6)
	%sum6=xor i32 %sum5,%r6
	%xp7=getelementptr i32,ptr %x,i32 7
	%cp7=getelementptr i32,ptr %counts,i32 7
	%x7=load i32,ptr %xp7
	%c7=load i32,ptr %cp7
	%r7=call i32 @llvm.fshl.i32(i32 %x7,i32 %x7,i32 %c7)
	%sum7=xor i32 %sum6,%r7
	ret i32 %sum7
}

; CHECK-LABEL: name: static_32
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
define i32 @static_32(ptr %x,ptr %counts) {
	%xp0=getelementptr i32,ptr %x,i32 0
	%cp0=getelementptr i32,ptr %counts,i32 0
	%x0=load i32,ptr %xp0
	%c0=load i32,ptr %cp0
	%r0=call i32 @llvm.fshl.i32(i32 %x0,i32 %x0,i32 %c0)
	%xp1=getelementptr i32,ptr %x,i32 1
	%cp1=getelementptr i32,ptr %counts,i32 1
	%x1=load i32,ptr %xp1
	%c1=load i32,ptr %cp1
	%r1=call i32 @llvm.fshl.i32(i32 %x1,i32 %x1,i32 %c1)
	%sum1=xor i32 %r0,%r1
	%xp2=getelementptr i32,ptr %x,i32 2
	%cp2=getelementptr i32,ptr %counts,i32 2
	%x2=load i32,ptr %xp2
	%c2=load i32,ptr %cp2
	%r2=call i32 @llvm.fshl.i32(i32 %x2,i32 %x2,i32 %c2)
	%sum2=xor i32 %sum1,%r2
	%xp3=getelementptr i32,ptr %x,i32 3
	%cp3=getelementptr i32,ptr %counts,i32 3
	%x3=load i32,ptr %xp3
	%c3=load i32,ptr %cp3
	%r3=call i32 @llvm.fshl.i32(i32 %x3,i32 %x3,i32 %c3)
	%sum3=xor i32 %sum2,%r3
	%xp4=getelementptr i32,ptr %x,i32 4
	%cp4=getelementptr i32,ptr %counts,i32 4
	%x4=load i32,ptr %xp4
	%c4=load i32,ptr %cp4
	%r4=call i32 @llvm.fshl.i32(i32 %x4,i32 %x4,i32 %c4)
	%sum4=xor i32 %sum3,%r4
	%xp5=getelementptr i32,ptr %x,i32 5
	%cp5=getelementptr i32,ptr %counts,i32 5
	%x5=load i32,ptr %xp5
	%c5=load i32,ptr %cp5
	%r5=call i32 @llvm.fshl.i32(i32 %x5,i32 %x5,i32 %c5)
	%sum5=xor i32 %sum4,%r5
	%xp6=getelementptr i32,ptr %x,i32 6
	%cp6=getelementptr i32,ptr %counts,i32 6
	%x6=load i32,ptr %xp6
	%c6=load i32,ptr %cp6
	%r6=call i32 @llvm.fshl.i32(i32 %x6,i32 %x6,i32 %c6)
	%sum6=xor i32 %sum5,%r6
	%xp7=getelementptr i32,ptr %x,i32 7
	%cp7=getelementptr i32,ptr %counts,i32 7
	%x7=load i32,ptr %xp7
	%c7=load i32,ptr %cp7
	%r7=call i32 @llvm.fshl.i32(i32 %x7,i32 %x7,i32 %c7)
	%sum7=xor i32 %sum6,%r7
	%xp8=getelementptr i32,ptr %x,i32 8
	%cp8=getelementptr i32,ptr %counts,i32 8
	%x8=load i32,ptr %xp8
	%c8=load i32,ptr %cp8
	%r8=call i32 @llvm.fshl.i32(i32 %x8,i32 %x8,i32 %c8)
	%sum8=xor i32 %sum7,%r8
	%xp9=getelementptr i32,ptr %x,i32 9
	%cp9=getelementptr i32,ptr %counts,i32 9
	%x9=load i32,ptr %xp9
	%c9=load i32,ptr %cp9
	%r9=call i32 @llvm.fshl.i32(i32 %x9,i32 %x9,i32 %c9)
	%sum9=xor i32 %sum8,%r9
	%xp10=getelementptr i32,ptr %x,i32 10
	%cp10=getelementptr i32,ptr %counts,i32 10
	%x10=load i32,ptr %xp10
	%c10=load i32,ptr %cp10
	%r10=call i32 @llvm.fshl.i32(i32 %x10,i32 %x10,i32 %c10)
	%sum10=xor i32 %sum9,%r10
	%xp11=getelementptr i32,ptr %x,i32 11
	%cp11=getelementptr i32,ptr %counts,i32 11
	%x11=load i32,ptr %xp11
	%c11=load i32,ptr %cp11
	%r11=call i32 @llvm.fshl.i32(i32 %x11,i32 %x11,i32 %c11)
	%sum11=xor i32 %sum10,%r11
	%xp12=getelementptr i32,ptr %x,i32 12
	%cp12=getelementptr i32,ptr %counts,i32 12
	%x12=load i32,ptr %xp12
	%c12=load i32,ptr %cp12
	%r12=call i32 @llvm.fshl.i32(i32 %x12,i32 %x12,i32 %c12)
	%sum12=xor i32 %sum11,%r12
	%xp13=getelementptr i32,ptr %x,i32 13
	%cp13=getelementptr i32,ptr %counts,i32 13
	%x13=load i32,ptr %xp13
	%c13=load i32,ptr %cp13
	%r13=call i32 @llvm.fshl.i32(i32 %x13,i32 %x13,i32 %c13)
	%sum13=xor i32 %sum12,%r13
	%xp14=getelementptr i32,ptr %x,i32 14
	%cp14=getelementptr i32,ptr %counts,i32 14
	%x14=load i32,ptr %xp14
	%c14=load i32,ptr %cp14
	%r14=call i32 @llvm.fshl.i32(i32 %x14,i32 %x14,i32 %c14)
	%sum14=xor i32 %sum13,%r14
	%xp15=getelementptr i32,ptr %x,i32 15
	%cp15=getelementptr i32,ptr %counts,i32 15
	%x15=load i32,ptr %xp15
	%c15=load i32,ptr %cp15
	%r15=call i32 @llvm.fshl.i32(i32 %x15,i32 %x15,i32 %c15)
	%sum15=xor i32 %sum14,%r15
	%xp16=getelementptr i32,ptr %x,i32 16
	%cp16=getelementptr i32,ptr %counts,i32 16
	%x16=load i32,ptr %xp16
	%c16=load i32,ptr %cp16
	%r16=call i32 @llvm.fshl.i32(i32 %x16,i32 %x16,i32 %c16)
	%sum16=xor i32 %sum15,%r16
	%xp17=getelementptr i32,ptr %x,i32 17
	%cp17=getelementptr i32,ptr %counts,i32 17
	%x17=load i32,ptr %xp17
	%c17=load i32,ptr %cp17
	%r17=call i32 @llvm.fshl.i32(i32 %x17,i32 %x17,i32 %c17)
	%sum17=xor i32 %sum16,%r17
	%xp18=getelementptr i32,ptr %x,i32 18
	%cp18=getelementptr i32,ptr %counts,i32 18
	%x18=load i32,ptr %xp18
	%c18=load i32,ptr %cp18
	%r18=call i32 @llvm.fshl.i32(i32 %x18,i32 %x18,i32 %c18)
	%sum18=xor i32 %sum17,%r18
	%xp19=getelementptr i32,ptr %x,i32 19
	%cp19=getelementptr i32,ptr %counts,i32 19
	%x19=load i32,ptr %xp19
	%c19=load i32,ptr %cp19
	%r19=call i32 @llvm.fshl.i32(i32 %x19,i32 %x19,i32 %c19)
	%sum19=xor i32 %sum18,%r19
	%xp20=getelementptr i32,ptr %x,i32 20
	%cp20=getelementptr i32,ptr %counts,i32 20
	%x20=load i32,ptr %xp20
	%c20=load i32,ptr %cp20
	%r20=call i32 @llvm.fshl.i32(i32 %x20,i32 %x20,i32 %c20)
	%sum20=xor i32 %sum19,%r20
	%xp21=getelementptr i32,ptr %x,i32 21
	%cp21=getelementptr i32,ptr %counts,i32 21
	%x21=load i32,ptr %xp21
	%c21=load i32,ptr %cp21
	%r21=call i32 @llvm.fshl.i32(i32 %x21,i32 %x21,i32 %c21)
	%sum21=xor i32 %sum20,%r21
	%xp22=getelementptr i32,ptr %x,i32 22
	%cp22=getelementptr i32,ptr %counts,i32 22
	%x22=load i32,ptr %xp22
	%c22=load i32,ptr %cp22
	%r22=call i32 @llvm.fshl.i32(i32 %x22,i32 %x22,i32 %c22)
	%sum22=xor i32 %sum21,%r22
	%xp23=getelementptr i32,ptr %x,i32 23
	%cp23=getelementptr i32,ptr %counts,i32 23
	%x23=load i32,ptr %xp23
	%c23=load i32,ptr %cp23
	%r23=call i32 @llvm.fshl.i32(i32 %x23,i32 %x23,i32 %c23)
	%sum23=xor i32 %sum22,%r23
	%xp24=getelementptr i32,ptr %x,i32 24
	%cp24=getelementptr i32,ptr %counts,i32 24
	%x24=load i32,ptr %xp24
	%c24=load i32,ptr %cp24
	%r24=call i32 @llvm.fshl.i32(i32 %x24,i32 %x24,i32 %c24)
	%sum24=xor i32 %sum23,%r24
	%xp25=getelementptr i32,ptr %x,i32 25
	%cp25=getelementptr i32,ptr %counts,i32 25
	%x25=load i32,ptr %xp25
	%c25=load i32,ptr %cp25
	%r25=call i32 @llvm.fshl.i32(i32 %x25,i32 %x25,i32 %c25)
	%sum25=xor i32 %sum24,%r25
	%xp26=getelementptr i32,ptr %x,i32 26
	%cp26=getelementptr i32,ptr %counts,i32 26
	%x26=load i32,ptr %xp26
	%c26=load i32,ptr %cp26
	%r26=call i32 @llvm.fshl.i32(i32 %x26,i32 %x26,i32 %c26)
	%sum26=xor i32 %sum25,%r26
	%xp27=getelementptr i32,ptr %x,i32 27
	%cp27=getelementptr i32,ptr %counts,i32 27
	%x27=load i32,ptr %xp27
	%c27=load i32,ptr %cp27
	%r27=call i32 @llvm.fshl.i32(i32 %x27,i32 %x27,i32 %c27)
	%sum27=xor i32 %sum26,%r27
	%xp28=getelementptr i32,ptr %x,i32 28
	%cp28=getelementptr i32,ptr %counts,i32 28
	%x28=load i32,ptr %xp28
	%c28=load i32,ptr %cp28
	%r28=call i32 @llvm.fshl.i32(i32 %x28,i32 %x28,i32 %c28)
	%sum28=xor i32 %sum27,%r28
	%xp29=getelementptr i32,ptr %x,i32 29
	%cp29=getelementptr i32,ptr %counts,i32 29
	%x29=load i32,ptr %xp29
	%c29=load i32,ptr %cp29
	%r29=call i32 @llvm.fshl.i32(i32 %x29,i32 %x29,i32 %c29)
	%sum29=xor i32 %sum28,%r29
	%xp30=getelementptr i32,ptr %x,i32 30
	%cp30=getelementptr i32,ptr %counts,i32 30
	%x30=load i32,ptr %xp30
	%c30=load i32,ptr %cp30
	%r30=call i32 @llvm.fshl.i32(i32 %x30,i32 %x30,i32 %c30)
	%sum30=xor i32 %sum29,%r30
	%xp31=getelementptr i32,ptr %x,i32 31
	%cp31=getelementptr i32,ptr %counts,i32 31
	%x31=load i32,ptr %xp31
	%c31=load i32,ptr %cp31
	%r31=call i32 @llvm.fshl.i32(i32 %x31,i32 %x31,i32 %c31)
	%sum31=xor i32 %sum30,%r31
	ret i32 %sum31
}

; CHECK-LABEL: name: loop
; CHECK: JSR
define i32 @loop(ptr %x, ptr %counts, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ 0, %entry ], [ %combined, %latch ]
	%xp0 = getelementptr i32, ptr %x, i32 %i
	%cp0 = getelementptr i32, ptr %counts, i32 %i
	%x0 = load i32, ptr %xp0
	%c0 = load i32, ptr %cp0
	%r0 = call i32 @llvm.fshl.i32(i32 %x0, i32 %x0, i32 %c0)
	%reduce0 = xor i32 %sum, %r0
	br label %latch
latch:
	%combined = xor i32 %reduce0, 0
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %finish, label %loop
finish:
	br label %exit
exit:
	%result = phi i32 [ 0, %entry ], [ %combined, %finish ]
	ret i32 %result
}

; CHECK-LABEL: name: independent_2
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
define i32 @independent_2(ptr %x, ptr %counts, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ 0, %entry ], [ %combined, %latch ]
	%xp0 = getelementptr i32, ptr %x, i32 %i
	%cp0 = getelementptr i32, ptr %counts, i32 %i
	%x0 = load i32, ptr %xp0
	%c0 = load i32, ptr %cp0
	%r0 = call i32 @llvm.fshl.i32(i32 %x0, i32 %x0, i32 %c0)
	%reduce0 = xor i32 %sum, %r0
	%index1 = add i32 %i, 1
	%xp1 = getelementptr i32, ptr %x, i32 %index1
	%cp1 = getelementptr i32, ptr %counts, i32 %index1
	%x1 = load i32, ptr %xp1
	%c1 = load i32, ptr %cp1
	%r1 = call i32 @llvm.fshl.i32(i32 %x1, i32 %x1, i32 %c1)
	%reduce1 = xor i32 %reduce0, %r1
	br label %latch
latch:
	%combined = xor i32 %reduce1, 0
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %finish, label %loop
finish:
	br label %exit
exit:
	%result = phi i32 [ 0, %entry ], [ %combined, %finish ]
	ret i32 %result
}

; CHECK-LABEL: name: retained_2
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
define i32 @retained_2(ptr %x, ptr %counts, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ 0, %entry ], [ %combined, %latch ]
	%xp0 = getelementptr i32, ptr %x, i32 %i
	%cp0 = getelementptr i32, ptr %counts, i32 %i
	%x0 = load i32, ptr %xp0
	%c0 = load i32, ptr %cp0
	%r0 = call i32 @llvm.fshl.i32(i32 %x0, i32 %x0, i32 %c0)
	%index1 = add i32 %i, 1
	%xp1 = getelementptr i32, ptr %x, i32 %index1
	%cp1 = getelementptr i32, ptr %counts, i32 %index1
	%x1 = load i32, ptr %xp1
	%c1 = load i32, ptr %cp1
	%r1 = call i32 @llvm.fshl.i32(i32 %x1, i32 %x1, i32 %c1)
	%reduce0 = xor i32 %sum, %r0
	%reduce1 = xor i32 %reduce0, %r1
	br label %latch
latch:
	%combined = xor i32 %reduce1, 0
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %finish, label %loop
finish:
	br label %exit
exit:
	%result = phi i32 [ 0, %entry ], [ %combined, %finish ]
	ret i32 %result
}

; CHECK-LABEL: name: independent_3
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
define i32 @independent_3(ptr %x, ptr %counts, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ 0, %entry ], [ %combined, %latch ]
	%xp0 = getelementptr i32, ptr %x, i32 %i
	%cp0 = getelementptr i32, ptr %counts, i32 %i
	%x0 = load i32, ptr %xp0
	%c0 = load i32, ptr %cp0
	%r0 = call i32 @llvm.fshl.i32(i32 %x0, i32 %x0, i32 %c0)
	%reduce0 = xor i32 %sum, %r0
	%index1 = add i32 %i, 1
	%xp1 = getelementptr i32, ptr %x, i32 %index1
	%cp1 = getelementptr i32, ptr %counts, i32 %index1
	%x1 = load i32, ptr %xp1
	%c1 = load i32, ptr %cp1
	%r1 = call i32 @llvm.fshl.i32(i32 %x1, i32 %x1, i32 %c1)
	%reduce1 = xor i32 %reduce0, %r1
	%index2 = add i32 %i, 2
	%xp2 = getelementptr i32, ptr %x, i32 %index2
	%cp2 = getelementptr i32, ptr %counts, i32 %index2
	%x2 = load i32, ptr %xp2
	%c2 = load i32, ptr %cp2
	%r2 = call i32 @llvm.fshl.i32(i32 %x2, i32 %x2, i32 %c2)
	%reduce2 = xor i32 %reduce1, %r2
	br label %latch
latch:
	%combined = xor i32 %reduce2, 0
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %finish, label %loop
finish:
	br label %exit
exit:
	%result = phi i32 [ 0, %entry ], [ %combined, %finish ]
	ret i32 %result
}

; CHECK-LABEL: name: unrelated_body
; CHECK: JSR
define i32 @unrelated_body(ptr %x, ptr %counts, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ 0, %entry ], [ %combined, %latch ]
	%xp0 = getelementptr i32, ptr %x, i32 %i
	%cp0 = getelementptr i32, ptr %counts, i32 %i
	%x0 = load i32, ptr %xp0
	%c0 = load i32, ptr %cp0
	%r0 = call i32 @llvm.fshl.i32(i32 %x0, i32 %x0, i32 %c0)
	%reduce0 = xor i32 %sum, %r0
	br label %latch
latch:
	%combined = xor i32 %reduce0, 0
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %finish, label %loop
finish:
	%extra0 = mul i32 %combined, 33
	%extra1 = mul i32 %extra0, 33
	%extra2 = mul i32 %extra1, 33
	%extra3 = mul i32 %extra2, 33
	%extra4 = mul i32 %extra3, 33
	%extra5 = mul i32 %extra4, 33
	%extra6 = mul i32 %extra5, 33
	%extra7 = mul i32 %extra6, 33
	%extra8 = mul i32 %extra7, 33
	%extra9 = mul i32 %extra8, 33
	%extra10 = mul i32 %extra9, 33
	%extra11 = mul i32 %extra10, 33
	%extra12 = mul i32 %extra11, 33
	%extra13 = mul i32 %extra12, 33
	%extra14 = mul i32 %extra13, 33
	%extra15 = mul i32 %extra14, 33
	%extra16 = mul i32 %extra15, 33
	%extra17 = mul i32 %extra16, 33
	%extra18 = mul i32 %extra17, 33
	%extra19 = mul i32 %extra18, 33
	%extra20 = mul i32 %extra19, 33
	%extra21 = mul i32 %extra20, 33
	%extra22 = mul i32 %extra21, 33
	%extra23 = mul i32 %extra22, 33
	%extra24 = mul i32 %extra23, 33
	%extra25 = mul i32 %extra24, 33
	%extra26 = mul i32 %extra25, 33
	%extra27 = mul i32 %extra26, 33
	%extra28 = mul i32 %extra27, 33
	%extra29 = mul i32 %extra28, 33
	%extra30 = mul i32 %extra29, 33
	%extra31 = mul i32 %extra30, 33
	%extra32 = mul i32 %extra31, 33
	%extra33 = mul i32 %extra32, 33
	%extra34 = mul i32 %extra33, 33
	%extra35 = mul i32 %extra34, 33
	%extra36 = mul i32 %extra35, 33
	%extra37 = mul i32 %extra36, 33
	%extra38 = mul i32 %extra37, 33
	%extra39 = mul i32 %extra38, 33
	%extra40 = mul i32 %extra39, 33
	%extra41 = mul i32 %extra40, 33
	%extra42 = mul i32 %extra41, 33
	%extra43 = mul i32 %extra42, 33
	%extra44 = mul i32 %extra43, 33
	%extra45 = mul i32 %extra44, 33
	%extra46 = mul i32 %extra45, 33
	%extra47 = mul i32 %extra46, 33
	%extra48 = mul i32 %extra47, 33
	%extra49 = mul i32 %extra48, 33
	%extra50 = mul i32 %extra49, 33
	%extra51 = mul i32 %extra50, 33
	%extra52 = mul i32 %extra51, 33
	%extra53 = mul i32 %extra52, 33
	%extra54 = mul i32 %extra53, 33
	%extra55 = mul i32 %extra54, 33
	%extra56 = mul i32 %extra55, 33
	%extra57 = mul i32 %extra56, 33
	%extra58 = mul i32 %extra57, 33
	%extra59 = mul i32 %extra58, 33
	%extra60 = mul i32 %extra59, 33
	%extra61 = mul i32 %extra60, 33
	%extra62 = mul i32 %extra61, 33
	%extra63 = mul i32 %extra62, 33
	%extra64 = mul i32 %extra63, 33
	%extra65 = mul i32 %extra64, 33
	%extra66 = mul i32 %extra65, 33
	%extra67 = mul i32 %extra66, 33
	%extra68 = mul i32 %extra67, 33
	%extra69 = mul i32 %extra68, 33
	%extra70 = mul i32 %extra69, 33
	%extra71 = mul i32 %extra70, 33
	%extra72 = mul i32 %extra71, 33
	%extra73 = mul i32 %extra72, 33
	%extra74 = mul i32 %extra73, 33
	%extra75 = mul i32 %extra74, 33
	%extra76 = mul i32 %extra75, 33
	%extra77 = mul i32 %extra76, 33
	%extra78 = mul i32 %extra77, 33
	%extra79 = mul i32 %extra78, 33
	%extra80 = mul i32 %extra79, 33
	%extra81 = mul i32 %extra80, 33
	%extra82 = mul i32 %extra81, 33
	%extra83 = mul i32 %extra82, 33
	%extra84 = mul i32 %extra83, 33
	%extra85 = mul i32 %extra84, 33
	%extra86 = mul i32 %extra85, 33
	%extra87 = mul i32 %extra86, 33
	%extra88 = mul i32 %extra87, 33
	%extra89 = mul i32 %extra88, 33
	%extra90 = mul i32 %extra89, 33
	%extra91 = mul i32 %extra90, 33
	%extra92 = mul i32 %extra91, 33
	%extra93 = mul i32 %extra92, 33
	%extra94 = mul i32 %extra93, 33
	%extra95 = mul i32 %extra94, 33
	br label %exit
exit:
	%result = phi i32 [ 0, %entry ], [ %extra95, %finish ]
	ret i32 %result
}

; CHECK-LABEL: name: shared
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
define i32 @shared(ptr %x, ptr %counts, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ 0, %entry ], [ %combined, %latch ]
	%xp0 = getelementptr i32, ptr %x, i32 %i
	%cp0 = getelementptr i32, ptr %counts, i32 %i
	%x0 = load i32, ptr %xp0
	%c0 = load i32, ptr %cp0
	%r0 = call i32 @llvm.fshl.i32(i32 %x0, i32 %x0, i32 %c0)
	%reduce0 = xor i32 %sum, %r0
	%index1 = add i32 %i, 1
	%xp1 = getelementptr i32, ptr %x, i32 %index1
	%cp1 = getelementptr i32, ptr %counts, i32 %index1
	%x1 = load i32, ptr %xp1
	%c1 = load i32, ptr %cp1
	%r1 = call i32 @llvm.fshr.i32(i32 %x0, i32 %x0, i32 %c0)
	%reduce1 = xor i32 %reduce0, %r1
	br label %latch
latch:
	%combined = xor i32 %reduce1, 0
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %finish, label %loop
finish:
	br label %exit
exit:
	%result = phi i32 [ 0, %entry ], [ %combined, %finish ]
	ret i32 %result
}

; CHECK-LABEL: name: live_count
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
define i32 @live_count(ptr %x, ptr %counts, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ 0, %entry ], [ %combined, %latch ]
	%xp0 = getelementptr i32, ptr %x, i32 %i
	%cp0 = getelementptr i32, ptr %counts, i32 %i
	%x0 = load i32, ptr %xp0
	%c0 = load i32, ptr %cp0
	%r0 = call i32 @llvm.fshl.i32(i32 %x0, i32 %x0, i32 %c0)
	%reduce0 = xor i32 %sum, %r0
	%live = xor i32 %reduce0, %c0
	br label %latch
latch:
	%combined = xor i32 %live, 0
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %finish, label %loop
finish:
	br label %exit
exit:
	%result = phi i32 [ 0, %entry ], [ %combined, %finish ]
	ret i32 %result
}

; CHECK-LABEL: name: early
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
define i32 @early(ptr %x, ptr %counts, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ 0, %entry ], [ %combined, %latch ]
	%xp0 = getelementptr i32, ptr %x, i32 %i
	%cp0 = getelementptr i32, ptr %counts, i32 %i
	%x0 = load i32, ptr %xp0
	%c0 = load i32, ptr %cp0
	%r0 = call i32 @llvm.fshl.i32(i32 %x0, i32 %x0, i32 %c0)
	%reduce0 = xor i32 %sum, %r0
	%stop = icmp eq i32 %x0, 0
	br i1 %stop, label %other, label %latch
other:
	ret i32 %sum
latch:
	%combined = xor i32 %reduce0, 0
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %finish, label %loop
finish:
	br label %exit
exit:
	%result = phi i32 [ 0, %entry ], [ %combined, %finish ]
	ret i32 %result
}

; CHECK-LABEL: name: sibling
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
define i32 @sibling(ptr %x, ptr %counts, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ 0, %entry ], [ %combined, %latch ]
	%xp0 = getelementptr i32, ptr %x, i32 %i
	%cp0 = getelementptr i32, ptr %counts, i32 %i
	%x0 = load i32, ptr %xp0
	%c0 = load i32, ptr %cp0
	%r0 = call i32 @llvm.fshl.i32(i32 %x0, i32 %x0, i32 %c0)
	%reduce0 = xor i32 %sum, %r0
	%stop = icmp eq i32 %x0, 0
	br i1 %stop, label %other, label %latch
other:
	%branchvalue = add i32 %sum, 37
	br label %latch
latch:
	%combined = phi i32 [ %reduce0, %loop ], [ %branchvalue, %other ]
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %finish, label %loop
finish:
	br label %exit
exit:
	%result = phi i32 [ 0, %entry ], [ %combined, %finish ]
	ret i32 %result
}

; CHECK-LABEL: name: boundary_12
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
define i32 @boundary_12(ptr %x, ptr %counts, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop, !prof !11
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ 0, %entry ], [ %combined, %latch ]
	%xp0 = getelementptr i32, ptr %x, i32 %i
	%cp0 = getelementptr i32, ptr %counts, i32 %i
	%x0 = load i32, ptr %xp0
	%c0 = load i32, ptr %cp0
	%r0 = call i32 @llvm.fshl.i32(i32 %x0, i32 %x0, i32 %c0)
	%reduce0 = xor i32 %sum, %r0
	br label %latch
latch:
	%combined = xor i32 %reduce0, 0
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %finish, label %loop, !prof !110
finish:
	br label %exit
exit:
	%result = phi i32 [ 0, %entry ], [ %combined, %finish ]
	ret i32 %result
}
!110 = !{!"branch_weights", i32 1, i32 11}
!11 = !{!"branch_weights", i32 0, i32 1}

; CHECK-LABEL: name: boundary_13
; CHECK: JSR
define i32 @boundary_13(ptr %x, ptr %counts, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop, !prof !21
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ 0, %entry ], [ %combined, %latch ]
	%xp0 = getelementptr i32, ptr %x, i32 %i
	%cp0 = getelementptr i32, ptr %counts, i32 %i
	%x0 = load i32, ptr %xp0
	%c0 = load i32, ptr %cp0
	%r0 = call i32 @llvm.fshl.i32(i32 %x0, i32 %x0, i32 %c0)
	%reduce0 = xor i32 %sum, %r0
	br label %latch
latch:
	%combined = xor i32 %reduce0, 0
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %finish, label %loop, !prof !20
finish:
	br label %exit
exit:
	%result = phi i32 [ 0, %entry ], [ %combined, %finish ]
	ret i32 %result
}
!20 = !{!"branch_weights", i32 1, i32 12}
!21 = !{!"branch_weights", i32 0, i32 1}

; A non-header inside a multi-entry cycle must not use inferred amortization.
; CHECK-LABEL: name: irreducible_interior
; CHECK-NOT: NATIVE_PSEUDO
; CHECK: CALL
declare void @side_a()
declare void @side_b()
define void @irreducible_interior(ptr %x, ptr %counts, ptr %out, ptr %flags, i1 %entry_choice) {
entry:
	br i1 %entry_choice, label %a, label %b
a:
	call void @side_a()
	br label %body
b:
	call void @side_b()
	br label %body
body:
	%v = load i32, ptr %x
	%c = load i32, ptr %counts
	%r = call i32 @llvm.fshl.i32(i32 %v, i32 %v, i32 %c)
	store i32 %r, ptr %out
	%stop = load volatile i32, ptr %flags
	%done = icmp eq i32 %stop, 0
	br i1 %done, label %exit, label %choice
choice:
	%pick = icmp eq i32 %stop, 1
	br i1 %pick, label %a, label %b
exit:
	ret void
}
