// RUN: %clang -### -target shle-unknown-linux-gnu -c %s 2>&1 | FileCheck %s --check-prefix=IAS
// IAS: "-cc1"
// IAS-SAME: "-emit-obj"
// IAS-NOT: sh4-linux-gnu-as

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=LE-LINUX-AS
// LE-LINUX-AS: "sh4-linux-gnu-as" "--little" "--isa=sh2"
// LE-LINUX-AS-NOT: "/usr/bin/as"

// RUN: env "PATH=" %clang -### -target sh-unknown-linux-gnu -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=BE-LINUX-AS
// BE-LINUX-AS: "sh-linux-gnu-as" "--big" "--isa=sh2"
// BE-LINUX-AS-NOT: "/usr/bin/as"

// RUN: env "PATH=" %clang -### -target shle-unknown-none-elf -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=LE-ELF-AS
// LE-ELF-AS: "sh-elf-as" "--little" "--isa=sh2"

// RUN: env "PATH=" %clang -### -target sh-unknown-none-elf -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=BE-ELF-AS
// BE-ELF-AS: "sh-elf-as" "--big" "--isa=sh2"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m1 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=LE-SH1-AS
// RUN: env "PATH=" %clang -### -target sh-unknown-none-elf -mcpu=sh1 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=BE-SH1-AS
// LE-SH1-AS: "sh4-linux-gnu-as" "--little" "--isa=sh"
// BE-SH1-AS: "sh-elf-as" "--big" "--isa=sh"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -mcpu=sh3 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=LE-SH3-AS
// RUN: env "PATH=" %clang -### -target sh-unknown-none-elf -mcpu=sh3 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=BE-SH3-AS
// LE-SH3-AS: "sh4-linux-gnu-as" "--little" "--isa=sh3"
// BE-SH3-AS: "sh-elf-as" "--big" "--isa=sh3"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m2e -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=LE-SH2E-AS
// RUN: env "PATH=" %clang -### -target sh-unknown-none-elf -m3e -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=BE-SH3E-AS
// LE-SH2E-AS: "sh4-linux-gnu-as" "--little" "--isa=sh2e"
// BE-SH3E-AS: "sh-elf-as" "--big" "--isa=sh3e"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m2a -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=LE-SH2A-AS
// RUN: env "PATH=" %clang -### -target sh-unknown-none-elf -m2a-single -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=BE-SH2A-AS
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m2a-single-only -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=LE-SH2A-AS
// RUN: env "PATH=" %clang -### -target sh-unknown-none-elf -m2a-nofpu -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=BE-SH2A-NOFPU-AS
// LE-SH2A-AS: "sh4-linux-gnu-as" "--little" "--isa=sh2a"
// BE-SH2A-AS: "sh-elf-as" "--big" "--isa=sh2a"
// BE-SH2A-NOFPU-AS: "sh-elf-as" "--big" "--isa=sh2a-nofpu"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -mcpu=sh4 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=LE-SH4-AS
// RUN: env "PATH=" %clang -### -target sh-unknown-none-elf -mcpu=sh4 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=BE-SH4-AS
// LE-SH4-AS: "sh4-linux-gnu-as" "--little" "--isa=sh4"
// LE-SH4-AS-NOT: "--isa=sh4a"
// BE-SH4-AS: "sh-elf-as" "--big" "--isa=sh4"
// BE-SH4-AS-NOT: "--isa=sh4a"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m2 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=ALIAS-SH2-AS
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m3 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=ALIAS-SH3-AS
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4 -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=ALIAS-SH4-AS
// ALIAS-SH2-AS: "sh4-linux-gnu-as" "--little" "--isa=sh2"
// ALIAS-SH3-AS: "sh4-linux-gnu-as" "--little" "--isa=sh3"
// ALIAS-SH4-AS: "sh4-linux-gnu-as" "--little" "--isa=sh4"
// ALIAS-SH4-AS-NOT: "--isa=sh4a"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-nofpu -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=ALIAS-SH4-NOFPU-AS
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-single -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=ALIAS-SH4-AS
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -m4-single-only -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=ALIAS-SH4-AS
// ALIAS-SH4-NOFPU-AS: "sh4-linux-gnu-as" "--little" "--isa=sh4-nofpu"
// ALIAS-SH4-NOFPU-AS-NOT: "--isa=sh4a"

