; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -relocation-model=static -function-sections -data-sections -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -relocation-model=static -function-sections -data-sections -filetype=obj < %s -o %t.le.o
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -relocation-model=pic -function-sections -data-sections -filetype=obj < %s -o %t.pic.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -relocation-model=pic -function-sections -data-sections -filetype=obj < %s -o %t.pic.le.o
; RUN: llvm-readobj --file-headers --sections --section-groups --symbols --relocations %t.be.o | FileCheck %s --check-prefixes=OBJECT,STATIC
; RUN: llvm-readobj --file-headers --sections --section-groups --symbols --relocations %t.le.o | FileCheck %s --check-prefixes=OBJECT,STATIC
; RUN: llvm-readobj --file-headers --sections --section-groups --symbols --relocations %t.pic.be.o | FileCheck %s --check-prefixes=OBJECT,PIC
; RUN: llvm-readobj --file-headers --sections --section-groups --symbols --relocations %t.pic.le.o | FileCheck %s --check-prefixes=OBJECT,PIC
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 < %s | FileCheck %s --check-prefix=ASM

$_ZTV1D = comdat any
$_ZTC1D0_1A = comdat any
$_ZTC1D8_1B = comdat any
$_ZTT1D = comdat any

@_ZTI1D = external global ptr
@_ZTI1A = external global ptr
@_ZTI1B = external global ptr
@_ZTV1D = linkonce_odr constant [6 x ptr] [ptr inttoptr (i32 20 to ptr), ptr null, ptr @_ZTI1D, ptr inttoptr (i32 12 to ptr), ptr inttoptr (i32 -8 to ptr), ptr @_ZTI1D], comdat, align 4
@_ZTC1D0_1A = linkonce_odr constant [3 x ptr] [ptr inttoptr (i32 20 to ptr), ptr null, ptr @_ZTI1A], comdat, align 4
@_ZTC1D8_1B = linkonce_odr constant [3 x ptr] [ptr inttoptr (i32 12 to ptr), ptr null, ptr @_ZTI1B], comdat, align 4
@_ZTT1D = linkonce_odr constant [4 x ptr] [ptr getelementptr inbounds ([6 x ptr], ptr @_ZTV1D, i32 0, i32 3), ptr getelementptr inbounds ([3 x ptr], ptr @_ZTC1D0_1A, i32 0, i32 3), ptr getelementptr inbounds ([3 x ptr], ptr @_ZTC1D8_1B, i32 0, i32 3), ptr getelementptr inbounds ([6 x ptr], ptr @_ZTV1D, i32 0, i32 6)], comdat, align 4

declare void @llvm.experimental.noalias.scope.decl(metadata)

define void @_ZTv0_n12_N1D6resultEi(ptr noalias sret([3 x i32]) align 4 %result, ptr %this, i32 %value) align 2 {
entry:
	%vtable = load ptr, ptr %this, align 4
	%offset.address = getelementptr inbounds i8, ptr %vtable, i32 -12
	%offset = load i32, ptr %offset.address, align 4
	%adjusted.this = getelementptr inbounds i8, ptr %this, i32 %offset
	call void @llvm.experimental.noalias.scope.decl(metadata !2)
	%field = getelementptr inbounds i8, ptr %adjusted.this, i32 4
	%loaded = load i32, ptr %field, align 4, !noalias !2
	store i32 %loaded, ptr %result, align 4, !alias.scope !2
	%second = getelementptr inbounds i32, ptr %result, i32 1
	store i32 %value, ptr %second, align 4, !alias.scope !2
	ret void
}

; ASM-LABEL: _ZTv0_n12_N1D6resultEi:
; ASM: mov.l @r4,
; ASM: mov.l @{{r[0-9]+}},
; ASM: add
; ASM: rts

; OBJECT: Format: elf32-sh
; OBJECT: Machine: EM_SH
; OBJECT-DAG: Name: _ZTV1D
; OBJECT-DAG: Name: _ZTC1D0_1A
; OBJECT-DAG: Name: _ZTC1D8_1B
; OBJECT-DAG: Name: _ZTT1D
; OBJECT-DAG: Name: _ZTv0_n12_N1D6resultEi
; OBJECT-DAG: Signature: _ZTV1D
; OBJECT-DAG: Signature: _ZTC1D0_1A
; OBJECT-DAG: Signature: _ZTC1D8_1B
; OBJECT-DAG: Signature: _ZTT1D
; STATIC-DAG: R_SH_DIR32 _ZTI1D
; STATIC-DAG: R_SH_DIR32 _ZTI1A
; STATIC-DAG: R_SH_DIR32 _ZTI1B
; STATIC-DAG: R_SH_DIR32 _ZTV1D
; STATIC-DAG: R_SH_DIR32 _ZTC1D0_1A
; STATIC-DAG: R_SH_DIR32 _ZTC1D8_1B
; PIC-DAG: R_SH_DIR32 _ZTI1D
; PIC-DAG: R_SH_DIR32 _ZTI1A
; PIC-DAG: R_SH_DIR32 _ZTI1B

!0 = distinct !{!0, i1 false, !"D::result"}
!1 = distinct !{!1, !0, !"result argument"}
!2 = !{!1}
