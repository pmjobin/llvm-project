LLVM SH RC1 history consolidation
=================================

The owner authorized a temporary RC1 unfreeze to incorporate the changes from
``fix/sh-msvc-portability`` and ``fix/linux-rc1-packaging`` into the existing
LLVM source and GitHub automation commits. The two replacement commits retain
the original authors and author dates, with new committer dates and object IDs.
Earlier ancestors, through ``47fd9c5754d5ff46c6d8812880e567410c15b195``, are
unchanged. No LLVM rebuild or package publication is part of this history edit.

Source commit and tag
---------------------

* Original source: ``a4ad03d58bd5a924de0ae286340ff7c6bb664ec2``.
* Replacement source: ``090c682597f7155373681807330fbb0ac343a33c``.
* Refrozen source tree: ``55640dd25d624b11b9e0f28a7903eea42a429254``.
* Original annotated tag object:
  ``272a3118d560572582c25d96cead1a219395f545``.

The replacement source folds in the MSVC-safe operand comparison in
``llvm/lib/Target/SH/SHISelLowering.cpp`` and all LLVM documentation from the
MSVC branch. The implementation file is byte-identical to that branch's final
version. Documentation distinguishes the original qualification runs from
the refrozen source identity. ``llvm-sh-rc1`` now identifies this replacement
source; its annotation records the authorized replacement and validation scope.

Automation commit and branch heads
----------------------------------

The commit containing this record replaces
``55d798a6bad82da5ae77212c7b2ddad4d98d7842`` and directly follows the replacement
source. It contains all ``.github`` changes from both repair branches:

* Native-MSVC development script/workflow, including the corrected sparse
  checkout and backend-first validation with optional full Clang testing.
* Linux smoke, packaging, installation, relocation and checksum checks.
* Manual platform selection, separate inexpensive shell/YAML validation and
  the historical Linux qualification report.
* Updated Linux and Windows packaging checks for the new source commit/tree.

No other build flags, smoke commands, workflow logic or upload gates change
as part of the fold. The source pin substitutions are the only changes to the
previously reviewed executable CI files. ``llvm-sh`` and both repair branches
are aligned to the consolidated automation commit; PRs #1 and #2 are superseded.
The rewritten history has two commits after the unchanged parent, with no
separate follow-up commits from either repair branch.

Original branch tips and folded commits
---------------------------------------

MSVC tip: ``2e3a05f5b99e7b058f9bae85660664710708b9a3``. Folded commits:
``2ec6088b23bafccaa1d78e5578f5c790a197d683``,
``f22b9e33b49f8e12d7442a6cad0b2e13ab8c0ed6``,
``fe92efc4d8f075200fdba51d0e5f8a2d18a11e79`` and the tip.

Linux tip: ``8c8ce50372e8c745dc198bc7baf0d808aa0d2943``. Folded commits:
``1ca6b1f50b72e49467d882238518b8e977e897ac``,
``28ade2b2ca0513e069e1b36eabaa12e9c7200c78``,
``cc23cf6092e8702881ebe01cd4b4bcfef434d039`` and the tip.

Validation and qualification scope
----------------------------------

Git tree/path comparisons verify that all branch changes are retained in the
appropriate commit and all unrelated paths are unchanged. Both source pins
match the refrozen commit/tree. Shell syntax, ShellCheck warnings and actionlint
checks are run before publication. The annotated tag and three branch updates
use an atomic push with explicit old-object leases.

The original native-MSVC backend results and Linux DEB/RPM/ZIP results retain
their original source/run identities. Historical hashes are evidence of those
binaries, not hashes for binaries built from the replacement source. New-source
Linux package qualification and full Windows Clang qualification remain
outstanding. The release artifact upload gate remains opt-in and disabled by
default; this history edit publishes no release artifacts.

Recovery
---------

Local recovery refs under ``refs/archive/rc1-history-fold/`` preserve
``source``, ``automation``, ``msvc``, ``linux`` and the old annotated ``tag``.
An incremental recovery bundle is stored at
``~/Workspace/codex/work/llvm-sh/runtime/rc1-history-fold/before.bundle``;
it requires the unchanged parent commit above. These recovery refs are not
published as extra branches or tags. Old-to-new object IDs and validation
results are also recorded in that runtime directory after the new commits
are created.
