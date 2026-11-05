SuperH target (experimental)
===========================

This document describes the compiler's intended interfaces and architectural
constraints. The target is experimental. Compiling an instruction or accepting
an ABI option does not establish runtime support on every processor, operating
system, simulator, or hardware board. GNU runtime compatibility and execution
qualification must be established for the exact compiler and runtime used.

Build and target selection
--------------------------

Enable SH through LLVM's experimental-target mechanism, using an out-of-source
build. For example, from the monorepo root::

  cmake -S llvm -B /external/llvm-build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_ENABLE_PROJECTS=clang -DLLVM_TARGETS_TO_BUILD=X86 \
    -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=SH -DLLVM_INCLUDE_TESTS=ON
  cmake --build /external/llvm-build --target llvm-test-depends clang-test-depends
  cmake --build /external/llvm-build --target check-llvm check-clang

Choose compilation, linking, and test parallelism for the host's resources.
The test dependency targets build tools that the ordinary default build does
not necessarily provide.

The canonical architecture names are ``sh`` for big endian and ``shle`` for
little endian. CPU selection, endianness, hardware FPU capabilities, argument
transport ABI, and operating-system runtime are separate choices. A driver
example for a GNU/Linux SH4 little-endian compilation is::

  clang --target=shle-unknown-linux-gnu -mcpu=sh4 -c example.c -o example.o

Hosted GNU linking requires a compatible GNU sysroot, GNU binutils, and
explicit library/tool search paths. For freestanding SH4, LLD now implements
static executable and relocatable ELF32 linking. The separate bounded runtime,
startup examples and application responsibilities are documented in
``llvm/examples/SHFreestanding/README.rst``. This does not supply a hosted GNU
SDK or complete C++ standard library. Cross compilation must not silently use
host target libraries or an older compiler executable.

The driver diagnoses unsupported selectors and incompatible CPU, FPU, tune,
or ABI combinations. Family encoding definitions and compile-only tests do
not imply SH2, SH3, SH2A, SH4A, DSP, or big-endian application qualification.

Native MSVC development on Windows
----------------------------------

``.github/workflows/llvm-sh-msvc-dev.yml`` provides a separate development
check on ``windows-2022`` with Visual Studio 2022's native ``cl.exe``, Ninja,
and sccache. Checkout includes LLVM, Clang, their CMake/third-party
dependencies, libc common headers, and the CI scripts, omitting unrelated
monorepo projects. The libc headers are an unconditional LLVM configuration
dependency; their presence does not enable any libc runtime build.
Pull requests targeting ``llvm-sh`` build the PR source commit;
manual runs build the selected branch commit. The first build target is
``LLVMSHCodeGen`` with no enabled Clang project, no other target backends, and
no packaging. Ninja stops scheduling work after the first compilation error.
LLVM support libraries and TableGen dependencies still need to build on a
cold runner. A bounded 2 GiB sccache disk cache is restored and saved using
GitHub Actions caching, following the existing LLVM CI cache pattern.

After pushing the development branch, open a PR targeting ``llvm-sh`` for
the default backend check. To run it manually with GitHub CLI::

  git push -u origin fix/sh-msvc-portability
  gh workflow run llvm-sh-msvc-dev.yml --ref fix/sh-msvc-portability

The Actions UI also supports selecting this workflow and branch. GitHub
requires a ``workflow_dispatch`` workflow to exist on the repository's
default branch before manual dispatch is available; the PR trigger can
test a newly added workflow before that. GitHub scopes caches created by a
PR run to that PR. After the workflow is merged, run the backend once on
``llvm-sh`` to seed a base-branch cache that future PRs can restore::

  gh workflow run llvm-sh-msvc-dev.yml --ref llvm-sh

Full validation is an explicit
manual opt-in, after the backend passes::

  gh workflow run llvm-sh-msvc-dev.yml --ref fix/sh-msvc-portability \
    -f full_validation=true
  gh run list --workflow llvm-sh-msvc-dev.yml --branch fix/sh-msvc-portability
  gh run view RUN_ID --log-failed

