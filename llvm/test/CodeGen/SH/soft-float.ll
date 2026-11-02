; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=SOFT,STATIC
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=SOFT,STATIC
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=SOFT,PIC
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=SOFT,PIC
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=SOFT,STATIC
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=SOFT,STATIC
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SH4 --implicit-check-not='xf{{[0-9]+}}' --implicit-check-not='xd{{[0-9]+}}' --implicit-check-not=frchg
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SH4 --implicit-check-not='xf{{[0-9]+}}' --implicit-check-not='xd{{[0-9]+}}' --implicit-check-not=frchg
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -relocation-model=pic -O0 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -relocation-model=pic -O0 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O1 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -filetype=obj -verify-machineinstrs < %s -o /dev/null
; RUN: llvm-as < %s -o %t.bc
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs %t.bc -o /dev/null

; SOFT-NOT: fadd
; SOFT-NOT: fsub
; SOFT-NOT: fmul
; SOFT-NOT: fdiv
; SOFT-NOT: fr{{[0-9]+}}
; SOFT-NOT: xf{{[0-9]+}}
; SOFT-NOT: dr{{[0-9]+}}
; SOFT-NOT: xd{{[0-9]+}}
; SOFT-NOT: fpul
; SOFT-NOT: fpscr
; STATIC: .long __addsf3
; STATIC: .long __subsf3
; STATIC: .long __mulsf3
; STATIC: .long __divsf3
; STATIC: .long __adddf3
; STATIC: .long __subdf3
; STATIC: .long __muldf3
; STATIC: .long __divdf3
; STATIC: .long fmodf
; STATIC: .long fmod
; PIC: .long __addsf3@PLT
; PIC: .long __subsf3@PLT
; PIC: .long __mulsf3@PLT
; PIC: .long __divsf3@PLT
; PIC: .long __adddf3@PLT
; PIC: .long __subdf3@PLT
; PIC: .long __muldf3@PLT
; PIC: .long __divdf3@PLT
; PIC: .long fmodf@PLT
; PIC: .long fmod@PLT

; SH4-LABEL: f32_arithmetic:
; SH4: fsub fr{{[0-9]+}},fr{{[0-9]+}}
; SH4: fadd fr{{[0-9]+}},fr{{[0-9]+}}
; SH4: fmul fr{{[0-9]+}},fr{{[0-9]+}}
; SH4: fdiv fr{{[0-9]+}},fr{{[0-9]+}}
; SH4-NOT: __addsf3
; SH4-LABEL: f64_arithmetic:
; SH4: fsub dr{{[0-9]+}},dr{{[0-9]+}}
; SH4: fadd dr{{[0-9]+}},dr{{[0-9]+}}
; SH4: fmul dr{{[0-9]+}},dr{{[0-9]+}}
; SH4: fdiv dr{{[0-9]+}},dr{{[0-9]+}}
; SH4-NOT: __adddf3
; SH4: .long fmodf
; SH4: .long fmod
; SH4-LABEL: f32_compare:
; SH4: fcmp/eq
; SH4: fcmp/gt
; SH4: movt
; SH4-LABEL: f64_compare:
; SH4: fcmp/eq
; SH4: fcmp/gt
; SH4: movt
; SH4-LABEL: i32_conversions:
; SH4: ftrc fr{{[0-9]+}},fpul
; SH4: rotr
; SH4: float fpul,fr{{[0-9]+}}
; SH4: ftrc dr{{[0-9]+}},fpul
; SH4: .long __floatunsisf
; SH4-NOT: __fixunssfsi
; SH4-LABEL: f32_to_u32:
; SH4: fcmp/gt
; SH4: fsub
; SH4: ftrc
; SH4: rotr
; SH4-NOT: __fixunssfsi
; SH4-LABEL: f64_to_u32:
; SH4: fcmp/gt
; SH4: fsub
; SH4: ftrc
; SH4: rotr
; SH4-NOT: __fixunsdfsi
; SH4-LABEL: i64_conversions:
; SH4: .long __floatdidf
; SH4: .long __fixsfdi
; SH4-LABEL: width_conversions:
; SH4: fcnvsd fpul,dr{{[0-9]+}}
; SH4: fadd dr{{[0-9]+}},dr{{[0-9]+}}
; SH4: fcnvds dr{{[0-9]+}},fpul
; SH4-LABEL: composition:
; SH4: fmov @r{{[0-9]+}},fr{{[0-9]+}}
; SH4: fneg fr{{[0-9]+}}
; SH4: fabs fr{{[0-9]+}}
; SH4: fmov fr{{[0-9]+}},@r{{[0-9]+}}
; SH4-LABEL: multiply_add:
; SH4: fmac fr0,fr{{[0-9]+}},fr{{[0-9]+}}
; SH4-LABEL: multiply_add_double:
; SH4: fmul dr{{[0-9]+}},dr{{[0-9]+}}
; SH4: fadd dr{{[0-9]+}},dr{{[0-9]+}}
; SH4-LABEL: classify_f32:
; SH4: fcmp/eq fr{{[0-9]+}},fr{{[0-9]+}}
; SH4-LABEL: classify_f64:
; SH4: fcmp/eq dr{{[0-9]+}},dr{{[0-9]+}}
; SH4-LABEL: add_one_f32:
; SH4: fldi1 fr{{[0-9]+}}
; SH4: fadd fr{{[0-9]+}},fr{{[0-9]+}}

