#!/usr/bin/env python3
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""Check bare-metal Meson installation on the native host before building LLVM.

This checks header/library directory placement, including drive paths and spaces
on Windows. It neither builds target code nor qualifies a target runtime.
"""

import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--logs-dir", type=Path, required=True)
    args = parser.parse_args()
    args.build_dir.mkdir(parents=True, exist_ok=True)
    args.logs_dir.mkdir(parents=True, exist_ok=True)
    spec = importlib.util.spec_from_file_location(
        "sh_libraries", Path(__file__).with_name("build-libraries.py")
    )
    recipe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(recipe)
    receipt = {"status": "RUNNING", "commands": [], "target_code_built": False}

    def run(command):
        command = list(map(str, command))
        result = subprocess.run(command, capture_output=True, text=True, timeout=120)
        receipt["commands"].append(
            {
                "command": command,
                "exit": result.returncode,
                "stdout": result.stdout,
                "stderr": result.stderr,
            }
        )
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    try:
        with tempfile.TemporaryDirectory(
            prefix="meson SH ", dir=args.build_dir.resolve()
        ) as temporary:
            root = Path(temporary)
            source, build, stage = (
                root / name for name in ("source", "build", "stage")
            )
            source.mkdir()
            (source / "meson.build").write_text(
                "project('sh-install-layout')\n"
                "install_data('sh-staging.h', install_dir: get_option('includedir'))\n"
                "install_data('sh-staging.txt', install_dir: get_option('libdir'))\n"
            )
            expected = {
                "include/sh-staging.h": b"/* SH install layout check */\n",
                "lib/sh-staging.txt": b"SH install layout check\n",
            }
            for name, content in expected.items():
                (source / Path(name).name).write_bytes(content)
            cross = root / "cross.ini"
            cross.write_text(
                "[host_machine]\n"
                "system = 'none'\n"
                "cpu_family = 'sh'\n"
                "cpu = 'sh4'\n"
                "endian = 'little'\n"
            )
            run(
                [
                    "meson",
                    "setup",
                    build,
                    source,
                    "--cross-file",
                    cross,
                    "--prefix",
                    recipe.MESON_PREFIX,
                    "--libdir",
                    "lib",
                ]
            )
            run(["meson", "install", "-C", build, "--no-rebuild", "--destdir", stage])
            actual = {
                path.relative_to(stage).as_posix(): path.read_bytes()
                for path in stage.rglob("*")
                if path.is_file()
            }
            if actual != expected:
                raise RuntimeError("Unexpected installed layout: " + repr(actual))
            receipt["install_prefix"] = recipe.MESON_PREFIX
            receipt["destdir"] = str(stage)
            receipt["installed_files"] = sorted(actual)
        receipt["status"] = "PASS"
    except Exception as error:
        receipt.update(status="FAIL", error=str(error))
        raise
    finally:
        (args.logs_dir / "receipt.json").write_text(
            json.dumps(receipt, indent=2) + "\n"
        )
        print("Install layout: " + receipt["status"])


if __name__ == "__main__":
    main()