Full validation installs Python's ``psutil`` dependency for lit timeouts,
reuses the same build tree, enables and builds Clang plus
LLVM/Clang test dependencies, and compiles a C function for both
``shle-unknown-linux-gnu`` and ``sh-unknown-linux-gnu``. It checks ELF32,
endianness, relocatable type, ``EM_SH``, code, and the function symbol.
It then runs ``llvm/test/CodeGen/SH``, ``llvm/test/MC/SH``, the Clang
``sh-*`` tests, and ``clang/test/CodeGen/SH`` through lit. Platform feature
requirements determine which tests are applicable; unsupported tests must
be distinguished from passing tests in the reports. This does not test
target execution, a GNU sysroot, or distribution packaging.

Each run records the source revision, native compiler path and ``/Bv``
identity, CMake/Ninja/sccache versions, elapsed command times and exit codes,
compiler diagnostics, and sccache statistics. Optional validation adds ELF
objects, inspection logs, and lit XML results. The job summary and the
``sh-msvc-dev-RUN_ID-ATTEMPT`` diagnostic artifact remain available after
failure; artifact retention is 14 days. Compare backend time and cache hits
on cold and warm runs before making speed claims.

For local iterations, start PowerShell 7 from an **x64 Native Tools Command
Prompt for VS 2022**, with Ninja, CMake, Python, and sccache on ``PATH``::

  pwsh -NoProfile
  ./.github/ci/test-llvm-sh-msvc.ps1 -Stage Backend
  # Run only when full validation is wanted:
  ./.github/ci/test-llvm-sh-msvc.ps1 -Stage Validate
  ./.github/ci/test-llvm-sh-msvc.ps1 -Stage Report

Local defaults put builds and logs in ``%TEMP%/llvm-sh-msvc-dev-build`` and
``%TEMP%/llvm-sh-msvc-dev-logs``. Optional ``-BuildDir`` and ``-LogDir``
arguments support other out-of-source locations; use the same directories
for each stage. Repeating the backend stage on a reused build directory
resets the enabled projects to the backend-only configuration.

The MSVC portability fix compares each instruction operand's explicit
``Use::get()`` value with the positive ``ConstantFP`` pointer. This preserves
the existing size-optimized signed-double-literal sharing policy while
avoiding the rejected heterogeneous comparison inside ``is_contained``.

Local validation on 2026-11-03 compiled ``SHISelLowering.cpp`` with Linux
Clang 22.1.8 (5.53 seconds) and GCC 16.2.1 (10.16 seconds), with no compiler
diagnostics. Matching existing generated headers and support libraries were
reused; all 2,406 LLVM header files and the other 79 SH files matched the
current sources. Replacing the compiled object in a separate SH archive and
relinking a separate LLVM library took 0.60 seconds. Against that rebuilt
library, all 641 LLVM SH CodeGen/MC tests passed (16.61 seconds), as did all
94 selected Clang SH tests (13.32 seconds). The signed-double-literal test
also passed independently in both endian modes and argument ABIs.
``actionlint`` 1.7.12 and PowerShell 7.6.9 parsing passed. Script control-flow
checks used simulated build commands to verify backend isolation, failure
propagation and report persistence; real Linux-hosted Clang smoke objects
were accepted and a corrupted ELF machine field was rejected. A separate
configure-only Ninja graph check confirmed that the backend build excludes
Clang sources and the optional validation targets exist. These checks
do not run native MSVC. Local sccache measurements are unavailable because
the local compilation used the Linux compilers directly.

