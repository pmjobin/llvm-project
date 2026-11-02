; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=O0

@CurrentScope = external global ptr

declare void @use(ptr)

declare void @target()

define ptr @across_two_calls() {
entry:
	%first = load ptr, ptr @CurrentScope, align 4
	call void @use(ptr %first)
	%second = load ptr, ptr @CurrentScope, align 4
	call void @use(ptr %second)
	%third = load ptr, ptr @CurrentScope, align 4
	ret ptr %third
}

define ptr @across_two_calls_optsize() #0 {
entry:
	%first = load ptr, ptr @CurrentScope, align 4
	call void @use(ptr %first)
	%second = load ptr, ptr @CurrentScope, align 4
	call void @use(ptr %second)
	%third = load ptr, ptr @CurrentScope, align 4
	ret ptr %third
}

define ptr @across_two_calls_minsize() #1 {
entry:
	%first = load ptr, ptr @CurrentScope, align 4
	call void @use(ptr %first)
	%second = load ptr, ptr @CurrentScope, align 4
	call void @use(ptr %second)
	%third = load ptr, ptr @CurrentScope, align 4
	ret ptr %third
}

define ptr @across_two_calls_optnone() #2 {
entry:
	%first = load ptr, ptr @CurrentScope, align 4
	call void @use(ptr %first)
	%second = load ptr, ptr @CurrentScope, align 4
	call void @use(ptr %second)
	%third = load ptr, ptr @CurrentScope, align 4
	ret ptr %third
}

define ptr @preserve_shared_address() {
entry:
	%first = load ptr, ptr @CurrentScope, align 4
	call void @use(ptr %first)
	%second = load ptr, ptr @CurrentScope, align 4
	call void @use(ptr %second)
	%third = load ptr, ptr @CurrentScope, align 4
	call void @use(ptr %third)
	%fourth = load ptr, ptr @CurrentScope, align 4
	ret ptr %fourth
}

define ptr @preserve_function_address() {
entry:
	call void @use(ptr @target)
	call void @use(ptr @target)
	ret ptr @target
}

; The optional address-rematerialization pass is omitted. The conventional
; allocator preserves the base across calls and retains every memory read.
; OPT-LABEL: across_two_calls:
; OPT: mov.l [[ADDRESS:.LCPI[0-9_]+]],[[BASE:r[0-9]+]]
; OPT-DAG: jsr
; OPT-DAG: mov.l @[[BASE]],r4
; OPT-DAG: jsr
; OPT-DAG: mov.l @[[BASE]],r4
; OPT-DAG: mov.l @[[BASE]],r0
; OPT: rts

; OPT-LABEL: across_two_calls_optsize:
; OPT: mov.l [[OPTSIZE_ADDRESS:.LCPI[0-9_]+]],[[BASE:r[0-9]+]]
; OPT-DAG: jsr
; OPT-DAG: mov.l @[[BASE]],r4
; OPT-DAG: jsr
; OPT-DAG: mov.l @[[BASE]],r4
; OPT-DAG: mov.l @[[BASE]],r0
; OPT: rts

; OPT-LABEL: across_two_calls_minsize:
; OPT: mov.l [[MINSIZE_ADDRESS:.LCPI[0-9_]+]],[[BASE:r[0-9]+]]
; OPT-DAG: jsr
; OPT-DAG: mov.l @[[BASE]],r4
; OPT-DAG: jsr
; OPT-DAG: mov.l @[[BASE]],r4
; OPT-DAG: mov.l @[[BASE]],r0
; OPT: rts

; optnone and the code-generation -O0 level retain the original shared address.
; OPT-LABEL: across_two_calls_optnone:
; OPT: mov.l	[[OPTNONE_ADDRESS:.LCPI[0-9_]+]],r{{[0-9]+}}
; OPT: jsr
; OPT-NOT: mov.l	[[OPTNONE_ADDRESS]],r{{[0-9]+}}
; OPT: jsr
; OPT-NOT: mov.l	[[OPTNONE_ADDRESS]],r{{[0-9]+}}
; OPT: [[OPTNONE_ADDRESS]]:

; Three post-call copies would cost more than preserving the shared data
; address. Function addresses are outside this data-symbol transformation.
; OPT-LABEL: preserve_shared_address:
; OPT: mov.l	[[SHARED_ADDRESS:.LCPI[0-9_]+]],r{{[0-9]+}}
; OPT: jsr
; OPT-NOT: mov.l	[[SHARED_ADDRESS]],r{{[0-9]+}}
; OPT: jsr
; OPT-NOT: mov.l	[[SHARED_ADDRESS]],r{{[0-9]+}}
; OPT: jsr
; OPT-NOT: mov.l	[[SHARED_ADDRESS]],r{{[0-9]+}}
; OPT: [[SHARED_ADDRESS]]:

; OPT-LABEL: preserve_function_address:
; OPT: mov.l	[[FUNCTION_ADDRESS:.LCPI[0-9_]+]],r{{[0-9]+}}
; OPT: jsr
; OPT-NOT: mov.l	[[FUNCTION_ADDRESS]],r{{[0-9]+}}
; OPT: jsr
; OPT-NOT: mov.l	[[FUNCTION_ADDRESS]],r{{[0-9]+}}
; OPT: [[FUNCTION_ADDRESS]]:
; OPT-NEXT: .long	target

; O0-LABEL: across_two_calls:
; O0: mov.l	[[O0_ADDRESS:.LCPI[0-9_]+]],r{{[0-9]+}}
; O0: jsr
; O0-NOT: mov.l	[[O0_ADDRESS]],r{{[0-9]+}}
; O0: jsr
; O0-NOT: mov.l	[[O0_ADDRESS]],r{{[0-9]+}}
; O0: [[O0_ADDRESS]]:

attributes #0 = { optsize }
attributes #1 = { minsize optsize }
attributes #2 = { noinline optnone }
