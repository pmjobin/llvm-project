// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -O0 -emit-llvm -o - %s | FileCheck %s --check-prefix=HARD
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefix=SOFT
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh4 -mfloat-abi=hard -O1 -S -mllvm -verify-machineinstrs -o %t.s %s
// RUN: FileCheck %s --check-prefix=ASM < %t.s
// RUN: FileCheck %s --check-prefix=GPR < %t.s

struct single_float {
	float value;
};

struct single_double {
	double value;
};

struct float_pair {
	float first;
	float second;
};

struct double_pair {
	double first;
	double second;
};

struct mixed_pair {
	int integer;
	float single;
};

typedef float _Complex complex_float;
typedef double _Complex complex_double;

// HARD: define{{.*}} float @pass_float(float{{.*}} %value)
// SOFT: define{{.*}} float @pass_float(float{{.*}} %value)
// ASM-LABEL: pass_float:
// ASM: rts
// ASM-NEXT: fmov fr5,fr0
float pass_float(float value) {
	return value;
}

// HARD: define{{.*}} double @pass_double(double{{.*}} %value)
// SOFT: define{{.*}} double @pass_double(double{{.*}} %value)
// ASM-LABEL: pass_double:
// ASM: fmov fr4,fr0
// ASM-NEXT: rts
// ASM-NEXT: fmov fr5,fr1
double pass_double(double value) {
	return value;
}

// HARD: define{{.*}} float @mixed(i32{{.*}} %integer, float{{.*}} %single, double{{.*}} %wide, i32{{.*}} %tail)
// ASM-LABEL: mixed:
// ASM: add r5,r4
// ASM: fadd fr5,{{fr[0-9]+}}
// ASM: fcnvds dr6,fpul
float mixed(int integer, float single, double wide, int tail) {
	return single + (float)(integer + tail) + (float)wide;
}

// HARD: define{{.*}} float @pass_single_float(float %value.coerce)
// SOFT: define{{.*}} %struct.single_float @pass_single_float(%struct.single_float %value.coerce)
// ASM-LABEL: pass_single_float:
// ASM: rts
// ASM-NEXT: fmov fr5,fr0
struct single_float pass_single_float(struct single_float value) {
	return value;
}

// HARD: define{{.*}} double @pass_single_double(double %value.coerce)
// SOFT: define{{.*}} %struct.single_double @pass_single_double(%struct.single_double %value.coerce)
// ASM-LABEL: pass_single_double:
// ASM: fmov fr4,fr0
// ASM-NEXT: rts
// ASM-NEXT: fmov fr5,fr1
struct single_double pass_single_double(struct single_double value) {
	return value;
}

// HARD: define{{.*}} i64 @pass_float_pair(ptr{{.*}} byval(%struct.float_pair){{.*}} %value)
// ASM-LABEL: pass_float_pair:
// ASM: mov r5,r1
// ASM-NEXT: mov r4,r0
struct float_pair pass_float_pair(struct float_pair value) {
	return value;
}

// A by-value aggregate that cannot fit wholly in the remaining GPR window is
// passed wholly on the stack.  It does not consume r6/r7, so the later
// two-word aggregate uses those registers.
// HARD: define{{.*}} i32 @aggregate_boundary(ptr{{.*}} byval(%struct.float_pair){{.*}} %first, ptr{{.*}} byval(%struct.double_pair){{.*}} %wide, ptr{{.*}} byval(%struct.mixed_pair){{.*}} %later)
// ASM-LABEL: aggregate_boundary:
// ASM: add #-16,r15
// ASM: lds r7,fpul
// ASM-NEXT: fsts fpul,[[LATER:fr[0-9]+]]
// ASM: ftrc [[LATER]],fpul
// ASM-NEXT: sts fpul,{{r[0-9]+}}
// ASM: lds r4,fpul
// ASM-NEXT: fsts fpul,[[FIRST:fr[0-9]+]]
// ASM: ftrc [[FIRST]],fpul
// ASM-NEXT: sts fpul,{{r[0-9]+}}
// ASM: lds r5,fpul
// ASM-NEXT: fsts fpul,[[SECOND:fr[0-9]+]]
// ASM: ftrc [[SECOND]],fpul
// ASM-NEXT: sts fpul,{{r[0-9]+}}
// ASM: add #16,r15
// Check the independent integer data flow separately from the conversions.
// The low words of the two doubles are at incoming SP+0 and SP+8; the
// 16-byte local frame shifts those addresses to SP+16 and SP+24.
// GPR-LABEL: aggregate_boundary:
// GPR: add #-16,r15
// GPR-DAG: mov.l @(16,r15),[[WIDE_FIRST:r[0-9]+]]
// GPR-DAG: mov.l @(24,r15),[[WIDE_SUM:r[0-9]+]]
// GPR-DAG: add [[WIDE_FIRST]],[[WIDE_SUM]]
// GPR-DAG: add r6,[[WIDE_SUM]]
// GPR-DAG: sts fpul,[[LATER_INT:r[0-9]+]]
// GPR: add [[WIDE_SUM]],[[LATER_INT]]
// GPR: add [[LATER_INT]],[[FIRST_INT:r[0-9]+]]
// GPR: add [[FIRST_INT]],[[SECOND_INT:r[0-9]+]]
// GPR: mov [[SECOND_INT]],[[RESULT:r[0-9]+]]
// GPR: add #16,r15
// GPR: rts
// GPR-NEXT: mov [[RESULT]],r0
int aggregate_boundary(struct float_pair first, struct double_pair wide, struct mixed_pair later) {
	union {
		double fp;
		unsigned long long bits;
	} first_wide = {wide.first}, second_wide = {wide.second};
	return (int)first_wide.bits + (int)second_wide.bits + later.integer + (int)later.single + (int)first.first + (int)first.second;
}

