// RUN: %clang_cc1 -triple shle -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -O2 -S -o /dev/null %s
// RUN: %clang_cc1 -triple shle -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -O0 -S -o /dev/null %s

typedef struct { _Complex float z; } W;
typedef struct { W w; } N;
typedef struct { _Complex double z; } D;
typedef struct { D d; } ND;
typedef struct __attribute__((packed)) { _Complex float z; } P;
typedef struct { float re, im; } M;
typedef union { _Complex float z; } U;
typedef struct __attribute__((aligned(16))) { _Complex float z; } A;
// CHECK-LABEL: define{{.*}} { float, float } @cf({ float, float }
W cf(W w) { return w; }
// CHECK-LABEL: define{{.*}} { float, float } @nested({ float, float }
N nested(N n) { return n; }
// CHECK-LABEL: define{{.*}} { double, double } @cd({ double, double }
D cd(D d) { return d; }
// CHECK-LABEL: define{{.*}} { double, double } @nested_double({ double, double }
ND nested_double(ND n) { return n; }
// CHECK-LABEL: define{{.*}} void @packed(ptr{{.*}}sret
P packed(P p) { return p; }
// CHECK-LABEL: define{{.*}} i64 @mixed(ptr{{.*}}byval
M mixed(M m) { return m; }
// CHECK-LABEL: define{{.*}} i64 @union_value(ptr{{.*}}byval
U union_value(U u) { return u; }
// CHECK-LABEL: define{{.*}} void @padded(ptr{{.*}}sret
A padded(A a) { return a; }
// CHECK-LABEL: define{{.*}} { float, float } @va_record(
// CHECK: vaarg.even.cursor
// CHECK: select i1
W va_record(float first, ...) {
  __builtin_va_list ap, copy;
  __builtin_va_start(ap, first);
  __builtin_va_copy(copy, ap);
  W w = __builtin_va_arg(ap, W), v = __builtin_va_arg(copy, W);
  __builtin_va_end(copy); __builtin_va_end(ap);
  return (W){w.z + v.z};
}
