; RUN: llc -mtriple=shle -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=sh -mcpu=sh2 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle -mcpu=sh3 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=sh -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle -mcpu=sh2 -O3 -enable-new-pm=1 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=sh -mcpu=sh2 -O0 -enable-new-pm=1 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR

; Keep the original no-loop failure, including control flow and PHI. The
; nonzero path must return an odd low word and a zero high word for any choice.
; MIR-LABEL: name: bench_entry
; MIR-NOT: IMPLICIT_DEF
; MIR: MOVri 0
; MIR-NOT: IMPLICIT_DEF
; MIR-LABEL: name: literal_i1
; MIR-NOT: IMPLICIT_DEF
; MIR: MOVri 0
; MIR-NOT: IMPLICIT_DEF
; MIR-LABEL: name: literal_i8
; MIR-NOT: IMPLICIT_DEF
; MIR: MOVri 0
; MIR-NOT: IMPLICIT_DEF
; MIR-LABEL: name: literal_i16
; MIR-NOT: IMPLICIT_DEF
; MIR: MOVri 0
; MIR-NOT: IMPLICIT_DEF
; MIR-LABEL: name: literal_i64
; MIR-NOT: IMPLICIT_DEF
; MIR: MOVri 0
; MIR-NOT: IMPLICIT_DEF
; MIR-LABEL: name: split_partial

define i64 @bench_entry(ptr %data, ptr %aux, i32 %n, i32 noundef %ignored) {
entry:
	%seed = freeze i32 poison
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %body
body:
	%added = add i32 %seed, 1
	br label %exit
exit:
	%total = phi i32 [ %seed, %entry ], [ %added, %body ]
	%live = xor i32 %total, %seed
	%wide = zext i32 %live to i64
	ret i64 %wide
}

define i1 @literal_i1(ptr %out) {
	%f = freeze i1 undef
	store i1 %f, ptr %out
	ret i1 %f
}

define i8 @literal_i8(ptr %out) {
	%f = freeze i8 poison
	store i8 %f, ptr %out
	ret i8 %f
}

define i16 @literal_i16(ptr %out) #0 {
	%f = freeze i16 undef
	store i16 %f, ptr %out
	ret i16 %f
}

define i64 @literal_i64(ptr %out) #1 {
	%f = freeze i64 poison
	store i64 %f, ptr %out
	ret i64 %f
}

; The unknown high half must not erase the defined low half.
; MIR: COPY $r4
; MIR: MOVri 0
; MIR: $r{{[01]}} = COPY
; MIR-LABEL: name: defined
; MIR: COPY $r4
; MIR: $r0 = COPY

define i64 @split_partial(i32 noundef %low) {
	%base = zext i32 %low to i64
	%unknown = and i64 undef, -4294967296
	%partial = or i64 %base, %unknown
	%f = freeze i64 %partial
	ret i64 %f
}

define i32 @defined(i32 %x) {
	%f = freeze i32 %x
	ret i32 %f
}

attributes #0 = { optsize }
attributes #1 = { minsize optsize }
