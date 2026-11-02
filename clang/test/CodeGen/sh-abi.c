// RUN: %clang_cc1 -triple sh-unknown-none-elf -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,BE
// RUN: %clang_cc1 -triple shle-unknown-none-elf -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,LE
// RUN: %clang_cc1 -triple sh-unknown-none-elf -O1 -disable-llvm-passes -emit-llvm -o - %s | FileCheck %s --check-prefix=OPT
// RUN: %clang_cc1 -triple shle-unknown-none-elf -O1 -disable-llvm-passes -emit-llvm -o - %s | FileCheck %s --check-prefix=OPT
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh1 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,BE
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh1 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,LE
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh1 -pic-level 2 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,LE,PIC-SH1
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh1 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh1 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh1 -pic-level 2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -pic-level 2 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,LE,PIC
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -pic-level 2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -O2 -S -o - %s | FileCheck %s --check-prefix=GNU-ASM
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh3 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,BE
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh3 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,LE
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh3 -pic-level 2 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,LE,PIC-SH3
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh3 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh3 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh3 -pic-level 2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh3 -O2 -S -o - %s | FileCheck %s --check-prefix=GNU-ASM
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh4 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,BE
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,LE
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -pic-level 2 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,LE,PIC-SH4
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh4 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -pic-level 2 -Os -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh4 -O2 -S -o - %s | FileCheck %s --check-prefix=GNU-ASM
// RUN: %clang --target=sh-unknown-none-elf -m4al -O0 -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,BE,SH4AL
// RUN: %clang --target=shle-unknown-none-elf -m4al -O0 -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,LE,SH4AL
// RUN: %clang --target=sh-unknown-none-elf -m4al -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -m4al -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s

struct pair {
	int first;
	int second;
};

struct triple {
	int first;
	int second;
	int third;
};

extern int external_value;
__thread int tls_value;

// BE: target datalayout = "E-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
// LE: target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
// COMMON: @tls_value = {{.*}}thread_local global i32 0, align 4

// COMMON: define{{.*}} i32 @use_pair(%struct.pair %value.coerce)
int use_pair(struct pair value) {
	return value.first + value.second;
}

// COMMON: define{{.*}} i32 @use_triple(ptr{{.*}} byval(%struct.triple) align 4 %value)
int use_triple(struct triple value) {
	return value.first + value.second + value.third;
}

// COMMON: define{{.*}} %struct.pair @return_pair(i32{{.*}} %value)
struct pair return_pair(int value) {
	struct pair result = {value, value + 1};
	return result;
}

// COMMON: define{{.*}} void @return_triple(ptr{{.*}} sret(%struct.triple) align 4 %agg.result, i32{{.*}} %value)
struct triple return_triple(int value) {
	struct triple result = {value, value + 1, value + 2};
	return result;
}

// COMMON: define{{.*}} signext i8 @narrow_signed(i8{{.*}} signext %value)
signed char narrow_signed(signed char value) {
	return value;
}

// COMMON: define{{.*}} zeroext i16 @narrow_unsigned(i16{{.*}} zeroext %value)
unsigned short narrow_unsigned(unsigned short value) {
	return value;
}

extern _Bool external_bool(void);

// OPT-LABEL: define{{.*}} i32 @consume_bool()
// OPT: call zeroext i1 @external_bool()
// OPT: zext i1
int consume_bool(void) {
	return external_bool();
}

// COMMON: define{{.*}} i64 @wide(i64{{.*}} %value)
long long wide(long long value) {
	return value;
}

// COMMON: define{{.*}} i32 @five_scalars(i32{{.*}}, i32{{.*}}, i32{{.*}}, i32{{.*}}, i32{{.*}} %fifth)
int five_scalars(int first, int second, int third, int fourth, int fifth) {
	return first + second + third + fourth + fifth;
}

// COMMON: define{{.*}} i32 @first_vararg(i32{{.*}} %fixed, ...)
// COMMON: call void @llvm.va_start.p0(ptr
// COMMON: call void @llvm.va_end.p0(ptr
int first_vararg(int fixed, ...) {
	__builtin_va_list args;
	__builtin_va_start(args, fixed);
	int result = __builtin_va_arg(args, int);
	__builtin_va_end(args);
	return result;
}

// COMMON: define{{.*}} i32 @read_tls()
int read_tls(void) {
	return tls_value;
}

// COMMON: define hidden{{.*}} i32 @hidden_value()
__attribute__((visibility("hidden"))) int hidden_value(void) {
	return external_value;
}

// PIC: attributes #{{[0-9]+}} = {{.*}}"target-cpu"="sh2" "target-features"="+sh2"
// PIC: !{{[0-9]+}} = !{i32 8, !"PIC Level", i32 2}
// PIC-SH1: attributes #{{[0-9]+}} = {{.*}}"target-cpu"="sh1"
// PIC-SH1-NOT: "target-features"
// PIC-SH1: !{{[0-9]+}} = !{i32 8, !"PIC Level", i32 2}
// PIC-SH3: attributes #{{[0-9]+}} = {{.*}}"target-cpu"="sh3" "target-features"="+banked-gprs,+dynamic-shift,+exception-state,+mmu,+prefetch,+s-bit-control,+sh2"
// PIC-SH3: !{{[0-9]+}} = !{i32 8, !"PIC Level", i32 2}
// PIC-SH4: attributes #{{[0-9]+}} = {{.*}}"target-cpu"="sh4" "target-features"="+banked-gprs,+double-fpu,+dynamic-shift,+exception-state,+fpu-register-bank,+fpu-size-mode,+fpu-vector,+fsqrt,+mmu,+prefetch,+s-bit-control,+sh2,+sh4-cache,+sh4-debug-state,+single-fpu"
// PIC-SH4: !{{[0-9]+}} = !{i32 8, !"PIC Level", i32 2}
// SH4AL: attributes #{{[0-9]+}} = {{.*}}"target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4al-dsp{{[^"]*}}"{{.*}}"use-soft-float"="true"

// GNU-ASM: .text
// GNU-ASM-NOT: .prefalign
// GNU-ASM-NOT: .addrsig
