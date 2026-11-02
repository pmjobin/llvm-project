// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct s1 { u8 value; };
struct s2 { u16 value; };
struct s3 { u8 value[3]; };
struct s4 { u32 value; };
struct s5 { u8 value[5]; };
struct s6 { u16 value[3]; };
struct s7 { u8 value[7]; };
struct s8 { u32 value[2]; };
struct s9 { u8 value[9]; };
struct s12 { u32 value[3]; };
struct s20 { u32 value[5]; };
struct nested4 { struct s3 inner; u8 tail; };
struct nested_return4 { struct s2 inner; u16 tail; } __attribute__((aligned(4)));
struct nested_large { struct s8 inner; u32 tail; };
struct packed4 { u8 first; u8 rest[3]; } __attribute__((packed));
struct padded4 { u8 first; u16 second; };
struct over8 { u32 value; } __attribute__((aligned(8)));

#define DECLARE_TAKE(N) extern u32 take_s##N(struct s##N, u32)
#define DEFINE_CALL(N) \
	u32 call_s##N(struct s##N value) { return take_s##N(value, N); }

DECLARE_TAKE(1);
DECLARE_TAKE(2);
DECLARE_TAKE(3);
DECLARE_TAKE(4);
DECLARE_TAKE(5);
DECLARE_TAKE(6);
DECLARE_TAKE(7);
DECLARE_TAKE(8);
DECLARE_TAKE(9);
DECLARE_TAKE(12);
DECLARE_TAKE(20);

// CHECK: call i32 @take_s1(%struct.s1 %{{.*}}, i32{{.*}} 1)
DEFINE_CALL(1)
// CHECK: call i32 @take_s2(%struct.s2 %{{.*}}, i32{{.*}} 2)
DEFINE_CALL(2)
// CHECK: call i32 @take_s3(%struct.s3 %{{.*}}, i32{{.*}} 3)
DEFINE_CALL(3)
// CHECK: call i32 @take_s4(%struct.s4 %{{.*}}, i32{{.*}} 4)
DEFINE_CALL(4)
// CHECK: call i32 @take_s5(%struct.s5 %{{.*}}, i32{{.*}} 5)
DEFINE_CALL(5)
// CHECK: call i32 @take_s6(%struct.s6 %{{.*}}, i32{{.*}} 6)
DEFINE_CALL(6)
// CHECK: call i32 @take_s7(%struct.s7 %{{.*}}, i32{{.*}} 7)
DEFINE_CALL(7)
// CHECK: call i32 @take_s8(%struct.s8 %{{.*}}, i32{{.*}} 8)
DEFINE_CALL(8)
// CHECK: call i32 @take_s9(ptr{{.*}} byval(%struct.s9) align 1 %{{.*}}, i32{{.*}} 9)
DEFINE_CALL(9)
// CHECK: call i32 @take_s12(ptr{{.*}} byval(%struct.s12) align 4 %{{.*}}, i32{{.*}} 12)
DEFINE_CALL(12)
// CHECK: call i32 @take_s20(ptr{{.*}} byval(%struct.s20) align 4 %{{.*}}, i32{{.*}} 20)
DEFINE_CALL(20)

extern u32 take_nested4(struct nested4, u32);
extern u32 take_nested_large(struct nested_large, u32);
extern u32 take_packed4(struct packed4, u32);
extern u32 take_padded4(struct padded4, u32);
extern u32 take_over8(struct over8, u32);

// CHECK: call i32 @take_nested4(%struct.nested4 %{{.*}}, i32{{.*}} 1)
u32 call_nested4(struct nested4 value) { return take_nested4(value, 1); }
// CHECK: call i32 @take_nested_large(ptr{{.*}} byval(%struct.nested_large) align 4 %{{.*}}, i32{{.*}} 2)
u32 call_nested_large(struct nested_large value) { return take_nested_large(value, 2); }
// CHECK: call i32 @take_packed4(%struct.packed4 %{{.*}}, i32{{.*}} 3)
u32 call_packed4(struct packed4 value) { return take_packed4(value, 3); }
// CHECK: call i32 @take_padded4(%struct.padded4 %{{.*}}, i32{{.*}} 4)
u32 call_padded4(struct padded4 value) { return take_padded4(value, 4); }
// CHECK: call i32 @take_over8(ptr{{.*}} byval(%struct.over8) align 8 %{{.*}}, i32{{.*}} 5)
u32 call_over8(struct over8 value) { return take_over8(value, 5); }

