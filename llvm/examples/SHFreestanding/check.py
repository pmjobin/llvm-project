#!/usr/bin/env python3
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""Qualify an installed SH4 freestanding toolchain using only its tools.

When --qemu is given, execute every linked program with an independently
installed QEMU SH4 Linux user emulator. The syscall adapter is a test harness,
not a hosted target runtime or a universal board image.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess


def elf_info(path, expected_type, data=None, expected_flags=9):
    """Check ELF32 independently of the toolchain's ELF reader."""
    if data is None:
        data = path.read_bytes()
    assert data[:7] == b"\x7fELF\x01\x01\x01", (path, "ELF32 little endian")
    h = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
    assert h[1:3] == (expected_type, 42) and h[7] == expected_flags, (path, h)
    headers = [
        struct.unpack_from("<IIIIIIII", data, h[5] + i * h[9]) for i in range(h[10])
    ]
    assert not any(p[0] in (2, 3) for p in headers), "dynamic ELF input"
    defined, undefined = set(), set()
    if expected_type == 2:
        loads = [p for p in headers if p[0] == 1]
        assert loads and all(p[5] >= p[4] for p in loads)
        assert any(p[2] <= h[4] < p[2] + p[5] and p[6] & 1 for p in loads)
        assert all(
            (0x10000 <= p[2] and p[2] + p[5] <= 0x110000)
            or (0x200000 <= p[2] and p[2] + p[5] <= 0x300000)
            for p in loads
        )
    table = [
        struct.unpack_from("<IIIIIIIIII", data, h[6] + i * h[11]) for i in range(h[12])
    ]
    strings = table[h[13]]
    names = data[strings[4] : strings[4] + strings[5]]
    sections = {names[s[0] :].split(b"\0", 1)[0].decode(): s for s in table}
    assert any(s[2] & 4 and s[5] for s in table), (path, "empty code")
    if expected_type == 2:
        assert ".text" in sections and sections[".text"][5]
        assert ".bss" in sections and sections[".bss"][1] == 8
        assert not any(s[1] in (4, 9) and s[5] for s in table)
    for s in table:
        if s[1] != 2:
            continue
        names_sec = table[s[6]]
        symbol_names = data[names_sec[4] : names_sec[4] + names_sec[5]]
        for off in range(s[4] + 16, s[4] + s[5], 16):
            sym = struct.unpack_from("<IIIBBH", data, off)
            name = symbol_names[sym[0] :].split(b"\0", 1)[0].decode()
            if sym[3] >> 4 in (1, 2):
                (defined if sym[5] else undefined).add(name)
            if sym[5] == 0 and expected_type == 2:
                assert sym[3] >> 4 == 2 and name in ("__cxa_finalize", "after_fini"), (
                    path,
                    "undefined",
                    name,
                )
    return {
        "entry": h[4],
        "type": h[1],
        "machine": h[2],
        "flags": h[7],
        "sha256": hashlib.sha256(data).hexdigest(),
        "size": len(data),
        "program_headers": headers,
        "defined": sorted(defined),
        "undefined": sorted(undefined),
    }


def archive_info(path):
    data = path.read_bytes()
    assert data[:8] == b"!<arch>\n", (path, "archive signature")
    offset, members = 8, []
    while offset < len(data):
        header = data[offset : offset + 60]
        assert len(header) == 60 and header[58:] == b"`\n"
        size = int(header[48:58])
        content = data[offset + 60 : offset + 60 + size]
        if content.startswith(b"\x7fELF"):
            label = str(path) + "(" + header[:16].decode().strip() + ")"
            members.append(elf_info(label, 1, content))
        offset += 60 + size + (size & 1)
    assert offset == len(data) and members, (path, "archive members")
    return {
        "sha256": hashlib.sha256(data).hexdigest(),
        "size": len(data),
        "members": members,
        "defined": sorted({s for m in members for s in m["defined"]}),
        "undefined": sorted({s for m in members for s in m["undefined"]}),
    }


