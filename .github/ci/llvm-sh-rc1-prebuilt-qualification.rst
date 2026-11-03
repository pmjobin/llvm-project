LLVM SH RC1 prebuilt release qualification
=========================================

This record covers binaries for the refrozen RC1 source, rather than the
earlier packages built from ``a4ad03d58bd5a924de0ae286340ff7c6bb664ec2``.
Only packages with passing native qualification and independently verified
downloaded checksums/provenance are attached to the release.
All three native jobs passed for this source. Seven staged, installed and
relocated smoke objects passed their SH4 checks; four binary packages and their
individual checksums are published. Windows SH regression-suite execution is
not included in this package qualification.

Identities and recipes
----------------------

* Source tag: ``llvm-sh-rc1``; annotated tag object:
  ``c5c7a56150522b4642a81365a071caa47e5f0c99``.
* Source commit: ``090c682597f7155373681807330fbb0ac343a33c``.
* Source tree: ``55640dd25d624b11b9e0f28a7903eea42a429254``.
* Automation branch: ``fix/rc1-prebuilt-packages``.
* Qualified build recipes: ``8cc110b2ee91b3645a8ea350b8c321eda5f74bc5``.
* Release: https://github.com/pmjobin/llvm-project/releases/tag/llvm-sh-rc1
* Build run: https://github.com/pmjobin/llvm-project/actions/runs/41766778569
* Dispatch: ``platform=all``, ``upload_artifacts=true``. The owner explicitly
  requested prebuilt release packages; the opt-in upload gate remains intact.

The source/tag identity is unchanged. The automation changes correct Windows
smoke target selection, check staged and extracted SH4 objects/resource headers,
record host DLL imports and timings, and hide original build/staging paths
during ZIP relocation. Both platforms embed source, automation and run identity
in ``BUILDINFO.txt``. RPM long-description metadata is corrected.

Local checks passed: Bash syntax, ShellCheck warning level, actionlint,
PowerShell 7.6.9 syntax/path handling, real SH4 header acceptance and rejection
of wrong machine, endian, CPU flags and ELF class. These local checks do not
substitute for native package qualification. The inexpensive Actions validation
also passed:
https://github.com/pmjobin/llvm-project/actions/runs/41766765407

Fedora 45 RPM and portable ZIP: PASS
-----------------------------------

Native job:
https://github.com/pmjobin/llvm-project/actions/runs/41766778569/job/126366882077

The job ran on 2026-11-04 from 03:23:11 to 03:52:40 UTC (29m29s). Its
build/package/test step took 27m34s. Measured phases: configure 11s; build
1561s (26m01s); stage 1s; CPack 70s; DNF install 2s; ZIP creation 6s.
Staged, installed and relocated smoke invocations each recorded 0s at
whole-second resolution; other verification is included in the overall step.

DNF installed ``llvm-sh-clang-0.1.0-0.rc1.1.x86_64`` and reported ``Complete!``.
Package metadata, dependencies and private-prefix contents passed inspection;
``rpm -V`` passed. The installed ``/opt/llvm-sh/rc1/bin/clang`` and installed
``llvm-readobj`` generated/verified little-endian ELF32 relocatable ``EM_SH``
objects with SH4 flags ``0x9``. The system compiler remained
``/usr/sbin/clang`` -> ``/usr/bin/clang-23``, with unchanged SHA-256
``d2dae44724b0897d21c5e0697c9991b24b5fddd7908a4b5c19f8cae92fd2907d``.

The ZIP came from the same staging tree used for RPM input, without another
LLVM build. Integrity, file contents, modes and symlinks matched after extraction.
The extracted compiler and private libraries worked with the
original build, staging and installed prefix hidden. Runtime libraries resolved
without ``LD_LIBRARY_PATH`` or ``LD_PRELOAD``.

Published files, checked against CI sidecars and GitHub release asset digests:

* ``llvm-sh-rc1-fedora45-x86_64.rpm``: 49,595,395 bytes; SHA-256
  ``8645e3a6e3929df803abe7423efdc511496199ea3a38560a0d116cf325f8a4c8``.
* ``llvm-sh-rc1-fedora45-linux-x86_64.zip``: 69,309,627 bytes; SHA-256
  ``a052725b1dcb5e895abf5773bb932dfc0a92d6e21a0ea66bc5b36256e82634f1``.

Each has its own published ``.sha256`` sidecar. Local inspection also checked
the embedded commit/tree/automation/run identities, RPM metadata/private paths,
ZIP licenses, relative symlinks, executable mode and host x86-64 ELF identity.

Fedora runtime requirements include the x86-64 ELF loader, libc, libm,
libstdc++, libgcc_s, libedit.so.0, libz.so.1, libzstd.so.1, liblzma.so.5,
libxml2.so.16 and libtinfo.so.6. Private LLVM/Clang libraries are included.
RPM symbol requirements include libm ``GLIBC_2.44``, ``GLIBC_ABI_DT_RELR``,
``GLIBCXX_3.4.30`` and ``CXXABI_1.3.13``. ZIP relocation establishes Fedora 45
compatibility with these libraries; other distributions and RHEL are untested.

