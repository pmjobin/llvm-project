import pathlib
import re
import subprocess
import sys

LLVM_MC = "llvm-mc"
LLVM_OBJCOPY = "llvm-objcopy"


def conditional_forms(instruction):
	return [instruction, "dct " + instruction, "dcf " + instruction]


CPU_FORMS = [
	"clrdmxy",
]
CPU_FORMS += [f"ldc r3,{reg}" for reg in ("mod", "re", "rs")]
CPU_FORMS += [f"ldc.l @r3+,{reg}" for reg in ("mod", "re", "rs")]
CPU_FORMS += ["ldrc r3", "ldrc #4", "ldre @(8,pc)", "ldrs @(8,pc)"]
CPU_FORMS += [f"lds r3,{reg}" for reg in ("dsr", "a0", "x0", "x1", "y0", "y1")]
CPU_FORMS += [f"lds.l @r3+,{reg}" for reg in ("dsr", "a0", "x0", "x1", "y0", "y1")]
CPU_FORMS += ["setdmx", "setdmy", "setrc r3", "setrc #4"]
CPU_FORMS += [f"stc {reg},r3" for reg in ("mod", "re", "rs")]
CPU_FORMS += [f"stc.l {reg},@-r3" for reg in ("mod", "re", "rs")]
CPU_FORMS += [f"sts {reg},r3" for reg in ("dsr", "a0", "x0", "x1", "y0", "y1")]
CPU_FORMS += [f"sts.l {reg},@-r3" for reg in ("dsr", "a0", "x0", "x1", "y0", "y1")]

X_TRANSFER_FORMS = ["nopx nopy"]
X_TRANSFER_FORMS += [f"movx.w {addr},x0 nopy" for addr in ("@r4", "@r4+", "@r4+r8")]
X_TRANSFER_FORMS += [f"movx.w {addr},x0 nopy" for addr in ("@r0", "@r0+", "@r0+r8")]
X_TRANSFER_FORMS += [f"movx.l {addr},x0 nopy" for addr in ("@r0", "@r0+", "@r0+r8")]
X_TRANSFER_FORMS += [f"movx.w a0,{addr} nopy" for addr in ("@r4", "@r4+", "@r4+r8")]
X_TRANSFER_FORMS += [f"movx.w x0,{addr} nopy" for addr in ("@r0", "@r0+", "@r0+r8")]
X_TRANSFER_FORMS += [f"movx.l x0,{addr} nopy" for addr in ("@r0", "@r0+", "@r0+r8")]

Y_TRANSFER_FORMS = ["nopx nopy"]
Y_TRANSFER_FORMS += [f"nopx movy.w {addr},y0" for addr in ("@r6", "@r6+", "@r6+r9")]
Y_TRANSFER_FORMS += [f"nopx movy.w {addr},y0" for addr in ("@r2", "@r2+", "@r2+r9")]
Y_TRANSFER_FORMS += [f"nopx movy.l {addr},y0" for addr in ("@r2", "@r2+", "@r2+r9")]
Y_TRANSFER_FORMS += [f"nopx movy.w a0,{addr}" for addr in ("@r6", "@r6+", "@r6+r9")]
Y_TRANSFER_FORMS += [f"nopx movy.w y0,{addr}" for addr in ("@r2", "@r2+", "@r2+r9")]
Y_TRANSFER_FORMS += [f"nopx movy.l y0,{addr}" for addr in ("@r2", "@r2+", "@r2+r9")]

SINGLE_TRANSFER_FORMS = []
for size in ("w", "l"):
	SINGLE_TRANSFER_FORMS += [f"movs.{size} {addr},x0" for addr in ("@-r4", "@r4", "@r4+", "@r4+r8")]
	SINGLE_TRANSFER_FORMS += [f"movs.{size} x0,{addr}" for addr in ("@-r4", "@r4", "@r4+", "@r4+r8")]

