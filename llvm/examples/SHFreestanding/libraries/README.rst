SH4 static C and C++ libraries
=============================

This opt-in profile combines Picolibc 1.8.12 (C and math), selected compiler-rt
helpers, and the matching in-tree libc++ and libc++abi. It targets little-endian
SH4's no-FPU instruction subset with the soft floating-point argument ABI,
static non-PIC/non-PIE ELF32 (EM_SH 42, e_flags 0x10).
All target libraries are built by the packaged Clang and LLVM archive tools.
The system compiler and the existing compile-only Linux triple are unchanged.

Use the shipped configurations, which resolve headers/libraries relative to
their own location and select the colocated LLD::

  clang --config=sh4-picolibc.cfg -O2 -c application.c -o application.o
  clang++ --config=sh4-libcxx.cfg -O2 -c application.cpp -o application.o
  clang++ --config=sh4-libcxx.cfg -Wl,--gc-sections -T board.ld \
    startup.o application.o libapplication.a -o application.elf

These configurations include ``-nostdlib`` and append an explicit static
library group after application inputs. They supply no board startup, memory
map or default linker script. C++ defaults to C++17, without exceptions or
RTTI. Do not override the profile's target/ABI options. A forced include checks
CPU, endianness, argument ABI, PIC/PIE, exceptions and RTTI for compiled sources.
LLD checks object machine flags; those flags cannot identify every argument ABI
in precompiled objects. All input providers must use matching options.

SH4 instruction selection and its argument ABI are independent on this backend.
``-msoft-float -mfloat-abi=soft`` alone still permits hardware arithmetic with
``-mcpu=sh4``. This configuration also selects the existing, tested
``-m4-nofpu`` mode, which emits the SH4 no-FPU machine flag 0x10 and uses
compiler-rt arithmetic. This is a real supported selector, separate from the
unsupported ``-m4`` spelling. The original Linux-target compile-only command
continues to emit SH4 flags 0x9. LLD rejects mixing 0x9 and 0x10 profiles;
compile startup and all input providers with matching options.
This profile runs on SH4 without requiring FPU initialization. A software
floating-point environment with selectable rounding/exception flags remains
unsupported; software helpers use round-to-nearest, ties to even.
The soft-ABI adaptation uses IEEE-754-2008 quiet/signaling NaN encoding to
match Clang constants and compiler-rt software operations, rather than the
opposite legacy SH hardware signaling-bit convention.

The sysroot is ``lib/clang-runtimes/shle-unknown-none-elf/sh4-soft``. Picolibc
includes its math implementation in ``libc.a``; the accompanying ``libm.a`` is
an upstream compatibility archive. Other archives are ``libc++.a``,
``libc++abi.a`` and ``libclang_rt.builtins.a``. The installed experimental C++
archive is a build component and is not an advertised experimental API profile.
``LIBRARIES.json`` records versions, source identities, recipe/patch hashes,
compiler identity, archive sizes and SHA-256s. Licenses and the exact Picolibc
source archive accompany the sysroot. ``runtimes/sh/picolibc.json`` pins the
upstream commit and source checksum; no moving upstream branch is used.

Runtime ownership and board contract
------------------------------------

Picolibc supplies memory/string functions, C stdio, math, allocation, errno,
``atexit``/``__cxa_atexit`` and init/fini array traversal. The board defines
aligned ``__heap_start``/``__heap_end`` bounds for its single-threaded ``sbrk``
allocator, or supplies its own correct ``sbrk`` implementation. libc++abi
supplies new/delete, guards and virtual-call ABI support. This profile's
compiler-rt archive contains compiler helpers, including complex arithmetic;
it omits the minimal runtime's duplicate memory and C++ definitions.

Do not combine these libraries with ``libclang_rt.sh4_cxx.a`` or the memory
providers in the older minimal profile. That independent profile remains at
``lib/shle-unknown-none-elf/sh4-soft`` with its original board-hook contract.

Supply reset/stack setup, data copying/BSS clearing, vectors, clocks,
MMU/cache initialization, a linker script and any other hardware setup.
After initialization, call ``__libc_init_array()``, then the application entry,
then ``exit(status)``. The example C++ entry uses ``extern "C" int main()``
because freestanding C++ does not give main the hosted linkage contract.
The script must retain the contiguous ``__bothinit_array_start/end`` range,
the ``__fini_array_start/end`` range, and ``.fini_array_onexit`` in the order
shown in ``memory.ld``. This is needed for registered C/C++ destruction.