// RUN: rm -f %t.missing-as.o %t.missing-ld
// RUN: not env "PATH=" %clang -target shle-unknown-linux-gnu -fno-integrated-as -c %s -o %t.missing-as.o 2>&1 | FileCheck %s --check-prefix=MISSING-AS
// RUN: not env "PATH=" %clang -target shle-unknown-none-elf -nostdlib %s -o %t.missing-ld 2>&1 | FileCheck %s --check-prefix=MISSING-LD
// RUN: not test -e %t.missing-as.o
// RUN: not test -e %t.missing-ld
// MISSING-AS: clang: error: unable to execute command:
// MISSING-AS: clang: error: assembler command failed
// MISSING-LD: clang: error: unable to execute command:
// MISSING-LD: clang: error: linker command failed

// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/bin
// RUN: touch %t.dir/bin/sh4-linux-gnu-as %t.dir/bin/sh4-linux-gnu-ld
// RUN: chmod +x %t.dir/bin/sh4-linux-gnu-as %t.dir/bin/sh4-linux-gnu-ld
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -B%t.dir/bin -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=B-AS
// B-AS: "{{.*}}.dir/bin/sh4-linux-gnu-as" "--little" "--isa=sh2"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -mcpu=sh3 -B%t.dir/bin -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=B-SH3-AS
// B-SH3-AS: "{{.*}}.dir/bin/sh4-linux-gnu-as" "--little" "--isa=sh3"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -mcpu=sh4 -B%t.dir/bin -fno-integrated-as -c %s 2>&1 | FileCheck %s --check-prefix=B-SH4-AS
// B-SH4-AS: "{{.*}}.dir/bin/sh4-linux-gnu-as" "--little" "--isa=sh4"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -B%t.dir/bin --sysroot=%t.dir/sysroot -nostdlib %s 2>&1 | FileCheck %s --check-prefix=LE-LINUX-LD
// LE-LINUX-LD: "{{.*}}.dir/bin/sh4-linux-gnu-ld"
// LE-LINUX-LD-SAME: "--sysroot={{.*}}.dir/sysroot"
// LE-LINUX-LD-SAME: "-m" "shlelf_linux"
// LE-LINUX-LD-SAME: "-X"
// LE-LINUX-LD-SAME: "-dynamic-linker" "/lib/ld-linux.so.2"
// LE-LINUX-LD-NOT: "/usr/lib"
// LE-LINUX-LD-NOT: "/usr/bin/ld"

// RUN: env "PATH=" %clang -### -target sh-unknown-linux-gnu --sysroot=%t.dir/sysroot -nostdlib %s 2>&1 | FileCheck %s --check-prefix=BE-LINUX-LD
// BE-LINUX-LD: "sh-linux-gnu-ld"
// BE-LINUX-LD-SAME: "--sysroot={{.*}}.dir/sysroot"
// BE-LINUX-LD-SAME: "-m" "shelf_linux"
// BE-LINUX-LD-SAME: "-X"
// BE-LINUX-LD-SAME: "-dynamic-linker" "/lib/ld-linux.so.2"
// BE-LINUX-LD-NOT: "/usr/bin/ld"

// RUN: env "PATH=" %clang -### -target shle-unknown-none-elf -nostdlib %s 2>&1 | FileCheck %s --check-prefix=LE-ELF-LD
// LE-ELF-LD: "sh-elf-ld"
// LE-ELF-LD-SAME: "-m" "shlelf"
// LE-ELF-LD-SAME: "-X"
// LE-ELF-LD-NOT: "-dynamic-linker"
// LE-ELF-LD-NOT: "/usr/lib"
// LE-ELF-LD-NOT: "/lib/"
// LE-ELF-LD-NOT: "/usr/bin/ld"

// RUN: env "PATH=" %clang -### -target sh-unknown-none-elf -nostdlib %s 2>&1 | FileCheck %s --check-prefix=BE-ELF-LD
// BE-ELF-LD: "sh-elf-ld"
// BE-ELF-LD-SAME: "-m" "shelf"
// BE-ELF-LD-SAME: "-X"
// BE-ELF-LD-NOT: "-dynamic-linker"
// BE-ELF-LD-NOT: "/usr/bin/ld"

