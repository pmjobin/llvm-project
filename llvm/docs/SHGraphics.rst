Explicit SH graphics operations
==============================

``<shintrin.h>`` exposes native ``sh_float2`` and ``sh_float4`` values and
an ordinary-memory ``sh_matrix4`` with four column vectors. These experimental
operations request the named hardware instruction. They do not replace ordinary
scalar arithmetic, ``sin``, ``cos`` or ``sqrt`` implicitly.

API and target requirements
---------------------------

* ``sh_fsca(unsigned phase)`` returns sine and cosine in lanes zero and one.
  The low sixteen phase bits represent one turn. ``sh_fsrra(float)`` requests
  the hardware reciprocal-square-root approximation. Both require SH4A and a
  single-precision FPU; ``__SH_APPROX_MATH__`` indicates that compile capability.
* ``sh_fipr(sh_float4 a, sh_float4 b)`` returns the hardware dot product.
  ``sh_ftrv(const sh_matrix4 *m, sh_float4 v)`` returns the matrix product.
  ``sh_ftrv4(sh_float4 *dst, const sh_float4 *src, const sh_matrix4 *m)``
  performs exactly four transforms, loading the matrix once.
  ``sh_ftrv_array(dst, src, unsigned count, m)`` retains that matrix for the
  whole array. ``sh_ftrv_compose(dst, src, count, left, right)`` first forms
  the matrix product with four FTRVs and retains it for the array. These require the
  target's vector-FPU feature; ``__SH_GRAPHICS__`` indicates that capability.
* ``sh_load4`` and ``sh_store4`` transfer four adjacent floats without
  arithmetic. ``sh_matrix4_load`` and ``sh_matrix4_store`` transfer sixteen.
  They accept ordinary float alignment. Matrix store snapshots the entire
  source before writing, including for partially overlapping storage.

The corresponding builtins have the same value types, with ordinary ``void *``
buffer parameters instead of header-specific nominal types:
``__builtin_sh_fsca``, ``__builtin_sh_fsrra``, ``__builtin_sh_fipr``,
``__builtin_sh_ftrv``, ``__builtin_sh_ftrv4``, ``__builtin_sh_ftrv_array`` and
``__builtin_sh_ftrv_compose``. Unsupported CPU/FPU combinations
diagnose. Vector operations require an enabled vector facility; no-FPU profiles,
SH4AL-DSP and SH2E/SH3E processors cannot execute them. SH4/SH4A single-only
profiles retain paired-single transfers, bank access and graphics while disabling
double arithmetic and retaining their GNU E-class ABI and 32-bit C ``double``.
SH2A single-only retains its narrower capability policy. Feature macros express
compiler support, not runtime ABI or execution qualification for a particular
processor or simulator.

Matrix entry ``m.column[j][i]`` is row ``i``, column ``j``; flat storage has
offset ``4*j+i``. The header checks size 64 and column offsets 16, 32 and 48.
Raw pointers must designate complete accessible objects with at least float
alignment. The batch accepts disjoint arrays or exact in-place operation.
Partial array overlap is unsupported, and the matrix must be disjoint from
both arrays. The composed operation reads both input matrices before writing
any output, so its matrices may overlap the arrays or each other. Each nonzero
count must designate that many complete vectors; the region does not preload
past the end. Count zero accesses no pointers and performs no FP operation,
including no composition. These are ordinary, nonvolatile, nonatomic accesses.

C example (compile with ``--target=shle-unknown-linux-gnu -m4``)::

  #include <shintrin.h>
  void transform_four(sh_float4 dst[4], const sh_float4 src[4],
                      const sh_matrix4 *matrix) {
    sh_ftrv4(dst, src, matrix);
  }

Matrix multiplication and retained composition::

  // C = A * B. A is disjoint from B and C; C may be exactly B.
  void multiply(sh_matrix4 *c, const sh_matrix4 *a, const sh_matrix4 *b) {
    sh_ftrv4(c->column, b->column, a);
  }

  // One XMTRX setup for the entire vertex array, including exact in-place use.
  void transform_vertices(sh_float4 *dst, const sh_float4 *src, unsigned n,
                          const sh_matrix4 *matrix) {
    sh_ftrv_array(dst, src, n, matrix);
  }

  // Compute projection * model, retain it in XMTRX, then transform n vertices.
  void project_vertices(sh_float4 *dst, const sh_float4 *src, unsigned n,
                        const sh_matrix4 *projection, const sh_matrix4 *model) {
    sh_ftrv_compose(dst, src, n, projection, model);
  }

