; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O2 -enable-new-pm -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O0 -enable-new-pm -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O2 -enable-new-pm -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O0 -enable-new-pm -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O0 -print-after=sh-prepare-ir -filetype=null %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=PREP

; Ordinary aggregates reach native DAG flattening unchanged. Volatile aggregates
; retain ordered scalar preparation, with their type, alignment and volatility.
; Scalar legalization, including integer bit representations of FP memory,
; owns unaligned accesses. Integer IR builders must never receive FP values.
%F = type <{ i8, float }>
%D = type <{ i8, double }>
%N = type <{ i8, [2 x %D], ptr }>

; PREP-LABEL: define float @read_float(
; PREP: load %F, ptr %p, align 1
define float @read_float(ptr %p) {
  %a = load %F, ptr %p, align 1
  %v = extractvalue %F %a, 1
  ret float %v
}

; PREP-LABEL: define void @write_float(
; PREP: store %F %a, ptr %p, align 1
define void @write_float(ptr %p, float %v) {
  %a = insertvalue %F zeroinitializer, float %v, 1
  store %F %a, ptr %p, align 1
  ret void
}

; PREP-LABEL: define double @read_double(
; PREP: load %D, ptr %p, align 1
define double @read_double(ptr %p) {
  %a = load %D, ptr %p, align 1
  %v = extractvalue %D %a, 1
  ret double %v
}

; PREP-LABEL: define void @write_double(
; PREP: store %D %a, ptr %p, align 1
define void @write_double(ptr %p, double %v) {
  %a = insertvalue %D zeroinitializer, double %v, 1
  store %D %a, ptr %p, align 1
  ret void
}

; Includes nested arrays, pointers and a field crossing a word boundary.
; PREP-LABEL: define void @copy_nested(
; PREP: %a = load %N, ptr %src, align 1
; PREP-NEXT: store %N %a, ptr %dst, align 1
define void @copy_nested(ptr %dst, ptr %src) optsize {
  %a = load %N, ptr %src, align 1
  store %N %a, ptr %dst, align 1
  ret void
}

; PREP-LABEL: define void @copy_volatile(
; PREP: load volatile i8, ptr {{.*}}, align 1
; PREP: load volatile double, ptr {{.*}}, align 1
; PREP: store volatile i8 {{.*}}, ptr {{.*}}, align 1
; PREP: store volatile double {{.*}}, ptr {{.*}}, align 1
define void @copy_volatile(ptr %dst, ptr %src) minsize optsize {
  %a = load volatile %D, ptr %src, align 1
  store volatile %D %a, ptr %dst, align 1
  ret void
}