OPERATION_FORMS = []
OPERATION_FORMS += conditional_forms("pabs x0,a0")
OPERATION_FORMS += conditional_forms("pabs y0,a0")
OPERATION_FORMS += ["padd a0,m0,a0 pmuls x0,y0,m0"]
OPERATION_FORMS += conditional_forms("padd x0,y0,a0")
OPERATION_FORMS += ["paddc x0,y0,m0"]
OPERATION_FORMS += conditional_forms("pand x0,y0,a0")
OPERATION_FORMS += ["pclr a0 pmuls x0,y0,m0"]
OPERATION_FORMS += conditional_forms("pclr a0")
OPERATION_FORMS += ["pcmp x0,y0"]
for mnemonic in ("pcopy", "pdec", "pdmsb", "pinc", "pneg", "prnd", "pswap"):
	OPERATION_FORMS += conditional_forms(f"{mnemonic} x0,a0")
	OPERATION_FORMS += conditional_forms(f"{mnemonic} y0,a0")
OPERATION_FORMS += conditional_forms("plds a0,mach")
OPERATION_FORMS += conditional_forms("plds a0,macl")
OPERATION_FORMS += ["pmuls x0,y0,m0"]
OPERATION_FORMS += conditional_forms("por x0,y0,a0")
OPERATION_FORMS += conditional_forms("psha x0,y0,a0")
OPERATION_FORMS += ["psha #-1,a0"]
OPERATION_FORMS += conditional_forms("pshl x0,y0,a0")
OPERATION_FORMS += ["pshl #-1,a0"]
OPERATION_FORMS += conditional_forms("psts mach,a0")
OPERATION_FORMS += conditional_forms("psts macl,a0")
OPERATION_FORMS += ["psub a0,m0,a0 pmuls x0,y0,m0"]
OPERATION_FORMS += conditional_forms("psub x0,y0,a0")
OPERATION_FORMS += conditional_forms("psub y0,x0,a0")
OPERATION_FORMS += ["psubc x0,y0,a0"]
OPERATION_FORMS += conditional_forms("pxor x0,y0,a0")
OPERATION_FORMS += ["pwsb x0,y0,a0", "pwad x0,y0,a0"]

FORMS = CPU_FORMS + X_TRANSFER_FORMS + Y_TRANSFER_FORMS + SINGLE_TRANSFER_FORMS + OPERATION_FORMS
WIDTHS = [2] * (len(CPU_FORMS) + len(X_TRANSFER_FORMS) + len(Y_TRANSFER_FORMS) + len(SINGLE_TRANSFER_FORMS)) + [4] * len(OPERATION_FORMS)

EXPECTED_BE_HEX = """
	0088435e437e436e43574377436743348a048e028c02436a437a438a439a43aa43ba436643764386439643a643b6009800c84314820403520372036243534373
	4363036a037a038a039a03aa03ba436243724382439243a243b2f000f004f008f00cf104f108f10cf114f118f11cf024f028f02cf164f168f16cf174f178f17c
	f000f001f002f003f201f202f203f221f222f223f011f012f013f291f292f293f2b1f2b2f2b3f480f484f488f48cf481f485f489f48df482f486f48af48ef483
	f487f48bf48ff8008807f8008a17f8008b17f800a807f800aa47f800ab47f80070a2f800b107f800b207f800b307f800b00cf8009507f8009607f8009707f800
	4012f8008d07f8008e07f8008f07f8008400f800d907f800da07f800db07f800f907f800fa07f800fb07f8008907f8008a07f8008b07f800a907f800aa07f800
	ab07f8009d07f8009e07f8009f07f800bd07f800be07f800bf07f8009907f8009a07f8009b07f800b907f800ba07f800bb07f800c907f800ca07f800cb07f800
	e907f800ea07f800eb07f8009807f8009a17f8009b17f800b807f800ba47f800bb47f8009d17f8009e17f8009f17f800bd47f800be47f800bf47f800ed07f800
	ee07f800ef07f800fd07f800fe07f800ff07f8004000f800b507f800b607f800b707f8009107f8009207f8009307f80017f7f8008107f8008207f8008307f800
	07f7f800cd07f800ce07f800cf07f800dd07f800de07f800df07f80060a2f800a107f800a207f800a307f8008507f8008607f8008707f800a007f800a507f800
	a607f800a707f800a407f800b407
"""


