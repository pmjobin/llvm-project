; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -relocation-model=static -function-sections -data-sections -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -relocation-model=static -function-sections -data-sections -filetype=obj < %s -o %t.le.o
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -relocation-model=pic -function-sections -data-sections -filetype=obj < %s -o %t.pic.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -relocation-model=pic -function-sections -data-sections -filetype=obj < %s -o %t.pic.le.o
; RUN: llvm-readobj --file-headers --sections --section-groups --symbols --relocations --hex-dump=.rodata.member_pointers %t.be.o | FileCheck %s --check-prefixes=OBJECT,BE
; RUN: llvm-readobj --file-headers --sections --section-groups --symbols --relocations --hex-dump=.rodata.member_pointers %t.le.o | FileCheck %s --check-prefixes=OBJECT,LE
; RUN: llvm-readobj --sections --relocations %t.pic.be.o | FileCheck %s --check-prefix=PIC
; RUN: llvm-readobj --sections --relocations %t.pic.le.o | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -relocation-model=static -function-sections -data-sections -filetype=obj < %s -o %t.sh4.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -relocation-model=static -function-sections -data-sections -filetype=obj < %s -o %t.sh4.le.o
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -relocation-model=pic -function-sections -data-sections -filetype=obj < %s -o %t.sh4.pic.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -relocation-model=pic -function-sections -data-sections -filetype=obj < %s -o %t.sh4.pic.le.o
; RUN: llvm-readobj --file-headers --sections --section-groups --symbols --relocations --hex-dump=.rodata.member_pointers %t.sh4.be.o | FileCheck %s --check-prefixes=OBJECT,BE,SH4
; RUN: llvm-readobj --file-headers --sections --section-groups --symbols --relocations --hex-dump=.rodata.member_pointers %t.sh4.le.o | FileCheck %s --check-prefixes=OBJECT,LE,SH4
; RUN: llvm-readobj --sections --relocations %t.sh4.pic.be.o | FileCheck %s --check-prefix=PIC
; RUN: llvm-readobj --sections --relocations %t.sh4.pic.le.o | FileCheck %s --check-prefix=PIC

$_ZTV6Object = comdat any
$_ZN6Object6inlineEv = comdat any

@_ZTI6Object = external global ptr
@_ZTV6Object = linkonce_odr constant [5 x ptr] [ptr null, ptr @_ZTI6Object, ptr @_ZN6ObjectD1Ev, ptr @_ZN6ObjectD0Ev, ptr @_ZN6Object6methodEv], comdat, align 4
@secondary_vtable_offset_to_top = constant ptr inttoptr (i32 -8 to ptr), align 4
@member_pointers = constant [5 x i32] [i32 -1, i32 0, i32 ptrtoint (ptr @_ZN6Object6methodEv to i32), i32 0, i32 9], align 4
@guard_byte = internal global i1 false, align 4
@global_object = global i32 0, align 4
@llvm.global_ctors = appending global [1 x { i32, ptr, ptr }] [{ i32, ptr, ptr } { i32 65535, ptr @_GLOBAL__sub_I_object, ptr null }]

declare ptr @observe_object(ptr)

define void @_ZN6ObjectC2Ev(ptr %this) align 2 {
	ret void
}

@_ZN6ObjectC1Ev = dso_local alias void (ptr), ptr @_ZN6ObjectC2Ev

define void @_ZN6ObjectD1Ev(ptr %this) align 2 {
	ret void
}

define void @_ZN6ObjectD0Ev(ptr %this) align 2 {
	ret void
}

define void @_ZN6Object6methodEv(ptr %this) align 2 {
	ret void
}

define linkonce_odr void @_ZN6Object6inlineEv(ptr %this) comdat align 2 {
	ret void
}

define i32 @_Z12stack_objectv() align 2 {
	%object = alloca [2 x i32], align 4
	call void @_ZN6ObjectC1Ev(ptr noundef nonnull align 4 dereferenceable(8) %object)
	%past = getelementptr inbounds i32, ptr %object, i32 2
	%element = getelementptr inbounds i32, ptr %past, i32 -2
	%done = icmp eq ptr %element, %object
	%observed = call ptr @observe_object(ptr nonnull %object)
	%same = icmp eq ptr %observed, %object
	%both = and i1 %done, %same
	%result = zext i1 %both to i32
	ret i32 %result
}

declare void @llvm.trap()

define void @_ZN8AbstractD0Ev(ptr %this) align 2 {
	call void @llvm.trap()
	unreachable
}

define internal void @_GLOBAL__sub_I_object() align 2 {
	call void @_ZN6ObjectC1Ev(ptr @global_object)
	ret void
}

; OBJECT: Format: elf32-sh
; OBJECT: Machine: EM_SH
; SH4: Flags [ (0x9)
; OBJECT: Section {{.*}} .rela.text._ZN8AbstractD0Ev {
; OBJECT: R_SH_DIR32 abort
; OBJECT: }
; OBJECT: Section {{.*}} .rela.rodata._ZTV6Object {
; OBJECT-DAG: R_SH_DIR32 _ZTI6Object
; OBJECT-DAG: R_SH_DIR32 _ZN6ObjectD1Ev
; OBJECT-DAG: R_SH_DIR32 _ZN6ObjectD0Ev
; OBJECT-DAG: R_SH_DIR32 _ZN6Object6methodEv
; OBJECT: }
; OBJECT: Section {{.*}} .rela.rodata.member_pointers {
; OBJECT: R_SH_DIR32 _ZN6Object6methodEv
; OBJECT: }
; OBJECT: Section {{.*}} .rela.init_array {
; OBJECT: R_SH_DIR32 _GLOBAL__sub_I_object
; OBJECT: }

; OBJECT-DAG: Name: _ZN6ObjectC2Ev
; OBJECT-DAG: Binding: Global
; OBJECT-DAG: Type: Function
; OBJECT-DAG: Name: _ZN6ObjectC1Ev
; OBJECT-DAG: Binding: Global
; OBJECT-DAG: Type: Function
; OBJECT-DAG: Name: _ZTV6Object
; OBJECT-DAG: Binding: Weak
; OBJECT-DAG: Name: _ZN6Object6inlineEv
; OBJECT-DAG: Binding: Weak

; BE: Hex dump of section '.rodata.member_pointers':
; BE-NEXT: 0x00000000 ffffffff 00000000 00000000 00000000
; BE-NEXT: 0x00000010 00000009
; LE: Hex dump of section '.rodata.member_pointers':
; LE-NEXT: 0x00000000 ffffffff 00000000 00000000 00000000
; LE-NEXT: 0x00000010 09000000

; OBJECT: Groups {
; OBJECT-DAG: Type: COMDAT
; OBJECT-DAG: Signature: _ZTV6Object
; OBJECT-DAG: .rodata._ZTV6Object
; OBJECT-DAG: Signature: _ZN6Object6inlineEv
; OBJECT-DAG: .text._ZN6Object6inlineEv

; PIC: Name: .rela.text._ZN8AbstractD0Ev
; PIC: Name: .rela.data.rel.ro._ZTV6Object
; PIC: Name: .rela.init_array
; PIC: Section {{.*}} .rela.text._ZN8AbstractD0Ev {
; PIC-DAG: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; PIC-DAG: R_SH_PLT32 abort
; PIC: }
