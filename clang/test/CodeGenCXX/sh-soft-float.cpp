// RUN: %clang_cc1 -triple sh-unknown-none-elf -O0 -emit-obj -fcxx-exceptions -fexceptions -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -O2 -emit-obj -fcxx-exceptions -fexceptions -pic-level 2 -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -Os -emit-obj -fcxx-exceptions -fexceptions -pic-level 2 -mllvm -verify-machineinstrs -o /dev/null %s

struct base {
	virtual double apply(double) = 0;
	virtual ~base();
};

extern thread_local double tls_value;
extern void may_throw();

double compose(base *object, float value, long long numerator, long long denominator) {
	try {
		may_throw();
		return object->apply((double)value * tls_value) +
		       (double)(numerator / denominator);
	} catch (...) {
		return -(double)(numerator % denominator);
	}
}
