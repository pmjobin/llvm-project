// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct s1 {
	u8 value;
};
struct s2 {
	u16 value;
};
struct s3 {
	u8 value[3];
};
struct s4 {
	u32 value;
};
struct s5 {
	u8 value[5];
};
struct s6 {
	u16 value[3];
};
struct s7 {
	u8 value[7];
};
struct s8 {
	u32 value[2];
};
struct s9 {
	u8 value[9];
};
struct s12 {
	u32 value[3];
};
struct s20 {
	u32 value[5];
};
struct nested4 {
	struct s3 inner;
	u8 tail;
};
struct nested_return4 {
	struct s2 inner;
	u16 tail;
} __attribute__((aligned(4)));
struct nested_large {
	struct s8 inner;
	u32 tail;
};
struct packed4 {
	u8 first;
	u8 rest[3];
} __attribute__((packed));
struct padded4 {
	u8 first;
	u16 second;
};
struct multi4 {
	u16 first;
	u16 second;
};
struct over8 {
	u32 value;
} __attribute__((aligned(8)));

// IR: define{{.*}} i32 @take_s1(%struct.s1 %value{{.*}}, i32{{.*}} %salt)
u32 take_s1(struct s1 value, u32 salt) {
	return value.value + salt;
}

// IR: define{{.*}} i32 @take_s2(%struct.s2 %value{{.*}}, i32{{.*}} %salt)
u32 take_s2(struct s2 value, u32 salt) {
	return value.value + salt;
}

// IR: define{{.*}} i32 @take_s3(%struct.s3 %value{{.*}}, i32{{.*}} %salt)
u32 take_s3(struct s3 value, u32 salt) {
	return value.value[0] + value.value[2] + salt;
}

// IR: define{{.*}} i32 @take_s4(%struct.s4 %value{{.*}}, i32{{.*}} %salt)
u32 take_s4(struct s4 value, u32 salt) {
	return value.value + salt;
}

// IR: define{{.*}} i32 @take_s5(%struct.s5 %value{{.*}}, i32{{.*}} %salt)
u32 take_s5(struct s5 value, u32 salt) {
	return value.value[0] + value.value[4] + salt;
}

// IR: define{{.*}} i32 @take_s6(%struct.s6 %value{{.*}}, i32{{.*}} %salt)
u32 take_s6(struct s6 value, u32 salt) {
	return value.value[0] + value.value[2] + salt;
}

// IR: define{{.*}} i32 @take_s7(%struct.s7 %value{{.*}}, i32{{.*}} %salt)
u32 take_s7(struct s7 value, u32 salt) {
	return value.value[0] + value.value[6] + salt;
}

// IR: define{{.*}} i32 @take_s8(%struct.s8 %value{{.*}}, i32{{.*}} %salt)
u32 take_s8(struct s8 value, u32 salt) {
	return value.value[0] + value.value[1] + salt;
}

// IR: define{{.*}} i32 @take_nested4(%struct.nested4 %value{{.*}}, i32{{.*}} %salt)
u32 take_nested4(struct nested4 value, u32 salt) {
	return value.inner.value[0] + value.inner.value[2] + value.tail + salt;
}

// IR: define{{.*}} i32 @take_packed4(%struct.packed4 %value{{.*}}, i32{{.*}} %salt)
u32 take_packed4(struct packed4 value, u32 salt) {
	return value.first + value.rest[2] + salt;
}

// IR: define{{.*}} i32 @take_padded4(%struct.padded4 %value{{.*}}, i32{{.*}} %salt)
u32 take_padded4(struct padded4 value, u32 salt) {
	return value.first + value.second + salt;
}

// IR: define{{.*}} i32 @take_multi4(%struct.multi4 %value{{.*}}, i32{{.*}} %salt)
u32 take_multi4(struct multi4 value, u32 salt) {
	return value.first + value.second + salt;
}

// IR: define{{.*}} i32 @take_s9(ptr{{.*}} byval(%struct.s9) align 1 %value, i32{{.*}} %salt)
u32 take_s9(struct s9 value, u32 salt) {
	return value.value[0] + value.value[8] + salt;
}

// IR: define{{.*}} i32 @take_s12(ptr{{.*}} byval(%struct.s12) align 4 %value, i32{{.*}} %salt)
u32 take_s12(struct s12 value, u32 salt) {
	return value.value[0] + value.value[2] + salt;
}

