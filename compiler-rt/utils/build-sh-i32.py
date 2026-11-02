#!/usr/bin/env python3
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Build the existing compiler-rt i32 div/rem family for ordinary SH Linux ABI.

This is a supplemental static library, used before libgcc. It is not a complete
--rtlib=compiler-rt distribution. No installed library is changed.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

NAMES = ("divsi3", "udivsi3", "modsi3", "umodsi3", "divmodsi4", "udivmodsi4")
HEADERS = ("int_lib.h", "int_types.h", "int_endianness.h", "int_util.h", "int_div_impl.inc")
# EF_SH2, EF_SH3 and EF_SH4 from llvm/include/llvm/BinaryFormat/ELF.h.
SH_ISA_FLAGS = {"2": 2, "3": 3, "4": 9}


def check_elf(data, cpu, endian, label):
	"""Check the ordinary non-PIC SH relocatable header, not the calling ABI."""
	if (len(data) < 52 or data[:7] != b"\x7fELF\x01" + bytes((1 if endian == "<" else 2, 1))
			or data[7] not in (0, 3) or data[8] != 0
			or struct.unpack_from(endian + "HHI", data, 16) != (1, 42, 1)
			or struct.unpack_from(endian + "H", data, 40)[0] != 52
			or struct.unpack_from(endian + "I", data, 36)[0] != SH_ISA_FLAGS[cpu]):
		raise ValueError("Expected ordinary ELF32 SH" + cpu + " relocatable, matched endian/ISA: " + label)


def nm_symbols(text, archive=None):
	"""Read GNU/LLVM BSD nm records, including -A archive member prefixes."""
	records, seen = [], set()
	for line in text.splitlines():
		if not line.strip():
			continue
		member = None
		if archive is not None:
			prefix = str(archive) + ":"
			if not line.startswith(prefix):
				raise ValueError("Unexpected nm archive prefix: " + line)
			member, separator, line = line[len(prefix):].partition(":")
			if not separator or not member or member in (".", "..") or "/" in member or "\\" in member:
				raise ValueError("Unexpected nm archive member: " + member)
		match = re.fullmatch(r"\s*(?:([0-9a-fA-F]+)\s+)?([ABCDGIRSTUVWuivw])\s+([A-Za-z_.$][A-Za-z_0-9.$]*)\s*", line)
		if not match:
			raise ValueError("Unexpected nm symbol format: " + line)
		address, kind, symbol = match.groups()
		if (kind == "U" and address is not None) or (kind not in "Uwv" and address is None):
			raise ValueError("Unexpected nm symbol address: " + line)
		key = member, symbol
		if key in seen:
			raise ValueError("Duplicate nm symbol: " + symbol)
		seen.add(key)
		records.append((member, kind, symbol))
	return records


def check_definitions(text, expected, archive=None):
	records = nm_symbols(text, archive)
	if set(records) != set(expected):
		raise ValueError("Unexpected public definitions: " + text)
	return records


def undefined_symbols(text, archive=None):
	records = nm_symbols(text, archive)
	if any(kind != "U" for member, kind, symbol in records):
		raise ValueError("Expected undefined-only nm output: " + text)
	return records


def check_archive_index(text):
	lines = text.lstrip().splitlines()
	if not lines or lines[0] not in ("Archive index:", "Archive map"):
		raise ValueError("Missing GNU/LLVM archive symbol index")
	entries = []
	for line in lines[1:]:
		if not line.strip():
			break
		entries.append(line)
	expected = {"__" + name + " in " + name + ".o" for name in NAMES}
	if len(entries) != len(expected) or set(entries) != expected:
		raise ValueError("Unexpected archive symbol index: " + repr(entries))


def sha(path):
	with Path(path).open("rb") as stream:
		return hashlib.file_digest(stream, "sha256").hexdigest()


def run(args, commands, binary=False):
	args = list(map(str, args))
	process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	try:
		out, err = process.communicate(timeout=120)
	except subprocess.TimeoutExpired:
		process.kill()
		process.communicate()
		raise
	commands.append({"argv": args, "pid": process.pid, "returncode": process.returncode, "stdout_sha256": hashlib.sha256(out).hexdigest(), "stderr_sha256": hashlib.sha256(err).hexdigest()})
	if process.returncode:
		raise RuntimeError("Command failed: " + repr(args) + "\n" + err.decode(errors="replace"))
	return out if binary else out.decode()


