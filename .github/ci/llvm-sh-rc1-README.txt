LLVM SH RC1: SH linker and static C/C++ libraries
================================================

Release: https://github.com/pmjobin/llvm-project/releases/tag/llvm-sh-rc1
Source: 2ab77bfe7ccd9bbe3f78f2582f3b28d4c10b7004
Tree:   fde861cc0c96b9f21801348c68a0fe140e1f6d56
Build recipes: 32d77950cf98585b9d62dfbfa34ea81ccec4a1b4
Build and package checks:
https://github.com/pmjobin/llvm-project/actions/runs/42332576014

Choose the package for the computer running the compiler:

  Ubuntu 26.04 amd64: llvm-sh-rc1-ubuntu2604-amd64.deb
  Fedora 45 x86_64:  llvm-sh-rc1-fedora45-x86_64.rpm
  Fedora 45 ZIP:     llvm-sh-rc1-fedora45-linux-x86_64.zip
  Windows x64 ZIP:   llvm-sh-rc1-windows-x64.zip

The Windows ZIP was tested on Windows Server 2022 x64. The Fedora ZIP is
relocatable on a compatible Fedora 45 runtime; it is not a universal Linux
binary. RHEL and other Linux distributions are untested.

Verify downloads
----------------

Download the .sha256 file alongside the chosen package, then use, for example:

  sha256sum --check llvm-sh-rc1-ubuntu2604-amd64.deb.sha256

SHA256SUMS covers all four package files if all have been downloaded.
On Windows, compare the result below with the supplied .sha256 text file:

  Get-FileHash .\llvm-sh-rc1-windows-x64.zip -Algorithm SHA256

Each package contains BUILDINFO.txt with its source commit/tree, automation
commit and exact GitHub Actions run URL. These packages are rebuilt from the
source above. They replace the previous RC1 downloads; refresh cached files
and check their new SHA-256 values. Build metadata calls them candidates
because the CI workflow builds candidates before release publication.
Extract each ZIP update into a fresh directory.

Ubuntu DEB
----------

  sudo apt-get install ./llvm-sh-rc1-ubuntu2604-amd64.deb
  /opt/llvm-sh/rc1/bin/clang --version

APT resolves the declared runtime dependencies. The package installs under
/opt/llvm-sh/rc1 and does not replace the system Clang.

Fedora RPM
----------

  sudo dnf install ./llvm-sh-rc1-fedora45-x86_64.rpm
  /opt/llvm-sh/rc1/bin/clang --version

DNF resolves the declared runtime dependencies. The package installs under
/opt/llvm-sh/rc1 and does not replace the system Clang. The RPM is unsigned;
the supplied checksum permits download-integrity verification.

Fedora portable ZIP
-------------------

Extract with a tool that preserves Unix permissions and symbolic links:

  unzip llvm-sh-rc1-fedora45-linux-x86_64.zip -d ./toolchains
  ./toolchains/rc1/bin/clang --version

The ZIP contains the same staged distribution used for the RPM input. Its
Clang and LLVM shared libraries are included and resolve relative to the
extracted prefix. No LD_LIBRARY_PATH override is required.

System dependencies include the x86-64 ELF loader, libc, libm, libstdc++,
libgcc_s, libedit.so.0, libz.so.1, libzstd.so.1, liblzma.so.5, libxml2.so.16
and libtinfo.so.6. Symbol requirements include libm GLIBC_2.44,
GLIBC_ABI_DT_RELR, GLIBCXX_3.4.30 and CXXABI_1.3.13. A host with older or
different libraries may not run this ZIP; use the native Ubuntu DEB on Ubuntu.

Windows portable ZIP
--------------------

From PowerShell in a directory where you can write files:

  Expand-Archive -LiteralPath .\llvm-sh-rc1-windows-x64.zip -DestinationPath .\toolchains
  & .\toolchains\llvm-sh-rc1\bin\clang.exe --version

The host tools are PE x64 executables built by native MSVC.
Imports include MSVCP140.dll, VCRUNTIME140.dll, VCRUNTIME140_1.dll, the
Universal CRT and standard Windows system DLLs. Runtime DLLs are not bundled.
Install a compatible Microsoft x64 v14 runtime if needed. Microsoft's current
runtime/download guidance is here:
https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist/

Static SH4 C and C++ programs
-----------------------------