def run(command, input_text=None):
	result = subprocess.run(command, input=input_text, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	if result.returncode:
		raise RuntimeError(f"{' '.join(command)} failed:\n{result.stderr}")
	return result.stdout


def assemble(source, triple, stem):
	object_path = TEMPORARY.with_name(TEMPORARY.name + f"-{stem}.o")
	section_path = TEMPORARY.with_name(TEMPORARY.name + f"-{stem}.text")
	run([LLVM_MC, "-triple=" + triple, "-mcpu=sh4al-dsp", "-filetype=obj", "-o", str(object_path)], source)
	run([LLVM_OBJCOPY, "--dump-section", f".text={section_path}", str(object_path)])
	return section_path.read_bytes()


def disassemble(raw, triple):
	input_text = " ".join(f"0x{byte:02x}" for byte in raw) + "\n"
	output = run([LLVM_MC, "-triple=" + triple, "-mcpu=sh4al-dsp", "-disassemble", "-show-encoding"], input_text)
	lines = [line.split("!", 1)[0].strip() for line in output.splitlines() if "encoding:" in line]
	encoded = []
	for line in output.splitlines():
		if "encoding:" not in line:
			continue
		match = re.search(r"encoding: \[([^]]+)\]", line)
		assert match, line
		encoded.extend(int(byte.strip(), 16) for byte in match.group(1).split(","))
	assert bytes(encoded) == raw, (len(raw), len(encoded), output)
	return lines


if len(sys.argv) == 2 and sys.argv[1] == "--dump-source":
	print("\n".join(FORMS))
	sys.exit(0)

TEMPORARY = pathlib.Path(sys.argv[1])
assert len(CPU_FORMS) == 45
assert len(X_TRANSFER_FORMS) == 19
assert len(Y_TRANSFER_FORMS) == 19
assert len(SINGLE_TRANSFER_FORMS) == 16
assert len(OPERATION_FORMS) == 98
assert len(FORMS) == 197
assert WIDTHS.count(2) == 99
assert WIDTHS.count(4) == 98
assert 179 + len(FORMS) == 376
assert 179 * 2 + sum(WIDTHS) == 948

source = "\n".join(FORMS) + "\n"
expected_be = bytes.fromhex(EXPECTED_BE_HEX)
assert len(expected_be) == sum(WIDTHS)
expected_le = b"".join(expected_be[offset:offset + 2][::-1] for offset in range(0, len(expected_be), 2))

for triple, expected, stem in (("sh-unknown-elf", expected_be, "be"), ("shle-unknown-elf", expected_le, "le")):
	assembled = assemble(source, triple, stem)
	assert assembled == expected, (stem, assembled.hex(), expected.hex())
	printed = disassemble(expected, triple)
	assert len(printed) == len(FORMS), (stem, len(printed), len(FORMS))
	roundtrip = assemble("\n".join(printed) + "\n", triple, stem + "-roundtrip")
	assert roundtrip == expected, (stem, roundtrip.hex(), expected.hex())

for triple in ("sh-unknown-elf", "shle-unknown-elf"):
	for unavailable in ("clrdmxy\n", "movx.w @r4,x0 nopy\n", "padd x0,y0,a0\n"):
		result = subprocess.run([LLVM_MC, "-triple=" + triple, "-mcpu=sh4a", "-filetype=null"], input=unavailable, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		assert result.returncode
		assert "instruction requires an unavailable feature" in result.stderr
	result = subprocess.run([LLVM_MC, "-triple=" + triple, "-mcpu=sh4al-dsp", "-filetype=null"], input="fadd fr1,fr2\n", text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	assert result.returncode
	assert "instruction requires an unavailable feature" in result.stderr
