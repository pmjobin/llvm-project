import argparse
import re
import sys


FDE_RE = re.compile(r"^\S+\s+\S+\s+\S+\s+FDE cie=")
ROW_RE = re.compile(r"^\s+0x[0-9a-f]+: CFA=R15(?:\+([0-9]+))?(?:: (.*))?$")
RULE_RE = re.compile(r"(R(?:8|9|10|11|12|13|14)|PR)=\[CFA(-[0-9]+)\]")

SCENARIOS = {
	"leaf-no-frame": [(0, {})],
	"leaf-fixed": [(0, {}), (4, {}), (0, {})],
	"call-only": [
		(0, {}),
		(4, {"PR": -4}),
		(0, {}),
	],
	"callee-saved-r8": [
		(0, {}),
		(4, {"PR": -4}),
		(8, {"PR": -4}),
		(8, {"R8": -8, "PR": -4}),
		(8, {"PR": -4}),
		(4, {"PR": -4}),
		(0, {}),
	],
	"fixed-twelve": [
		(0, {}),
		(4, {"PR": -4}),
		(12, {"PR": -4}),
		(4, {"PR": -4}),
		(0, {}),
	],
	"several-callee-saved": [
		(0, {}),
		(4, {"PR": -4}),
		(36, {"PR": -4}),
		(36, {"R8": -8, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "R10": -16, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "R10": -16, "R11": -20, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "R10": -16, "R11": -20,
		      "R12": -24, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "R10": -16, "R11": -20,
		      "R12": -24, "R13": -28, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "R10": -16, "R11": -20,
		      "R12": -24, "R13": -28, "R14": -32, "PR": -4}),
		(48, {"R8": -8, "R9": -12, "R10": -16, "R11": -20,
		      "R12": -24, "R13": -28, "R14": -32, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "R10": -16, "R11": -20,
		      "R12": -24, "R13": -28, "R14": -32, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "R10": -16, "R11": -20,
		      "R12": -24, "R13": -28, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "R10": -16, "R11": -20,
		      "R12": -24, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "R10": -16, "R11": -20, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "R10": -16, "PR": -4}),
		(36, {"R8": -8, "R9": -12, "PR": -4}),
		(36, {"R8": -8, "PR": -4}),
		(36, {"PR": -4}),
		(4, {"PR": -4}),
		(0, {}),
	],
	"byval-sret": [
		(0, {}),
		(16, {}),
		(20, {"PR": -20}),
		(24, {"PR": -20}),
		(24, {"R8": -24, "PR": -20}),
		(24, {"PR": -20}),
		(20, {"PR": -20}),
		(16, {}),
		(0, {}),
	],
	"outgoing-sixty": [
		(0, {}),
		(4, {"PR": -4}),
		(64, {"PR": -4}),
		(4, {"PR": -4}),
		(0, {}),
	],
	"varargs-12": [
		(0, {}),
		(12, {}),
		(16, {"PR": -16}),
		(20, {"PR": -16}),
		(24, {"PR": -16}),
		(20, {"PR": -16}),
		(16, {"PR": -16}),
		(12, {}),
		(0, {}),
	],
	"varargs-8": [
		(0, {}),
		(8, {}),
		(12, {"PR": -12}),
		(16, {"PR": -12}),
		(20, {"PR": -12}),
		(16, {"PR": -12}),
		(12, {"PR": -12}),
		(8, {}),
		(0, {}),
	],
	"varargs-4": [
		(0, {}),
		(4, {}),
		(8, {"PR": -8}),
		(12, {"PR": -8}),
		(16, {"PR": -8}),
		(12, {"PR": -8}),
		(8, {"PR": -8}),
		(4, {}),
		(0, {}),
	],
	"tas-leaf": [(0, {})],
	"singlethread-fence": [(0, {})],
	"extended-atomic": [
		(0, {}),
		(4, {"PR": -4}),
		(72, {"PR": -4}),
		(68, {"PR": -4}),
		(72, {"PR": -4}),
		(80, {"PR": -4}),
		(72, {"PR": -4}),
		(4, {"PR": -4}),
		(0, {}),
	],
	"multiple-epilogues": [
		(0, {}),
		(4, {"PR": -4}),
		(8, {"PR": -4}),
		(4, {"PR": -4}),
		(0, {}),
		(8, {"PR": -4}),
		(4, {"PR": -4}),
		(0, {}),
		(8, {"PR": -4}),
		(4, {"PR": -4}),
		(0, {}),
	],
}


def fail(message):
	print("unwind model failure: " + message, file=sys.stderr)
	sys.exit(1)


def parse_fdes(lines):
	fdes = []
	current = None
	for line in lines:
		if FDE_RE.match(line):
			current = []
			fdes.append(current)
			continue
		if current is None:
			continue
		match = ROW_RE.match(line)
		if not match:
			continue
		offset = int(match.group(1) or 0)
		rules = {}
		for register, relative in RULE_RE.findall(match.group(2) or ""):
			rules[register] = int(relative)
		current.append((offset, rules))
	return fdes


def validate_fde(name, index, rows):
	expected_rows = SCENARIOS.get(name)
	if expected_rows is None:
		fail("unknown scenario " + name)
	if rows != expected_rows:
		fail(name + " CFI rows do not match the independent frame plan:\n" +
		     "expected " + repr(expected_rows) + "\n" +
		     "actual   " + repr(rows))

	caller_cfa = 0x10000000 + index * 0x1000
	caller_registers = {"PR": 0x51000000 + index}
	for register in range(8, 15):
		caller_registers["R" + str(register)] = (
			0x60000000 + index * 0x100 + register
		)

	stack = {}
	for _, rules in expected_rows:
		for register, relative in rules.items():
			address = caller_cfa + relative
			expected = caller_registers[register]
			if address in stack and stack[address] != expected:
				fail(name + " assigns two registers to one stack word")
			stack[address] = expected

	recoveries = 0
	for row_number, (cfa_offset, rules) in enumerate(rows):
		current_sp = caller_cfa - cfa_offset
		recovered_cfa = current_sp + cfa_offset
		if recovered_cfa != caller_cfa:
			fail(name + " row " + str(row_number) + " recovers the wrong CFA")

		for register, expected in caller_registers.items():
			if register in rules:
				address = caller_cfa + rules[register]
				if address not in stack:
					fail(name + " row " + str(row_number) +
					     " references an unexpected stack word")
				recovered = stack[address]
			else:
				recovered = expected
			if recovered != expected:
				fail(name + " row " + str(row_number) +
				     " recovers the wrong " + register)
			recoveries += 1

		if recovered_cfa != caller_cfa:
			fail(name + " recovers the wrong caller SP")
	return recoveries


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("--scenarios", required=True)
	args = parser.parse_args()
	scenarios = [name for name in args.scenarios.split(",") if name]
	fdes = parse_fdes(sys.stdin)
	if len(fdes) != len(scenarios):
		fail("expected " + str(len(scenarios)) + " FDEs, found " +
		     str(len(fdes)))

	row_count = 0
	recovery_count = 0
	for index, (name, rows) in enumerate(zip(scenarios, fdes)):
		row_count += len(rows)
		recovery_count += validate_fde(name, index, rows)

	print("scenarios: " + str(len(scenarios)))
	print("rows: " + str(row_count))
	print("recoveries: " + str(recovery_count))
	print("caller CFA, PR, r8-r14, and SP recovered")


if __name__ == "__main__":
	main()
