; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define i32 @eq(i32 %a, i32 %b) #0 {
; CHECK-LABEL: eq:
; CHECK: cmp/eq	r5,r4
; CHECK-NEXT: bf	{{.*}}
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{mov(\.l)?}}
	%condition = icmp eq i32 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 %a
false:
	ret i32 %b
}

define i32 @ne(i32 %a, i32 %b) #0 {
; CHECK-LABEL: ne:
; CHECK: cmp/eq	r5,r4
; CHECK-NEXT: bt	{{.*}}
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{mov(\.l)?}}
	%condition = icmp ne i32 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 %a
false:
	ret i32 %b
}

define i32 @sgt(i32 %a, i32 %b) #0 {
; CHECK-LABEL: sgt:
; CHECK: cmp/gt	r5,r4
; CHECK-NEXT: bf	{{.*}}
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{mov(\.l)?}}
	%condition = icmp sgt i32 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 %a
false:
	ret i32 %b
}

define i32 @sge(i32 %a, i32 %b) #0 {
; CHECK-LABEL: sge:
; CHECK: cmp/ge	r5,r4
; CHECK-NEXT: bf	{{.*}}
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{mov(\.l)?}}
	%condition = icmp sge i32 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 %a
false:
	ret i32 %b
}

define i32 @slt(i32 %a, i32 %b) #0 {
; CHECK-LABEL: slt:
; CHECK: cmp/ge	r5,r4
; CHECK-NEXT: bt	{{.*}}
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{mov(\.l)?}}
	%condition = icmp slt i32 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 %a
false:
	ret i32 %b
}

define i32 @sle(i32 %a, i32 %b) #0 {
; CHECK-LABEL: sle:
; CHECK: cmp/gt	r5,r4
; CHECK-NEXT: bt	{{.*}}
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{mov(\.l)?}}
	%condition = icmp sle i32 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 %a
false:
	ret i32 %b
}

define i32 @ugt(i32 %a, i32 %b) #0 {
; CHECK-LABEL: ugt:
; CHECK: cmp/hi	r5,r4
; CHECK-NEXT: bf	{{.*}}
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{mov(\.l)?}}
	%condition = icmp ugt i32 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 %a
false:
	ret i32 %b
}

define i32 @uge(i32 %a, i32 %b) #0 {
; CHECK-LABEL: uge:
; CHECK: cmp/hs	r5,r4
; CHECK-NEXT: bf	{{.*}}
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{mov(\.l)?}}
	%condition = icmp uge i32 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 %a
false:
	ret i32 %b
}

define i32 @ult(i32 %a, i32 %b) #0 {
; CHECK-LABEL: ult:
; CHECK: cmp/hs	r5,r4
; CHECK-NEXT: bt	{{.*}}
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{mov(\.l)?}}
	%condition = icmp ult i32 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 %a
false:
	ret i32 %b
}

define i32 @ule(i32 %a, i32 %b) #0 {
; CHECK-LABEL: ule:
; CHECK: cmp/hi	r5,r4
; CHECK-NEXT: bt	{{.*}}
; CHECK-NEXT: bra	{{.*}}
; CHECK-NEXT: nop
; CHECK: {{mov(\.l)?}}
	%condition = icmp ule i32 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 %a
false:
	ret i32 %b
}

attributes #0 = { noinline optnone }