def build(args):
	sources = Path(__file__).resolve().parents[1] / "lib/builtins"
	output = args.output.resolve()
	if output == sources or sources in output.parents:
		raise ValueError("Build outside the source tree")
	output.mkdir(parents=True, exist_ok=True)
	allowed = {"manifest.json", "libclang_rt.sh_i32.a", *(name + suffix for name in NAMES for suffix in (".s", ".o"))}
	if any(p.is_symlink() or not p.is_file() or p.name not in allowed for p in output.iterdir()):
		raise ValueError("Output contains unrelated files")
	manifest = output / "manifest.json"
	if any(output.iterdir()) and not manifest.is_file():
		raise ValueError("Nonempty output has no previous SH i32 manifest")
	if manifest.exists():
		old = json.loads(manifest.read_text())
		if old.get("format") != "compiler-rt-sh-i32-v1":
			raise ValueError("Unrecognized previous manifest")
		for name, digest in old["outputs"].items():
			if name not in allowed or not (output / name).is_file() or sha(output / name) != digest:
				raise ValueError("Previous output changed: " + name)
		for name in old["outputs"]:
			(output / name).unlink()
		manifest.unlink()
	commands = []
	flags = ["--no-default-config", "--target=" + args.target, "-m" + args.cpu, "-Os", "-ffreestanding", "-fno-builtin", "-fno-stack-protector", "-fno-pic", "-fno-pie"]
	endian = "<" if args.target.startswith("shle-") else ">"
	symbols, graph = {}, {}
	for name in NAMES:
		assembly, obj = output / (name + ".s"), output / (name + ".o")
		run([args.clang, *flags, "-S", sources / (name + ".c"), "-o", assembly], commands)
		run([args.assembler, "--isa=sh" + args.cpu, "--little" if endian == "<" else "--big", assembly, "-o", obj], commands)
		check_elf(obj.read_bytes(), args.cpu, endian, name)
		defined = run([args.nm, "-g", "--defined-only", obj], commands)
		check_definitions(defined, [(None, "T", "__" + name)])
		symbols[name] = defined.strip()
		undefined = run([args.nm, "-u", obj], commands)
		graph["__" + name] = [symbol for member, kind, symbol in undefined_symbols(undefined)]
	providers = {}
	external = {child for children in graph.values() for child in children if child not in graph}
	if external:
		if not args.libgcc or not external <= {"__ashlsi3", "__lshrsi3", "__ashrsi3"}:
			raise ValueError("SH2 shift dependencies need a matching --libgcc: " + repr(sorted(external)))
		defined = nm_symbols(run([args.nm, "-A", "-g", "--defined-only", args.libgcc], commands), args.libgcc)
		undefined = undefined_symbols(run([args.nm, "-A", "-u", args.libgcc], commands), args.libgcc)
		for symbol in sorted(external):
			matches = [(member, kind) for member, kind, name in defined if name == symbol]
			if len(matches) != 1 or matches[0][1] != "T":
				raise ValueError("Missing or ambiguous libgcc provider: " + symbol)
			member = matches[0][0]
			data = run([args.ar, "p", args.libgcc, member], commands, binary=True)
			check_elf(data, args.cpu, endian, "libgcc provider " + symbol)
			if any(origin == member for origin, kind, name in undefined):
				raise ValueError("Unexpected libgcc shift dependency: " + member)
			providers[symbol] = {"archive": str(args.libgcc.resolve()), "archive_sha256": sha(args.libgcc), "member": member, "member_sha256": hashlib.sha256(data).hexdigest(), "ELF_flags": struct.unpack_from(endian + "I", data, 36)[0], "ABI_source": "GCC libgcc/config/sh/lib1funcs.S; r4 value, r5 count, r0 result"}
	def visit(name, active):
		if name in providers:
			return
		if name not in graph or name in active:
			raise ValueError("Unresolved or recursive helper dependency: " + name)
		for child in graph[name]:
			visit(child, active | {name})
	for name in graph:
		visit(name, set())
	archive = output / "libclang_rt.sh_i32.a"
	run([args.ar, "rcsD", archive, *(output / (name + ".o") for name in NAMES)], commands)
	# GNU -s means --print-armap; LLVM -s instead requires segment/section names.
	index = run([args.nm, "--print-armap", archive], commands)
	check_archive_index(index)
	check_definitions(run([args.nm, "-A", "-g", "--defined-only", archive], commands),
		[(name + ".o", "T", "__" + name) for name in NAMES], archive)
	versions = {name: run([getattr(args, name), "--version"], commands).splitlines()[0] for name in ("clang", "assembler", "ar", "nm")}
	document = {"format": "compiler-rt-sh-i32-v1", "target": args.target, "cpu": "sh" + args.cpu, "abi": "ordinary Linux C ABI; no FDPIC or E-float", "flags": flags, "license": "Apache-2.0 WITH LLVM-exception", "source_sha256": {name: sha(sources / name) for name in [*(n + ".c" for n in NAMES), *HEADERS]}, "recipe_sha256": sha(__file__), "tools": {name: {"path": str(getattr(args, name).resolve()), "sha256": sha(getattr(args, name)), "version": versions[name]} for name in versions}, "exports": symbols, "dependencies": graph, "external_shift_providers": providers, "archive_index": index, "commands": commands, "link_order": ["user objects", "libclang_rt.sh_i32.a", "libgcc and ordinary platform libraries"], "outputs": {p.name: sha(p) for p in output.iterdir() if p.name != "manifest.json"}}
	manifest.write_text(json.dumps(document, indent="\t") + "\n")
	print(str(archive))


def main():
	parser = argparse.ArgumentParser(description=__doc__)
	for name in ("clang", "assembler", "ar", "nm"):
		parser.add_argument("--" + name, type=Path, required=True)
	parser.add_argument("--target", choices=("sh-unknown-linux-gnu", "shle-unknown-linux-gnu"), required=True)
	parser.add_argument("--cpu", choices=("2", "3", "4"), required=True)
	parser.add_argument("--libgcc", type=Path, help="Matching existing libgcc for SH2 variable-shift dependencies")
	parser.add_argument("--output", type=Path, required=True)
	build(parser.parse_args())


if __name__ == "__main__":
	main()