define float @f32_arithmetic(float %a, float %b) {
	%add = fadd float %a, %b
	%sub = fsub float %a, %b
	%mul = fmul float %add, %sub
	%result = fdiv float %mul, %b
	ret float %result
}

define double @f64_arithmetic(double %a, double %b) {
	%add = fadd double %a, %b
	%sub = fsub double %a, %b
	%mul = fmul double %add, %sub
	%result = fdiv double %mul, %b
	ret double %result
}

define float @f32_remainder(float %a, float %b) {
	%result = frem float %a, %b
	ret float %result
}

define double @f64_remainder(double %a, double %b) {
	%result = frem double %a, %b
	ret double %result
}

; SOFT-DAG: .long __eqsf2{{(@PLT)?}}
; SOFT-DAG: .long __nesf2{{(@PLT)?}}
; SOFT-DAG: .long __ltsf2{{(@PLT)?}}
; SOFT-DAG: .long __lesf2{{(@PLT)?}}
; SOFT-DAG: .long __gtsf2{{(@PLT)?}}
; SOFT-DAG: .long __gesf2{{(@PLT)?}}
; SOFT-DAG: .long __unordsf2{{(@PLT)?}}
define i32 @f32_compare(float %a, float %b) {
	%eq = fcmp oeq float %a, %b
	%ne = fcmp une float %a, %b
	%lt = fcmp olt float %a, %b
	%le = fcmp ole float %a, %b
	%gt = fcmp ogt float %a, %b
	%ge = fcmp oge float %a, %b
	%uo = fcmp uno float %a, %b
	%eq.i = zext i1 %eq to i32
	%ne.i = zext i1 %ne to i32
	%lt.i = zext i1 %lt to i32
	%le.i = zext i1 %le to i32
	%gt.i = zext i1 %gt to i32
	%ge.i = zext i1 %ge to i32
	%uo.i = zext i1 %uo to i32
	%p0 = or i32 %eq.i, %ne.i
	%p1 = or i32 %lt.i, %le.i
	%p2 = or i32 %gt.i, %ge.i
	%p3 = or i32 %p0, %p1
	%p4 = or i32 %p2, %uo.i
	%result = or i32 %p3, %p4
	ret i32 %result
}

