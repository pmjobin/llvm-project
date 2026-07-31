#!/usr/bin/env python3

import re
import subprocess
import sys


SECTION_RE = re.compile(
	r"^\s*\[\s*(\d+)\]\s+(\S*)\s+(\S+)\s+([0-9a-fA-F]+)\s+"
	r"([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+(.*?)\s+"
	r"(\d+)\s+(\d+)\s+\d+\s*$"
)
RELOCATION_HEADER_RE = re.compile(r"^Relocation section '([^']+)'")
RELOCATION_RE = re.compile(r"^([0-9a-fA-F]+)\s+\S+\s+(R_SH_\S+)")
INPUT_PIC_RELOCATIONS = {
	"R_SH_GOT32",
	"R_SH_PLT32",
	"R_SH_GOTOFF",
	"R_SH_GOTPC",
}


def inspect(readelf, path):
	output = subprocess.check_output(
		[readelf, "-hSWrd", path], stderr=subprocess.STDOUT, text=True
	)
	match = re.search(r"^\s*Type:\s+(\S+)", output, re.MULTILINE)
	if not match:
		raise RuntimeError("ELF type is missing")
	elf_type = match.group(1)

	sections = {}
	for line in output.splitlines():
		match = SECTION_RE.match(line)
		if not match:
			continue
		index, name, section_type, address, offset, size, flags, link, info = (
			match.groups()
		)
		sections[int(index)] = {
			"name": name,
			"type": section_type,
			"address": int(address, 16),
			"size": int(size, 16),
			"flags": flags.replace(" ", ""),
			"info": int(info),
		}

	errors = []
	if re.search(r"\bTEXTREL\b", output):
		errors.append("DT_TEXTREL is present")

	current_relocation_section = None
	relocation_count = 0
	for line in output.splitlines():
		match = RELOCATION_HEADER_RE.match(line)
		if match:
			current_relocation_section = match.group(1)
			continue
		match = RELOCATION_RE.match(line)
		if not match:
			continue
		offset = int(match.group(1), 16)
		relocation_type = match.group(2)
		relocation_count += 1

		target = None
		for section in sections.values():
			if section["name"] == current_relocation_section:
				target = sections.get(section["info"])
				break
		if target and "X" in target["flags"] and relocation_type == "R_SH_DIR32":
			errors.append(
				f"{relocation_type} targets executable section {target['name']}"
			)

		if elf_type == "DYN":
			for section in sections.values():
				start = section["address"]
				if (
					"X" in section["flags"]
					and start <= offset < start + section["size"]
				):
					errors.append(
						f"{relocation_type} at 0x{offset:x} lies in executable "
						f"section {section['name']}"
					)
			if relocation_type in INPUT_PIC_RELOCATIONS:
				errors.append(
					f"compiler input relocation {relocation_type} survived shared linking"
				)

	if errors:
		for error in errors:
			print(f"{path}: {error}", file=sys.stderr)
		return False
	print(f"{path}: checked {relocation_count} relocations, no PIC text relocation")
	return True


def main():
	if len(sys.argv) < 3:
		print(
			"usage: check-sh-pic-elf.py <llvm-readelf> <elf>...",
			file=sys.stderr,
		)
		return 2
	readelf = sys.argv[1]
	success = True
	for path in sys.argv[2:]:
		try:
			success = inspect(readelf, path) and success
		except (OSError, subprocess.CalledProcessError, RuntimeError) as error:
			print(f"{path}: {error}", file=sys.stderr)
			success = False
	return 0 if success else 1


if __name__ == "__main__":
	sys.exit(main())
