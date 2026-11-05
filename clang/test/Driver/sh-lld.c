// Host file paths in -### output can contain escaped Windows separators.
// RUN: %clang -### --target=shle-unknown-none-elf -mcpu=sh4 -ffreestanding -nostdlib -static %s 2>&1 | FileCheck %s --check-prefixes=LLD,LE,STATIC
// RUN: %clang -### --target=sh-unknown-none-elf -mcpu=sh4 -ffreestanding -nostdlib -static -fuse-ld=lld %s 2>&1 | FileCheck %s --check-prefixes=LLD,BE,STATIC
// RUN: %clang -### --target=shle-unknown-none-elf -mcpu=sh4 -ffreestanding -nostdlib -r %s 2>&1 | FileCheck %s --check-prefixes=LLD,LE,PARTIAL
// LLD: "{{.*}}ld.lld{{(.exe)?}}"
// LE-SAME: "-m" "shlelf"
// BE-SAME: "-m" "shelf"
// STATIC-SAME: "-static"
// PARTIAL-SAME: "-r"
// LLD-NOT: "-dynamic-linker"
// LLD-NOT: "-lc"
// LLD-NOT: "-lgcc"
// LLD-NOT: "-L{{[/\\]+}}usr
// LLD-NOT: "-L{{[/\\]+}}lib

// An explicit GNU cross-linker override remains possible. This test only
// inspects driver commands; completed links are tested by the SH example.
// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/bin
// RUN: touch %t.dir/bin/sh-elf-ld
// RUN: chmod +x %t.dir/bin/sh-elf-ld
// RUN: env "PATH=" %clang -### --target=shle-unknown-none-elf -mcpu=sh4 -nostdlib -B%t.dir/bin --ld-path=sh-elf-ld %s 2>&1 | FileCheck %s --check-prefix=GNU
// GNU: "{{.*}}.dir{{[/\\]+}}bin{{[/\\]+}}sh-elf-ld"
// GNU-SAME: "-m" "shlelf"

// A missing explicit linker is a diagnostic, with no GNU/host fallback.
// RUN: not %clang -### --target=shle-unknown-none-elf -mcpu=sh4 -nostdlib --ld-path=%t.missing %s 2>&1 | FileCheck %s --check-prefix=MISSING
// MISSING: error: invalid linker name in argument
// MISSING-NOT: "sh-elf-ld"
// MISSING-NOT: "{{[/\\]+}}usr{{[/\\]+}}bin{{[/\\]+}}ld"

int main(void) { return 0; }