// The same whole-aggregate rule applies when only r7 remains: the two-word
// aggregate starts on the stack and the later scalar uses r7.
// HARD: define{{.*}} i32 @small_aggregate_boundary(i32{{.*}} %first, i32{{.*}} %second, i32{{.*}} %third, ptr{{.*}} byval(%struct.float_pair){{.*}} %value, i32{{.*}} %tail)
// ASM-LABEL: small_aggregate_boundary:
// ASM: add #-4,r15
// ASM: mov #4,r0
// ASM-NEXT: fmov.s @(r0,r15),[[FIRST:fr[0-9]+]]
// Preserve both incoming stack offsets and their conversion sources, including
// the first conversion before a second load can reuse its floating register.
// ASM: ftrc [[FIRST]],fpul
// ASM-NEXT: sts fpul,{{r[0-9]+}}
// ASM: mov #8,r0
// ASM-NEXT: fmov.s @(r0,r15),[[SECOND:fr[0-9]+]]
// ASM: ftrc [[SECOND]],fpul
// ASM-NEXT: sts fpul,{{r[0-9]+}}
// ASM: add #4,r15
// Integer adds may occur before or after the second load. All four incoming
// scalar registers must contribute to the sum of the converted stack values.
// GPR-LABEL: small_aggregate_boundary:
// GPR: add #-4,r15
// GPR-DAG: add r5,r4
// GPR-DAG: add r6,r4
// GPR-DAG: add r7,r4
// GPR-DAG: sts fpul,[[FIRST_INT:r[0-9]+]]
// GPR: add r4,[[FIRST_INT]]
// GPR: sts fpul,[[SECOND_INT:r[0-9]+]]
// GPR: add [[FIRST_INT]],[[SECOND_INT]]
// GPR: mov [[SECOND_INT]],[[RESULT:r[0-9]+]]
// GPR: add #4,r15
// GPR: rts
// GPR-NEXT: mov [[RESULT]],r0
int small_aggregate_boundary(int first, int second, int third, struct float_pair value, int tail) {
	return first + second + third + (int)value.first + (int)value.second + tail;
}

// GNU SH treats complex scalar values as FPU modes rather than ordinary
// aggregates.  Preserve their element types in IR so LLVM can use fr0/fr1
// and dr0/dr2 for returns.
// HARD: define{{.*}} { float, float } @pass_complex_float({ float, float }{{.*}} %value.coerce)
// SOFT: define{{.*}} { float, float } @pass_complex_float({ float, float }{{.*}} %value.coerce)
// ASM-LABEL: pass_complex_float:
// ASM-DAG: fmov fr4,fr0
// ASM-DAG: fmov fr5,fr1
complex_float pass_complex_float(complex_float value) {
	return value;
}

// HARD: define{{.*}} { double, double } @pass_complex_double({ double, double }{{.*}} %value.coerce)
// SOFT: define{{.*}} void @pass_complex_double(ptr{{.*}} sret({ double, double }){{.*}}, ptr{{.*}} byval({ double, double })
// ASM-LABEL: pass_complex_double:
// ASM-DAG: fmov fr4,fr0
// ASM-DAG: fmov fr5,fr1
// ASM-DAG: fmov fr6,fr2
// ASM-DAG: fmov fr7,fr3
complex_double pass_complex_double(complex_double value) {
	return value;
}

// HARD: define{{.*}} double @read_variadic(i32{{.*}} %count, ...)
// HARD: alloca %struct.__va_list_tag
// HARD: call void @llvm.va_start.p0
// HARD: getelementptr inbounds nuw %struct.__va_list_tag, ptr {{.*}}, i32 0, i32 2
// HARD: getelementptr inbounds nuw %struct.__va_list_tag, ptr {{.*}}, i32 0, i32 3
// HARD: call void @llvm.va_end.p0
// SOFT: alloca ptr
// ASM-LABEL: read_variadic:
// ASM-DAG: mov.l r5,@({{[0-9]+}},r13)
// ASM-DAG: mov.l r6,@({{[0-9]+}},r13)
// ASM-DAG: mov.l r7,@({{[0-9]+}},r13)
// ASM-DAG: fmov.s fr5,@-r0
// ASM-DAG: fmov.s fr4,@-r0
// ASM-DAG: fmov.s fr7,@-r0
// ASM-DAG: fmov.s fr6,@-r0
// ASM-DAG: fmov.s fr9,@-r0
// ASM-DAG: fmov.s fr8,@-r0
// ASM-DAG: fmov.s fr11,@-r0
// ASM-DAG: fmov.s fr10,@-r0
double read_variadic(int count, ...) {
	__builtin_va_list values;
	__builtin_va_start(values, count);
	double result = __builtin_va_arg(values, double);
	struct single_float aggregate = __builtin_va_arg(values, struct single_float);
	__builtin_va_end(values);
	return result + aggregate.value;
}

typedef float (*callback_type)(int, float, double, int);

// HARD: define{{.*}} float @call_indirect(ptr{{.*}} %callback, i32{{.*}} %integer, float{{.*}} %single, double{{.*}} %wide, i32{{.*}} %tail)
// HARD: call float %{{.*}}(i32{{.*}}, float{{.*}}, double{{.*}}, i32{{.*}})
// ASM-LABEL: call_indirect:
// ASM: mov r4,r0
// ASM-NEXT: mov r5,r4
// ASM-NEXT: jsr @r0
// ASM-NEXT: mov r6,r5
float call_indirect(callback_type callback, int integer, float single, double wide, int tail) {
	return callback(integer, single, wide, tail);
}

// HARD: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-hard-float"}
// SOFT: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}
