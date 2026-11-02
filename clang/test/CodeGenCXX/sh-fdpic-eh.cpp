// RUN: %clang_cc1 -triple sh2-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 2 -mconstructor-aliases -fcxx-exceptions -fexceptions -O2 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple sh2eb-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 2 -mconstructor-aliases -fcxx-exceptions -fexceptions -O2 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple sh2-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 2 -mconstructor-aliases -fcxx-exceptions -fexceptions -O2 -emit-obj -o %t.le.o %s
// RUN: %clang_cc1 -triple sh2eb-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -pic-level 2 -mconstructor-aliases -fcxx-exceptions -fexceptions -O2 -emit-obj -o %t.be.o %s
// RUN: llvm-readobj --file-headers --sections --relocations %t.le.o %t.be.o | FileCheck %s --check-prefix=OBJECT
// RUN: llvm-dwarfdump --verify %t.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
// RUN: llvm-dwarfdump --verify %t.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY

extern void may_throw(int);

struct VirtualCleanup {
	virtual ~VirtualCleanup();
};

struct DerivedCleanup : VirtualCleanup {
	~DerivedCleanup() override;
};

VirtualCleanup::~VirtualCleanup() {}
DerivedCleanup::~DerivedCleanup() {}

void virtual_destructor_unwind() {
	DerivedCleanup cleanup;
	may_throw(1);
}

int nested_catch_and_rethrow() {
	try {
		try {
			may_throw(2);
		} catch (int value) {
			if (value == 2)
				throw;
		}
	} catch (...) {
		return 3;
	}
	return 0;
}

// ASM-LABEL: _Z25virtual_destructor_unwindv:
// ASM: .cfi_personality 176, __gxx_personality_v0
// ASM-NEXT: .cfi_lsda 27,
// ASM: .long	_ZN14DerivedCleanupD1Ev@PLT
// ASM: .long	_Unwind_Resume@PLT

// ASM-LABEL: _Z24nested_catch_and_rethrowv:
// ASM: .cfi_personality 176, __gxx_personality_v0
// ASM-NEXT: .cfi_lsda 27,
// ASM: .byte	187
// ASM: .long	_ZTIi@GOT

// ASM: _ZTV14DerivedCleanup:
// ASM: .long	_ZN14DerivedCleanupD1Ev@FUNCDESC
// ASM-NEXT: .long	_ZN14DerivedCleanupD0Ev@FUNCDESC

// ASM: _ZN14VirtualCleanupD1Ev = _ZN14VirtualCleanupD2Ev
// ASM-NOT: _ZN14VirtualCleanupD1Ev = _ZN14VirtualCleanupD2Ev@FUNCDESC
// ASM: _ZN14DerivedCleanupD2Ev = _ZN14VirtualCleanupD2Ev
// ASM-NOT: _ZN14DerivedCleanupD2Ev = _ZN14VirtualCleanupD2Ev@FUNCDESC
// ASM: _ZN14DerivedCleanupD1Ev = _ZN14DerivedCleanupD2Ev
// ASM-NOT: _ZN14DerivedCleanupD1Ev = _ZN14DerivedCleanupD2Ev@FUNCDESC

// OBJECT-COUNT-2: Flags [ (0x8002)
// OBJECT: Name: .gcc_except_table
// OBJECT: Name: .eh_frame
// OBJECT-DAG: R_SH_GOTFUNCDESC __gxx_personality_v0
// OBJECT-DAG: R_SH_GOT32 _ZTIi
// OBJECT-DAG: R_SH_FUNCDESC _ZN14DerivedCleanupD1Ev
// OBJECT-DAG: R_SH_FUNCDESC _ZN14DerivedCleanupD0Ev
// OBJECT-DAG: R_SH_PLT32 _Unwind_Resume
// OBJECT-DAG: R_SH_PLT32 __cxa_rethrow

// VERIFY-NOT: error:
// VERIFY: No errors.
