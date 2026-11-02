; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %s -o %t.o3.le.o
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %s -o %t.o3.be.o
; RUN: llvm-readobj -h -r %t.le.o | FileCheck %s --check-prefixes=OBJ,LEOBJ
; RUN: llvm-readobj -h -r %t.be.o | FileCheck %s --check-prefixes=OBJ,BEOBJ
; RUN: llc -mtriple=shle-unknown-linux-musl -mcpu=sh2 -O2 -verify-machineinstrs -relocation-model=pic -filetype=obj %s -o %t.gnu.o
; RUN: llvm-readobj -h -r %t.gnu.o | FileCheck %s --check-prefix=GNU

@local_data = internal global i32 1, align 4
@hidden_data = hidden global i32 2, align 4
@default_data = global i32 3, align 4
@external_data = external global i32

@local_pointer = global ptr @local_function, align 4
@hidden_pointer = global ptr @hidden_function, align 4
@protected_pointer = global ptr @protected_function, align 4
@default_pointer = global ptr @default_function, align 4
@external_pointer = global ptr @external_function, align 4
@weak_pointer = global ptr @weak_function, align 4
@default_pointer_integer = global i32 ptrtoint (ptr @default_function to i32), align 4
@null_pointer = global ptr null, align 4
@callback_array = global [3 x ptr] [ptr @local_function, ptr @hidden_function, ptr @external_function], align 4
@mixed = global { ptr, ptr, i32 } { ptr @default_data, ptr @local_function, i32 7 }, align 4

define internal i32 @local_function(i32 %value) noinline {
	%data = load i32, ptr @local_data, align 4
	%result = add i32 %value, %data
	ret i32 %result
}

define hidden i32 @hidden_function(i32 %value) noinline {
	ret i32 %value
}

define protected i32 @protected_function(i32 %value) noinline {
	ret i32 %value
}

define i32 @default_function(i32 %value) noinline {
	ret i32 %value
}

declare i32 @external_function(i32)
declare extern_weak i32 @weak_function(i32)
declare extern_weak hidden i32 @hidden_weak_function(i32)
declare void @first_tail_callee()
declare void @second_tail_callee()
declare void @shared_tail_callee() noreturn

define ptr @address_local() {
	ret ptr @local_function
}

define ptr @address_hidden() {
	ret ptr @hidden_function
}

define ptr @address_protected() {
	ret ptr @protected_function
}

define ptr @address_external() {
	ret ptr @external_function
}

define ptr @address_default() {
	ret ptr @default_function
}

define i32 @address_default_integer() {
	ret i32 ptrtoint (ptr @default_function to i32)
}

define ptr @integer_function_pointer(i32 %value) {
	%pointer = inttoptr i32 %value to ptr
	ret ptr %pointer
}

define i32 @call_local(i32 %value) {
	%result = call i32 @local_function(i32 %value)
	ret i32 %result
}

define i32 @call_hidden(i32 %value) {
	%result = call i32 @hidden_function(i32 %value)
	ret i32 %result
}

define i32 @call_protected(i32 %value) {
	%result = call i32 @protected_function(i32 %value)
	ret i32 %result
}

define i32 @call_default(i32 %value) {
	%result = call i32 @default_function(i32 %value)
	ret i32 %result
}

define i32 @call_external(i32 %value) {
	%result = call i32 @external_function(i32 %value)
	ret i32 %result
}

define i32 @call_weak(i32 %value) {
	%result = call i32 @weak_function(i32 %value)
	ret i32 %result
}

define i32 @call_hidden_weak(i32 %value) {
	%result = call i32 @hidden_weak_function(i32 %value)
	ret i32 %result
}

define void @call_shared_tail(i1 %condition) {
	br i1 %condition, label %first, label %second

first:
	call void @first_tail_callee()
	br label %tail

second:
	call void @second_tail_callee()
	br label %tail

tail:
	call void @shared_tail_callee()
	unreachable
}

define i32 @call_indirect(ptr %callback, i32 %value) {
	%result = call i32 %callback(i32 %value)
	ret i32 %result
}

define ptr @return_callback(ptr %callback) {
	ret ptr %callback
}