// IR: define{{.*}} i32 @take_s20(ptr{{.*}} byval(%struct.s20) align 4 %value, i32{{.*}} %salt)
u32 take_s20(struct s20 value, u32 salt) {
	return value.value[0] + value.value[4] + salt;
}

// IR: define{{.*}} i32 @take_nested_large(ptr{{.*}} byval(%struct.nested_large) align 4 %value, i32{{.*}} %salt)
u32 take_nested_large(struct nested_large value, u32 salt) {
	return value.inner.value[0] + value.tail + salt;
}

// IR: define{{.*}} i32 @take_over8(ptr{{.*}} byval(%struct.over8) align 8 %value, i32{{.*}} %salt)
u32 take_over8(struct over8 value, u32 salt) {
	return value.value + salt;
}

// IR: define{{.*}} i32 @cross_boundary(i32{{.*}}, i32{{.*}}, i32{{.*}}, %struct.s5 %value{{.*}}, i32{{.*}} %after)
u32 cross_boundary(u32 first, u32 second, u32 third, struct s5 value, u32 after) {
	return first + second + third + value.value[0] + value.value[4] + after;
}

// IR: define{{.*}} i32 @stack_only(i32{{.*}}, i32{{.*}}, i32{{.*}}, i32{{.*}}, %struct.s3 %value{{.*}})
u32 stack_only(u32 first, u32 second, u32 third, u32 fourth, struct s3 value) {
	return first + second + third + fourth + value.value[0] + value.value[2];
}

// IR: define{{.*}} i32 @consecutive(%struct.s1 %first{{.*}}, %struct.s2 %second{{.*}}, %struct.s3 %third{{.*}})
u32 consecutive(struct s1 first, struct s2 second, struct s3 third) {
	return first.value + second.value + third.value[0];
}

// IR: define{{.*}} i32 @between_scalars(i32{{.*}}, %struct.s5 %value{{.*}}, i32{{.*}})
u32 between_scalars(u32 first, struct s5 value, u32 last) {
	return first + value.value[0] + value.value[4] + last;
}

// IR: declare void @variadic_boundary(i32{{.*}}, %struct.s5, ...)
void variadic_boundary(u32 fixed, struct s5 value, ...);

void call_variadic_boundary(struct s5 value) {
	variadic_boundary(1, value, 2);
}

// IR: define{{.*}} %struct.s1 @return_s1(
struct s1 return_s1(u32 value) {
	return (struct s1){value};
}

// IR: define{{.*}} %struct.s2 @return_s2(
struct s2 return_s2(u32 value) {
	return (struct s2){value};
}

// IR: define{{.*}} void @return_s3(ptr{{.*}} sret(%struct.s3) align 1
struct s3 return_s3(u32 value) {
	return (struct s3){{value, value + 1, value + 2}};
}

// IR: define{{.*}} %struct.s4 @return_s4(
struct s4 return_s4(u32 value) {
	return (struct s4){value};
}

// IR: define{{.*}} void @return_s5(ptr{{.*}} sret(%struct.s5) align 1
struct s5 return_s5(u32 value) {
	return (struct s5){{value, value + 1, value + 2, value + 3, value + 4}};
}

// IR: define{{.*}} void @return_s6(ptr{{.*}} sret(%struct.s6) align 2
struct s6 return_s6(u32 value) {
	return (struct s6){{value, value + 1, value + 2}};
}

// IR: define{{.*}} void @return_s7(ptr{{.*}} sret(%struct.s7) align 1
struct s7 return_s7(u32 value) {
	return (struct s7){{value, value + 1, value + 2, value + 3, value + 4, value + 5, value + 6}};
}

// IR: define{{.*}} %struct.s8 @return_s8(
struct s8 return_s8(u32 value) {
	return (struct s8){{value, value + 1}};
}

// IR: define{{.*}} void @return_s12(ptr{{.*}} sret(%struct.s12) align 4
struct s12 return_s12(u32 value) {
	return (struct s12){{value, value + 1, value + 2}};
}

// IR: define{{.*}} %struct.nested_return4 @return_nested4(
struct nested_return4 return_nested4(u32 value) {
	return (struct nested_return4){{value}, value + 1};
}
