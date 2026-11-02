; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -exception-model=dwarf -relocation-model=static -verify-machineinstrs -filetype=obj < %s -o %t.static.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -exception-model=dwarf -relocation-model=static -verify-machineinstrs -filetype=obj < %s -o %t.static.le.o
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -exception-model=dwarf -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.pic.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -exception-model=dwarf -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.pic.le.o
; RUN: llvm-dwarfdump --verify %t.pic.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --verify %t.pic.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --eh-frame %t.pic.be.o | FileCheck %s --check-prefix=FRAME
; RUN: llvm-dwarfdump --eh-frame %t.pic.le.o | FileCheck %s --check-prefix=FRAME
; RUN: llvm-readobj --relocations %t.pic.be.o | FileCheck %s --check-prefix=PIC
; RUN: llvm-readobj --relocations %t.pic.le.o | FileCheck %s --check-prefix=PIC
; RUN: llvm-as %s -o %t.bc
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null

declare void @consume(ptr)
declare i32 @five(i32, i32, i32, i32, i32)
declare ptr @llvm.stacksave.p0()
declare void @llvm.stackrestore.p0(ptr)
declare i32 @__gxx_personality_v0(...)

; ASM-LABEL: one_vla:
; ASM: mov.l	r14,@{{.*}}r15
; ASM: mov	r15,r14
; ASM-NEXT: .cfi_def_cfa r14,
; ASM: sub	{{r[0-9]+}},{{r[0-9]+}}
; ASM: mov	{{r[0-9]+}},r15
; ASM: mov	r14,r15
; ASM: mov.l	@{{.*}}r15{{.*}},r14
define i32 @one_vla(i32 %count, i32 %value) uwtable {
	%fixed = alloca i32, align 4
	%dynamic = alloca i32, i32 %count, align 4
	store volatile i32 %value, ptr %fixed, align 4
	store volatile i32 %value, ptr %dynamic, align 4
	call void @consume(ptr %dynamic)
	%result = call i32 @five(i32 %value, i32 %value, i32 %value, i32 %value, i32 %value)
	%loaded = load volatile i32, ptr %fixed, align 4
	%sum = add i32 %result, %loaded
	ret i32 %sum
}

; ASM-LABEL: nested_conditional_vlas:
; ASM: mov	r15,r14
; ASM: sub	{{r[0-9]+}},{{r[0-9]+}}
; ASM: mov	{{r[0-9]+}},r15
; ASM: mov	{{r[0-9]+}},r15
; ASM: mov	r14,r15
define i32 @nested_conditional_vlas(i32 %outer.count, i32 %inner.count, i32 %condition, i32 %value) uwtable {
entry:
	%outer.saved = call ptr @llvm.stacksave.p0()
	%outer = alloca i32, i32 %outer.count, align 8
	store volatile i32 %value, ptr %outer, align 8
	%take.inner = icmp ne i32 %condition, 0
	br i1 %take.inner, label %inner, label %done

inner:
	%inner.saved = call ptr @llvm.stacksave.p0()
	%inner.object = alloca i16, i32 %inner.count, align 16
	store volatile i16 7, ptr %inner.object, align 16
	call void @consume(ptr %inner.object)
	call void @llvm.stackrestore.p0(ptr %inner.saved)
	br label %done

done:
	%result = load volatile i32, ptr %outer, align 8
	call void @llvm.stackrestore.p0(ptr %outer.saved)
	ret i32 %result
}

; ASM-LABEL: zero_size_vla:
; ASM: add	#-4,r15
; ASM: jsr
; ASM: add	#4,r15
define void @zero_size_vla() uwtable {
	%object = alloca i8, i32 0, align 4
	call void @consume(ptr %object)
	ret void
}

; ASM-LABEL: fixed_realign_16:
; ASM: mov	r15,r14
; ASM: and	{{r[0-9]+}},r15
; ASM: mov	r15,r13
; ASM: mov	r14,r15
define i32 @fixed_realign_16(i32 %value) uwtable {
	%object = alloca i32, align 16
	store volatile i32 %value, ptr %object, align 16
	call void @consume(ptr %object)
	%result = load volatile i32, ptr %object, align 16
	ret i32 %result
}

; ASM-LABEL: dynamic_realign_32:
; ASM: mov	r15,r14
; ASM: and	{{r[0-9]+}},r15
; ASM: mov	r15,r13
; ASM: and	{{r[0-9]+}},{{r[0-9]+}}
; ASM: mov	r14,r15
define i32 @dynamic_realign_32(i32 %count, i32 %value) uwtable {
	%fixed = alloca i32, align 32
	%dynamic = alloca i32, i32 %count, align 32
	store volatile i32 %value, ptr %fixed, align 32
	store volatile i32 %value, ptr %dynamic, align 32
	call void @consume(ptr %fixed)
	call void @consume(ptr %dynamic)
	%a = load volatile i32, ptr %fixed, align 32
	%b = load volatile i32, ptr %dynamic, align 32
	%sum = add i32 %a, %b
	ret i32 %sum
}

; ASM-LABEL: maximum_representable_alignment:
; ASM: mov	r15,r14
; ASM: and	{{r[0-9]+}},r15
; ASM: mov	r15,r13
; ASM: mov	r14,r15
define i32 @maximum_representable_alignment(i32 %value) uwtable {
	%object = alloca i32, align 1073741824
	store volatile i32 %value, ptr %object, align 1073741824
	%result = load volatile i32, ptr %object, align 1073741824
	ret i32 %result
}

; ASM-LABEL: vla_exception_cleanup:
; ASM: .cfi_personality
; ASM: mov	r15,r14
; ASM: mov	r14,r15
; ASM: .section	.gcc_except_table
define void @vla_exception_cleanup(i32 %count) uwtable personality ptr @__gxx_personality_v0 {
entry:
	%saved = call ptr @llvm.stacksave.p0()
	%object = alloca i32, i32 %count, align 16
	invoke void @consume(ptr %object) to label %normal unwind label %lpad

normal:
	call void @llvm.stackrestore.p0(ptr %saved)
	ret void

lpad:
	%landing = landingpad { ptr, i32 }
		cleanup
	call void @llvm.stackrestore.p0(ptr %saved)
	resume { ptr, i32 } %landing
}

; VERIFY-NOT: error:
; VERIFY: No errors.

; FRAME: DW_CFA_def_cfa: R14
; FRAME: DW_CFA_offset: R13
; FRAME: DW_CFA_offset: R14
; FRAME: DW_CFA_restore: R14
; FRAME: DW_CFA_restore: R13

; PIC: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; PIC: R_SH_PLT32 consume
; PIC: Section {{.*}} .rela.eh_frame {
; PIC: R_SH_REL32
; PIC-NOT: R_SH_DIR32 consume
