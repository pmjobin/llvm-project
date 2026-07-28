; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=phi-node-elimination %s -o - | FileCheck %s --check-prefix=PHIELIM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=greedy %s -o - | FileCheck %s --check-prefix=RA

define i64 @diamond_i64(i32 %selector, i64 %left_value, i64 %right_value) {
entry:
	%condition = icmp ne i32 %selector, 0
	br i1 %condition, label %left, label %right
left:
	br label %done
right:
	br label %done
done:
	%result = phi i64 [ %left_value, %left ], [ %right_value, %right ]
	ret i64 %result
}

define i64 @critical_edge_i64(i32 %selector, i64 %direct_value, i64 %edge_value) {
entry:
	%condition = icmp ne i32 %selector, 0
	br i1 %condition, label %done, label %edge
edge:
	br label %done
done:
	%result = phi i64 [ %direct_value, %entry ], [ %edge_value, %edge ]
	ret i64 %result
}

define i64 @loop_i64(i32 %count, i64 %start) {
entry:
	br label %loop
loop:
	%index = phi i32 [ %count, %entry ], [ %next_index, %loop ]
	%value = phi i64 [ %start, %entry ], [ %next_value, %loop ]
	%next_value = add i64 %value, 1
	%next_index = sub i32 %index, 1
	%done = icmp eq i32 %next_index, 0
	br i1 %done, label %exit, label %loop
exit:
	ret i64 %next_value
}

define internal i64 @identity_i64(i64 %value) {
	ret i64 %value
}

define i64 @phi_around_call(i32 %selector, i64 %a, i64 %b) {
entry:
	%called = call i64 @identity_i64(i64 %a)
	%condition = icmp ne i32 %selector, 0
	br i1 %condition, label %left, label %right
left:
	br label %done
right:
	br label %done
done:
	%result = phi i64 [ %called, %left ], [ %b, %right ]
	ret i64 %result
}

define i64 @phi_around_shift(i32 %selector, i64 %value, i64 %count, i64 %other) {
entry:
	%shifted = shl i64 %value, %count
	%condition = icmp ne i32 %selector, 0
	br i1 %condition, label %left, label %right
left:
	br label %done
right:
	br label %done
done:
	%result = phi i64 [ %shifted, %left ], [ %other, %right ]
	ret i64 %result
}

; ISEL: name: diamond_i64
; ISEL-COUNT-2: PHI
; ISEL: name: critical_edge_i64
; ISEL-COUNT-2: PHI
; ISEL: name: loop_i64
; ISEL-COUNT-3: PHI
; ISEL: name: phi_around_call
; ISEL-COUNT-2: PHI
; ISEL: name: phi_around_shift
; ISEL-COUNT-3: PHI
; ISEL: SHLL
; ISEL-NEXT: ROTCL
; ISEL-COUNT-4: PHI
; ISEL-NOT: BR_CC64_PSEUDO

; PHIELIM-NOT: PHI
; RA-NOT: %{{[0-9]+}}