define i32 @f32_oeq(float %a, float %b) {
	%result = fcmp oeq float %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f32_une(float %a, float %b) {
	%result = fcmp une float %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f32_olt(float %a, float %b) {
	%result = fcmp olt float %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f32_ole(float %a, float %b) {
	%result = fcmp ole float %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f32_ogt(float %a, float %b) {
	%result = fcmp ogt float %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f32_oge(float %a, float %b) {
	%result = fcmp oge float %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f32_uno(float %a, float %b) {
	%result = fcmp uno float %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

; SOFT-DAG: .long __eqdf2{{(@PLT)?}}
; SOFT-DAG: .long __nedf2{{(@PLT)?}}
; SOFT-DAG: .long __ltdf2{{(@PLT)?}}
; SOFT-DAG: .long __ledf2{{(@PLT)?}}
; SOFT-DAG: .long __gtdf2{{(@PLT)?}}
; SOFT-DAG: .long __gedf2{{(@PLT)?}}
; SOFT-DAG: .long __unorddf2{{(@PLT)?}}
define i32 @f64_compare(double %a, double %b) {
	%eq = fcmp oeq double %a, %b
	%ne = fcmp une double %a, %b
	%lt = fcmp olt double %a, %b
	%le = fcmp ole double %a, %b
	%gt = fcmp ogt double %a, %b
	%ge = fcmp oge double %a, %b
	%uo = fcmp uno double %a, %b
	%eq.i = zext i1 %eq to i32
	%ne.i = zext i1 %ne to i32
	%lt.i = zext i1 %lt to i32
	%le.i = zext i1 %le to i32
	%gt.i = zext i1 %gt to i32
	%ge.i = zext i1 %ge to i32
	%uo.i = zext i1 %uo to i32
	%p0 = or i32 %eq.i, %ne.i
	%p1 = or i32 %lt.i, %le.i
	%p2 = or i32 %gt.i, %ge.i
	%p3 = or i32 %p0, %p1
	%p4 = or i32 %p2, %uo.i
	%result = or i32 %p3, %p4
	ret i32 %result
}

define i32 @f64_oeq(double %a, double %b) {
	%result = fcmp oeq double %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f64_une(double %a, double %b) {
	%result = fcmp une double %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f64_olt(double %a, double %b) {
	%result = fcmp olt double %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f64_ole(double %a, double %b) {
	%result = fcmp ole double %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f64_ogt(double %a, double %b) {
	%result = fcmp ogt double %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f64_oge(double %a, double %b) {
	%result = fcmp oge double %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

define i32 @f64_uno(double %a, double %b) {
	%result = fcmp uno double %a, %b
	%extended = zext i1 %result to i32
	ret i32 %extended
}

; SOFT-DAG: .long __floatsisf{{(@PLT)?}}
; SOFT-DAG: .long __floatunsisf{{(@PLT)?}}
; SOFT-DAG: .long __fixsfsi{{(@PLT)?}}
; SOFT-DAG: .long __fixunssfsi{{(@PLT)?}}
; SOFT-DAG: .long __fixdfsi{{(@PLT)?}}
define i32 @i32_conversions(i32 %s, i32 %u, float %f, double %d) {
	%ssf = sitofp i32 %s to float
	%usf = uitofp i32 %u to float
	%sdf = sitofp i32 %s to double
	%udf = uitofp i32 %u to double
	%fs = fptosi float %f to i32
	%fu = fptoui float %usf to i32
	%ds = fptosi double %d to i32
	%du = fptoui double %udf to i32
	%a = bitcast float %ssf to i32
	%b = bitcast float %usf to i32
	%p0 = xor i32 %a, %b
	%p1 = xor i32 %fs, %fu
	%p2 = xor i32 %ds, %du
	%p3 = xor i32 %p0, %p1
	%p4 = xor i32 %p2, %s
	%result = xor i32 %p3, %p4
	ret i32 %result
}

define i32 @f32_to_u32(float %value) {
	%result = fptoui float %value to i32
	ret i32 %result
}

define i32 @f64_to_u32(double %value) {
	%result = fptoui double %value to i32
	ret i32 %result
}

define double @s32_to_f64(i32 %value) {
	%result = sitofp i32 %value to double
	ret double %result
}

define float @u32_to_f32(i32 %value) {
	%result = uitofp i32 %value to float
	ret float %result
}

; SOFT-DAG: .long __floatdidf{{(@PLT)?}}
; SOFT-DAG: .long __floatundisf{{(@PLT)?}}
; SOFT-DAG: .long __fixunssfdi{{(@PLT)?}}
; SOFT-DAG: .long __fixsfdi{{(@PLT)?}}
; SOFT-DAG: .long __floatundidf{{(@PLT)?}}
; SOFT-DAG: .long __fixdfdi{{(@PLT)?}}
; SOFT-DAG: .long __fixunsdfdi{{(@PLT)?}}
define i64 @i64_conversions(i64 %s, i64 %u, float %f, double %d) {
	%ssf = sitofp i64 %s to float
	%usf = uitofp i64 %u to float
	%sdf = sitofp i64 %s to double
	%udf = uitofp i64 %u to double
	%fs = fptosi float %f to i64
	%fu = fptoui float %usf to i64
	%ds = fptosi double %d to i64
	%du = fptoui double %udf to i64
	%a = bitcast double %sdf to i64
	%b = bitcast double %udf to i64
	%p0 = xor i64 %a, %b
	%p1 = xor i64 %fs, %fu
	%p2 = xor i64 %ds, %du
	%p3 = xor i64 %p0, %p1
	%p4 = xor i64 %p2, %s
	%result = xor i64 %p3, %p4
	ret i64 %result
}

define float @s64_to_f32(i64 %value) {
	%result = sitofp i64 %value to float
	ret float %result
}

define double @u64_to_f64(i64 %value) {
	%result = uitofp i64 %value to double
	ret double %result
}

; SOFT-DAG: .long __extendsfdf2{{(@PLT)?}}
; SOFT-DAG: .long __truncdfsf2{{(@PLT)?}}
define float @width_conversions(float %f, double %d) {
	%extended = fpext float %f to double
	%combined = fadd double %extended, %d
	%result = fptrunc double %combined to float
	ret float %result
}

declare float @llvm.fabs.f32(float)
declare float @llvm.copysign.f32(float, float)
declare float @llvm.fmuladd.f32(float, float, float)
declare double @llvm.fmuladd.f64(double, double, double)
declare i1 @llvm.is.fpclass.f32(float, i32 immarg)
declare i1 @llvm.is.fpclass.f64(double, i32 immarg)

define float @composition(ptr %address, float %other, i32 %condition) {
	%loaded = load float, ptr %address, align 4
	%negative = fneg float %loaded
	%absolute = call float @llvm.fabs.f32(float %other)
	%signed = call float @llvm.copysign.f32(float %absolute, float %negative)
	%take_negative = icmp ne i32 %condition, 0
	%selected = select i1 %take_negative, float %negative, float %signed
	store float %selected, ptr %address, align 4
	ret float %selected
}

; SOFT-DAG: .long __mulsf3{{(@PLT)?}}
; SOFT-DAG: .long __addsf3{{(@PLT)?}}
define float @multiply_add(float %a, float %b, float %c) {
	%result = call float @llvm.fmuladd.f32(float %a, float %b, float %c)
	ret float %result
}

; SOFT-DAG: .long __muldf3{{(@PLT)?}}
; SOFT-DAG: .long __adddf3{{(@PLT)?}}
define double @multiply_add_double(double %a, double %b, double %c) {
	%result = call double @llvm.fmuladd.f64(double %a, double %b, double %c)
	ret double %result
}

define i32 @classify_f32(float %value) {
	%classified = call i1 @llvm.is.fpclass.f32(float %value, i32 3)
	%result = zext i1 %classified to i32
	ret i32 %result
}

define i32 @classify_f64(double %value) {
	%classified = call i1 @llvm.is.fpclass.f64(double %value, i32 3)
	%result = zext i1 %classified to i32
	ret i32 %result
}

define float @add_one_f32(float %value) {
	%result = fadd float %value, 1.000000e+00
	ret float %result
}