Composition has the rounding and FP effects of forming each product column
with FTRV, then applying that product. It does not reassociate the operation
into ``projection * (model * vertex)``. A private 64-byte temporary produced by
``sh_ftrv4`` and immediately consumed by another batch or a positive constant
count can also be fused. Observable temporaries, intervening effects and
possible zero counts retain their original operations.

C++17 example::

  #include <shintrin.h>
  sh_float4 transform(const sh_matrix4 &matrix, const float (&point)[4]) {
    return sh_ftrv(&matrix, sh_load4(point));
  }

Allocation, memory and state
---------------------------

FV0/FV4/FV8/FV12 are allocatable aliases of their four scalar registers.
Scalar FR and double DR interference is therefore handled by the ordinary
allocator. Vector copies, spills, build/extract/insert and memory transfers
are implemented; unsupported vector arithmetic and shuffles use ordinary
legalization. This is not an automatic vectorization or custom allocator API.
Public vectors retain GNU integer argument transport. A vector goes wholly
in the available integer-register group or wholly on the stack; a failed
allocation leaves the register cursor available to following arguments.
Both vector sizes return through the GNU hidden R2 pointer. Clang emits that
explicit ``sret`` IR; unlowered direct vector-result function/call IR diagnoses.

FIPR preserves reused inputs through normal tied-operand allocation and extracts
the destination's fourth lane. FSCA copies phase bits to FPUL and uses an even
register pair for two singles, never an f64 mathematical value.

The matrix region owns XMTRX only between its described setup and teardown.
It loads XD pairs directly and preserves XD12/XD14 in a private aligned frame
area. FV12 use preserves FR12--FR15 through ordinary callee-save handling.
Reusable internal XMTRX range load/store pseudos support full 64-byte transfers
and preserved-tail transfers. No source interface exposes bank/size toggles
or persistent matrix-register contents.

The four-vector batch and counted loop expose their ordinary FV temporaries
before allocation. The loop pipelines groups of three vectors and uses a
bounded tail; small known positive counts up to four expand directly. It
performs one matrix setup and one preserved-tail restoration across the whole
array. Composition keeps four product columns live, then copies eight DR pairs
directly to XD pairs. No intermediate product is stored and reloaded. Other
live vectors may still spill through the normal allocator.

The existing FPSCR owner includes PR=0/SZ=1/FR=0. Graphics arithmetic requires
PR=0 independently of transfer width. Eight-byte-aligned paired transfers keep
ascending single-word lane order in both endian modes; the little-endian f64
word convention is different. An align4 matrix is copied with integer words
into the aligned private buffer. An align4 batch uses scalar transfers within
one bounded size-mode interval. The private setup area is 16 bytes for an
aligned matrix, or 80 bytes for the align4 fallback, in addition to ordinary
frame/callee saves. Nonvolatile, eight-byte-aligned, matrix-sized memory copies
can use paired vector transfers; overlapping copies load all columns before
storing. Ordinary float-pointer alignment alone does not promise paired moves.
Mode changes preserve rounding, denormal and
exception-enable controls and arithmetic status; a saved whole FPSCR word is
not restored over newly raised status. Calls, environment accesses and unknown
mode writes end the tracked state.

Planned mode transitions retain their absolute destination across allocation.
The late state pass revalidates toggle legality and XOR masks against any
intervening scalar copies or spills. Thus a spill cannot leave an FSCHG planned
for PR0 executing in PR1, and register-reused masks retain their source-mode
proof. Unknown incoming states use a control-bit update that retains status.

Numerical and execution boundaries
----------------------------------

Hardware graphics arithmetic can differ from scalar multiply/add or libm.
These intrinsics have explicit FP-state effects and are not freely speculatable
or removable when their numerical results are unused. The initial contract is
normal return with masked exceptions. Strict FP IR and graphics in hardware
handlers diagnose. The SH frontend already diagnoses unsupported strict
exception options; use ``-Werror=unsupported-floating-point-opt`` to make
that target-wide warning fatal. Resumable traps, nonlocal exits from a matrix
region, hardware approximation fidelity and hardware timing are unqualified.