The first `native Windows PR run
<https://github.com/pmjobin/llvm-project/actions/runs/41666512976>`_ passed
on commit ``2ec6088b23bafccaa1d78e5578f5c790a197d683`` with Visual Studio
2022 x64 ``cl.exe`` 19.44.35229.0, CMake 3.31.6, Ninja 1.13.2, and sccache
0.17.0. CMake identified both compilers as MSVC and targeted SH only; the
log records compilation of all 27 SH CodeGen sources, including
``SHISelLowering.cpp``, and creation of ``LLVMSHCodeGen.lib``. Configuration
took 31.88 seconds and the cold backend build took 3263.87 seconds
(54 minutes 23.87 seconds). The complete job took 57 minutes 12 seconds.
The cold cache recorded 1,722 misses, zero hits, zero compilation failures,
zero cache errors, and zero non-cacheable compilations; all entries fit in
834 MiB of the 2 GiB cache, which was saved successfully. The empty-cache
build is costly because LLVM support dependencies compile before SH.
The `warm-cache PR update
<https://github.com/pmjobin/llvm-project/actions/runs/41676793100>`_ also
passed on a fresh Windows runner after documentation-only commit
``f22b9e33b49f8e12d7442a6cad0b2e13ab8c0ed6``. GitHub restored the saved
cache successfully. The backend build took 283.80 seconds (4 minutes
43.80 seconds), 11.50 times faster than the cold build. Configuration took
48.32 seconds and the complete job took 9 minutes 2 seconds. sccache
reported 1,719 hits and three misses from 1,722 requests (99.83% hits),
zero compilation/cache errors, and a final cache size of 837 MiB. These
are measured runs with two build jobs, not guarantees for every runner.
Source/compiler changes and cache eviction can increase later build times.

The warm run spent 2 minutes 34 seconds checking out the full monorepo.
The workflow now narrows checkout to the source directories listed above.
The subset must include ``libc`` for ``FindLibcCommonUtils`` even though
``LLVM_ENABLE_RUNTIMES`` is empty. This dependency is checked during CMake
configuration, before the compiler cache can help. An isolated sparse
worktree configuration checked both backend and optional Clang validation
dependency targets without building LLVM/Clang. Directory symlinks are
unsuitable for this check because relative paths can escape into the
complete source tree.

The native build emitted two nonfatal C4018 warnings at the existing
unsigned-value comparisons with positive signed limits in
``SHInstrInfo.cpp`` (lines 307 and 318), plus an upstream C4996 deprecation
warning in ``llvm/lib/IR/Core.cpp``. No further MSVC compilation errors
occurred. The runner also reported that the pinned MSVC environment action
targets deprecated Node.js 20 and was successfully run with Node.js 24.
These diagnostics do not require changing unrelated LLVM components.
The optional full stage was skipped: Windows-hosted Clang ELF objects,
Windows lit applicability, and Windows regression results remain unqualified.
The ``llvm-sh-rc1`` tag and release-packaging workflow were unchanged during
those qualification runs. Their original commit IDs above remain the evidence
identities. A subsequent owner-authorized history fold incorporates this MSVC
fix into the RC1 source commit and consolidates the workflows separately. It
does not turn the earlier Linux package results into qualification of the
rewritten RC1 source. Full Windows Clang validation remains outstanding.

GNU ABI and C++
---------------

The GNU layout uses 32-bit pointers and integer registers with 32-bit stack
alignment. Ordinary 64-bit integer and double alignment is 32 bits. GCC's
optional ``-mdalign`` ABI is not modeled. Hard-float, soft argument transport,
single-precision modes, E-class ABI, and FDPIC must agree across callers,
callees, runtime libraries, and assembly providers.

The intended hosted GNU C++ object ABI is C++17 with libstdc++, exceptions, RTTI,
construction/destruction, static initialization, and GNU-compatible unwinding.
A build using ``-fno-exceptions`` or ``-fno-rtti`` does not establish this
contract. Separate translation units and mixed GNU/Clang producer directions
are needed to test ABI agreement independently.

The GNU hard-float ``va_list`` has separate integer, floating-point, and stack
cursors. Direct complex values and records containing a complex value require
independent ABI classification. A single nonempty member or base can be
recognized recursively only when its offset, total size, and alignment satisfy
the ABI rule; similarly sized aggregates are not interchangeable by size alone.

GNU 14.2 has a documented odd-FPR-cursor complex-float consumer defect. Its
consumer may reverse real and imaginary components even with its own caller.
Tests must retain the literal oracle and both producer directions. GNU/GNU
agreement is not a substitute for defined language semantics, and an external
reference failure must not be relabeled as a compiler pass.