extern u32 cross_boundary(u32, u32, u32, struct s5, u32);
extern u32 stack_only(u32, u32, u32, u32, struct s3);
extern u32 consecutive(struct s1, struct s2, struct s3);
extern u32 between_scalars(u32, struct s5, u32);

// CHECK: call i32 @cross_boundary(i32{{.*}} 1, i32{{.*}} 2, i32{{.*}} 3, %struct.s5 %{{.*}}, i32{{.*}} 4)
u32 call_cross_boundary(struct s5 value) { return cross_boundary(1, 2, 3, value, 4); }
// CHECK: call i32 @stack_only(i32{{.*}} 1, i32{{.*}} 2, i32{{.*}} 3, i32{{.*}} 4, %struct.s3 %{{.*}})
u32 call_stack_only(struct s3 value) { return stack_only(1, 2, 3, 4, value); }
// CHECK: call i32 @consecutive(%struct.s1 %{{.*}}, %struct.s2 %{{.*}}, %struct.s3 %{{.*}})
u32 call_consecutive(struct s1 first, struct s2 second, struct s3 third) { return consecutive(first, second, third); }
// CHECK: call i32 @between_scalars(i32{{.*}} 1, %struct.s5 %{{.*}}, i32{{.*}} 2)
u32 call_between_scalars(struct s5 value) { return between_scalars(1, value, 2); }

#define DECLARE_RETURN(N) extern struct s##N return_s##N(u32)
#define DEFINE_RETURN_CALL(N) \
	struct s##N call_return_s##N(u32 value) { return return_s##N(value); }

DECLARE_RETURN(1);
DECLARE_RETURN(2);
DECLARE_RETURN(3);
DECLARE_RETURN(4);
DECLARE_RETURN(5);
DECLARE_RETURN(6);
DECLARE_RETURN(7);
DECLARE_RETURN(8);
DECLARE_RETURN(12);
extern struct nested_return4 return_nested4(u32);

// CHECK: call %struct.s1 @return_s1(i32{{.*}} %{{.*}})
DEFINE_RETURN_CALL(1)
// CHECK: call %struct.s2 @return_s2(i32{{.*}} %{{.*}})
DEFINE_RETURN_CALL(2)
// CHECK: call void @return_s3(ptr{{.*}} sret(%struct.s3) align 1 %{{.*}}, i32{{.*}} %{{.*}})
DEFINE_RETURN_CALL(3)
// CHECK: call %struct.s4 @return_s4(i32{{.*}} %{{.*}})
DEFINE_RETURN_CALL(4)
// CHECK: call void @return_s5(ptr{{.*}} sret(%struct.s5) align 1 %{{.*}}, i32{{.*}} %{{.*}})
DEFINE_RETURN_CALL(5)
// CHECK: call void @return_s6(ptr{{.*}} sret(%struct.s6) align 2 %{{.*}}, i32{{.*}} %{{.*}})
DEFINE_RETURN_CALL(6)
// CHECK: call void @return_s7(ptr{{.*}} sret(%struct.s7) align 1 %{{.*}}, i32{{.*}} %{{.*}})
DEFINE_RETURN_CALL(7)
// CHECK: call %struct.s8 @return_s8(i32{{.*}} %{{.*}})
DEFINE_RETURN_CALL(8)
// CHECK: call void @return_s12(ptr{{.*}} sret(%struct.s12) align 4 %{{.*}}, i32{{.*}} %{{.*}})
DEFINE_RETURN_CALL(12)
// CHECK: call %struct.nested_return4 @return_nested4(i32{{.*}} %{{.*}})
struct nested_return4 call_return_nested4(u32 value) { return return_nested4(value); }
