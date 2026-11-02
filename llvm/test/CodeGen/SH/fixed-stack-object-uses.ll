; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=ADDRESS-ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=prolog-epilog %s -o - | FileCheck %s --check-prefix=ADDRESS-PEI

%Record = type { i32, [13 x i16], [34 x i8] }
%Packed = type <{ i8, i32, [67 x i8] }>
%Nested = type { [2 x %Record], %Packed }

@source = global [128 x i8] zeroinitializer, align 4
@destination = global [128 x i8] zeroinitializer, align 4
@published_address = global ptr null, align 4

declare void @llvm.memset.p0.i32(ptr captures(none), i8, i32, i1 immarg)
declare void @llvm.memcpy.p0.p0.i32(ptr captures(none), ptr captures(none), i32, i1 immarg)
declare void @llvm.memmove.p0.p0.i32(ptr captures(none), ptr captures(none), i32, i1 immarg)
declare void @llvm.lifetime.start.p0(ptr captures(none))
declare void @llvm.lifetime.end.p0(ptr captures(none))
declare i32 @llvm.objectsize.i32.p0(ptr, i1 immarg, i1 immarg, i1 immarg)
declare ptr @llvm.launder.invariant.group.p0(ptr)
declare ptr @llvm.strip.invariant.group.p0(ptr)
declare i32 @consume_eight(i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @inspect(ptr)
declare ptr @return_derived_pointer(ptr)
declare i32 @inspect_with_stack_args(i32, i32, i32, i32, ptr, i32)

define i32 @ordinary_fixed_address_call() {
entry:
	%object = alloca i32, align 4
	store i32 37, ptr %object, align 4
	%result = call i32 @inspect(ptr %object)
	ret i32 %result
}

; A pointer-valued fixed address may be stored in another supported local
; stack object, reloaded, passed to an ordinary helper, returned by that helper,
; and consumed before the original object's lifetime ends.
define i32 @stored_fixed_address_call() {
entry:
	%object = alloca i32, align 4
	%pointer = alloca ptr, align 4
	store i32 41, ptr %object, align 4
	store ptr %object, ptr %pointer, align 4
	%reloaded = load ptr, ptr %pointer, align 4
	%derived = call ptr @return_derived_pointer(ptr %reloaded)
	%result = load i32, ptr %derived, align 4
	ret i32 %result
}

define i32 @constant_offset_fixed_address_call() {
entry:
	%object = alloca [1024 x i8], align 4
	%subobject = getelementptr [1024 x i8], ptr %object, i32 0, i32 1000
	store i8 43, ptr %subobject, align 1
	%result = call i32 @inspect_with_stack_args(i32 1, i32 2, i32 3, i32 4, ptr %subobject, i32 5)
	ret i32 %result
}

define i32 @indirect_fixed_address_call(ptr %callee) {
entry:
	%object = alloca i32, align 4
	store i32 47, ptr %object, align 4
	%result = call i32 %callee(ptr %object)
	ret i32 %result
}

define i32 @nullable_fixed_address_phi(i32 %condition.input) {
entry:
	%object = alloca [2 x i32], align 4
	%subobject = getelementptr [2 x i32], ptr %object, i32 0, i32 1
	%condition = icmp ne i32 %condition.input, 0
	br i1 %condition, label %local, label %null

local:
	br label %merge

null:
	br label %merge

merge:
	%pointer = phi ptr [ %subobject, %local ], [ null, %null ]
	%comparison = icmp eq ptr %pointer, %subobject
	%result = zext i1 %comparison to i32
	ret i32 %result
}

define i32 @nullable_fixed_address_select(i32 %condition.input) {
entry:
	%object = alloca i32, align 4
	%condition = icmp ne i32 %condition.input, 0
	%pointer = select i1 %condition, ptr %object, ptr null
	%comparison = icmp eq ptr %pointer, %object
	%result = zext i1 %comparison to i32
	ret i32 %result
}

define void @publish_fixed_address(i32 %value) {
entry:
	%object = alloca i32, align 4
	store i32 %value, ptr %object, align 4
	store ptr %object, ptr @published_address, align 4
	ret void
}

define ptr @return_fixed_address(i32 %value) {
entry:
	%object = alloca i32, align 4
	store i32 %value, ptr %object, align 4
	ret ptr %object
}

define i32 @fixed_address_integer() {
entry:
	%object = alloca i32, align 4
	%integer = ptrtoint ptr %object to i32
	ret i32 %integer
}

define i32 @mixed_fixed_address_select(i32 %condition.input) {
entry:
	%object = alloca i32, align 4
	%condition = icmp ne i32 %condition.input, 0
	%pointer = select i1 %condition, ptr %object, ptr @source
	%value = load i32, ptr %pointer, align 4
	ret i32 %value
}

define i32 @mixed_block_address_select(i32 %condition.input) {
entry:
	%object = alloca i32, align 4
	%condition = icmp ne i32 %condition.input, 0
	%pointer = select i1 %condition, ptr %object, ptr blockaddress(@mixed_block_address_select, %target)
	%value = load i32, ptr %pointer, align 4
	br label %target

target:
	ret i32 %value
}

define void @inline_asm_fixed_address() {
entry:
	%object = alloca i32, align 4
	call void asm sideeffect "", "r"(ptr %object)
	ret void
}

; ADDRESS-ISEL-LABEL: name:            ordinary_fixed_address_call
; ADDRESS-ISEL: LEA_FI %stack.0.object, 0
; ADDRESS-ISEL: $r4 = COPY
; ADDRESS-ISEL: JSR
; ADDRESS-ISEL-LABEL: name:            stored_fixed_address_call
; ADDRESS-ISEL: LEA_FI %stack.0.object, 0
; ADDRESS-ISEL: MOVL_store_disp
; ADDRESS-ISEL: JSR
; ADDRESS-ISEL-LABEL: name:            constant_offset_fixed_address_call
; ADDRESS-ISEL: ADJCALLSTACKDOWN 8, 0
; ADDRESS-ISEL: LEA_FI %stack.0.object, 1000
; ADDRESS-ISEL: JSR
; ADDRESS-ISEL: ADJCALLSTACKUP 8, 0
; ADDRESS-ISEL-LABEL: name:            indirect_fixed_address_call
; ADDRESS-ISEL: LEA_FI %stack.0.object, 0
; ADDRESS-ISEL: JSR
; ADDRESS-ISEL-LABEL: name:            nullable_fixed_address_phi
; ADDRESS-ISEL: LEA_FI %stack.0.object, 4
; ADDRESS-ISEL-LABEL: name:            nullable_fixed_address_select
; ADDRESS-ISEL: LEA_FI %stack.0.object, 0
; ADDRESS-ISEL-LABEL: name:            publish_fixed_address
; ADDRESS-ISEL: LEA_FI %stack.0.object, 0
; ADDRESS-ISEL-LABEL: name:            return_fixed_address
; ADDRESS-ISEL: LEA_FI %stack.0.object, 0
; ADDRESS-ISEL-LABEL: name:            fixed_address_integer
; ADDRESS-ISEL: LEA_FI %stack.0.object, 0
; ADDRESS-ISEL-LABEL: name:            mixed_fixed_address_select
; ADDRESS-ISEL: LEA_FI %stack.0.object, 0
; ADDRESS-ISEL: PHI
; ADDRESS-ISEL-LABEL: name:            mixed_block_address_select
; ADDRESS-ISEL: LEA_FI %stack.0.object, 0
; ADDRESS-ISEL: PHI
; ADDRESS-ISEL-LABEL: name:            inline_asm_fixed_address
; ADDRESS-ISEL: LEA_FI %stack.0.object, 0
;
; ADDRESS-PEI-LABEL: name:            ordinary_fixed_address_call
; ADDRESS-PEI-NOT: LEA_FI
; ADDRESS-PEI: JSR
; ADDRESS-PEI-LABEL: name:            stored_fixed_address_call
; ADDRESS-PEI-NOT: LEA_FI
; ADDRESS-PEI: JSR
; ADDRESS-PEI-LABEL: name:            constant_offset_fixed_address_call
; ADDRESS-PEI-NOT: LEA_FI
; ADDRESS-PEI: JSR
; ADDRESS-PEI-LABEL: name:            indirect_fixed_address_call
; ADDRESS-PEI-NOT: LEA_FI
; ADDRESS-PEI: JSR
; ADDRESS-PEI-LABEL: name:            nullable_fixed_address_phi
; ADDRESS-PEI-NOT: LEA_FI
; ADDRESS-PEI-LABEL: name:            nullable_fixed_address_select
; ADDRESS-PEI-NOT: LEA_FI
; ADDRESS-PEI-LABEL: name:            publish_fixed_address
; ADDRESS-PEI-NOT: LEA_FI
; ADDRESS-PEI-LABEL: name:            return_fixed_address
; ADDRESS-PEI-NOT: LEA_FI
; ADDRESS-PEI-LABEL: name:            fixed_address_integer
; ADDRESS-PEI-NOT: LEA_FI
; ADDRESS-PEI-LABEL: name:            mixed_fixed_address_select
; ADDRESS-PEI-NOT: LEA_FI
; ADDRESS-PEI-LABEL: name:            mixed_block_address_select
; ADDRESS-PEI-NOT: LEA_FI
; ADDRESS-PEI-LABEL: name:            inline_asm_fixed_address
; ADDRESS-PEI-NOT: LEA_FI

define i32 @fixed_aggregate_intrinsics(i32 %index, i32 %fill.input) {
entry:
	%bytes = alloca [4096 x i8], align 1
	%record = alloca %Record, align 4
	%packed = alloca %Packed, align 1
	%nested = alloca %Nested, align 4
	%fill = trunc i32 %fill.input to i8
	call void @llvm.lifetime.start.p0(ptr %bytes)
	call void @llvm.memset.p0.i32(ptr align 1 %bytes, i8 %fill, i32 4096, i1 false)
	%record.first = getelementptr %Record, ptr %record, i32 0, i32 0
	call void @llvm.memset.p0.i32(ptr align 4 %record.first, i8 17, i32 64, i1 false)
	%packed.bytes = getelementptr %Packed, ptr %packed, i32 0, i32 2, i32 0
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 %packed.bytes, ptr align 1 @source, i32 67, i1 false)
	%nested.packed = getelementptr %Nested, ptr %nested, i32 0, i32 1
	%nested.cast = bitcast ptr %nested.packed to ptr
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 @destination, ptr align 1 %nested.cast, i32 32, i1 false)
	%masked = and i32 %index, 4095
	%element = getelementptr inbounds [4096 x i8], ptr %bytes, i32 0, i32 %masked
	%value = load i8, ptr %element, align 1
	call void @llvm.lifetime.end.p0(ptr %bytes)
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @local_memcpy_memmove(i32 %index, i32 %length) {
entry:
	%source.local = alloca [512 x i8], align 4
	%destination.local = alloca [512 x i8], align 2
	call void @llvm.memset.p0.i32(ptr align 4 %source.local, i8 42, i32 512, i1 false)
	call void @llvm.memcpy.p0.p0.i32(ptr align 2 %destination.local, ptr align 4 %source.local, i32 512, i1 false)
	%source.partial = getelementptr inbounds i8, ptr %source.local, i32 7
	%destination.partial = getelementptr inbounds i8, ptr %destination.local, i32 5
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 %destination.partial, ptr align 1 %source.partial, i32 31, i1 false)
	call void @llvm.memmove.p0.p0.i32(ptr align 1 %destination.partial, ptr align 1 %destination.local, i32 64, i1 false)
	call void @llvm.memmove.p0.p0.i32(ptr align 1 %destination.local, ptr align 1 %destination.partial, i32 64, i1 false)
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 %destination.local, ptr align 1 %source.local, i32 0, i1 false)
	call void @llvm.memmove.p0.p0.i32(ptr align 1 %destination.local, ptr align 1 %source.local, i32 0, i1 false)
	call void @llvm.memcpy.p0.p0.i32(ptr align 1 %destination.local, ptr align 1 %source.local, i32 %length, i1 false)
	%masked = and i32 %index, 511
	%element = getelementptr inbounds [512 x i8], ptr %destination.local, i32 0, i32 %masked
	%value = load i8, ptr %element, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @volatile_local_intrinsics(i32 %index) {
entry:
	%source.local = alloca [16 x i8], align 4
	%destination.local = alloca [16 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %source.local, i8 52, i32 16, i1 true)
	call void @llvm.memcpy.p0.p0.i32(ptr align 4 %destination.local, ptr align 4 %source.local, i32 16, i1 true)
	call void @llvm.memmove.p0.p0.i32(ptr align 4 %source.local, ptr align 4 %destination.local, i32 16, i1 true)
	%masked = and i32 %index, 15
	%element = getelementptr inbounds [16 x i8], ptr %source.local, i32 0, i32 %masked
	%value = load volatile i8, ptr %element, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @selected_local_pointer(i32 %condition.input, i32 %index) {
entry:
	%left = alloca [64 x i8], align 4
	%right = alloca [64 x i8], align 4
	%condition = icmp ne i32 %condition.input, 0
	%selected = select i1 %condition, ptr %left, ptr %right
	call void @llvm.memset.p0.i32(ptr align 4 %selected, i8 85, i32 64, i1 false)
	%masked = and i32 %index, 63
	%element = getelementptr inbounds i8, ptr %selected, i32 %masked
	%value = load i8, ptr %element, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @loop_phi_local_pointer(i32 %iterations) {
entry:
	%bytes = alloca [32 x i8], align 1
	br label %loop

loop:
	%index = phi i32 [ 0, %entry ], [ %next.index, %loop ]
	%pointer = phi ptr [ %bytes, %entry ], [ %next.pointer, %loop ]
	store i8 102, ptr %pointer, align 1
	%next.pointer = getelementptr inbounds i8, ptr %pointer, i32 1
	%next.index = add nuw i32 %index, 1
	%continue = icmp ult i32 %next.index, %iterations
	br i1 %continue, label %loop, label %exit

exit:
	%value = load i8, ptr %bytes, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @invariant_local_pointer() {
entry:
	%bytes = alloca [16 x i8], align 4
	%laundered = call ptr @llvm.launder.invariant.group.p0(ptr %bytes)
	%stripped = call ptr @llvm.strip.invariant.group.p0(ptr %laundered)
	store i32 287454020, ptr %stripped, align 4
	%size = call i32 @llvm.objectsize.i32.p0(ptr %stripped, i1 false, i1 true, i1 false)
	ret i32 %size
}

define i32 @atomic_local_access(i32 %input) {
entry:
	%slot = alloca i32, align 4
	store atomic i32 %input, ptr %slot seq_cst, align 4
	%value = load atomic i32, ptr %slot seq_cst, align 4
	ret i32 %value
}

define i32 @local_with_outgoing_arguments(i32 %input) {
entry:
	%bytes = alloca [1024 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %bytes, i8 119, i32 1024, i1 false)
	%called = call i32 @consume_eight(i32 %input, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7)
	%last = getelementptr inbounds [1024 x i8], ptr %bytes, i32 0, i32 1023
	%value = load i8, ptr %last, align 1
	%extended = zext i8 %value to i32
	%result = add i32 %called, %extended
	ret i32 %result
}
