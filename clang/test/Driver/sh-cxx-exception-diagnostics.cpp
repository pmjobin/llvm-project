// RUN: not %clang --target=sh-unknown-none-elf -fno-exceptions -fsyntax-only -DTEST_THROW %s 2>&1 | FileCheck %s --check-prefix=THROW
// RUN: not %clang --target=shle-unknown-linux-gnu -fno-cxx-exceptions -fsyntax-only -DTEST_TRY %s 2>&1 | FileCheck %s --check-prefix=TRY

#if defined(TEST_THROW)
void throwing() {
	throw 1;
}
// THROW: error: cannot use 'throw' with exceptions disabled
#elif defined(TEST_TRY)
void catching() {
	try {
		throw 1;
	} catch (...) {
	}
}
// TRY: error: cannot use 'try' with exceptions disabled
#endif