define i1 @callback_is_null(ptr %callback) {
	%result = icmp eq ptr %callback, null
	ret i1 %result
}

define i1 @callbacks_equal(ptr %left, ptr %right) {
	%result = icmp eq ptr %left, %right
	ret i1 %result
}

define ptr @load_callback(ptr %slot) {
	%callback = load ptr, ptr %slot, align 4
	ret ptr %callback
}

define void @store_callback(ptr %slot, ptr %callback) {
	store ptr %callback, ptr %slot, align 4
	ret void
}

define i32 @call_then_global(i32 %value) {
	%result = call i32 @external_function(i32 %value)
	%data = load i32, ptr @external_data, align 4
	%sum = add i32 %result, %data
	ret i32 %sum
}

define i32 @call_indirect_then_global(ptr %callback, i32 %value) {
	%result = call i32 %callback(i32 %value)
	%data = load i32, ptr @external_data, align 4
	%sum = add i32 %result, %data
	ret i32 %sum
}

define internal i32 @callback_target(i32 %value) noinline {
	%result = call i32 @external_function(i32 %value)
	ret i32 %result
}

define i32 @callback_bridge(ptr %callback, i32 %value) noinline {
	%result = call i32 %callback(i32 %value)
	ret i32 %result
}

define i32 @direct_to_callback(i32 %value) {
	%result = call i32 @callback_bridge(ptr @callback_target, i32 %value)
	ret i32 %result
}

define i32 @callback_then_direct(ptr %callback, i32 %value) {
	%first = call i32 %callback(i32 %value)
	%second = call i32 @external_function(i32 %first)
	ret i32 %second
}

; ASM-LABEL: address_local:
; ASM-DAG: mov r12,[[LOCAL_BASE:r[0-9]+]]
; ASM-DAG: mov.l [[LOCAL_DESC:\.LCPI[0-9_]+]],[[LOCAL_OFFSET:r[0-9]+]]
; ASM: add [[LOCAL_BASE]],[[LOCAL_OFFSET]]
; ASM: [[LOCAL_DESC]]:
; ASM-NEXT: .long local_function@GOTOFFFUNCDESC

; ASM-LABEL: address_hidden:
; ASM: .long hidden_function@GOTOFFFUNCDESC

; ASM-LABEL: address_protected:
; ASM-DAG: mov r12,[[PROTECTED_BASE:r[0-9]+]]
; ASM-DAG: mov.l [[PROTECTED_DESC:\.LCPI[0-9_]+]],[[PROTECTED_SLOT:r[0-9]+]]
; ASM: mov.l @([[PROTECTED_BASE]],[[PROTECTED_SLOT]]),r0
; ASM: [[PROTECTED_DESC]]:
; ASM-NEXT: .long protected_function@GOTFUNCDESC

; ASM-LABEL: address_external:
; ASM: .long external_function@GOTFUNCDESC

; ASM-LABEL: address_default:
; ASM: .long default_function@GOTFUNCDESC

; ASM-LABEL: address_default_integer:
; ASM: .long default_function@GOTFUNCDESC

; ASM-LABEL: integer_function_pointer:
; ASM: rts
; ASM: mov r4,r0

; ASM-LABEL: call_local:
; ASM: mov.l {{.*}},r0
; ASM: bsrf r0
; ASM: .long local_function-{{.*}}

; ASM-LABEL: call_hidden:
; ASM: bsrf
; ASM: .long hidden_function-{{.*}}

; ASM-LABEL: call_protected:
; ASM: bsrf
; ASM: .long protected_function-{{.*}}

; ASM-LABEL: call_default:
; ASM: bsrf
; ASM: .long default_function@PLT-({{.*}}-{{.*}})

; ASM-LABEL: call_external:
; ASM: bsrf
; ASM: .long external_function@PLT-({{.*}}-{{.*}})

; ASM-LABEL: call_weak:
; ASM: bsrf
; ASM: .long weak_function@PLT-({{.*}}-{{.*}})

; ASM-LABEL: call_hidden_weak:
; ASM: bsrf
; ASM: .long hidden_weak_function-{{.*}}