FDPIC code requires the correct incoming context and function descriptors.
Function addresses used for checks such as UBSan prologue inspection must
resolve to code addresses rather than descriptor storage. Hardware handler
entry does not provide a general FDPIC function-entry contract.

Backend responsibilities
------------------------

The implementation uses SelectionDAG, TableGen, and LLVM's ordinary register
allocator, coalescer, and schedulers. Register aliases, tied operands, call
regmasks, memory operands, and architectural state dependencies express SH
constraints. There is no separate vector calling convention for the graphics
operations described in :doc:`SHGraphics`.

Atomic validation, IR preparation, and IR validation establish the accepted
surface before instruction selection. Legacy and new code-generation pass
manager entry points share those implementations. Unsupported atomic, ABI,
or constrained-FP forms must diagnose rather than reach invalid selection.

Frame lowering, scavenging, and calls preserve stack adjustments, callee saves,
register ties, and DWARF CFI/EH. Large, dynamic, and realigned frames need their
own caller/provider tests; ordinary small frames do not cover them.

Literal placement and branch relaxation precede jump-table compression and
delay-slot filling. Final range verification remains necessary: alignment can
increase a PC-relative displacement even when an instruction sequence shrinks.
Inline-assembly size estimation retains parsing and conservative bounds.
Physical-line hash comments and section-sensitive data directives must agree
between textual assembly and integrated object emission.

Post-allocation folds check physical-register liveness, subregister aliases,
condition state, memory effects, and call clobbers. The early invocation handles
allocated patterns; the late invocation also sees frame adjustments inserted
by prologue/epilogue insertion. Reordering these invocations changes their
legality inputs and requires separate evidence.

FPSCR and graphics state
-----------------------

PR, SZ, and FR are architectural state. Full FPSCR operands also carry status
and non-mode control dependencies. State repair runs before allocation and
again after allocation with scratch-register preservation. Scalar spills can
insert mode changes after the first state pass.

Mode intent must survive allocation as an absolute destination. The late pass
revalidates immediate and reused-register XOR masks against the actual incoming
state before selecting a legal relative toggle or repairing the transition.
An earlier assumption about the incoming mode can otherwise make an FSCHG
illegal. Removing dependence edges to improve scheduling is not a substitute
for preserving this state contract.

FV, DR, and scalar FR registers overlap. Their copies, spills, and memory
transfers use the ordinary allocator's alias model. Matrix operations have
explicit ordinary-memory lifetime and overlap contracts; they do not expose
persistent user-owned XMTRX state or arbitrary mode toggles. Matrix composition
preserves the explicit product's rounding and status sequence.

See :doc:`SHGraphics` for API types, alignment, zero-count behavior, exact
in-place operations, unsupported partial overlap, and FP-state requirements.
Normal return with masked exceptions is the initial bounded graphics contract.
Traps, nonlocal exits, hardware approximation fidelity, and timing require
separate qualification. FSCA instruction emission alone does not establish a
validated FSCA execution oracle.

Hardware handlers and support boundaries
---------------------------------------

GNU-style interrupt and TRAPA attributes require their documented signatures
and preservation rules. Platform entry software owns assumptions about banked
registers, GBR/thread context, finite hardware banks, alternate-stack storage,
and nesting. A compiler-generated handler is not an assurance of interrupt-safe
runtime services. Ordinary CFI does not establish unwind-through-handler or
cross-stack unwinding support.

Family and FPU capability diagnostics, endian object flags, ELF relocations,
branch/literal bounds, memory alignment, varargs, CFI, TLS/GBR, FPSCR, and
graphics register aliases require both positive and negative regression tests.
Assembly/MC legality, simulator execution, and physical hardware results must
be reported separately. Historical compiler results do not qualify a port to
a different LLVM revision.

Qualification fixtures and their provenance must remain available when tools
are moved. In particular, the functional harness consumes its application
catalog, and source accounting consumes its recovery provenance. Replacing
either data source requires explicit versioned data-root support and a tested
equivalence check; an archive receipt alone does not prove consumer migration.
