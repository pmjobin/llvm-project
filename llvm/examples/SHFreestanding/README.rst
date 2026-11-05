SH4 freestanding static toolchain
================================

This example uses Clang, its SH integrated assembler or ``llvm-mc``, LLVM
archives and native SH ELF support in ``ld.lld``. No GNU SH linker, libgcc,
GNU SDK, target sysroot or host target library is required for this profile.
It is a freestanding toolchain, with the bounded runtime described below.

An additional opt-in Picolibc/libc++/libc++abi profile is described in
``libraries/README.rst``. It has separate startup, sysroot and library ownership
requirements; the minimal runtime documented here remains independent.

Profile
-------

Use ``--target=shle-unknown-none-elf -mcpu=sh4 -msoft-float
-mfloat-abi=soft -ffreestanding``. Use the same ABI options for every object,
assembly provider and runtime. The target produces little-endian ELF32 SH4
objects (EM_SH 42, e_flags 9). Links are static, non-PIE ET_EXEC or ``-r``
ET_REL. LLD also implements big-endian relocation encoding and the ``shelf``
emulation; the bundled runtime is **little endian only**.

Use code/data labels for short PC-relative branch and literal-load references.
Absolute numeric short PC-relative relocations retain a compile-only assembler
convention and are rejected by LLD rather than silently losing their target.

``shle-unknown-linux-gnu -mcpu=sh4 -c`` continues to produce the previously
supported Linux-target objects. It does not select this freestanding runtime
or supply a Linux SDK. Linux-target executable links need explicit static
options and compatible inputs; hosted Linux dynamic linking is unsupported.

The freestanding driver defaults to the colocated ``ld.lld`` (``ld.lld.exe``
on Windows). ``-fuse-ld=lld`` is also supported. To deliberately use another
linker, specify ``--ld-path=/absolute/path/to/sh-elf-ld``. A missing linker is
a diagnostic; the freestanding default does not fall back to GNU or host ld.

Always use ``-nostdlib`` for the workflow here, supplying your own startup,
linker script and the explicit runtime archive. This avoids implicit hosted
startup, C/C++ libraries or GNU runtime requests. For example, after compiling
the sources and assembling startup, a C executable can be linked with::

  clang --target=shle-unknown-none-elf -mcpu=sh4 -nostdlib -static \
    -fuse-ld=lld -Wl,--gc-sections -T memory.ld \
    start.o start-c.o application.o libapplication.a \
    /path/to/toolchain/lib/shle-unknown-none-elf/sh4-soft/lib/libclang_rt.builtins.a \
    -o application.elf

The runtime is installed under ``lib/shle-unknown-none-elf/sh4-soft/``;
add its ``include/`` directory for the bounded C headers. Archive resolution,
not a host library search, supplies helpers. For the optional C++ subset,
add ``lib/libclang_rt.sh4_cxx.a`` before ``lib/libclang_rt.builtins.a`` and
compile with ``-fno-exceptions -fno-rtti``. A C++ entry called by the supplied
C startup must be declared ``extern "C" int main()``: freestanding C++ does
not give an ordinary function named main the hosted entry contract.

Startup and execution
---------------------

``start-board.s`` and ``board-memory.ld`` illustrate stack setup, copying
initialized data from its ROM load address, clearing BSS, separate ROM/RAM
regions and calling ``sh4_start``. Replace the addresses and sizes for the
actual board. Supply boot code, reset/interrupt/trap vectors, clocks, address
mapping, privilege/MMU/cache configuration and any required hardware setup.
Provide ``platform_write`` and ``platform_exit`` for this example. The
template is not a universal board image and has not qualified a named board.
Account for ELF load addresses versus virtual addresses when programming ROM.

``start.c`` calls the preinit and init arrays before main, finalizes registered
C++ destructors, and runs the fini array in reverse order. The optional
``after_fini`` test hook checks destruction after main returns. Ordinary
applications need not define it. The board controls what exiting means.

``start-qemu-user.s`` is a separate execution harness. QEMU's Linux-user ELF
loader supplies data/BSS; the harness supplies a stack and uses Linux write
and exit traps for observation. The application and target runtimes remain
freestanding, with no target libc. This demonstrates SH instruction and ABI
behavior under that emulator, not reset/vector/interrupt behavior on hardware.
The QEMU executable is an independent test prerequisite and is not shipped in
the compiler distribution.

