; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/dynamic-alloca.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/large-frame.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/overaligned.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/alloca-escape.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/store-pointer-global.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/store-pointer-unknown.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/ordinary-call.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/inline-asm.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/ptrtoint.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/mixed-select.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/mixed-phi.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/generic-intrinsics.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/direct-memory-call.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/memory-call-result-escape.ll -o /dev/null
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/address-space-cast.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/nonzero-alloca.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/blockaddress.ll -o /dev/null
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/stack-probe.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=STACK-PROBE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/frame-address-depth.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-DEPTH
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/return-address-depth.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-DEPTH
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/unrepresentable-alignment.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ALIGNMENT

; STACK-PROBE: error: {{.*}}SH stack probing is not supported
; ADDRESS-SPACE: error: {{.*}}SH nonzero address spaces are not supported
; ADDRESS-DEPTH: error: {{.*}}SH frame and return address intrinsics only support depth zero
; ALIGNMENT: error: {{.*}}SH stack object alignment cannot exceed 1073741824 bytes

;--- dynamic-alloca.ll
define i32 @dynamic_alloca(i32 %count, i32 %value) {
	%slot = alloca i32, i32 %count, align 4
	store volatile i32 %value, ptr %slot, align 4
	%result = load volatile i32, ptr %slot, align 4
	ret i32 %result
}

;--- frame-address-depth.ll
declare ptr @llvm.frameaddress.p0(i32 immarg)

define ptr @frame_address_depth_one() {
	%address = call ptr @llvm.frameaddress.p0(i32 1)
	ret ptr %address
}

;--- return-address-depth.ll
declare ptr @llvm.returnaddress(i32 immarg)

define ptr @return_address_depth_one() {
	%address = call ptr @llvm.returnaddress(i32 1)
	ret ptr %address
}

;--- large-frame.ll
define void @large_frame(i32 %value) {
	%slots = alloca [16385 x i32], align 4
	%last = getelementptr [16385 x i32], ptr %slots, i32 0, i32 16384
	store volatile i32 %value, ptr %slots, align 4
	store volatile i32 %value, ptr %last, align 4
	ret void
}

;--- unrepresentable-alignment.ll
define void @unrepresentable_alignment(i32 %value) {
	%object = alloca i32, align 2147483648
	store volatile i32 %value, ptr %object, align 2147483648
	ret void
}

;--- overaligned.ll
define i32 @overaligned(i32 %value) {
	%slot = alloca i32, align 8
	store volatile i32 %value, ptr %slot, align 4
	%result = load volatile i32, ptr %slot, align 4
	ret i32 %result
}

;--- alloca-escape.ll
define ptr @alloca_escape() {
	%slot = alloca i32, align 4
	ret ptr %slot
}

;--- store-pointer-global.ll
@escaped = global ptr null, align 4

define void @store_pointer_global() {
	%slot = alloca i32, align 4
	store ptr %slot, ptr @escaped, align 4
	ret void
}

;--- store-pointer-unknown.ll
define void @store_pointer_unknown(ptr %destination) {
	%slot = alloca i32, align 4
	store ptr %slot, ptr %destination, align 4
	ret void
}

; An ordinary call operand is representable through the fixed FrameIndex
; address-materialization path, independently of capture attributes.
;
;--- ordinary-call.ll
declare void @ordinary(ptr)

define void @ordinary_call() {
	%slot = alloca i32, align 4
	call void @ordinary(ptr %slot)
	ret void
}

;--- inline-asm.ll
define void @inline_asm() {
	%slot = alloca i32, align 4
	call void asm sideeffect "", "r"(ptr %slot)
	ret void
}

;--- ptrtoint.ll
define i32 @local_ptrtoint() {
	%slot = alloca i32, align 4
	%integer = ptrtoint ptr %slot to i32
	ret i32 %integer
}

;--- mixed-select.ll
@global = global i32 0, align 4

define i32 @mixed_select(i32 %condition.input) {
	%condition = icmp ne i32 %condition.input, 0
	%slot = alloca i32, align 4
	%selected = select i1 %condition, ptr %slot, ptr @global
	%value = load i32, ptr %selected, align 4
	ret i32 %value
}

;--- mixed-phi.ll
@global = global i32 0, align 4

define i32 @mixed_phi(i32 %condition.input) {
entry:
	%slot = alloca i32, align 4
	%condition = icmp ne i32 %condition.input, 0
	br i1 %condition, label %local, label %global
local:
	br label %join
global:
	br label %join
join:
	%selected = phi ptr [ %next, %loop ], [ %slot, %local ], [ @global, %global ]
	br label %loop
loop:
	%next = getelementptr i8, ptr %selected, i32 0
	br i1 %condition, label %join, label %exit
exit:
	%value = load i32, ptr %selected, align 4
	ret i32 %value
}

;--- generic-intrinsics.ll
declare ptr @llvm.ptrmask.p0.i32(ptr, i32)
declare void @llvm.sideeffect()

define i32 @generic_intrinsics() {
	%slot = alloca i32, align 4
	%masked = call ptr @llvm.ptrmask.p0.i32(ptr %slot, i32 -4)
	%value = load i32, ptr %masked, align 4
	call void @llvm.sideeffect()
	ret i32 %value
}

;--- direct-memory-call.ll
declare ptr @memset(ptr, i32, i32)

define i32 @direct_memory_call() {
	%slot = alloca [64 x i8], align 4
	%ignored = call ptr @memset(ptr %slot, i32 0, i32 64)
	%value = load i8, ptr %slot, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

;--- memory-call-result-escape.ll
declare ptr @memset(ptr captures(none), i32, i32)

define ptr @memory_call_result_escape() {
	%slot = alloca [64 x i8], align 4
	%result = call ptr @memset(ptr captures(none) %slot, i32 0, i32 64)
	ret ptr %result
}

;--- address-space-cast.ll
define i32 @address_space_cast() {
	%slot = alloca i32, align 4
	%converted = addrspacecast ptr %slot to ptr addrspace(1)
	%value = load i32, ptr addrspace(1) %converted, align 4
	ret i32 %value
}

;--- nonzero-alloca.ll
define i32 @nonzero_alloca() {
	%slot = alloca i32, align 4, addrspace(1)
	store i32 1, ptr addrspace(1) %slot, align 4
	%value = load i32, ptr addrspace(1) %slot, align 4
	ret i32 %value
}

;--- blockaddress.ll
define i32 @blockaddress_local(i32 %condition.input) {
entry:
	%slot = alloca i32, align 4
	%condition = icmp ne i32 %condition.input, 0
	%selected = select i1 %condition, ptr %slot, ptr blockaddress(@blockaddress_local, %target)
	%value = load i32, ptr %selected, align 4
	br label %target
target:
	ret i32 %value
}

;--- stack-probe.ll
define void @stack_probe() "probe-stack"="inline-asm" {
	%slot = alloca [1024 x i8], align 4
	store volatile i8 1, ptr %slot, align 1
	ret void
}

;--- offset-64.ll
define i32 @offset_64(ptr %p) {
	%address = getelementptr i8, ptr %p, i32 64
	%value = load volatile i32, ptr %address, align 4
	ret i32 %value
}

;--- negative-offset.ll
define i32 @negative_offset(ptr %p) {
	%address = getelementptr i8, ptr %p, i32 -4
	%value = load volatile i32, ptr %address, align 4
	ret i32 %value
}
