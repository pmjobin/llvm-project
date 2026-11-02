; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -relocation-model=static -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -relocation-model=static -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs %s -o /dev/null
; RUN: llvm-as %s -o %t.bc
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -relocation-model=static -verify-machineinstrs %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -relocation-model=static -verify-machineinstrs %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -filter-print-funcs=copy_packed -print-after=sh-prepare-ir -filetype=null %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=PREP
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -filter-print-funcs=copy_packed -print-after=sh-prepare-ir -filetype=null %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=PREP
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -enable-new-pm -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR

%S1 = type { i1 }
%S11 = type { i1, i1 }
%Mixed = type { i1, i8, i16, i32 }
%Nested = type { %S1, [3 x i1] }
%Packed = type <{ i1, i16 }>
%Padded = type { i8, i32, [2 x i16] }

@global_s1 = global %S1 { i1 true }, align 1

; ASM-LABEL: copy_aggregate_i1:
; ASM: mov.b	@r4,
; ASM: and
; ASM: mov.b	{{.*}},@r5
define void @copy_aggregate_i1(ptr %source, ptr %destination) {
entry:
	%value = load %S1, ptr %source, align 1
	store %S1 %value, ptr %destination, align 1
	ret void
}

; Two i1 fields occupy two independent bytes.
; ASM-LABEL: copy_two:
; ASM-COUNT-2: mov.b	@
; ASM-COUNT-2: mov.b	{{.*}},@
define void @copy_two(ptr %dst, ptr %src) {
	%value = load %S11, ptr %src, align 1
	store %S11 %value, ptr %dst, align 1
	ret void
}

; The mixed fields use their exact byte offsets and access widths. Padding is
; not copied, so no access can extend beyond a field or corrupt an adjacent
; field.
; ASM-LABEL: copy_mixed:
; ASM-COUNT-2: mov.b	@
; ASM-DAG: mov.w	@
; ASM-DAG: mov.l	@
; ASM-DAG: mov.l	{{.*}},@
; ASM-DAG: mov.w	{{.*}},@
; ASM-DAG: mov.b	{{.*}},@
; ASM-DAG: mov.b	{{.*}},@
define void @copy_mixed(ptr %dst, ptr %src) {
	%value = load %Mixed, ptr %src, align 4
	store %Mixed %value, ptr %dst, align 4
	ret void
}

; ASM-LABEL: copy_nested:
; ASM-COUNT-4: mov.b	@
; ASM-COUNT-4: mov.b	{{.*}},@
define void @copy_nested(ptr %dst, ptr %src) {
	%value = load %Nested, ptr %src, align 1
	store %Nested %value, ptr %dst, align 1
	ret void
}

; An unaligned i16 beside i1 is decomposed byte-accurately by scalar
; legalization. Native DAG flattening retains the field offset and alignment; the
; unaligned-integer-memory tests check byte significance for each endianness.
; ASM-LABEL: copy_packed:
; ASM-COUNT-3: mov.b	@
; ASM-NOT: mov.w	@
; ASM-COUNT-3: mov.b	{{.*}},@
define void @copy_packed(ptr %dst, ptr %src) {
	%value = load %Packed, ptr %src, align 1
	store %Packed %value, ptr %dst, align 1
	ret void
}

; PREP-LABEL: define void @copy_packed(
; PREP: %value = load %Packed, ptr %src, align 1
; PREP-NEXT: store %Packed %value, ptr %dst, align 1

define i32 @local_round_trip(i32 %input) {
	%local = alloca %S1, align 1
	%condition = icmp ne i32 %input, 0
	%value = insertvalue %S1 poison, i1 %condition, 0
	store volatile %S1 %value, ptr %local, align 1
	%loaded = load volatile %S1, ptr %local, align 1
	%field = extractvalue %S1 %loaded, 0
	%result = zext i1 %field to i32
	ret i32 %result
}

define i32 @global_round_trip(i32 %input) {
	%condition = icmp ne i32 %input, 0
	%value = insertvalue %S1 poison, i1 %condition, 0
	store volatile %S1 %value, ptr @global_s1, align 1
	%loaded = load volatile %S1, ptr @global_s1, align 1
	%field = extractvalue %S1 %loaded, 0
	%result = zext i1 %field to i32
	ret i32 %result
}

define void @pointer_derived(ptr %dst, ptr %src) {
	%derived.src = getelementptr i8, ptr %src, i32 3
	%derived.dst = getelementptr i8, ptr %dst, i32 5
	%value = load %S1, ptr %derived.src, align 1
	store %S1 %value, ptr %derived.dst, align 1
	ret void
}

define void @select_store(i32 %choice, ptr %dst) {
	%condition = icmp ne i32 %choice, 0
	%value = select i1 %condition, %S11 { i1 true, i1 false }, %S11 { i1 false, i1 true }
	store %S11 %value, ptr %dst, align 1
	ret void
}

; Native flattening preserves field offsets, widths, alignment and AA metadata.
; The bytes between the first and second field are padding, not memory operands.
; MIR-LABEL: name: copy_padded
; MIR-DAG: {{.*}} :: (load (s8) from %ir.src, align 4, !alias.scope {{![0-9]+}})
; MIR-DAG: {{.*}} :: (load (s32) from %ir.src + 4, !alias.scope {{![0-9]+}})
; MIR-DAG: {{.*}} :: (load (s16) from %ir.src + 8, align 4, !alias.scope {{![0-9]+}})
; MIR-DAG: {{.*}} :: (load (s16) from %ir.src + 10, basealign 4, !alias.scope {{![0-9]+}})
; MIR-DAG: {{.*}} :: (store (s8) into %ir.dst, align 4, !noalias {{![0-9]+}})
; MIR-DAG: {{.*}} :: (store (s32) into %ir.dst + 4, !noalias {{![0-9]+}})
; MIR-DAG: {{.*}} :: (store (s16) into %ir.dst + 8, align 4, !noalias {{![0-9]+}})
; MIR-DAG: {{.*}} :: (store (s16) into %ir.dst + 10, basealign 4, !noalias {{![0-9]+}})
define void @copy_padded(ptr %dst, ptr %src) {
  %value = load %Padded, ptr %src, align 4, !alias.scope !0
  store %Padded %value, ptr %dst, align 4, !noalias !0
  ret void
}

!0 = !{!1}
!1 = distinct !{!1, !2, !"scope"}
!2 = distinct !{!2, i1 false, !"domain"}