Remaining nonfatal Fedora diagnostics: upstream ``GNUInstallDirs`` developer
warnings and DNF skipping OpenPGP checks for the unsigned local RPM. The long
description now contains the intended toolchain description.

Ubuntu 26.04 DEB: PASS
---------------------

Native job:
https://github.com/pmjobin/llvm-project/actions/runs/41766778569/job/126366881663

The job ran on 2026-11-04 from 03:23:11 to 04:34:05 UTC (1h10m54s). Its
build/package/test step took 1h09m21s. Measured phases: configure 27s; build
4107s (1h08m27s); stage 1s; CPack 20s; APT install 3s. Both staged and installed
smokes recorded 0s at whole-second resolution.

APT installed ``llvm-sh-clang 0.1.0~rc1-1 amd64`` and ``dpkg-query`` reported
``install ok installed``. ``dpkg -V`` passed. Metadata, generated dependencies,
private-prefix payload and installed compiler execution passed. Installed Clang
and ``llvm-readobj`` generated/verified little-endian ELF32 relocatable
``EM_SH`` objects with SH4 flags ``0x9``; runtime libraries resolved without
an ``LD_LIBRARY_PATH`` override. System ``/usr/bin/clang`` still resolved to
``/usr/lib/llvm-21/bin/clang``, with unchanged SHA-256
``78e9dd2ccc9e88859b3a29109a44bfab00e3d6cb70f00f3daae063eed3d05251``.

Published ``llvm-sh-rc1-ubuntu2604-amd64.deb``: 69,481,548 bytes; SHA-256
``a3c757c0e61fc8fa537d290350153c0ba5219174766998851ee010d356165dc7``.
CI sidecar, independently downloaded file and GitHub asset digest match.
Local DEB metadata, private payload paths and embedded source/tree/automation/run
identities passed independent inspection.

Generated dependencies: ``libc6 (>= 2.38)``,
``libedit2 (>= 2.11-20080614-0)``, ``libgcc-s1 (>= 4.3)``,
``liblzma5 (>= 5.1.1alpha+20110809)``, ``libstdc++6 (>= 12)``,
``libxml2-16 (>= 2.14.1)``, ``libzstd1 (>= 1.5.5)`` and
``zlib1g (>= 1:1.2.0)``. Remaining nonfatal Ubuntu diagnostics were CMake's
CMP0219 policy notice and ``GNUInstallDirs`` author warnings.

Windows x64 ZIP: PASS
---------------------

Native job:
https://github.com/pmjobin/llvm-project/actions/runs/41766778569/job/126366882029

The job ran on 2026-11-04 from 03:23:12 to 04:24:37 UTC (1h01m25s). The
build/package/test step took 59m11s. CMake identified the native C/C++ compiler
as MSVC 19.44.35229.0, using Visual Studio 2022 x64 tools and Windows SDK
10.0.26100.0. Measured phases: configure 69.29s; full Clang/tool build 3459.73s
(57m39.73s); stage 1.81s; staged smoke 0.40s; ZIP 14.80s; extraction 2.40s;
relocated smoke 0.63s.

Both the staged and extracted ``clang.exe`` reported source commit
``090c682597f7155373681807330fbb0ac343a33c``. The smoke includes ``stddef.h``;
resource queries and compilation used the headers inside the tested prefix.
Each object's packaged ``llvm-readobj.exe`` output passed all nine checks:
ELF32 SH little-endian format/architecture/class/encoding, relocatable type,
``EM_SH``, flags ``0x9``, nonempty text and the ``sh_add`` symbol.
The relocated compiler ran with the original build/staging locations hidden.

Published ``llvm-sh-rc1-windows-x64.zip``: 311,319,083 bytes; SHA-256
``f74889acf7e557fe79097e837851536b486534bd3389e41fd49d67ebf669bb31``.
The CI sidecar, independently downloaded file and GitHub asset digest match.
Local ZIP integrity, PE x64 machine identity, license files and embedded
source/tree/automation/run identities passed inspection.

The ZIP was tested on Windows Server 2022 x64. PE imports include
``MSVCP140.dll``, ``VCRUNTIME140.dll``, ``VCRUNTIME140_1.dll``, Universal CRT
API sets and standard Windows system DLLs. Runtime DLLs are not bundled.
Use a compatible x64 v14 runtime as described by `Microsoft's runtime
documentation <https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist/>`_.
Other Windows versions are untested.

Nonfatal diagnostics: four MASM A4018 notices for ``/MP``, two existing SH
C4018 signed/unsigned comparisons and an upstream C4996 deprecation warning.
The optional DIA SDK was not found. No compilation, staging or relocation
failure remained. Full Windows SH regression-suite qualification was not run
by this package job and remains separate from these passing package checks.

Scope and recovery
-------------------

Packages contain LLVM/Clang tools, Clang resource headers and license texts.
They do not add a GNU SH SDK/sysroot, emulator, Doom or target runtime bundle.
The SH4 smoke is compile-only and does not qualify target linking/execution.
No source/tag rewrite or branch merge is part of this package publication.

Raw logs, original release notes, downloaded artifacts, metadata/checksum
inspection, published asset receipts and scripts used for this delivery are
retained under
``~/Workspace/codex/work/llvm-sh/runtime/rc1-prebuilt-packages/``.
