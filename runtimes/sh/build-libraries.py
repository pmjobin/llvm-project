#!/usr/bin/env python3
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""Build the static SH4 soft-float C and C++ sysroot with LLVM target tools.

Picolibc is downloaded only from its pinned, checksummed source archive. Supply
--picolibc-archive for an offline build. Host prerequisites are Python, Meson,
Ninja and CMake. No GNU target compiler, assembler, linker or runtime is used.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import urllib.request


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tools", type=Path, required=True, help="host bin directory")
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--sysroot", type=Path, required=True)
    parser.add_argument("--picolibc-archive", type=Path)
    parser.add_argument(
        "--llvm-source", type=Path, default=Path(__file__).resolve().parents[2]
    )
    parser.add_argument("--jobs", type=int, default=min(os.cpu_count() or 1, 8))
    parser.add_argument("--c-only", action="store_true")
    parser.add_argument("--source-commit")
    parser.add_argument("--source-tree")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    build = args.build_dir.resolve()
    sysroot = args.sysroot.resolve()
    llvm = args.llvm_source.resolve()
    tools = args.tools.resolve()
    source_commit = (
        args.source_commit
        or subprocess.check_output(
            ["git", "-C", str(llvm), "rev-parse", "HEAD"], text=True
        ).strip()
    )
    source_tree = (
        args.source_tree
        or subprocess.check_output(
            ["git", "-C", str(llvm), "rev-parse", "HEAD^{tree}"], text=True
        ).strip()
    )
    build.mkdir(parents=True, exist_ok=True)
    sysroot.mkdir(parents=True, exist_ok=True)
    suffix = ".exe" if os.name == "nt" else ""
    clang = tools / ("clang" + suffix)
    ar = tools / ("llvm-ar" + suffix)
    commands = []

    def run(command, name):
        command = list(map(str, command))
        logfile = build / (name + ".log")
        print("Running " + name, flush=True)
        with logfile.open("w", encoding="utf-8") as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
        commands.append(
            {"command": command, "exit": result.returncode, "log": str(logfile)}
        )
        (build / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        if result.returncode:
            print(logfile.read_text(encoding="utf-8", errors="replace")[-12000:])
            raise SystemExit("Failed " + name + "; see " + str(logfile))

    lock = json.loads((Path(__file__).parent / "picolibc.json").read_text())
    archive = args.picolibc_archive or build / (
        "picolibc-" + lock["commit"] + ".tar.gz"
    )
    if not archive.is_file():
        if args.picolibc_archive:
            parser.error("--picolibc-archive does not exist")
        with urllib.request.urlopen(lock["url"], timeout=60) as src, archive.open(
            "wb"
        ) as dst:
            shutil.copyfileobj(src, dst)
    if digest(archive) != lock["sha256"]:
        raise SystemExit("Picolibc source checksum mismatch: " + str(archive))
    pico = build / ("picolibc-" + lock["commit"])
    if not pico.exists():
        with tarfile.open(archive) as src:
            src.extractall(build, filter="data")
    # GNU's SH ELF assembly convention prefixes C symbols with '_'; this
    # Clang target uses the unprefixed ELF convention. Use Clang's actual
    # predefined label prefix instead of changing the library's C ABI.
    asm = pico / "libc/machine/sh/asm.h"
    original = "# define _C_LABEL(x)    _ ## x"
    replacement = (
        "# define _SH_CONCAT_IMPL(a, b) a ## b\n"
        "# define _SH_CONCAT(a, b) _SH_CONCAT_IMPL(a, b)\n"
        "# define _C_LABEL(x) _SH_CONCAT(__USER_LABEL_PREFIX__, x)"
    )
    content = asm.read_text()
    if original in content:
        asm.write_text(content.replace(original, replacement))
    elif replacement not in content:
        raise SystemExit("Unexpected pinned SH assembly label definitions")
    # A cached extraction is still required to match the pinned archive. Apart
    # from the explicit assembly adaptation, local source edits are rejected.
    with tarfile.open(archive) as original_source:
        for member in original_source.getmembers():
            path = build / member.name
            if member.isfile():
                expected = original_source.extractfile(member).read()
                if path == asm:
                    expected = expected.replace(original.encode(), replacement.encode())
                if not path.is_file() or path.read_bytes() != expected:
                    raise SystemExit("Picolibc source differs from pin: " + member.name)
            elif member.issym() and (
                not path.is_symlink() or os.readlink(path) != member.linkname
            ):
                raise SystemExit(
                    "Picolibc source link differs from pin: " + member.name
                )

    run(
        [
            sys.executable,
            llvm / "compiler-rt/utils/build-sh4-freestanding.py",
            "--clang",
            clang,
            "--ar",
            ar,
            "--build-dir",
            build / "builtins",
            "--install-dir",
            sysroot,
            "--jobs",
            args.jobs,
            "--with-libc",
            "--source-commit",
            source_commit,
            "--source-tree",
            source_tree,
        ],
        "builtins",
    )
    resource = Path(
        subprocess.check_output([str(clang), "--print-resource-dir"], text=True).strip()
    )
    flags = [
        "--target=shle-unknown-none-elf",
        "-mcpu=sh4",
        "-msoft-float",
        "-mfloat-abi=soft",
        "-ffreestanding",
        "-fno-builtin",
        "-fno-pic",
        "-fno-pie",
        "-fno-stack-protector",
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
        "-nostdinc",
        "-isystem",
        str(resource / "include"),
        "-nostdlib",
        "-ffile-prefix-map=" + str(pico) + "=picolibc",
        "-ffile-prefix-map=" + str(build) + "=sh-runtime-build",
    ]

    def meson_string(value):
        return "'" + str(value).replace("\\", "/").replace("'", "\\'") + "'"

    cross = build / "picolibc-cross.ini"
    lines = [
        "[binaries]",
        "c = [" + ", ".join(map(meson_string, [clang, *flags])) + "]",
    ]
    for name, tool in (("ar", "llvm-ar"), ("strip", "llvm-strip"), ("nm", "llvm-nm")):
        lines.append(name + " = " + meson_string(tools / (tool + suffix)))
    lines += [
        "[host_machine]",
        "system = 'none'",
        "cpu_family = 'sh'",
        "cpu = 'sh4'",
        "endian = 'little'",
        "[properties]",
        "skip_sanity_check = true",
        "needs_exe_wrapper = true",
        "librt = " + meson_string(sysroot / "lib/libclang_rt.builtins.a"),
    ]
    cross.write_text("\n".join(lines) + "\n")
    pico_build = build / "picolibc-build"
    configure = [
        "meson",
        "setup",
        pico_build,
        pico,
        "--cross-file",
        cross,
        "--prefix",
        sysroot,
        "--libdir",
        "lib",
        "--buildtype",
        "release",
        "-Dmultilib=false",
        "-Dthread-local-storage=false",
        "-Dthread-local-storage-api=false",
        "-Dsingle-thread=true",
        "-Datomic-ungetc=false",
        "-Dpicocrt=false",
        "-Dpicocrt-lib=false",
        "-Dsemihost=false",
        "-Dspecsdir=none",
        "-Dio-long-long=true",
        "-Dposix-console=true",
        "-Dtests=false",
    ]
    if (pico_build / "meson-private/coredata.dat").exists():
        configure.append("--reconfigure")
    run(configure, "picolibc-configure")
    run(["ninja", "-C", pico_build, "-j", args.jobs], "picolibc-build")
    run(["meson", "install", "-C", pico_build, "--no-rebuild"], "picolibc-install")
    # Never distribute dummy OS implementations as a board support layer.
    for name in ("libdummyhost.a", "libg.a", "libnosys.a"):
        (sysroot / "lib" / name).unlink(missing_ok=True)
    licenses = sysroot / "share/licenses"
    licenses.mkdir(parents=True, exist_ok=True)
    shutil.copy2(pico / "COPYING.picolibc", licenses / "COPYING.picolibc")
    shutil.copy2(archive, sysroot / ("src/picolibc-" + lock["version"] + ".tar.gz"))
    if not args.c_only:
        cxx_flags = [
            *flags,
            "--sysroot=" + str(sysroot),
            "-isystem",
            str(sysroot / "include"),
            "-fno-exceptions",
            "-ffile-prefix-map=" + str(llvm) + "=llvm-project",
        ]

        def cmake_flags(values):
            return " ".join(
                '"' + str(v).replace("\\", "/").replace('"', '\\"') + '"'
                for v in values
            )

        cxx_build = build / "cxx-build"
        run(
            [
                "cmake",
                "--fresh",
                "-G",
                "Ninja",
                "-S",
                llvm / "runtimes",
                "-B",
                cxx_build,
                "-C",
                Path(__file__).parent / "SH4Soft.cmake",
                "-DCMAKE_BUILD_TYPE=Release",
                "-DCMAKE_SYSTEM_NAME=Generic",
                "-DCMAKE_SYSTEM_PROCESSOR=sh",
                "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY",
                "-DCMAKE_C_COMPILER=" + str(clang),
                "-DCMAKE_CXX_COMPILER=" + str(tools / ("clang++" + suffix)),
                "-DCMAKE_ASM_COMPILER=" + str(clang),
                "-DCMAKE_AR=" + str(ar),
                "-DCMAKE_RANLIB=" + str(tools / ("llvm-ranlib" + suffix)),
                "-DCMAKE_C_FLAGS=" + cmake_flags(cxx_flags),
                "-DCMAKE_CXX_FLAGS=" + cmake_flags(cxx_flags),
                "-DCMAKE_FIND_ROOT_PATH=" + str(sysroot),
                "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY",
                "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY",
                "-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY",
                "-DCMAKE_INSTALL_PREFIX=" + str(sysroot),
            ],
            "cxx-configure",
        )
        run(
            [
                "cmake",
                "--build",
                cxx_build,
                "--target",
                "cxx_static",
                "cxxabi_static",
                "cxx_experimental",
                "--parallel",
                args.jobs,
            ],
            "cxx-build",
        )
        run(["cmake", "--install", cxx_build], "cxx-install")
        for project in ("libcxx", "libcxxabi"):
            shutil.copy2(llvm / project / "LICENSE.TXT", licenses / (project + ".txt"))
    config_dir = sysroot / "share/clang"
    config_dir.mkdir(parents=True, exist_ok=True)
    for config in Path(__file__).parent.glob("*.cfg"):
        shutil.copy2(config, config_dir / config.name)
    relative_resource = os.path.relpath(resource / "include", config_dir).replace(
        "\\", "/"
    )
    (config_dir / "sh4-resource.cfg").write_text(
        '-isystem "<CFGDIR>/' + relative_resource + '"\n'
    )
    shutil.copy2(Path(__file__).parent / "sh4-library-profile.h", sysroot / "include")
    provenance = sysroot / "src/sh"
    shutil.copytree(
        Path(__file__).parent,
        provenance,
        ignore=shutil.ignore_patterns("__pycache__"),
        dirs_exist_ok=True,
    )
    manifest = {
        "profile": "SH4 little-endian soft-float static, single-thread, no TLS",
        "picolibc": lock,
        "flags": flags,
        "cxx": not args.c_only,
        "picolibc_patches": {"libc/machine/sh/asm.h": digest(asm)},
        "recipe_sha256": digest(Path(__file__)),
        "llvm_source_commit": source_commit,
        "llvm_source_tree": source_tree,
        "llvm_worktree_status": subprocess.check_output(
            ["git", "-C", str(llvm), "status", "--porcelain"], text=True
        ).splitlines(),
        "clang_version": subprocess.check_output([str(clang), "--version"], text=True),
        "archives": {
            p.name: {"sha256": digest(p), "size": p.stat().st_size}
            for p in sorted((sysroot / "lib").glob("*.a"))
        },
    }
    (sysroot / "LIBRARIES.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest["archives"], indent=2))


if __name__ == "__main__":
    main()