def main():
    if not __debug__:
        raise SystemExit("Run qualification without Python optimization (-O)")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--runtime-dir", type=Path)
    parser.add_argument("--source-dir", type=Path, default=Path(__file__).parent)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--qemu", type=Path)
    parser.add_argument("--require-execution", action="store_true")
    args = parser.parse_args()
    if args.require_execution and not args.qemu:
        parser.error("--require-execution needs an independently installed --qemu")
    prefix = args.prefix.resolve()
    source = args.source_dir.resolve()
    build = args.build_dir.resolve()
    runtime = (
        args.runtime_dir or prefix / "lib/shle-unknown-none-elf/sh4-soft"
    ).resolve()
    build.mkdir(parents=True, exist_ok=True)
    suffix = ".exe" if os.name == "nt" else ""
    tools = {
        name: prefix / "bin" / (name + suffix)
        for name in (
            "clang",
            "clang++",
            "llvm-mc",
            "llvm-ar",
            "llvm-ranlib",
            "ld.lld",
            "llvm-readobj",
            "llvm-objdump",
            "llvm-nm",
        )
    }
    for tool in tools.values():
        assert tool.is_file(), ("missing packaged tool", tool)
    for name in ("libclang_rt.builtins.a", "libclang_rt.sh4_cxx.a"):
        assert (runtime / "lib" / name).is_file(), ("missing target runtime", name)
    env = os.environ.copy()
    env["PATH"] = str(prefix / "bin")
    env.pop("COMPILER_PATH", None)
    env.pop("LIBRARY_PATH", None)
    env.pop("CPATH", None)
    env.pop("C_INCLUDE_PATH", None)
    env.pop("CPLUS_INCLUDE_PATH", None)
    env.pop("LD_LIBRARY_PATH", None)
    env.pop("LD_PRELOAD", None)
    receipt = {
        "status": "RUNNING",
        "prefix": str(prefix),
        "commands": [],
        "programs": [],
        "execution_requested": bool(args.qemu),
    }

    def run(command):
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
            }
        )
        assert result.returncode == 0, (command, result.returncode, result.stderr)
        return result

    try:
        builtins = archive_info(runtime / "lib/libclang_rt.builtins.a")
        cxx = archive_info(runtime / "lib/libclang_rt.sh4_cxx.a")
        manifest = json.loads((runtime / "RUNTIME.json").read_text(encoding="utf-8"))
        assert manifest["profile"] == "shle-unknown-none-elf/sh4-soft"
        for name, info in (
            ("libclang_rt.builtins.a", builtins),
            ("libclang_rt.sh4_cxx.a", cxx),
        ):
            assert manifest["archives"][name] == {
                "sha256": info["sha256"],
                "size": info["size"],
            }, ("runtime archive differs from manifest", name)
        for name, expected in manifest["sources"].items():
            path = runtime / "src/builtins" / name
            assert hashlib.sha256(path.read_bytes()).hexdigest() == expected, (
                "runtime source differs from manifest",
                name,
            )
        assert (
            hashlib.sha256(
                (runtime / "src/build-sh4-freestanding.py").read_bytes()
            ).hexdigest()
            == manifest["recipe_sha256"]
        ), "runtime recipe differs from manifest"
        receipt["runtime_manifest"] = manifest
        assert not set(builtins["undefined"]) - set(
            builtins["defined"]
        ), "runtime helper gap"
        assert set(cxx["undefined"]) - set(cxx["defined"]) - set(
            builtins["defined"]
        ) == {"sh4_allocate", "sh4_deallocate", "sh4_panic"}, "C++ hook contract"
        receipt["runtime"] = {"builtins": builtins, "cxx": cxx}
        for tool in tools.values():
            run([tool, "--version"])
        if args.qemu:
            run([args.qemu.resolve(), "--version"])
        resource = Path(
            run([tools["clang"], "--print-resource-dir"]).stdout.strip()
        ).resolve()
        assert prefix in resource.parents, (
            "resource headers outside package",
            resource,
        )
        common = [
            "--target=shle-unknown-none-elf",
            "-mcpu=sh4",
            "-msoft-float",
            "-mfloat-abi=soft",
            "-ffreestanding",
            "-fno-builtin",
            "-fno-pic",
            "-fno-pie",
            "-fno-stack-protector",
            "-fno-exceptions",
            "-fno-unwind-tables",
            "-fno-asynchronous-unwind-tables",
            "-ffunction-sections",
            "-fdata-sections",
            "-nostdinc",
            "-isystem",
            resource / "include",
            "-I",
            runtime / "include",
        ]
        for opt in ("0", "2"):
            objects = {}
            for name in (
                "start.c",
                "archive.c",
                "arithmetic.c",
                "link-test.c",
                "runtime-test.c",
                "cxx-test.cpp",
            ):
                obj = build / (name + ".O" + opt + ".o")
                cpp = name.endswith(".cpp")
                run(
                    [
                        tools["clang++" if cpp else "clang"],
                        *common,
                        "-O" + opt,
                        *(["-std=c++17", "-fno-rtti"] if cpp else []),
                        "-c",
                        source / name,
                        "-o",
                        obj,
                    ]
                )
                elf_info(obj, 1)
                objects[name] = obj
            for name in ("start-qemu-user.s", "helper-abi.s"):
                obj = build / (name + ".o")
                run(
                    [
                        tools["llvm-mc"],
                        "-triple=shle-unknown-none-elf",
                        "-mcpu=sh4",
                        "-filetype=obj",
                        source / name,
                        "-o",
                        obj,
                    ]
                )
                elf_info(obj, 1)
                objects[name] = obj
            archive = build / ("libexample.O" + opt + ".a")
            archive.unlink(missing_ok=True)
            run(
                [
                    tools["llvm-ar"],
                    "rcsD",
                    archive,
                    objects["archive.c"],
                    objects["arithmetic.c"],
                ]
            )
            run([tools["llvm-ranlib"], "-D", archive])
            programs = [
                ("link-test.c", "SH4 C link PASS\n"),
                ("runtime-test.c", "SH4 runtime PASS\n"),
                ("cxx-test.cpp", "SH4 C++ lifecycle PASS\n"),
            ]
            for main_source, expected in programs:
                inputs = [
                    objects["start-qemu-user.s"],
                    objects["start.c"],
                    objects[main_source],
                    objects["helper-abi.s"],
                    archive,
                ]
                if main_source.endswith(".cpp"):
                    inputs.append(runtime / "lib/libclang_rt.sh4_cxx.a")
                inputs.append(runtime / "lib/libclang_rt.builtins.a")
                for mode in ("direct", "default", "explicit", "partial"):
                    name = main_source + ".O" + opt + "." + mode
                    elf = build / (name + ".elf")
                    if mode == "partial":
                        partial = build / (name + ".o")
                        run([tools["ld.lld"], "-r", *inputs, "-o", partial])
                        elf_info(partial, 1)
                        inputs_for_link = [partial]
                    else:
                        inputs_for_link = inputs
                    if mode in ("direct", "partial"):
                        command = [tools["ld.lld"], "-static", "--gc-sections"]
                    else:
                        command = [
                            tools[
                                "clang++" if main_source.endswith(".cpp") else "clang"
                            ],
                            "--target=shle-unknown-none-elf",
                            "-mcpu=sh4",
                            "-nostdlib",
                            "-static",
                            "-Wl,--gc-sections",
                            "-v",
                        ]
                        if mode == "explicit":
                            command.append("-fuse-ld=lld")
                    run(
                        [
                            *command,
                            "-T",
                            source / "memory.ld",
                            *inputs_for_link,
                            "-o",
                            elf,
                        ]
                    )
                    info = elf_info(elf, 2)
                    run([tools["llvm-readobj"], "-h", "-l", "-S", "-s", "-r", elf])
                    run([tools["llvm-objdump"], "-dr", elf])
                    run([tools["llvm-nm"], elf])
                    execution = None
                    if args.qemu:
                        result = run(
                            [args.qemu.resolve(), "-cpu", "sh7750r", "-strace", elf]
                        )
                        assert result.stdout == expected, (
                            name,
                            result.stdout,
                            expected,
                        )
                        execution = {
                            "exit": result.returncode,
                            "stdout": result.stdout,
                            "trace": result.stderr,
                        }
                    receipt["programs"].append(
                        {"name": name, "elf": info, "execution": execution}
                    )
                    print(
                        name
                        + (": executed" if execution else ": linked; execution UNRUN")
                    )
        receipt["status"] = "PASS" if args.qemu else "PARTIAL"
    except Exception as error:
        receipt["status"] = "FAIL"
        receipt["error"] = str(error)
        raise
    finally:
        (build / "receipt.json").write_text(
            json.dumps(receipt, indent=2) + "\n", encoding="utf-8"
        )


if __name__ == "__main__":
    main()
