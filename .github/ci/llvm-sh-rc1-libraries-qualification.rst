LLVM SH RC1: static C/C++ library release qualification
=======================================================

RC1 was rebuilt from the consolidated library source below. The four
downloads replace the previous LLD-only RC1 packages. Qualification uses
native source regressions, package installation/relocation, verified target
archive bytes and actual SH program outputs.

Identities
----------

* Source tag: ``llvm-sh-rc1``.
* Source commit: ``2ab77bfe7ccd9bbe3f78f2582f3b28d4c10b7004``.
* Source tree: ``fde861cc0c96b9f21801348c68a0fe140e1f6d56``.
* Annotated tag object: ``d431e9243028a4a9630e8e8730867e122f3aca3f``.
* Packaging automation: ``32d77950cf98585b9d62dfbfa34ea81ccec4a1b4``.
* Native build run: https://github.com/pmjobin/llvm-project/actions/runs/42332576014
* Packaging syntax validation: https://github.com/pmjobin/llvm-project/actions/runs/42332575637
* Release: https://github.com/pmjobin/llvm-project/releases/tag/llvm-sh-rc1

The source checkout excludes the CI helpers. Linux and Windows recipes
load those helpers from the separately pinned automation checkout and
retain a clean source tree. BUILDINFO.txt, RUNTIME.json and LIBRARIES.json
were checked against the source commit/tree and the exact workflow run.

Native checks
-------------

All three native package jobs passed. Source-suite counts below come from
their uploaded XML records; skipped cases are reported separately.

Ubuntu and Windows used their hosted native images. Fedora 45 userspace
ran in its x86_64 container on an Ubuntu 26.04 runner. The independent
downloaded-ZIP execution checks ran on a Fedora 45 workstation.

llvm-sh-rc1-ubuntu2604-amd64.deb
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* llvm-sh: 645 passed, 0 skipped, 0 failed.
* clang-sh: 131 passed, 0 skipped, 0 failed.
* lld-sh: 9 passed, 0 skipped, 0 failed.
* sh-unit: 93 passed, 0 skipped, 0 failed.

Native job: https://github.com/pmjobin/llvm-project/actions/runs/42332576014/job/128282224297
Started: 2026-11-07T03:08:24Z; completed: 2026-11-07T04:25:14Z.

llvm-sh-rc1-fedora45-x86_64.rpm
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* llvm-sh: 645 passed, 0 skipped, 0 failed.
* clang-sh: 131 passed, 0 skipped, 0 failed.
* lld-sh: 9 passed, 0 skipped, 0 failed.
* sh-unit: 93 passed, 0 skipped, 0 failed.

Native job: https://github.com/pmjobin/llvm-project/actions/runs/42332576014/job/128282224263
Started: 2026-11-07T03:08:23Z; completed: 2026-11-07T03:41:45Z.

llvm-sh-rc1-windows-x64.zip
~~~~~~~~~~~~~~~~~~~~~~~~~~~

* llvm-sh: 645 passed, 0 skipped, 0 failed.
* clang-sh: 131 passed, 0 skipped, 0 failed.
* lld-sh: 9 passed, 0 skipped, 0 failed.
* sh-unit: 93 passed, 0 skipped, 0 failed.

Native job: https://github.com/pmjobin/llvm-project/actions/runs/42332576014/job/128282334994
Started: 2026-11-07T03:08:50Z; completed: 2026-11-07T04:42:13Z.

Installed and relocated tools
-----------------------------

Ubuntu staged/installed DEB, Fedora staged/installed RPM and relocated ZIP,
and native Windows staged/relocated ZIP receipts passed their independent
ELF32/EM_SH/layout checks. Minimal-profile objects use e_flags 0x9; the
Picolibc/libc++ profile uses SH4 no-FPU e_flags 0x10. All archive members
and their package manifests were checked. Ubuntu and Fedora target archives
are byte-identical. Windows libc++abi.a differs by three source-path separator
bytes in cxa_demangle.cpp.obj diagnostic strings; normalizing only those
slashes makes the entire archive identical to Linux. All other target
archives match across hosts. Per-host hashes retain the original bytes.

* Native Linux package checks recorded 180 exact-output program executions.
* Windows-native receipts record compile/archive/link checks and unrun SH execution.
* 72 Windows-produced SH executables were independently executed under Fedora QEMU;
  each matched its recorded SHA-256, expected stdout and exit status 0.
  Unix execute permission, reset by artifact download, was restored before
  emulation; executable bytes and their SHA-256 values were unchanged.
* The downloaded Linux ZIP was independently extracted and its installed
  checkers executed 36 more programs with exact expected stdout and exit 0.

The Windows path-with-spaces Meson install-layout check also passed. It
checks header/library placement and does not itself build target code.

Packages and checksums
----------------------

* ``llvm-sh-rc1-ubuntu2604-amd64.deb``: 80894590 bytes;
  SHA-256 ``c7392acc0efce4d5ca980f23112de222d066f2f516022f505ee760777a760cf0``.
* ``llvm-sh-rc1-fedora45-x86_64.rpm``: 59192960 bytes;
  SHA-256 ``1e0d54e5e392c9071c1128ed973243139a9a5ccdfcfb8f22f99744635491c79d``.
* ``llvm-sh-rc1-fedora45-linux-x86_64.zip``: 82369902 bytes;
  SHA-256 ``95b311339a6604480c02a4e10305fece4f76c79ea787a05d0bd1228091f4c6a4``.
* ``llvm-sh-rc1-windows-x64.zip``: 473154008 bytes;
  SHA-256 ``63b3ecd9afe90b2b87ffd626694df3b39c5e15a97355e84ac29d1e15feee1ea2``.

CI sidecars matched independently hashed downloads. ZIP CRC checks,
DEB/RPM metadata and the embedded provenance were checked before publication.
The original candidate package bytes are retained; publication gives them
the canonical filenames above and regenerates filename-specific sidecars.
SHA256SUMS covers the release package files and supporting documents.

Supported scope
---------------

The opt-in static library profile provides pinned Picolibc 1.8.12 and
libc++/libc++abi for little-endian SH4 no-FPU software arithmetic and soft
floating-point arguments. It is single-threaded and requires application
startup, a linker script, heap bounds and platform I/O hooks. The separate
minimal freestanding profile and the Linux-target compile-only workflow
remain available.

Exceptions/unwinding, application RTTI, TLS, threads, inter-thread atomics,
filesystem/locale/iostream, dynamic/shared/PIE/FDPIC and a hosted GNU/Linux
SDK are outside this library profile. Big-endian libraries and other CPU
or argument ABI profiles are unqualified. QEMU Linux-user checks do not
establish hardware reset, interrupt or named-board compatibility.

See the release README and the installed SH4-LIBRARIES.rst and
SH4-FREESTANDING.rst for the complete contracts and rebuild instructions.