; ASM-LABEL: call_indirect:
; ASM: mov.l @r4,r1
; ASM-NEXT: mov.l @(4,r4),r12
; ASM: jsr @r1

; ASM-LABEL: return_callback:
; ASM: rts
; ASM: mov r4,r0

; ASM-LABEL: callback_is_null:
; ASM: tst r4,r4

; ASM-LABEL: callbacks_equal:
; ASM: cmp/eq r5,r4

; ASM-LABEL: load_callback:
; ASM: mov.l @r4,r0

; ASM-LABEL: store_callback:
; ASM: mov.l r5,@r4

; ASM-LABEL: call_then_global:
; ASM: bsrf
; ASM-NEXT: mov r12,[[BASE:r[89]|r1[013]]]
; ASM: mov.l {{.*}},[[SLOT:r[0-9]+]]
; ASM: mov r0,[[RESULT:r[0-9]+]]
; ASM: mov [[SLOT]],r0
; ASM: mov.l @(r0,[[BASE]]),r0
; ASM: mov.l @r0,r0
; ASM: add [[RESULT]],r0
; ASM: .long external_function@PLT-({{.*}}-{{.*}})
; ASM: .long external_data@GOT

; ASM-LABEL: call_indirect_then_global:
; ASM: mov r12,[[INDIRECT_BASE:r[89]|r1[013]]]
; ASM: mov.l @r4,r1
; ASM-NEXT: mov.l @(4,r4),r12
; ASM: jsr @r1
; ASM-DAG: mov.l {{.*}},[[INDIRECT_SLOT:r[0-9]+]]
; ASM-DAG: mov [[INDIRECT_BASE]],r0
; ASM: mov.l @(r0,[[INDIRECT_SLOT]]),r0
; ASM: .long external_data@GOT

; ASM-LABEL: callback_target:
; ASM: bsrf
; ASM: .long external_function@PLT-({{.*}}-{{.*}})

; ASM-LABEL: callback_bridge:
; ASM: mov.l @r4,r1
; ASM-NEXT: mov.l @(4,r4),r12
; ASM: jsr @r1

; ASM-LABEL: direct_to_callback:
; ASM: .long callback_target@GOTOFFFUNCDESC
; ASM: .long callback_bridge@PLT-({{.*}}-{{.*}})

; ASM-LABEL: callback_then_direct:
; ASM: mov r12,[[CALLBACK_BASE:r[89]|r1[013]]]
; ASM: mov.l @r4,r1
; ASM-NEXT: mov.l @(4,r4),r12
; ASM: jsr @r1
; ASM: mov [[CALLBACK_BASE]],r12
; ASM: bsrf
; ASM: .long external_function@PLT-({{.*}}-{{.*}})

; ASM: default_pointer_integer:
; ASM-NEXT: .long default_function@FUNCDESC

; LEOBJ: Format: elf32-shl
; BEOBJ: Format: elf32-sh
; OBJ: Flags [ (0x8002)
; OBJ: 0x8000
; OBJ-DAG: R_SH_GOTOFF local_data
; OBJ-DAG: R_SH_GOTOFFFUNCDESC local_function
; OBJ-DAG: R_SH_GOTOFFFUNCDESC hidden_function
; OBJ-DAG: R_SH_GOTFUNCDESC protected_function
; OBJ-DAG: R_SH_GOTFUNCDESC external_function
; OBJ-DAG: R_SH_PLT32 default_function
; OBJ-DAG: R_SH_PLT32 external_function
; OBJ-DAG: R_SH_PLT32 weak_function
; OBJ-DAG: R_SH_REL32 hidden_weak_function
; OBJ-DAG: R_SH_FUNCDESC local_function
; OBJ-DAG: R_SH_FUNCDESC hidden_function
; OBJ-DAG: R_SH_FUNCDESC protected_function
; OBJ-DAG: R_SH_FUNCDESC default_function
; OBJ-DAG: R_SH_FUNCDESC external_function
; OBJ-DAG: R_SH_FUNCDESC weak_function

; GNU: Flags [ (0x2)
; GNU-NOT: 0x8000
; GNU-NOT: FUNCDESC
