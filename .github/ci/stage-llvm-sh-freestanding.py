#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Stage the SH target runtimes and reproducible examples after host tools."""

import argparse
import re
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source", type=Path, required=True)
    p.add_argument("--prefix", type=Path, required=True)
    p.add_argument("--build-dir", type=Path, required=True)
    p.add_argument("--commit", required=True)
    p.add_argument("--tree", required=True)
    args = p.parse_args()
    if not all(
        re.fullmatch("[0-9a-f]{40}", value) for value in (args.commit, args.tree)
    ):
        p.error("--commit and --tree must be full Git identities")
    source = args.source.resolve()
    prefix = args.prefix.resolve()
    suffix = ".exe" if sys.platform == "win32" else ""
    runtime = prefix / "lib/shle-unknown-none-elf/sh4-soft"
    subprocess.run(
        [
            sys.executable,
            str(source / "compiler-rt/utils/build-sh4-freestanding.py"),
            "--clang",
            str(prefix / ("bin/clang" + suffix)),
            "--ar",
            str(prefix / ("bin/llvm-ar" + suffix)),
            "--build-dir",
            str(args.build_dir),
            "--install-dir",
            str(runtime),
            "--source-commit",
            args.commit,
            "--source-tree",
            args.tree,
        ],
        check=True,
    )
    examples = prefix / "share/llvm-sh/examples/SHFreestanding"
    shutil.copytree(
        source / "llvm/examples/SHFreestanding",
        examples,
        ignore=shutil.ignore_patterns("__pycache__"),
        dirs_exist_ok=True,
    )
    licenses = prefix / "share/doc/llvm-sh"
    licenses.mkdir(parents=True, exist_ok=True)
    for project in ("lld", "compiler-rt"):
        shutil.copy2(
            source / project / "LICENSE.TXT", licenses / ("LICENSE-" + project + ".txt")
        )
    shutil.copy2(examples / "README.rst", licenses / "SH4-FREESTANDING.rst")


if __name__ == "__main__":
    main()
