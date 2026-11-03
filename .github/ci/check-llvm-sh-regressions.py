#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Run the source SH suites; source tests do not replace installed-tool tests."""

import argparse
import json
from pathlib import Path
import subprocess
import sys


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source", type=Path, required=True)
    p.add_argument("--build", type=Path, required=True)
    p.add_argument("--logs", type=Path, required=True)
    p.add_argument("--configuration", default="")
    args = p.parse_args()
    args.logs.mkdir(parents=True, exist_ok=True)
    binary = args.build / args.configuration / "bin"
    candidates = [binary / "llvm-lit.py", binary / "llvm-lit"]
    lit = next((c for c in candidates if c.is_file()), None)
    if lit is None:
        raise SystemExit("Missing built llvm-lit in " + str(binary))
    source = args.source
    clang_tests = sorted(
        p
        for p in (source / "clang/test").glob("*/sh*")
        if p.is_file() and p.suffix in (".c", ".cpp", ".s", ".ll")
    )
    clang_tests.append(source / "clang/test/CodeGen/SH")
    suites = {
        "llvm-sh": [source / "llvm/test/MC/SH", source / "llvm/test/CodeGen/SH"],
        "clang-sh": clang_tests,
        "lld-sh": sorted((source / "lld/test/ELF").glob("sh-*")),
    }
    receipt = []
    failed = False
    for name, tests in suites.items():
        command = [
            sys.executable,
            str(lit),
            "-v",
            "-j",
            "2",
            "--timeout=120",
            "--xunit-xml-output",
            str(args.logs / (name + ".xml")),
            *(
                ["--param", "build_mode=" + args.configuration]
                if args.configuration
                else []
            ),
            *map(str, tests),
        ]
        with (args.logs / (name + ".log")).open("w", encoding="utf-8") as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
        receipt.append({"suite": name, "command": command, "exit": result.returncode})
        output = (args.logs / (name + ".log")).read_text(encoding="utf-8")
        print(output if result.returncode else output[-3000:])
        failed |= result.returncode != 0
    (args.logs / "commands.json").write_text(
        json.dumps(receipt, indent=2) + "\n", encoding="utf-8"
    )
    if failed:
        raise SystemExit("SH regression failure; see logs and XML reports")


if __name__ == "__main__":
    main()