From an installed or relocated distribution, run::

  python3 /path/to/toolchain/share/llvm-sh/examples/SHFreestanding/check.py \
    --prefix /path/to/toolchain --build-dir /path/to/scratch/sh4-check \
    --qemu /usr/bin/qemu-sh4-static --require-execution

On Windows, use ``py -3`` or the installed Python executable and native paths.
Without ``--qemu`` the checker records **PARTIAL**, with execution explicitly
unrun. It requires every packaged tool, clears external compiler/library
search variables, independently checks ELF headers and archive members, and
executes C, runtime and C++ lifecycle programs at O0/O2 through direct LLD,
default Clang, explicit LLD and relocatable links. It writes exact commands,
outputs, exit statuses, ELF sizes/hashes and observations in ``receipt.json``.

Runtime and language scope
--------------------------

The portable compiler-rt source selection is explicit in
``compiler-rt/utils/build-sh4-freestanding.py``. The archive supplies the
observed integer division/remainder, 64-bit arithmetic/conversion and
software binary32/binary64 helpers, plus memcpy, memmove, memset and memcmp.
The checker includes direct assembly verification of the 64-bit argument
and result registers. Software floating point uses round-to-nearest, ties to
even; a floating-point environment and hardware exception/status flags are
not provided. C arithmetic with undefined behavior (for example division by
zero or signed division overflow) is not a supported numerical result.
Freestanding abort raises SH trap 255 and stops if its handler returns.

The optional C++ archive supports single-threaded global/local construction,
static guards (including abort/retry), virtual dispatch without RTTI, scalar
and array new/delete, sized delete and at most **64 lifetime destructor
registrations**. Finalization is in reverse registration order, including
destructors registered by a running destructor, and is idempotent. This small
implementation does not reclaim registration slots.
The application supplies ``sh4_allocate``, ``sh4_deallocate`` and non-returning
``sh4_panic`` as declared in ``sh4_freestanding.h``. The example uses a bounded
heap; it is not a general allocator. Recursive local initialization calls the
panic hook. Allocation failure also calls that hook. Over-aligned allocation
forms, nothrow allocation and concurrent initialization are not provided.

=============================== =============================================
Feature                         Status in this profile
=============================== =============================================
C integer/memory operations     Supplied helpers and execution examples
Software float/double           Portable helpers; no fenv/status interface
C++ constructors/destructors    Bounded single-threaded ABI subset above
C++ allocation                  Conditional on application hooks
Exceptions / stack unwinding    Unsupported runtime; metadata relocation only
RTTI / dynamic_cast             Unsupported runtime
TLS / threading / atomics       Unsupported runtime; TLS sections rejected
libc++ / hosted C++ library     Not supplied or qualified
Hosted C / I/O / malloc          Not supplied; platform services belong to app
Complex / extended precision    No general runtime support claim
Hard-float / other argument ABI  Not qualified with the supplied soft runtime
Other CPUs / PIC / FDPIC         Rejected by this initial SH4 ELF linker
Dynamic/shared/PIE links         Rejected
=============================== =============================================

CPU flags do not encode all SH calling-convention and FPU choices. LLD
rejects incompatible machine/unknown flags but cannot detect every ABI
mismatch that producers do not record. Do not mix hard and soft argument
transport merely because their e_flags happen to agree.

Full compiler-rt CMake target enablement, LLVM libc, libc++, libc++abi and
libunwind are separate ports. Compiler-rt's generic sources are feasible for
this bounded recipe, but its supported-architecture configuration does not
yet enable SH generally. libc has no SH bare-metal configuration; libunwind
has no SH register/context implementation. Hosted libc++/libc++abi sources
require further platform services, atomics and an unwinder. Their presence
in the source tree is not evidence of working SH libraries.

The runtime sources and recipe are included with the distribution, with
Apache-2.0 WITH LLVM-exception license texts, source hashes and build flags in
``RUNTIME.json``. Rebuild using the packaged Clang and llvm-ar and the same
profile; no executable GNU target tools or host target libraries are inputs.