The frozen Full48 QEMU 10.0.11 is not the graphics oracle. A separately pinned
diagnostic build uses ``utils/sh/patches/qemu-sh4-graphics.patch`` to repair
PR/index, bank selection, FSRRA decoder fallthrough and paired-single lane order.
The patch retains QEMU source attribution and licensing. Independent GNU
assembly must validate that binary before candidate execution. FSCA still has
no decoder in this oracle: source/IR/MC coverage is not an FSCA runtime pass.
FIPR/FTRV use scalar SoftFloat in the diagnostic emulator, which does not prove
hardware approximation accuracy for cancellation or arbitrary inputs.

``utils/sh/validate-feature-oracle.py`` retains all independent outcomes,
including absent FSCA. ``graphics-state.py`` tests GNU positive and deliberate
clobber/status-loss controls before candidate register/FPSCR sentinels.
``xmtrx-roundtrip.py`` tests full internal matrix transfers in both endian
modes with distinct bits, signed zero and NaN payloads. ``run-functional.py
--case graphics`` covers mixed GNU vector calls, dynamic indexing, shuffles,
pressure/spills, matrix alias updates, align4 accesses and in-place batches.
Supply fresh external build/runtime directories and the validated feature
oracle explicitly; no recipe replaces the corpus emulator.

``graphics-stream.py`` adds counted, constant-count, private-product fusion and
matrix-copy controls in both endians and both pass managers. Its GNU callers
check guarded buffers, exact raw payloads, legal overlaps, tails, callee saves
and FPSCR; deliberate clobber/overrun/payload mutants must fail. The recipe
verifies the actual IR alignment and paired instruction selection before
claiming paired-path coverage. It also records complete assembly for the
packaged ``matrix-kernels.c`` examples.

The counted region was informed by the register reuse and resident-matrix
approach in Pierre-Marc Jobin's 2008 Maelstrom ``shcrt/src/3dmath.asm``. This
implementation uses LLVM's allocator, explicit ownership and the GNU call
boundary; it does not import Maelstrom's ambient matrix/bank convention or its
speculative final preload. Instruction counts and simulator dispatch counts
are not hardware speed measurements.

Architectural references: Renesas SH-4 Software Manual Rev.6, sections 6.6.2
and the FIPR/FTRV/FMOV instruction descriptions, and SH-4A Software Manual
Rev.1.50, FSCA/FSRRA descriptions. The diagnostic patch cites the upstream
QEMU PR/index correction by Randy Schifflin separately from local repairs.

Architectural state access
==========================

The following header-free GCC-compatible builtins are available::

  void *__builtin_thread_pointer(void);
  void __builtin_set_thread_pointer(void *);
  unsigned int __builtin_sh_get_fpscr(void);
  void __builtin_sh_set_fpscr(unsigned int);

The explicit thread-pointer operations read and write GBR in program order,
including across out-of-line context-management wrappers. They require no FPU.
A setter persists after an ordinary function returns; its epilogue does not
restore the previous GBR. Ordinary ABI calls continue to preserve GBR.

These operations support system context management. They do not promise that
ordinary language TLS expressions rebase after a midfunction GBR change: LLVM
may cache or hoist a TLS address independently of these explicit operations.
Previously computed TLS pointers still refer to their original objects. The
initial-exec GOT offset is invariant independently of the mutable thread base.
Code temporarily installing an artificial thread pointer must use explicit
non-TLS storage and restore GBR before calling libc or using ordinary TLS.

FPSCR access requires an enabled single-precision FPU, independently of the
argument calling convention. These control operations are supported in strict
FP functions, but currently diagnosed in hardware handlers. The getter observes
the current register without requesting a precision, transfer-size or bank
transition. The setter computes::

  new_fpscr = (current_fpscr & 0x00380000u) | (value & ~0x00380000u);

Thus current PR, SZ and FR remain owned by the compiler. Other fields, including
status, are replaced by the requested value. Backend mode transitions instead
preserve current nonmode fields, including newly raised status. Explicit graphics
operations and FPSCR access share ordered FP-state effects. These builtins do not
make ordinary unconstrained floating-point expressions observe the floating-point
environment; use the supported strict/constrained FP contract where required.