The packages include native SH ELF32 LLD, compiler-rt helpers, Picolibc
1.8.12 and static libc++/libc++abi. Target libraries are built with the
packaged compiler and archive tools. Select the installed configurations:

  clang --config=sh4-picolibc.cfg -O2 -c application.c -o application.o
  clang++ --config=sh4-libcxx.cfg -O2 -c application.cpp -o application.o
  clang++ --config=sh4-libcxx.cfg -Wl,--gc-sections -T board.ld \
    startup.o application.o libapplication.a -o application.elf

Use the clang/clang++ in /opt/llvm-sh/rc1/bin or the extracted ZIP's bin
directory. Supply the application's startup, linker script, heap bounds and
required platform I/O hooks. The configurations supply no board startup or
default memory map. C++ defaults to C++17 without exceptions or RTTI.

This library profile is little-endian SH4, -m4-nofpu, soft floating-point
arguments and software arithmetic, static non-PIC/non-PIE ELF32 EM_SH 42
with e_flags 0x10. Use matching options for startup and every input object.
LLD rejects mixing the 0x10 library profile with the older 0x9 SH4 profile.
The configurations and headers resolve relative to the installed toolchain;
no GNU SH linker, target libgcc or external target SDK is required.

The library sysroot is:

  lib/clang-runtimes/shle-unknown-none-elf/sh4-soft

LIBRARIES.json in that sysroot records source identities, compiler version,
recipe hashes and archive checksums. Installed archives include libc.a,
libm.a, libc++.a, libc++abi.a and libclang_rt.builtins.a. The exact pinned
Picolibc source archive, its license material and the library build recipe
are included. See share/doc/llvm-sh/SH4-LIBRARIES.rst for the complete board
contract, configuration options, supported facilities and rebuild steps.

The independent minimal freestanding runtime remains at:

  lib/shle-unknown-none-elf/sh4-soft

It has its own board allocation/panic hooks and 0x9 SH4 profile. Do not mix
its memory or C++ lifecycle providers with the Picolibc/libc++ profile.
See share/doc/llvm-sh/SH4-FREESTANDING.rst for its contract and examples.

Installed examples and checks are under:

  share/llvm-sh/examples/SHFreestanding
  share/llvm-sh/examples/SHFreestanding/libraries

On a compatible Linux host with an independently installed SH4 QEMU:

  python3 /path/to/toolchain/share/llvm-sh/examples/SHFreestanding/libraries/check.py \
    --prefix /path/to/toolchain --build-dir /path/to/scratch/library-check \
    --qemu /usr/bin/qemu-sh4-static --require-execution

QEMU is not bundled. The supplied Linux-user console adapter exercises
target code under an emulator; it is not a universal board image. On Windows
the checker compiles, archives and links, and explicitly records execution
as unrun when no emulator is supplied.

Existing Linux-target compile-only workflow
-------------------------------------------

The canonical target architecture is shle for little endian and sh for big
endian. For a little-endian SH4 compile-only object using a native Linux package:

  /opt/llvm-sh/rc1/bin/clang --target=shle-unknown-linux-gnu -mcpu=sh4 -c example.c -o example.o
  /opt/llvm-sh/rc1/bin/llvm-readobj --file-headers example.o

For ZIPs, use the corresponding extracted program path, such as
./toolchains/rc1/bin/clang or .\toolchains\llvm-sh-rc1\bin\clang.exe. The
expected object is ELF32, little endian, relocatable, EM_SH, flags 0x9 (SH4).
Do not use the unrecognized sh4-unknown-linux-gnu architecture spelling.

Scope
-----

This remains an experimental release candidate with bounded, single-threaded
C and C++ library support. Tested examples do not establish full language or
library conformance, hardware reset/interrupt behavior or compatibility with
a particular board. Supply board startup, memory layout, I/O and heap setup.

Exceptions, unwinding, application RTTI, TLS, threads and inter-thread atomics
are unsupported. The libc++ profile disables filesystem, locale/iostream,
wide-character facilities, random_device, monotonic clock and the time-zone
database. File/process/clock/entropy APIs require platform implementations.
Math uses IEEE return-value semantics without math errno reporting; a
configurable floating-point environment is unsupported.

Big-endian libraries, other CPUs, hard argument ABIs, dynamic/shared/PIE,
FDPIC and hosted GNU/Linux SDK compatibility are outside this library profile.
Big-endian LLD regression coverage does not qualify a big-endian runtime.

The release qualification report records native source regressions, installed
and relocated package checks, and actual execution coverage separately.
LLVM, Clang, LLD and compiler-rt license texts are under share/doc/llvm-sh;
target-library license material is included in the library sysroot.
