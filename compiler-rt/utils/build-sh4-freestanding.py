#!/usr/bin/env python3
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""Build the bounded SH4 little-endian soft-float freestanding runtime.

Use target Clang and llvm-ar, including on Windows; no host target libraries or
GNU cross tools are consulted. This recipe deliberately selects portable
compiler-rt builtins rather than enabling unported hosted runtimes.
"""

import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

BUILTINS = """
adddf3 addsf3 ashldi3 ashrdi3 bswapdi2 bswapsi2 clzdi2 clzsi2 cmpdi2
comparedf2 comparesf2 ctzdi2 ctzsi2 divdf3 divdi3 divmoddi4 divmodsi4
divsf3 divsi3 extendsfdf2 ffsdi2 ffssi2 fixdfdi fixdfsi fixsfdi fixsfsi
fixunsdfdi fixunsdfsi fixunssfdi fixunssfsi floatdidf floatdisf floatsidf
floatsisf floatundidf floatundisf floatunsidf floatunsisf fp_mode
int_util lshrdi3 moddi3 modsi3 muldf3 muldi3 mulodi4 mulosi4 mulsf3
negdf2 negdi2 negsf2 paritydi2 paritysi2 popcountdi2 popcountsi2
powidf2 powisf2 subdf3 subsf3 truncdfsf2 ucmpdi2 udivdi3 udivmoddi4
udivmodsi4 udivsi3 umoddi3 umodsi3
""".split()

PROFILE = "shle-unknown-none-elf/sh4-soft"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang", type=Path, required=True)
    parser.add_argument("--ar", type=Path, required=True)
    default_source = Path(__file__).resolve().parents[1] / "lib/builtins"
    if not default_source.is_dir():
        default_source = Path(__file__).resolve().parent / "builtins"
    parser.add_argument("--source-dir", type=Path, default=default_source)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--install-dir", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=min(os.cpu_count() or 1, 8))
    parser.add_argument("--source-commit")
    parser.add_argument("--source-tree")
    parser.add_argument(
        "--with-libc",
        action="store_true",
        help="build compiler helpers only; libc and libc++abi own memory/C++ APIs",
    )
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    source = args.source_dir.resolve()
    build = args.build_dir.resolve()
    install = args.install_dir.resolve()
    build.mkdir(parents=True, exist_ok=True)
    install.mkdir(parents=True, exist_ok=True)
    clang = args.clang.resolve()
    ar = args.ar.resolve()
    resource = Path(
        subprocess.check_output([str(clang), "--print-resource-dir"], text=True).strip()
    )
    flags = [
        "--target=shle-unknown-none-elf",
        "-mcpu=sh4",
        "-msoft-float",
        "-mfloat-abi=soft",
        "-O2",
        "-ffreestanding",
        "-fno-builtin",
        "-fno-pic",
        "-fno-pie",
        "-fno-stack-protector",
        "-fno-ident",
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
        "-ffunction-sections",
        "-fdata-sections",
        "-fvisibility=hidden",
        "-nostdinc",
        "-isystem",
        str(resource / "include"),
        "-ffile-prefix-map=" + str(source) + "=compiler-rt/builtins",
    ]
    sources = [Path(name + ".c") for name in BUILTINS]
    if args.with_libc:
        flags.append("-m4-nofpu")
        sources += [
            Path(name + ".c") for name in ("divdc3", "divsc3", "muldc3", "mulsc3")
        ]
    else:
        sources += [Path("sh/freestanding-memory.c"), Path("sh/freestanding-cxx.cpp")]
    commands = []

    def compile_one(name):
        obj = build / (name.stem + ".o")
        command = [str(clang), *flags]
        if name.suffix == ".cpp":
            command += [
                "-std=c++17",
                "-fno-exceptions",
                "-fno-rtti",
                "-fno-threadsafe-statics",
            ]
        command += ["-c", str(source / name), "-o", str(obj)]
        result = subprocess.run(command, capture_output=True, text=True)
        return name, obj, command, result

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(compile_one, sources))
    failed = False
    for name, obj, command, result in results:
        commands.append(
            {
                "command": command,
                "exit": result.returncode,
                "stdout": result.stdout,
                "stderr": result.stderr,
            }
        )
        if result.returncode:
            failed = True
            print(str(name) + ":\n" + result.stderr)
    (build / "compile-commands.json").write_text(
        json.dumps(commands, indent=2) + "\n", encoding="utf-8"
    )
    if failed:
        raise SystemExit("SH4 runtime compilation failed; see compile-commands.json")

    archives = {}
    groups = {
        "libclang_rt.builtins.a": [
            obj for name, obj, _, _ in results if name.suffix == ".c"
        ],
    }
    if not args.with_libc:
        groups["libclang_rt.sh4_cxx.a"] = [build / "freestanding-cxx.o"]
    for name, objects in groups.items():
        archive = install / "lib" / name
        archive.parent.mkdir(parents=True, exist_ok=True)
        archive.unlink(missing_ok=True)
        subprocess.run([str(ar), "rcsD", str(archive), *map(str, objects)], check=True)
        archives[name] = {"sha256": digest(archive), "size": archive.stat().st_size}
    if not args.with_libc:
        shutil.copytree(source / "sh/include", install / "include", dirs_exist_ok=True)
    provenance = install / "src/builtins"
    source_files = sources + [
        p.relative_to(source)
        for p in sorted(source.iterdir())
        if p.suffix in (".h", ".inc")
    ]
    if not args.with_libc:
        source_files += [
            p.relative_to(source) for p in sorted((source / "sh/include").iterdir())
        ]
    for name in source_files:
        dest = provenance / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        if dest.resolve() != (source / name).resolve():
            shutil.copy2(source / name, dest)
    recipe = install / "src/build-sh4-freestanding.py"
    if recipe.resolve() != Path(__file__).resolve():
        shutil.copy2(__file__, recipe)
    license_file = source.parents[1] / "LICENSE.TXT"
    if license_file.resolve() != (install / "LICENSE.TXT").resolve():
        shutil.copy2(license_file, install / "LICENSE.TXT")
    manifest = {
        "profile": PROFILE,
        "with_libc": args.with_libc,
        "flags": flags,
        "archives": archives,
        "source_commit": args.source_commit,
        "source_tree": args.source_tree,
        "clang_version": subprocess.check_output([str(clang), "--version"], text=True),
        "sources": {str(n): digest(source / n) for n in source_files},
        "recipe_sha256": digest(Path(__file__)),
    }
    (install / "RUNTIME.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps(archives, indent=2))


if __name__ == "__main__":
    main()
