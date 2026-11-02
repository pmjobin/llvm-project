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

  clang --target=shle-unknown-linux-gnu -m4 -S example.c

Supply a compatible GNU sysroot, GNU binutils, and explicit library/tool search
paths for linking. This target does not provide an SH LLD implementation or a
complete compiler-rt/runtime distribution. A cross compilation must not
silently use host libraries or an older compiler executable.

The driver diagnoses unsupported selectors and incompatible CPU, FPU, tune,
or ABI combinations. Family encoding definitions and compile-only tests do
not imply SH2, SH3, SH2A, SH4A, DSP, or big-endian application qualification.

GNU ABI and C++
---------------

The GNU layout uses 32-bit pointers and integer registers with 32-bit stack
alignment. Ordinary 64-bit integer and double alignment is 32 bits. GCC's
optional ``-mdalign`` ABI is not modeled. Hard-float, soft argument transport,
single-precision modes, E-class ABI, and FDPIC must agree across callers,
callees, runtime libraries, and assembly providers.

The intended C++ baseline is GNU C++17 with libstdc++, exceptions, RTTI,
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
