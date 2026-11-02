// RUN: %clang_cc1 -triple shle -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh -target-cpu sh4 -target-abi gnu-hard-float -mfloat-abi hard -O0 -S -o /dev/null %s
struct Natural { _Complex float z; };
struct Nested { Natural n; };
struct Empty {};
struct EmptyBase : Empty { _Complex float z; };
struct Inherited : Natural {};
struct Nonempty { int n; };
struct Mixed : Nonempty { _Complex float z; };
struct UserCtor { _Complex float z; UserCtor(); };
struct Copy { _Complex float z; Copy(const Copy &); };
struct Dtor { _Complex float z; ~Dtor(); };
extern "C" {
// CHECK-LABEL: define{{.*}} { float, float } @natural({ float, float }
Natural natural(Natural x) { return x; }
// CHECK-LABEL: define{{.*}} { float, float } @nested({ float, float }
Nested nested(Nested x) { return x; }
// CHECK-LABEL: define{{.*}} { float, float } @empty_base({ float, float }
EmptyBase empty_base(EmptyBase x) { return x; }
// CHECK-LABEL: define{{.*}} { float, float } @inherited({ float, float }
Inherited inherited(Inherited x) { return x; }
// CHECK-LABEL: define{{.*}} { float, float } @user_ctor({ float, float }
UserCtor user_ctor(UserCtor x) { return x; }
// CHECK-LABEL: define{{.*}} void @mixed(ptr{{.*}}sret
Mixed mixed(Mixed x) { return x; }
// CHECK-LABEL: define{{.*}} void @copy(ptr{{.*}}sret
Copy copy(Copy x) { return x; }
// CHECK-LABEL: define{{.*}} void @dtor(ptr{{.*}}sret
Dtor dtor(Dtor x) { return x; }
}