Implement ``_exit`` and the I/O hooks required by the application. This
configuration uses POSIX-shaped ``read``, ``write``, ``close``, ``lseek`` and,
when opening files, ``open``. File metadata, clocks, entropy and process APIs
require additional platform implementations. A declaration or archive member
does not supply an OS or filesystem. Dummy OS stubs are not distributed.

The test ``platform.c`` exposes stdout/stderr through the independently
installed QEMU Linux-user emulator, gives stdin EOF, and reports unsupported
file descriptors/seeks with errors. ``fmemopen`` tests operate on target RAM.
This adapter supplies neither a filesystem nor a universal board image.

Scope and qualification
------------------------

The checker exercises multiple translation units and archive extraction at
O0/O2 through direct LLD, relocatable LLD and configured Clang links. It checks
ELF32/EM_SH/SH4 flags, program headers and load addresses independently, verifies
the installed library checksums, then optionally executes every output::

  python3 /path/to/toolchain/share/llvm-sh/examples/SHFreestanding/libraries/check.py \
    --prefix /path/to/toolchain --build-dir /path/to/scratch/library-check \
    --qemu /usr/bin/qemu-sh4-static --require-execution

Without an emulator it records ``LINK_ONLY`` and explicitly unrun execution.
The emulator is not bundled. This proves instruction/ABI behavior under QEMU,
not reset, interrupt or named-board behavior. Exact commands, versions, hashes,
outputs and execution exit statuses are retained in ``receipt.json``.

The corpus covers memory alignment/lengths, allocation/reallocation, sorting,
64-bit and floating varargs, conversion/errno, real and complex arithmetic,
binary32/binary64 comparisons (finite, NaN, signed zero and infinity),
quiet/signaling NaN classification and invalid square roots,
RAM stdio, setjmp/longjmp, atexit, global/local constructors and destructors,
guards, strings, vector/map/unordered_map, smart pointers, aligned allocation,
nothrow allocation failure/new-handler behavior, PMR and integer charconv.
Successful smoke tests are not full C/C++ conformance qualification.

* C memory, allocation, formatting, math and RAM streams: exercised by this
  corpus; board I/O, heap and startup requirements still apply.
* Math uses IEEE return-value semantics without math errno reporting. A
  configurable floating-point environment and exception flags are unsupported.
* C++17 containers, algorithms, strings, smart pointers, guards, static
  initialization and allocation: exercised; single-threaded only.
* Exceptions and unwinding: unsupported; no SH libunwind port is bundled.
* RTTI: unsupported for applications. libc++abi internally builds its type-info
  implementation with RTTI; this is not application RTTI qualification.
* TLS, threads, atomics used for inter-thread synchronization: unsupported.
* Filesystem, locale/iostream, wide-character C++ facilities, random_device,
  monotonic clock and time-zone database: disabled in libc++.
* C OS/process/clock/entropy APIs and file-backed stdio: conditional on board
  hooks; not qualified by the supplied console/RAM adapter.
* Big-endian libraries, other CPUs, hard-argument ABIs, shared libraries, PIE,
  hosted GNU/Linux SDK compatibility and FDPIC: not qualified by this profile.

Rebuild
-------

With a matching LLVM source checkout, Python 3.12+, CMake 3.24+, Meson and Ninja::

  python3 runtimes/sh/build-libraries.py --tools /path/to/toolchain/bin \
    --llvm-source /path/to/llvm-project --build-dir /path/to/scratch/libraries \
    --sysroot /path/to/output/sysroot

For an offline build, pass ``--picolibc-archive`` with the shipped pinned source
archive. The recipe verifies its SHA-256 before extraction. Its SH assembly
label-prefix adaptation uses Clang's ``__USER_LABEL_PREFIX__``; the soft-ABI
NaN adaptation is explicit in the recipe. The original archive and hashes of
both modified files are retained. Runtime builds use only LLVM
target tools, and are separate from the native host compiler build.
Compiler, archive-tool, source-pin, profile and recipe fingerprints invalidate
runtime object caches when their inputs change. Compiler provenance is recorded
in the manifest; target objects omit compiler-identification comment sections.
