// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -emit-llvm -o - %s | FileCheck %s --check-prefixes=HARD,LE
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -emit-llvm -o - %s | FileCheck %s --check-prefixes=HARD,BE
// RUN: %clang --target=shle-unknown-linux-gnu -m4-single-only -S -emit-llvm -o - %s | FileCheck %s --check-prefix=EFLOAT
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4 -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4-single -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4-single-only -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4-single-only -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -m2e -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -m3e -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -m2 -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -m3 -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s

typedef float _Complex CF;
typedef double _Complex CD;

// Complex float consumes two FPR slots without rounding away an odd cursor.
// Little-endian SH4 swaps the saved components only for an even register cursor;
// the stack and odd register cursor keep real/imaginary memory order.
// HARD-LABEL: define{{.*}} @read_cf(
// HARD: getelementptr inbounds nuw %struct.__va_list_tag, ptr %ap, i32 0, i32 2
// HARD: getelementptr inbounds nuw %struct.__va_list_tag, ptr %ap, i32 0, i32 3
// HARD: %vaarg.next = load ptr
// HARD-NOT: ptrtoint
// HARD: %vaarg.reg.next = getelementptr i8, ptr %vaarg.next, i32 8
// HARD: %vaarg.in.regs = icmp ule ptr %vaarg.reg.next, %vaarg.limit
// HARD: %vaarg.addr = select i1 %vaarg.in.regs, ptr %vaarg.next, ptr %vaarg.stack
// LE: ptrtoint ptr %vaarg.next to i32
// LE: and i32 {{.*}}, 4
// LE: %vaarg.even.cursor = icmp eq i32 {{.*}}, 0
// LE: %vaarg.swap.components = and i1 %vaarg.in.regs, %vaarg.even.cursor
// LE: select i1 %vaarg.swap.components, float {{.*}}, float {{.*}}
// LE: select i1 %vaarg.swap.components, float {{.*}}, float {{.*}}
// BE-NOT: vaarg.swap.components
// HARD: ret { float, float }
// EFLOAT-LABEL: define{{.*}} @read_cf(
// EFLOAT: getelementptr inbounds nuw %struct.__va_list_tag, ptr %ap, i32 0, i32 0
// EFLOAT: getelementptr inbounds nuw %struct.__va_list_tag, ptr %ap, i32 0, i32 1
// EFLOAT-NOT: vaarg.swap.components
// EFLOAT: ret { float, float }
CF read_cf(int fixed, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, fixed);
  CF z = __builtin_va_arg(ap, CF);
  __builtin_va_end(ap);
  return z;
}

// Complex double rounds to an even FPR cursor and requires all four slots.
// The speculative bias/end addresses may exceed the saved-register object:
// neither GEP can be inbounds before the fit comparison.
// HARD-LABEL: define{{.*}} @read_cd(
// HARD: getelementptr inbounds nuw %struct.__va_list_tag, ptr %ap, i32 0, i32 2
// HARD: getelementptr inbounds nuw %struct.__va_list_tag, ptr %ap, i32 0, i32 3
// HARD: [[BIASED:%.*]] = getelementptr i8, ptr %vaarg.next, i32 7
// HARD: %vaarg.next.aligned = call ptr @llvm.ptrmask.p0.i32(ptr [[BIASED]], i32 -8)
// HARD: %vaarg.reg.next = getelementptr i8, ptr %vaarg.next.aligned, i32 16
// HARD: %vaarg.in.regs = icmp ule ptr %vaarg.reg.next, %vaarg.limit
// HARD: %vaarg.stack.next = getelementptr inbounds i8, ptr %vaarg.stack, i32 16
// HARD: select i1 %vaarg.in.regs, ptr %vaarg.reg.next, ptr %vaarg.next
// HARD-NOT: vaarg.swap.components
// HARD: ret { double, double }
CD read_cd(float named, int fixed, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, fixed);
  CD z = __builtin_va_arg(ap, CD);
  __builtin_va_end(ap);
  return z;
}

extern void consume(int, ...);

// The E-float ABI passes complex float in the integer area, unlike its FPR
// return. Preserve that distinction for named and unnamed arguments alike.
// HARD-LABEL: define{{.*}} @pass_cf({ float, float }{{.*}}
// HARD: call void (i32, ...) @consume(i32{{.*}} 0, { float, float } {{.*}})
// EFLOAT-LABEL: define{{.*}} @pass_cf(ptr{{.*}} byval({ float, float })
// EFLOAT: call void (i32, ...) @consume(i32{{.*}} 0, ptr{{.*}} byval({ float, float })
void pass_cf(CF z) { consume(0, z); }
