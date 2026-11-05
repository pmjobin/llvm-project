#!/usr/bin/env python3
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""Compile, archive, link and optionally execute the installed SH C/C++ libraries."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import struct
import sys

# The enclosing example supplies an independent ELF header/layout validator.
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from check import elf_info


def archive_members(path):
    data = path.read_bytes()
    assert data[:8] == b"!<arch>\n", (path, "archive signature")
    offset, count = 8, 0
    while offset < len(data):
        header = data[offset : offset + 60]
        assert len(header) == 60 and header[58:] == b"`\n", (path, offset)
        size = int(header[48:58])
        member = data[offset + 60 : offset + 60 + size]
        assert len(member) == size
        if member.startswith(b"\x7fELF"):
            assert member[:7] == b"\x7fELF\x01\x01\x01", (path, header[:16])
            assert struct.unpack_from("<HH", member, 16) == (1, 42)
            assert struct.unpack_from("<I", member, 36)[0] == 0x10
            count += 1
        else:
            assert header[:16].strip() in (b"/", b"//"), (path, header[:16])
        offset += 60 + size + (size & 1)
    assert offset == len(data) and count
    return count


def main():
    if not __debug__:
        raise SystemExit("Qualification requires Python without -O")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--sysroot", type=Path)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--qemu", type=Path)
    languages = parser.add_mutually_exclusive_group()
    languages.add_argument("--c-only", action="store_true")
    languages.add_argument("--cxx-only", action="store_true")
    parser.add_argument("--require-execution", action="store_true")
    args = parser.parse_args()
    if args.require_execution and not args.qemu:
        parser.error("--require-execution requires independently installed --qemu")
    prefix = args.prefix.resolve()
    sysroot = (
        args.sysroot or prefix / "lib/clang-runtimes/shle-unknown-none-elf/sh4-soft"
    ).resolve()
    build = args.build_dir.resolve()
    source = Path(__file__).resolve().parent
    build.mkdir(parents=True, exist_ok=True)
    suffix = ".exe" if os.name == "nt" else ""
    tools = {
        name: prefix / "bin" / (name + suffix)
        for name in (
            "clang",
            "clang++",
            "llvm-ar",
            "llvm-ranlib",
            "ld.lld",
            "llvm-readobj",
            "llvm-objdump",
            "llvm-nm",
        )
    }
    env = os.environ.copy()
    env["PATH"] = str(prefix / "bin")
    for key in (
        "CPATH",
        "C_INCLUDE_PATH",
        "CPLUS_INCLUDE_PATH",
        "COMPILER_PATH",
        "LIBRARY_PATH",
        "LD_LIBRARY_PATH",
        "LD_PRELOAD",
    ):
        env.pop(key, None)
    receipt = {
        "status": "RUNNING",
        "commands": [],
        "programs": [],
        "execution_requested": bool(args.qemu),
        "sysroot": str(sysroot),
    }

    def run(command, reject=False):
        command = list(map(str, command))
        result = subprocess.run(
            command, env=env, capture_output=True, text=True, timeout=120
        )
        receipt["commands"].append(
            {
                "command": command,
                "exit": result.returncode,
                "stdout": result.stdout,
                "stderr": result.stderr,
                "expected_rejection": reject,
            }
        )
        if reject:
            assert result.returncode != 0 and (
                "profile requires" in result.stderr
                or "unsupported option '-mfloat-abi=hard' for target" in result.stderr
            ), (
                command,
                result.returncode,
                result.stderr,
            )
        else:
            assert result.returncode == 0, (
                command,
                result.returncode,
                result.stdout,
                result.stderr,
            )
        return result.stdout

    try:
        for tool in tools.values():
            run([tool, "--version"])
        if args.qemu:
            run([args.qemu.resolve(), "--version"])
        manifest = json.loads((sysroot / "LIBRARIES.json").read_text())
        receipt["manifest"] = manifest
        receipt["archive_members"] = {}
        for name, info in manifest["archives"].items():
            archive = sysroot / "lib" / name
            assert archive.stat().st_size == info["size"]
            assert hashlib.sha256(archive.read_bytes()).hexdigest() == info["sha256"]
            receipt["archive_members"][name] = archive_members(archive)
        if not args.c_only:
            probe = build / "rejection.cpp"
            probe.write_text("int probe() { return 0; }\n")
            config = sysroot / "share/clang/sh4-libcxx.cfg"
            for option in (
                "-mfloat-abi=hard",
                "--target=sh-unknown-none-elf",
                "-mcpu=sh2",
                "-mcpu=sh4a",
                "-fexceptions",
                "-frtti",
                "-fPIC",
                "-fPIE",
            ):
                run(
                    [
                        tools["clang++"],
                        "--config=" + str(config),
                        option,
                        "-c",
                        probe,
                        "-o",
                        build / "rejection.o",
                    ],
                    reject=True,
                )
        resource = Path(run([tools["clang"], "--print-resource-dir"]).strip())
        common = [
            "--target=shle-unknown-none-elf",
            "-mcpu=sh4",
            "-m4-nofpu",
            "-msoft-float",
            "-mfloat-abi=soft",
            "-ffreestanding",
            "-fno-builtin",
            "-fno-pic",
            "-fno-pie",
            "-fno-stack-protector",
            "-fno-unwind-tables",
            "-fno-asynchronous-unwind-tables",
            "-ffunction-sections",
            "-fdata-sections",
            "-nostdinc",
            "-isystem",
            resource / "include",
            "-isystem",
            sysroot / "include",
        ]
        languages = [] if args.cxx_only else [("c", "c-test.c", "SH4 PICOLIBC PASS\n")]
        if not args.c_only:
            languages.append(
                ("cxx", "cxx-test.cpp", "SH4 LIBCXX LOCAL DTOR PASS\nSH4 LIBCXX PASS\n")
            )
        for opt in ("O0", "O2"):
            objects = []
            for file in (
                source.parent / "start-qemu-user.s",
                source / "platform.c",
                source / "archive.c",
            ):
                obj = build / (opt + "-" + file.stem + ".o")
                run([tools["clang"], *common, "-" + opt, "-c", file, "-o", obj])
                objects.append(obj)
            archive = build / (opt + "-example.a")
            run([tools["llvm-ar"], "rcsD", archive, objects.pop()])
            run([tools["llvm-ranlib"], archive])
            for language, file, expected in languages:
                obj = build / (opt + "-" + language + ".o")
                flags = []
                if language == "cxx":
                    flags = [
                        "-std=c++17",
                        "-fno-exceptions",
                        "-fno-rtti",
                        "-nostdinc++",
                        "-isystem",
                        sysroot / "include/c++/v1",
                    ]
                run(
                    [
                        tools["clang++" if language == "cxx" else "clang"],
                        *flags,
                        *common,
                        "-" + opt,
                        "-c",
                        source / file,
                        "-o",
                        obj,
                    ]
                )
                libraries = [] if language == "c" else ["-lc++", "-lc++abi"]
                for mode in ("direct", "partial", "config"):
                    name = opt + "-" + language + "-" + mode
                    inputs = [*objects, obj, archive]
                    if mode == "partial":
                        merged = build / (name + ".o")
                        run(
                            [
                                tools["ld.lld"],
                                "-m",
                                "shlelf",
                                "-r",
                                *inputs,
                                "-o",
                                merged,
                            ]
                        )
                        elf_info(merged, 1, expected_flags=0x10)
                        inputs = [merged]
                    output = build / (name + ".elf")
                    if mode == "config":
                        compiler = tools["clang++" if language == "cxx" else "clang"]
                        config = (
                            sysroot
                            / "share/clang"
                            / (
                                "sh4-libcxx.cfg"
                                if language == "cxx"
                                else "sh4-picolibc.cfg"
                            )
                        )
                        configured_obj = build / (name + ".o")
                        run(
                            [
                                compiler,
                                "--config=" + str(config),
                                "-" + opt,
                                "-c",
                                source / file,
                                "-o",
                                configured_obj,
                            ]
                        )
                        inputs = [*objects, configured_obj, archive]
                        run(
                            [
                                compiler,
                                "--config=" + str(config),
                                "-Wl,--gc-sections",
                                "-T",
                                source / "memory.ld",
                                *inputs,
                                "-o",
                                output,
                            ]
                        )
                    else:
                        run(
                            [
                                tools["ld.lld"],
                                "-m",
                                "shlelf",
                                "-static",
                                "--gc-sections",
                                "-T",
                                source / "memory.ld",
                                *inputs,
                                "-L",
                                sysroot / "lib",
                                "--start-group",
                                *libraries,
                                "-lc",
                                "-lm",
                                "-lclang_rt.builtins",
                                "--end-group",
                                "-o",
                                output,
                            ]
                        )
                    program = {
                        "name": name,
                        **elf_info(output, 2, expected_flags=0x10),
                        "executed": False,
                    }
                    receipt["programs"].append(program)
                    run(
                        [
                            tools["llvm-readobj"],
                            "--file-headers",
                            "--program-headers",
                            output,
                        ]
                    )
                    run([tools["llvm-nm"], "--undefined-only", output])
                    if args.qemu:
                        stdout = run([args.qemu.resolve(), output])
                        assert stdout == expected, (name, stdout, expected)
                        program.update({"executed": True, "exit": 0, "stdout": stdout})
        receipt["status"] = "PASS" if args.qemu else "LINK_ONLY"
    except Exception as error:
        receipt["status"] = "FAIL"
        receipt["error"] = repr(error)
        raise
    finally:
        (build / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
        print(receipt["status"] + ": " + str(build / "receipt.json"))


if __name__ == "__main__":
    main()
