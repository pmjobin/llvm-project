; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 < %s | FileCheck %s --check-prefix=JOIN
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -join-globalcopies=false < %s | FileCheck %s --check-prefix=NOJOIN

%struct.DCtab = type { i8, i8 }

@DC_long = external constant [32 x %struct.DCtab]

define internal fastcc void @test(ptr %decoder, i32 %cc, ptr %dest, i32 %stride, i32 %value, i1 %condition) optsize {
entry:
	%one = shl i32 1, 0
	br i1 %condition, label %if.then, label %if.end

if.then:
	%merged = or i32 %value, %one
	br label %if.end

if.end:
	%selected = phi i32 [ %merged, %if.then ], [ 1, %entry ]
	%byte = load i8, ptr @DC_long, align 1
	%offset = getelementptr i8, ptr %decoder, i32 476
	%word = load i32, ptr %offset, align 4
	%truncated = trunc i32 %word to i16
	store i16 %truncated, ptr %decoder, align 4
	store i16 0, ptr null, align 2
	%amount = zext i8 %byte to i32
	%shifted = shl i32 %selected, %amount
	%twice = shl i32 %shifted, 1
	store i32 %twice, ptr %decoder, align 4
	store ptr null, ptr null, align 4
	ret void
}

; JOIN-LABEL: test:
; JOIN: tst
; JOIN-NEXT: bt
; JOIN-NOT: bra
; JOIN: or r0,r1

; NOJOIN-LABEL: test:
; NOJOIN: tst
; NOJOIN-NEXT: b{{[ft]}}
; NOJOIN: bra
; NOJOIN-NEXT: or r1,r0
; NOJOIN: mov #1,r0
