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
    p.add_argument("--picolibc-archive", type=Path)
    p.add_argument("--logs-dir", type=Path)
    args = p.parse_args()
    if not all(
        re.fullmatch("[0-9a-f]{40}", value) for value in (args.commit, args.tree)
    ):
        p.error("--commit and --tree must be full Git identities")
    if args.logs_dir and (
        args.logs_dir.resolve() == args.build_dir.resolve()
        or args.build_dir.resolve() in args.logs_dir.resolve().parents
    ):
        p.error("--logs-dir must be outside --build-dir")
    try:
        stage(args)
    finally:
        if args.logs_dir:
            for path in args.build_dir.rglob("*"):
                if path.is_file() and (
                    path.suffix == ".log"
                    or path.name
                    in (
                        "compile-commands.json",
                        "commands.json",
                        "toolchain.json",
                        "CMakeConfigureLog.yaml",
                    )
                ):
                    dest = args.logs_dir / path.relative_to(args.build_dir)
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(path, dest)


def stage(args):
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
    libraries = prefix / "lib/clang-runtimes/shle-unknown-none-elf/sh4-soft"
    command = [
        sys.executable,
        str(source / "runtimes/sh/build-libraries.py"),
        "--tools",
        str(prefix / "bin"),
        "--build-dir",
        str(args.build_dir / "libraries"),
        "--sysroot",
        str(libraries),
        "--llvm-source",
        str(source),
        "--source-commit",
        args.commit,
        "--source-tree",
        args.tree,
    ]
    if args.picolibc_archive:
        command += ["--picolibc-archive", str(args.picolibc_archive.resolve())]
    subprocess.run(command, check=True)
    for name in ("sh4-picolibc.cfg", "sh4-libcxx.cfg"):
        (prefix / "bin" / name).write_text(
            "@../lib/clang-runtimes/shle-unknown-none-elf/sh4-soft/share/clang/"
            + name
            + "\n",
            encoding="utf-8",
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
    shutil.copy2(examples / "libraries/README.rst", licenses / "SH4-LIBRARIES.rst")


if __name__ == "__main__":
    main()
