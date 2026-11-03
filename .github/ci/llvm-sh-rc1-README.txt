LLVM SH RC1 prebuilt packages
============================

Release: https://github.com/pmjobin/llvm-project/releases/tag/llvm-sh-rc1
Source: 090c682597f7155373681807330fbb0ac343a33c
Tree:   55640dd25d624b11b9e0f28a7903eea42a429254
Build recipes: 8cc110b2ee91b3645a8ea350b8c321eda5f74bc5
Build and package checks:
https://github.com/pmjobin/llvm-project/actions/runs/41766778569

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
commit and exact GitHub Actions run URL. The package SHA-256 values describe
these binaries, not the earlier builds made before the RC1 history fold.

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

The host tools are PE x64 executables built by native MSVC 19.44.35229.0.
Imports include MSVCP140.dll, VCRUNTIME140.dll, VCRUNTIME140_1.dll, the
Universal CRT and standard Windows system DLLs. Runtime DLLs are not bundled.
Install a compatible Microsoft x64 v14 runtime if needed. Microsoft's current
runtime/download guidance is here:
https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist/

SH4 compilation
----------------

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

This is an experimental release candidate. Packages provide LLVM/Clang tools,
Clang resource headers and license files. They do not include a GNU SH
sysroot/SDK, emulator, Doom or complete target runtime distribution. Linking
SH programs needs a separately compatible target toolchain/sysroot. Compile
checks do not establish execution or application compatibility on hardware.

The Windows package passed full tool compilation and staged/relocated SH4
object checks. The Windows SH regression suite was not run by the package job.
LLVM and Clang license texts are included under share/doc/llvm-sh.