// RUN: mkdir -p %t.dir/gcc/lib/gcc/sh4-linux-gnu/14.2.0
// RUN: touch %t.dir/gcc/lib/gcc/sh4-linux-gnu/14.2.0/crtbegin.o %t.dir/gcc/lib/gcc/sh4-linux-gnu/14.2.0/crtend.o
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -B%t.dir/bin --gcc-toolchain=%t.dir/gcc --sysroot=%t.dir/sysroot -nostartfiles %s 2>&1 | FileCheck %s --check-prefix=GCC-TOOLCHAIN
// GCC-TOOLCHAIN: "{{.*}}.dir/bin/sh4-linux-gnu-ld"
// GCC-TOOLCHAIN-SAME: "-L{{.*}}.dir/gcc/lib/gcc/sh4-linux-gnu/14.2.0"
// GCC-TOOLCHAIN-NOT: "/usr/lib/gcc"

// RUN: env "PATH=" %clang -### -target shle-unknown-none-elf -B%t.dir/bin --gcc-toolchain=%t.dir/gcc --sysroot=%t.dir/sysroot %s 2>&1 | FileCheck %s --check-prefix=BARE-ISOLATED
// BARE-ISOLATED: "sh-elf-ld"
// BARE-ISOLATED-NOT: sh4-linux-gnu
// BARE-ISOLATED-NOT: "-dynamic-linker"
// BARE-ISOLATED-NOT: "-L/usr"
// BARE-ISOLATED-NOT: "-L/lib"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -B%t.dir/bin --sysroot=%t.dir/sysroot -nostdlib -no-pie %s 2>&1 | FileCheck %s --check-prefix=NO-PIE
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -B%t.dir/bin --sysroot=%t.dir/sysroot -nostdlib -pie %s 2>&1 | FileCheck %s --check-prefix=PIE
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -B%t.dir/bin --sysroot=%t.dir/sysroot -nostdlib -shared %s 2>&1 | FileCheck %s --check-prefix=SHARED
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -B%t.dir/bin --sysroot=%t.dir/sysroot -nostdlib -static %s 2>&1 | FileCheck %s --check-prefix=STATIC
// NO-PIE: "{{.*}}.dir/bin/sh4-linux-gnu-ld"
// NO-PIE-NOT: "-pie"
// PIE: "{{.*}}.dir/bin/sh4-linux-gnu-ld"
// PIE-SAME: "-pie"
// SHARED: "{{.*}}.dir/bin/sh4-linux-gnu-ld"
// SHARED-SAME: "-shared"
// SHARED-NOT: "-dynamic-linker"
// STATIC: "{{.*}}.dir/bin/sh4-linux-gnu-ld"
// STATIC-SAME: "-static"
// STATIC-NOT: "-dynamic-linker"

// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -B%t.dir/bin --sysroot=%t.dir/sysroot -nostdlib %s 2>&1 | FileCheck %s --check-prefix=NOSTDLIB
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -B%t.dir/bin --sysroot=%t.dir/sysroot -nostartfiles %s 2>&1 | FileCheck %s --check-prefix=NOSTARTFILES
// RUN: env "PATH=" %clang -### -target shle-unknown-linux-gnu -B%t.dir/bin --sysroot=%t.dir/sysroot -nodefaultlibs %s 2>&1 | FileCheck %s --check-prefix=NODEFAULTLIBS
// NOSTDLIB: "{{.*}}.dir/bin/sh4-linux-gnu-ld"
// NOSTDLIB-NOT: crt{{[^\"]*}}.o
// NOSTDLIB-NOT: "-lc"
// NOSTARTFILES: "{{.*}}.dir/bin/sh4-linux-gnu-ld"
// NOSTARTFILES-NOT: crt{{[^\"]*}}.o
// NOSTARTFILES-SAME: "-lc"
// NODEFAULTLIBS: "{{.*}}.dir/bin/sh4-linux-gnu-ld"
// NODEFAULTLIBS-SAME: crt{{[^\"]*}}.o
// NODEFAULTLIBS-NOT: "-lc"

int main(void) {
	return 0;
}
