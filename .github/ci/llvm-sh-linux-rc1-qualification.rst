LLVM SH RC1 Linux packaging qualification
========================================

Automation and source identities
-------------------------------

* Repository: ``pmjobin/llvm-project``; base branch: ``llvm-sh``.
* Repair branch: ``fix/linux-rc1-packaging`` from
  ``55d798a6bad82da5ae77212c7b2ddad4d98d7842``.
* Automation changes: ``1ca6b1f50b72e49467d882238518b8e977e897ac``;
  checksum logging: ``28ade2b2ca0513e069e1b36eabaa12e9c7200c78``.
* Compiler tag: ``llvm-sh-rc1`` (unchanged annotated tag object
  ``272a3118d560572582c25d96cead1a219395f545``).
* Compiler commit: ``a4ad03d58bd5a924de0ae286340ff7c6bb664ec2``.
* Compiler tree: ``afbc1ebbfcbae3811208ff4dcfb0930208d3f8a1``.
* Separate automation and immutable source checkouts are retained. The build
  script rejects a mismatched commit or tree before configuring LLVM.
* MSVC PR #1 and ``fix/sh-msvc-portability`` remain at
  ``2e3a05f5b99e7b058f9bae85660664710708b9a3``.

Changed files
-------------

* ``.github/ci/build-llvm-sh-linux.sh``: correct target selection; assert ELF32,
  little endian, relocatable, EM_SH and SH4 flags; inspect and install native
  packages; check runtime libraries and preservation of system Clang; compare
  ZIP contents, modes and symlinks and execute with original locations hidden;
  record timings, sizes and verified checksums.
* ``.github/workflows/llvm-sh-rc1-packages.yml``: add manual platform selection
  (``all`` by default, or ``linux`` / ``windows``); include qualification text
  in explicitly opted-in artifact uploads. Windows build steps are unchanged.
* ``.github/workflows/llvm-sh-packaging-validation.yml``: inexpensive shell
  syntax, ShellCheck warning-level and checksum-pinned actionlint validation on
  relevant pushes and pull requests; no compiler builds.
* This file: qualification evidence and scope.

Driver investigation
--------------------

The original command rejected ``-m4`` for ``sh4-unknown-linux-gnu``. Removing
``-m4`` exposed ``unknown target triple 'sh4-unknown-linux-gnu'`` in the actual
RC1 compiler. RC1 ``llvm/lib/TargetParser/Triple.cpp`` recognizes ``sh`` and
``shle``; it does not recognize ``sh4``. RC1
``llvm/include/llvm/TargetParser/SHTargetParser.h`` defaults to SH2.

The qualified smoke command is therefore::

  <tested-prefix>/bin/clang --target=shle-unknown-linux-gnu -mcpu=sh4 \
    -c llvm-sh-smoke.c -o smoke-sh4.o
  <tested-prefix>/bin/llvm-readobj --file-headers smoke-sh4.o

Required output: ``elf32-shl``, ``shle``, 32-bit, LittleEndian,
``Relocatable (0x1)``, ``EM_SH (0x2A)``, flags ``0x9`` (``EF_SH4``).
The compiler's ``-###`` output is logged for each tested prefix. No compiler
implementation, source tag, pinned source identity or code generation changed.

GitHub Actions evidence
-----------------------

* Draft PR: https://github.com/pmjobin/llvm-project/pull/2
* Initial run (cancelled early to add visible checksum values while keeping
  uploads disabled):
  https://github.com/pmjobin/llvm-project/actions/runs/41723994735
* Qualification run on ``28ade2b2c``: PASS (2026-11-03):
  https://github.com/pmjobin/llvm-project/actions/runs/41724679129
* Ubuntu 26.04 DEB: PASS:
  https://github.com/pmjobin/llvm-project/actions/runs/41724679129/job/126226600926
* Fedora 45 RPM and portable ZIP: PASS:
  https://github.com/pmjobin/llvm-project/actions/runs/41724679129/job/126226600673
* Shell/YAML validation on ``28ade2b2c``: PASS:
  https://github.com/pmjobin/llvm-project/actions/runs/41724671385

The Fedora job ran on 2026-11-03 from 21:46:22 to 22:32:24 UTC (46m02s).
Its build/package/test step took 42m15s. Fedora image digest:
``sha256:a7d72ba2745b100a3351087472e5ee7b2450af7cf331e7673007e3817612839c``.
Ubuntu ran from 21:46:22 to 22:45:01 UTC (58m39s); its build/package/test step
took 52m18s. Windows was skipped by platform selection. All five smoke objects
(staged and installed on each distribution, plus the relocated ZIP) met every
required SH4 header check. Local Fedora 44 preflights are not counted as
distribution qualification. Later commits only document these results; the
qualified CI scripts are byte-identical to ``28ade2b2c``.

Package results
---------------

Generated files (all three ``sha256sum --check`` results were ``OK``):

* ``llvm-sh-rc1-ubuntu2604-amd64.deb``: 69,481,340 bytes.
  SHA-256:
  ``d3dda176690e71bdab9597a76125881194f95e496c4019bc36c3f3ae353a97ff``.
* ``llvm-sh-rc1-fedora45-x86_64.rpm``: 49,601,374 bytes.
  SHA-256:
  ``8adb0833e7a734fc772b9ab1cbf50de143854295cf08106fb82d688af144ced0``.
* ``llvm-sh-rc1-fedora45-linux-x86_64.zip``: 69,309,474 bytes.
  SHA-256:
  ``1561e80d7d55319e834139ef2c88ec049e604ef54db0faa5573e020b31ff1e12``.

Measured phase durations in seconds:

==================== ============ ===========
Phase                Ubuntu DEB   Fedora RPM
==================== ============ ===========
Configure            22           12
Build                3090         2416
Stage                1            1
Staged smoke         0            0
CPack                18           91
Package installation 4            2
Installed smoke      0            0
ZIP creation         --           9
Relocated smoke      --           0
==================== ============ ===========

Build durations were 51m30s on Ubuntu and 40m16s on Fedora. Smoke invocations
recorded 0s at whole-second resolution. Archive integrity/extraction,
mode/content comparison, library checks and checksum verification are also
included in the overall build/package/test step durations above.

Ubuntu installation evidence
----------------------------

APT installed exactly one new package, with no upgrades or removals. It logged
``Setting up llvm-sh-clang (0.1.0~rc1-1)`` at 22:44:55 UTC.
``dpkg-query`` confirmed
``llvm-sh-clang 0.1.0~rc1-1 amd64 install ok installed``; ``dpkg -V`` passed.
DEB metadata included the intended maintainer, section, description and
architecture; its payload stayed under the private prefix, including ancestor
directories. Generated dependencies were::

  libc6 (>= 2.38), libedit2 (>= 2.11-20080614-0), libgcc-s1 (>= 4.3),
  liblzma5 (>= 5.1.1alpha+20110809), libstdc++6 (>= 12),
  libxml2-16 (>= 2.14.1), libzstd1 (>= 1.5.5), zlib1g (>= 1:1.2.0)

The installed ``/opt/llvm-sh/rc1/bin/clang`` reported the frozen source commit
and generated the verified SH4 object using the installed ``llvm-readobj``.
All checked runtime libraries resolved without ``LD_LIBRARY_PATH``.
The system ``/usr/bin/clang`` still resolved to ``/usr/lib/llvm-21/bin/clang``
with unchanged SHA-256
``78e9dd2ccc9e88859b3a29109a44bfab00e3d6cb70f00f3daae063eed3d05251``.

Nonfatal Ubuntu warnings were CMake's CMP0219 policy notice and two
``GNUInstallDirs`` author warnings during symlink installation. No Ubuntu
package-creation, installation, runtime-resolution or smoke failure remained.

Fedora installation and relocation evidence
-------------------------------------------

DNF installed ``llvm-sh-clang-0.1.0-0.rc1.1.x86_64`` from ``@commandline`` at
22:32:07 UTC; transaction output reported ``Complete!``. ``rpm -qi`` verified
name, version, release, architecture, license and install date; ``rpm -V``
passed. Payload paths stayed under the private prefix (including its ancestor
directories). ``/opt/llvm-sh/rc1/bin/clang`` reported the frozen RC1 commit and
generated a verified SH4 object using its packaged ``llvm-readobj``.

The system Clang path remained ``/usr/sbin/clang`` (resolving to
``/usr/bin/clang-23``), with unchanged SHA-256
``d2dae44724b0897d21c5e0697c9991b24b5fddd7908a4b5c19f8cae92fd2907d``.

The ZIP passed integrity, file-content, executable-mode and symlink comparisons
against RPM staging. Its compiler executed from
``/__w/_temp/llvm-sh-rc1-zip-relocation/rc1/bin/clang`` while original locations
were hidden, and generated the same required SH4 object format. ``ldd`` resolved
private LLVM/Clang libraries inside that extracted tree; no runtime library was
missing. No second LLVM build occurred.

Fedora runtime requirements and warnings
---------------------------------------

Observed external runtime SONAMEs (including transitive dependencies):
``libc.so.6``, ``libm.so.6``, ``libstdc++.so.6``, ``libgcc_s.so.1``,
``libedit.so.0``, ``libz.so.1``, ``libzstd.so.1``, ``liblzma.so.5``,
``libxml2.so.16``, ``libtinfo.so.6`` and the x86-64 ELF loader
``ld-linux-x86-64.so.2``. ``libLLVM.so.24.0git`` and
``libclang-cpp.so.24.0git`` are included in the distribution.

RPM dependency inspection includes ``GLIBC_2.44`` on libm, ``GLIBC_ABI_DT_RELR``,
``GLIBCXX_3.4.30`` and ``CXXABI_1.3.13``. The ZIP uses the same built libraries
and requires compatible versions of these system libraries. Successful
relocation on Fedora 45 does not establish compatibility with older glibc,
other Linux distributions, or RHEL.

Nonfatal Fedora warnings: two upstream CMake ``GNUInstallDirs`` developer
warnings during symlink installation, and DNF skipping OpenPGP checks for the
unsigned local RPM. RPM summary/license are present; its long description uses
CPack's generic ``no package description available`` fallback. No functional
package-creation, installation, runtime-resolution or smoke failure remained.

Compatibility and publication scope
-----------------------------------

The native packages use ``/opt/llvm-sh/rc1``. The Fedora ZIP is made directly
from the same staged distribution used for the RPM; it does not rebuild LLVM.
The relocation check hides the original build, staging and installation paths,
and unsets ``LD_LIBRARY_PATH`` and ``LD_PRELOAD``.

The compiler distribution contains LLVM/Clang and Clang resource headers, with
no GNU SH sysroot/SDK, emulator, Doom or LLVM runtimes. The compile-only smoke
does not demonstrate linking or execution of SH programs. The portable ZIP
does not imply universal Linux compatibility: it needs compatible system
runtime libraries. RHEL and other distributions are untested.

All qualification dispatches use ``platform=linux`` and
``upload_artifacts=false``. The existing upload opt-in gate remains intact.
No GitHub Release, downloadable package upload, tag update, merge or change to
MSVC PR #1 is part of this work.
The completed qualification run has zero uploaded artifacts. Windows build
validation remains outside this Linux task; its build script is unchanged.
