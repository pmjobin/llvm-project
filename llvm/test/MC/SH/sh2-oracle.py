# RUN: %python %s %t

import pathlib
import re
import subprocess
import sys

LLVM_MC = "llvm-mc"
LLVM_OBJCOPY = "llvm-objcopy"
TEMPORARY = pathlib.Path(sys.argv[1])

ORACLE = """\
mov\tRm,Rn\t0110nnnnmmmm0011
mov\t#imm,Rn\t1110nnnniiiiiiii
mova\t@(disp,PC),R0\t11000111dddddddd
mov.w\t@(disp,PC),Rn\t1001nnnndddddddd
mov.l\t@(disp,PC),Rn\t1101nnnndddddddd
mov.b\t@Rm,Rn\t0110nnnnmmmm0000
mov.w\t@Rm,Rn\t0110nnnnmmmm0001
mov.l\t@Rm,Rn\t0110nnnnmmmm0010
mov.b\tRm,@Rn\t0010nnnnmmmm0000
mov.w\tRm,@Rn\t0010nnnnmmmm0001
mov.l\tRm,@Rn\t0010nnnnmmmm0010
mov.b\t@Rm+,Rn\t0110nnnnmmmm0100
mov.w\t@Rm+,Rn\t0110nnnnmmmm0101
mov.l\t@Rm+,Rn\t0110nnnnmmmm0110
mov.b\tRm,@-Rn\t0010nnnnmmmm0100
mov.w\tRm,@-Rn\t0010nnnnmmmm0101
mov.l\tRm,@-Rn\t0010nnnnmmmm0110
mov.b\t@(disp,Rm),R0\t10000100mmmmdddd
mov.w\t@(disp,Rm),R0\t10000101mmmmdddd
mov.l\t@(disp,Rm),Rn\t0101nnnnmmmmdddd
mov.b\tR0,@(disp,Rn)\t10000000nnnndddd
mov.w\tR0,@(disp,Rn)\t10000001nnnndddd
mov.l\tRm,@(disp,Rn)\t0001nnnnmmmmdddd
mov.b\t@(R0,Rm),Rn\t0000nnnnmmmm1100
mov.w\t@(R0,Rm),Rn\t0000nnnnmmmm1101
mov.l\t@(R0,Rm),Rn\t0000nnnnmmmm1110
mov.b\tRm,@(R0,Rn)\t0000nnnnmmmm0100
mov.w\tRm,@(R0,Rn)\t0000nnnnmmmm0101
mov.l\tRm,@(R0,Rn)\t0000nnnnmmmm0110
mov.b\t@(disp,GBR),R0\t11000100dddddddd
mov.w\t@(disp,GBR),R0\t11000101dddddddd
mov.l\t@(disp,GBR),R0\t11000110dddddddd
mov.b\tR0,@(disp,GBR)\t11000000dddddddd
mov.w\tR0,@(disp,GBR)\t11000001dddddddd
mov.l\tR0,@(disp,GBR)\t11000010dddddddd
movt\tRn\t0000nnnn00101001
swap.b\tRm,Rn\t0110nnnnmmmm1000
swap.w\tRm,Rn\t0110nnnnmmmm1001
xtrct\tRm,Rn\t0010nnnnmmmm1101
add\tRm,Rn\t0011nnnnmmmm1100
add\t#imm,Rn\t0111nnnniiiiiiii
addc\tRm,Rn\t0011nnnnmmmm1110
addv\tRm,Rn\t0011nnnnmmmm1111
cmp/eq\t#imm,R0\t10001000iiiiiiii
cmp/eq\tRm,Rn\t0011nnnnmmmm0000
cmp/hs\tRm,Rn\t0011nnnnmmmm0010
cmp/ge\tRm,Rn\t0011nnnnmmmm0011
cmp/hi\tRm,Rn\t0011nnnnmmmm0110
cmp/gt\tRm,Rn\t0011nnnnmmmm0111
cmp/pl\tRn\t0100nnnn00010101
cmp/pz\tRn\t0100nnnn00010001
cmp/str\tRm,Rn\t0010nnnnmmmm1100
div0s\tRm,Rn\t0010nnnnmmmm0111
div0u\t\t0000000000011001
div1\tRm,Rn\t0011nnnnmmmm0100
dmuls.l\tRm,Rn\t0011nnnnmmmm1101
dmulu.l\tRm,Rn\t0011nnnnmmmm0101
dt\tRn\t0100nnnn00010000
exts.b\tRm,Rn\t0110nnnnmmmm1110
exts.w\tRm,Rn\t0110nnnnmmmm1111
extu.b\tRm,Rn\t0110nnnnmmmm1100
extu.w\tRm,Rn\t0110nnnnmmmm1101
mac.l\t@Rm+,@Rn+\t0000nnnnmmmm1111
mac.w\t@Rm+,@Rn+\t0100nnnnmmmm1111
mul.l\tRm,Rn\t0000nnnnmmmm0111
muls.w\tRm,Rn\t0010nnnnmmmm1111
mulu.w\tRm,Rn\t0010nnnnmmmm1110
neg\tRm,Rn\t0110nnnnmmmm1011
negc\tRm,Rn\t0110nnnnmmmm1010
sub\tRm,Rn\t0011nnnnmmmm1000
subc\tRm,Rn\t0011nnnnmmmm1010
subv\tRm,Rn\t0011nnnnmmmm1011
and\tRm,Rn\t0010nnnnmmmm1001
and\t#imm,R0\t11001001iiiiiiii
and.b\t#imm,@(R0,GBR)\t11001101iiiiiiii
not\tRm,Rn\t0110nnnnmmmm0111
or\tRm,Rn\t0010nnnnmmmm1011
or\t#imm,R0\t11001011iiiiiiii
or.b\t#imm,@(R0,GBR)\t11001111iiiiiiii
tas.b\t@Rn\t0100nnnn00011011
tst\tRm,Rn\t0010nnnnmmmm1000
tst\t#imm,R0\t11001000iiiiiiii
tst.b\t#imm,@(R0,GBR)\t11001100iiiiiiii
xor\tRm,Rn\t0010nnnnmmmm1010
xor\t#imm,R0\t11001010iiiiiiii
xor.b\t#imm,@(R0,GBR)\t11001110iiiiiiii
rotcl\tRn\t0100nnnn00100100
rotcr\tRn\t0100nnnn00100101
rotl\tRn\t0100nnnn00000100
rotr\tRn\t0100nnnn00000101
shal\tRn\t0100nnnn00100000
shar\tRn\t0100nnnn00100001
shll\tRn\t0100nnnn00000000
shll2\tRn\t0100nnnn00001000
shll8\tRn\t0100nnnn00011000
shll16\tRn\t0100nnnn00101000
shlr\tRn\t0100nnnn00000001
shlr2\tRn\t0100nnnn00001001
shlr8\tRn\t0100nnnn00011001
shlr16\tRn\t0100nnnn00101001
bf\tlabel\t10001011dddddddd
bf/s\tlabel\t10001111dddddddd
bt\tlabel\t10001001dddddddd
bt/s\tlabel\t10001101dddddddd
bra\tlabel\t1010dddddddddddd
braf\tRm\t0000mmmm00100011
bsr\tlabel\t1011dddddddddddd
bsrf\tRm\t0000mmmm00000011
jmp\t@Rm\t0100mmmm00101011
jsr\t@Rm\t0100mmmm00001011
rts\t\t0000000000001011
clrmac\t\t0000000000101000
clrt\t\t0000000000001000
ldc\tRm,SR\t0100mmmm00001110
ldc.l\t@Rm+,SR\t0100mmmm00000111
ldc\tRm,GBR\t0100mmmm00011110
ldc.l\t@Rm+,GBR\t0100mmmm00010111
ldc\tRm,VBR\t0100mmmm00101110
ldc.l\t@Rm+,VBR\t0100mmmm00100111
lds\tRm,MACH\t0100mmmm00001010
lds.l\t@Rm+,MACH\t0100mmmm00000110
lds\tRm,MACL\t0100mmmm00011010
lds.l\t@Rm+,MACL\t0100mmmm00010110
lds\tRm,PR\t0100mmmm00101010
lds.l\t@Rm+,PR\t0100mmmm00100110
nop\t\t0000000000001001
rte\t\t0000000000101011
sett\t\t0000000000011000
sleep\t\t0000000000011011
stc\tSR,Rn\t0000nnnn00000010
stc.l\tSR,@-Rn\t0100nnnn00000011
stc\tGBR,Rn\t0000nnnn00010010
stc.l\tGBR,@-Rn\t0100nnnn00010011
stc\tVBR,Rn\t0000nnnn00100010
stc.l\tVBR,@-Rn\t0100nnnn00100011
sts\tMACH,Rn\t0000nnnn00001010
sts.l\tMACH,@-Rn\t0100nnnn00000010
sts\tMACL,Rn\t0000nnnn00011010
sts.l\tMACL,@-Rn\t0100nnnn00010010
sts\tPR,Rn\t0000nnnn00101010
sts.l\tPR,@-Rn\t0100nnnn00100010
trapa\t#imm\t11000011iiiiiiii
"""

SH3_DELTA = """\
shad\tRm,Rn\t0100nnnnmmmm1100
shld\tRm,Rn\t0100nnnnmmmm1101
clrs\t\t0000000001001000
sets\t\t0000000001011000
ldc\tRm,SSR\t0100mmmm00111110
ldc.l\t@Rm+,SSR\t0100mmmm00110111
stc\tSSR,Rn\t0000nnnn00110010
stc.l\tSSR,@-Rn\t0100nnnn00110011
ldc\tRm,SPC\t0100mmmm01001110
ldc.l\t@Rm+,SPC\t0100mmmm01000111
stc\tSPC,Rn\t0000nnnn01000010
stc.l\tSPC,@-Rn\t0100nnnn01000011
ldc\tRm,Rn_BANK\t0100mmmm1nnn1110
ldc.l\t@Rm+,Rn_BANK\t0100mmmm1nnn0111
stc\tRm_BANK,Rn\t0000nnnn1mmm0010
stc.l\tRm_BANK,@-Rn\t0100nnnn1mmm0011
ldtlb\t\t0000000000111000
pref\t@Rn\t0000nnnn10000011
"""

SH4_DELTA = """\
ldc\tRm,DBR\t0100mmmm11111010
ldc.l\t@Rm+,DBR\t0100mmmm11110110
stc\tDBR,Rn\t0000nnnn11111010
stc.l\tDBR,@-Rn\t0100nnnn11110010
stc\tSGR,Rn\t0000nnnn00111010
stc.l\tSGR,@-Rn\t0100nnnn00110010
movca.l\tR0,@Rn\t0000nnnn11000011
ocbi\t@Rn\t0000nnnn10010011
ocbp\t@Rn\t0000nnnn10100011
ocbwb\t@Rn\t0000nnnn10110011
fmov\tFRm,FRn\t1111nnnnmmmm1100
fmov.s\t@Rm,FRn\t1111nnnnmmmm1000
fmov.s\tFRm,@Rn\t1111nnnnmmmm1010
fmov.s\t@Rm+,FRn\t1111nnnnmmmm1001
fmov.s\tFRm,@-Rn\t1111nnnnmmmm1011
fmov.s\t@(R0,Rm),FRn\t1111nnnnmmmm0110
fmov.s\tFRm,@(R0,Rn)\t1111nnnnmmmm0111
fmov\tDRm,DRn\t1111nnn0mmm01100
fmov\tDRm,XDn\t1111nnn1mmm01100
fmov\tXDm,DRn\t1111nnn0mmm11100
fmov\tXDm,XDn\t1111nnn1mmm11100
fmov.d\t@Rm,DRn\t1111nnn0mmmm1000
fmov.d\t@Rm,XDn\t1111nnn1mmmm1000
fmov.d\tDRm,@Rn\t1111nnnnmmm01010
fmov.d\tXDm,@Rn\t1111nnnnmmm11010
fmov.d\t@Rm+,DRn\t1111nnn0mmmm1001
fmov.d\t@Rm+,XDn\t1111nnn1mmmm1001
fmov.d\tDRm,@-Rn\t1111nnnnmmm01011
fmov.d\tXDm,@-Rn\t1111nnnnmmm11011
fmov.d\t@(R0,Rm),DRn\t1111nnn0mmmm0110
fmov.d\t@(R0,Rm),XDn\t1111nnn1mmmm0110
fmov.d\tDRm,@(R0,Rn)\t1111nnnnmmm00111
fmov.d\tXDm,@(R0,Rn)\t1111nnnnmmm10111
fldi0\tFRn\t1111nnnn10001101
fldi1\tFRn\t1111nnnn10011101
flds\tFRm,FPUL\t1111mmmm00011101
fsts\tFPUL,FRn\t1111nnnn00001101
fabs\tFRn\t1111nnnn01011101
fneg\tFRn\t1111nnnn01001101
fadd\tFRm,FRn\t1111nnnnmmmm0000
fsub\tFRm,FRn\t1111nnnnmmmm0001
fmul\tFRm,FRn\t1111nnnnmmmm0010
fmac\tFR0,FRm,FRn\t1111nnnnmmmm1110
fdiv\tFRm,FRn\t1111nnnnmmmm0011
fsqrt\tFRn\t1111nnnn01101101
fcmp/eq\tFRm,FRn\t1111nnnnmmmm0100
fcmp/gt\tFRm,FRn\t1111nnnnmmmm0101
float\tFPUL,FRn\t1111nnnn00101101
ftrc\tFRm,FPUL\t1111mmmm00111101
fipr\tFVm,FVn\t1111nnmm11101101
ftrv\tXMTRX,FVn\t1111nn0111111101
fabs\tDRn\t1111nnn001011101
fneg\tDRn\t1111nnn001001101
fadd\tDRm,DRn\t1111nnn0mmm00000
fsub\tDRm,DRn\t1111nnn0mmm00001
fmul\tDRm,DRn\t1111nnn0mmm00010
fdiv\tDRm,DRn\t1111nnn0mmm00011
fsqrt\tDRn\t1111nnn001101101
fcmp/eq\tDRm,DRn\t1111nnn0mmm00100
fcmp/gt\tDRm,DRn\t1111nnn0mmm00101
float\tFPUL,DRn\t1111nnn000101101
ftrc\tDRm,FPUL\t1111mmm000111101
fcnvds\tDRm,FPUL\t1111mmm010111101
fcnvsd\tFPUL,DRn\t1111nnn010101101
lds\tRm,FPSCR\t0100mmmm01101010
sts\tFPSCR,Rn\t0000nnnn01101010
lds.l\t@Rm+,FPSCR\t0100mmmm01100110
sts.l\tFPSCR,@-Rn\t0100nnnn01100010
lds\tRm,FPUL\t0100mmmm01011010
sts\tFPUL,Rn\t0000nnnn01011010
lds.l\t@Rm+,FPUL\t0100mmmm01010110
sts.l\tFPUL,@-Rn\t0100nnnn01010010
frchg\t\t1111101111111101
fschg\t\t1111001111111101
"""

SH2A_NEW = """\
movi20\t#imm20,Rn\t0000nnnniiii0000iiiiiiiiiiiiiiii
movi20s\t#imm20,Rn\t0000nnnniiii0001iiiiiiiiiiiiiiii
mov.b\t@-Rm,R0\t0100mmmm11001011
mov.w\t@-Rm,R0\t0100mmmm11011011
mov.l\t@-Rm,R0\t0100mmmm11101011
mov.b\tR0,@Rn+\t0100nnnn10001011
mov.w\tR0,@Rn+\t0100nnnn10011011
mov.l\tR0,@Rn+\t0100nnnn10101011
mov.b\t@(disp12,Rm),Rn\t0011nnnnmmmm00010100dddddddddddd
movu.b\t@(disp12,Rm),Rn\t0011nnnnmmmm00011000dddddddddddd
mov.w\t@(disp12,Rm),Rn\t0011nnnnmmmm00010101dddddddddddd
movu.w\t@(disp12,Rm),Rn\t0011nnnnmmmm00011001dddddddddddd
mov.l\t@(disp12,Rm),Rn\t0011nnnnmmmm00010110dddddddddddd
mov.b\tRm,@(disp12,Rn)\t0011nnnnmmmm00010000dddddddddddd
mov.w\tRm,@(disp12,Rn)\t0011nnnnmmmm00010001dddddddddddd
mov.l\tRm,@(disp12,Rn)\t0011nnnnmmmm00010010dddddddddddd
movml.l\tRm,@-R15\t0100mmmm11110001
movml.l\t@R15+,Rn\t0100nnnn11110101
movmu.l\tRm,@-R15\t0100mmmm11110000
movmu.l\t@R15+,Rn\t0100nnnn11110100
movrt\tRn\t0000nnnn00111001
nott\t\t0000000001101000
band.b\t#imm3,@(disp12,Rn)\t0011nnnn0iii10010100dddddddddddd
bandnot.b\t#imm3,@(disp12,Rn)\t0011nnnn0iii10011100dddddddddddd
bclr.b\t#imm3,@(disp12,Rn)\t0011nnnn0iii10010000dddddddddddd
bclr\t#imm3,Rn\t10000110nnnn0iii
bld.b\t#imm3,@(disp12,Rn)\t0011nnnn0iii10010011dddddddddddd
bld\t#imm3,Rn\t10000111nnnn1iii
bldnot.b\t#imm3,@(disp12,Rn)\t0011nnnn0iii10011011dddddddddddd
bor.b\t#imm3,@(disp12,Rn)\t0011nnnn0iii10010101dddddddddddd
bornot.b\t#imm3,@(disp12,Rn)\t0011nnnn0iii10011101dddddddddddd
bset.b\t#imm3,@(disp12,Rn)\t0011nnnn0iii10010001dddddddddddd
bset\t#imm3,Rn\t10000110nnnn1iii
bst.b\t#imm3,@(disp12,Rn)\t0011nnnn0iii10010010dddddddddddd
bst\t#imm3,Rn\t10000111nnnn0iii
bxor.b\t#imm3,@(disp12,Rn)\t0011nnnn0iii10010110dddddddddddd
clips.b\tRn\t0100nnnn10010001
clips.w\tRn\t0100nnnn10010101
clipu.b\tRn\t0100nnnn10000001
clipu.w\tRn\t0100nnnn10000101
divs\tR0,Rn\t0100nnnn10010100
divu\tR0,Rn\t0100nnnn10000100
mulr\tR0,Rn\t0100nnnn10000000
jsr/n\t@Rm\t0100mmmm01001011
jsr/n\t@@(disp8,TBR)\t10000011dddddddd
rts/n\t\t0000000001101011
rtv/n\tRm\t0000mmmm01111011
ldbank\t@Rm,R0\t0100mmmm11100101
ldc\tRm,TBR\t0100mmmm01001010
resbank\t\t0000000001011011
stbank\tR0,@Rn\t0100nnnn11100001
stc\tTBR,Rn\t0000nnnn01001010
fmov.s\t@(disp12,Rm),FRn\t0011nnnnmmmm00010111dddddddddddd
fmov.s\tFRm,@(disp12,Rn)\t0011nnnnmmmm00010011dddddddddddd
fmov.d\t@(disp12,Rm),DRn\t0011nnn0mmmm00010111dddddddddddd
fmov.d\tDRm,@(disp12,Rn)\t0011nnnnmmm000010011dddddddddddd
"""


def run(command, input_text=None):
	result = subprocess.run(command, input=input_text, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	if result.returncode:
		raise RuntimeError(f"{' '.join(command)} failed:\n{result.stderr}")
	return result.stdout


def run_failure(command, input_text, diagnostic=None):
	result = subprocess.run(command, input=input_text, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	assert result.returncode, (command, input_text, result.stdout, result.stderr)
	if diagnostic:
		assert diagnostic in result.stderr, (command, input_text, result.stderr)
	return result.stderr


def instantiate(mnemonic, operands):
	text = operands
	for old, new in (
		("#imm20", "#1280" if mnemonic == "movi20s" else "#5"),
		("#imm3", "#5"),
		("@@(disp8,TBR)", "@@(0,tbr)"),
		("Rm_BANK", "r2_bank"),
		("Rn_BANK", "r3_bank"),
		("XMTRX", "xmtrx"),
		("FVm", "fv8"),
		("FVn", "fv12"),
		("DRm", "dr4"),
		("DRn", "dr6"),
		("XDm", "xd4"),
		("XDn", "xd6"),
		("FRm", "fr2"),
		("FRn", "fr3"),
		("@(disp,PC)", "@(0,pc)"),
		("@(disp,GBR)", "@(0,gbr)"),
		("@(disp12,Rm)", "@(64,r2)" if mnemonic == "mov.l" else "@(0,r2)"),
		("@(disp12,Rn)", "@(64,r3)" if mnemonic == "mov.l" else "@(0,r3)"),
		("@(disp,Rm)", "@(0,r2)"),
		("@(disp,Rn)", "@(0,r3)"),
		("@(R0,Rm)", "@(r0,r2)"),
		("@(R0,Rn)", "@(r0,r3)"),
		("@Rm+", "@r2+"),
		("@Rn+", "@r3+"),
		("@R15+", "@r15+"),
		("@-R15", "@-r15"),
		("@-Rn", "@-r3"),
		("@Rm", "@r2"),
		("@Rn", "@r3"),
		("#imm", "#5"),
	):
		text = text.replace(old, new)
	text = re.sub(r"\bRm\b", "r2", text)
	text = re.sub(r"\bRn\b", "r3", text)
	for special in ("SR", "SSR", "SPC", "SGR", "DBR", "GBR", "VBR", "TBR", "MACH", "MACL", "PR", "PC", "R0", "R15", "FR0", "FPUL", "FPSCR"):
		text = re.sub(rf"\b{special}\b", special.lower(), text)
	if text == "label":
		return [f"{mnemonic} 1f", "nop", "1:"]
	return [mnemonic + ((" " + text) if text else "")]


def expected_word(pattern):
	bits = pattern
	for name, value in (("n", 3), ("m", 2), ("i", 5), ("d", 0)):
		count = bits.count(name)
		if count:
			for bit in f"{value:0{count}b}":
				bits = bits.replace(name, bit, 1)
	assert not set(bits) - {"0", "1"}, pattern
	return int(bits, 2)


def canonical_disassembly_mnemonic(mnemonic):
	if mnemonic in ("fmov.s", "fmov.d"):
		return "fmov"
	return mnemonic


def extract_text(object_path, section_path):
	run([LLVM_OBJCOPY, "--dump-section", f".text={section_path}", str(object_path)])
	return section_path.read_bytes()


def assemble(source, triple, cpu, stem, features=None):
	object_path = TEMPORARY.with_name(TEMPORARY.name + f"-{stem}.o")
	section_path = TEMPORARY.with_name(TEMPORARY.name + f"-{stem}.text")
	command = [LLVM_MC, "-triple=" + triple, "-mcpu=" + cpu]
	if features:
		command.append("-mattr=" + features)
	command.extend(["-filetype=obj", "-o", str(object_path)])
	run(command, source)
	return extract_text(object_path, section_path)


def disassemble(raw, triple, cpu, features=None):
	input_text = " ".join(f"0x{byte:02x}" for byte in raw) + "\n"
	command = [LLVM_MC, "-triple=" + triple, "-mcpu=" + cpu]
	if features:
		command.append("-mattr=" + features)
	command.extend(["-disassemble", "-show-encoding"])
	output = run(command, input_text)
	lines = [line.split("!", 1)[0].strip() for line in output.splitlines() if "encoding:" in line]
	encodings = []
	for line in output.splitlines():
		if "encoding:" not in line:
			continue
		match = re.search(r"encoding: \[([^]]+)\]", line)
		assert match, line
		encodings.extend(int(byte.strip(), 16) for byte in match.group(1).split(","))
	assert bytes(encodings) == raw, (len(raw), len(encodings), bytes(encodings).hex(), raw.hex(), output)
	return lines


base_rows = [line.split("\t") for line in ORACLE.splitlines()]
delta_rows = [line.split("\t") for line in SH3_DELTA.splitlines()]
sh4_delta_rows = [line.split("\t") for line in SH4_DELTA.splitlines()]
sh2a_new_rows = [line.split("\t") for line in SH2A_NEW.splitlines()]
assert len(base_rows) == 142
assert len(delta_rows) == 18
assert len(sh4_delta_rows) == 74
assert len(sh2a_new_rows) == 56
sh2_only_forms = {
	("dmuls.l", "Rm,Rn"),
	("dmulu.l", "Rm,Rn"),
	("dt", "Rn"),
	("mac.l", "@Rm+,@Rn+"),
	("mul.l", "Rm,Rn"),
	("bf/s", "label"),
	("bt/s", "label"),
	("braf", "Rm"),
	("bsrf", "Rm"),
}
sh1_rows = [row for row in base_rows if tuple(row[:2]) not in sh2_only_forms]
sh2_delta_rows = [row for row in base_rows if tuple(row[:2]) in sh2_only_forms]
assert len(sh1_rows) == 133
assert len(sh2_delta_rows) == 9
sh4_integer_rows = sh4_delta_rows[:10]
sh4_fpu_rows = sh4_delta_rows[10:]
sh4_single_only_fpu_rows = (
	sh4_delta_rows[10:17] + sh4_delta_rows[33:49] + sh4_delta_rows[64:72])
e_common_fpu_rows = [row for row in sh4_single_only_fpu_rows if row[0] != "fsqrt"]
assert len(sh4_integer_rows) == 10
assert len(sh4_fpu_rows) == 64
assert len(sh4_single_only_fpu_rows) == 31
assert len(e_common_fpu_rows) == 30
sh2a_shared_integer_rows = [row for row in delta_rows if row[0] in {"shad", "shld", "pref"}]
sh2a_shared_fpu_rows = [row for row in sh4_fpu_rows
	if "XD" not in row[1] and "FV" not in row[1] and "XMTRX" not in row[1]
	and row[0] != "frchg"]
sh2a_integer_rows = base_rows + sh2a_shared_integer_rows + sh2a_new_rows[:-4]
sh2a_full_rows = sh2a_integer_rows + sh2a_shared_fpu_rows + sh2a_new_rows[-4:]
sh2a_single_only_fpu_rows = sh4_single_only_fpu_rows + sh2a_new_rows[-4:-2]
sh2a_single_only_rows = sh2a_integer_rows + sh2a_single_only_fpu_rows
assert len(sh2a_shared_integer_rows) == 3
assert len(sh2a_shared_fpu_rows) == 52
assert len(sh2a_integer_rows) == 197
assert len(sh2a_full_rows) == 253
assert len(sh2a_single_only_rows) == 230

for cpu, rows in (("sh1", sh1_rows), ("sh2", base_rows),
				  ("sh2e", base_rows + e_common_fpu_rows),
				  ("sh3", base_rows + delta_rows),
				  ("sh3e", base_rows + delta_rows + sh4_single_only_fpu_rows),
				  ("sh4", base_rows + delta_rows + sh4_delta_rows)):
	source_lines = []
	expected_words = []
	expected_mnemonics = []
	for mnemonic, operands, pattern in rows:
		instantiated = instantiate(mnemonic, operands)
		source_lines.extend(instantiated)
		expected_words.append(expected_word(pattern))
		expected_mnemonics.append(canonical_disassembly_mnemonic(mnemonic))
		if operands == "label":
			expected_words.append(0x0009)
			expected_mnemonics.append("nop")
	source = "\n".join(source_lines) + "\n"

	for triple, byteorder, stem in (("sh-unknown-elf", "big", "be"), ("shle-unknown-elf", "little", "le")):
		expected = b"".join(word.to_bytes(2, byteorder) for word in expected_words)
		assembled = assemble(source, triple, cpu, f"{cpu}-{stem}")
		assert assembled == expected, (cpu, stem, assembled.hex(), expected.hex())
		printed = disassemble(expected, triple, cpu)
		printed_mnemonics = [line.split(None, 1)[0] for line in printed]
		assert printed_mnemonics == expected_mnemonics, (
			cpu, stem,
			[(index, expected, actual) for index, (expected, actual) in
			 enumerate(zip(expected_mnemonics, printed_mnemonics))
			 if expected != actual])
		roundtrip = assemble("\n".join(printed) + "\n", triple, cpu, f"{cpu}-{stem}-roundtrip")
		assert roundtrip == expected, (cpu, stem, roundtrip.hex(), expected.hex())

profile_configs = (
	("sh4-nofpu", "-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode",
	 base_rows + delta_rows + sh4_integer_rows, sh4_fpu_rows),
	("sh4-single", "+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,+fpu-register-bank,+fpu-vector,+fpu-single-mode",
	 base_rows + delta_rows + sh4_delta_rows, ()),
	("sh4-single-only", "+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode",
	 base_rows + delta_rows + sh4_integer_rows + sh4_single_only_fpu_rows,
	 [row for row in sh4_fpu_rows if row not in sh4_single_only_fpu_rows]),
)
assert [len(rows) for _, _, rows, _ in profile_configs] == [170, 234, 201]

e_cpu_configs = (
	("sh2e", delta_rows + [row for row in sh4_delta_rows if row not in e_common_fpu_rows]),
	("sh3e", [row for row in sh4_delta_rows if row not in sh4_single_only_fpu_rows]),
)
assert [172, 191] == [len(base_rows + e_common_fpu_rows),
	len(base_rows + delta_rows + sh4_single_only_fpu_rows)]

for triple in ("sh-unknown-elf", "shle-unknown-elf"):
	for mnemonic, operands, pattern in sh2_delta_rows + delta_rows + sh4_delta_rows:
		unavailable_source = "\n".join(instantiate(mnemonic, operands)) + "\n"
		run_failure([LLVM_MC, "-triple=" + triple, "-mcpu=sh1",
			"-filetype=null"], unavailable_source)

	byteorder = "big" if triple.startswith("sh-") else "little"
	raw = b"".join(expected_word(pattern).to_bytes(2, byteorder)
		for mnemonic, operands, pattern in sh2_delta_rows)
	input_text = " ".join(f"0x{byte:02x}" for byte in raw) + "\n"
	result = subprocess.run([LLVM_MC, "-triple=" + triple, "-mcpu=sh1",
		"-disassemble", "-show-encoding"], input=input_text, text=True,
		stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	assert result.returncode == 0, (triple, result.stderr)
	assert "encoding:" not in result.stdout, (triple, result.stdout)
	assert result.stderr.count("warning: invalid instruction encoding") == 9, (
		triple, result.stderr)

for cpu, unavailable_rows in e_cpu_configs:
	for triple in ("sh-unknown-elf", "shle-unknown-elf"):
		for mnemonic, operands, pattern in unavailable_rows:
			unavailable_source = "\n".join(instantiate(mnemonic, operands)) + "\n"
			run_failure([LLVM_MC, "-triple=" + triple, "-mcpu=" + cpu,
				"-filetype=null"], unavailable_source,
				"instruction requires an unavailable feature")

for profile, features, rows, unavailable_rows in profile_configs:
	source_lines = []
	expected_words = []
	expected_mnemonics = []
	for mnemonic, operands, pattern in rows:
		instantiated = instantiate(mnemonic, operands)
		source_lines.extend(instantiated)
		expected_words.append(expected_word(pattern))
		expected_mnemonics.append(canonical_disassembly_mnemonic(mnemonic))
		if operands == "label":
			expected_words.append(0x0009)
			expected_mnemonics.append("nop")
	source = "\n".join(source_lines) + "\n"
	for triple, byteorder, stem in (("sh-unknown-elf", "big", "be"), ("shle-unknown-elf", "little", "le")):
		expected = b"".join(word.to_bytes(2, byteorder) for word in expected_words)
		assembled = assemble(source, triple, "sh4", f"{profile}-{stem}", features)
		assert assembled == expected, (profile, stem, assembled.hex(), expected.hex())
		printed = disassemble(expected, triple, "sh4", features)
		printed_mnemonics = [line.split(None, 1)[0] for line in printed]
		assert printed_mnemonics == expected_mnemonics, (
			profile, stem,
			[(index, wanted, actual) for index, (wanted, actual) in
			 enumerate(zip(expected_mnemonics, printed_mnemonics))
			 if wanted != actual])
		roundtrip = assemble("\n".join(printed) + "\n", triple, "sh4",
			f"{profile}-{stem}-roundtrip", features)
		assert roundtrip == expected, (profile, stem, roundtrip.hex(), expected.hex())
		for mnemonic, operands, pattern in unavailable_rows:
			unavailable_source = "\n".join(instantiate(mnemonic, operands)) + "\n"
			run_failure([LLVM_MC, "-triple=" + triple, "-mcpu=sh4",
				"-mattr=" + features, "-filetype=null"], unavailable_source,
				"instruction requires an unavailable feature")

sh2a_profile_configs = (
	("sh2a-full", None, sh2a_full_rows, (), False),
	("sh2a-nofpu", "-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode",
		sh2a_integer_rows, sh2a_shared_fpu_rows + sh2a_new_rows[-4:], False),
	("sh2a-single", "+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode",
		sh2a_full_rows, (), True),
	("sh2a-single-only", "+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode",
		sh2a_single_only_rows,
		[row for row in sh2a_shared_fpu_rows + sh2a_new_rows[-4:]
		 if row not in sh2a_single_only_fpu_rows], True),
)
assert [len(rows) for _, _, rows, _, _ in sh2a_profile_configs] == [253, 197, 253, 230]

def sh2a_expected_bytes(pattern, byteorder):
	value = expected_word(pattern)
	if len(pattern) == 16:
		return value.to_bytes(2, byteorder)
	assert len(pattern) == 32
	return (value >> 16).to_bytes(2, byteorder) + (value & 0xffff).to_bytes(2, byteorder)

def sh2a_disassembly_mnemonic(mnemonic, operands, pattern, single_mode):
	if len(pattern) == 32 and mnemonic in ("fmov.s", "fmov.d"):
		if single_mode or (mnemonic == "fmov.s" and operands.startswith("@(disp12")):
			return "fmov.s"
		return "fmov.d"
	return canonical_disassembly_mnemonic(mnemonic)

for profile, features, rows, unavailable_rows, single_mode in sh2a_profile_configs:
	source_lines = []
	expected_patterns = []
	expected_mnemonics = []
	for mnemonic, operands, pattern in rows:
		instantiated = [line.replace("@(0,pc)", "@(4,pc)")
			for line in instantiate(mnemonic, operands)]
		source_lines.extend(instantiated)
		effective_pattern = pattern
		if mnemonic == "mov.l" and "disp12" in operands:
			effective_pattern = pattern.replace("dddddddddddd", "000000010000")
		expected_patterns.append(effective_pattern)
		expected_mnemonics.append(sh2a_disassembly_mnemonic(
			mnemonic, operands, effective_pattern, single_mode))
		if operands == "label":
			expected_patterns.append("0000000000001001")
			expected_mnemonics.append("nop")
	source = "\n".join(source_lines) + "\n"
	for triple, byteorder, stem in (("sh-unknown-elf", "big", "be"), ("shle-unknown-elf", "little", "le")):
		expected = b"".join(sh2a_expected_bytes(pattern, byteorder)
			for pattern in expected_patterns)
		assembled = assemble(source, triple, "sh2a", f"{profile}-{stem}", features)
		assert assembled == expected, (profile, stem, assembled.hex(), expected.hex())
		printed = disassemble(expected, triple, "sh2a", features)
		printed_mnemonics = [line.split(None, 1)[0] for line in printed]
		assert printed_mnemonics == expected_mnemonics, (
			profile, stem,
			[(index, wanted, actual) for index, (wanted, actual) in
			 enumerate(zip(expected_mnemonics, printed_mnemonics))
			 if wanted != actual])
		roundtrip = assemble("\n".join(printed) + "\n", triple, "sh2a",
			f"{profile}-{stem}-roundtrip", features)
		assert roundtrip == expected, (profile, stem, roundtrip.hex(), expected.hex())
		for mnemonic, operands, pattern in unavailable_rows:
			unavailable_source = "\n".join(instantiate(mnemonic, operands)) + "\n"
			command = [LLVM_MC, "-triple=" + triple, "-mcpu=sh2a"]
			if features:
				command.append("-mattr=" + features)
			command.append("-filetype=null")
			run_failure(command, unavailable_source,
				"instruction requires an unavailable feature")

for triple in ("sh-unknown-elf", "shle-unknown-elf"):
	for mnemonic, operands, pattern in sh2a_new_rows:
		unavailable_source = "\n".join(instantiate(mnemonic, operands)) + "\n"
		run_failure([LLVM_MC, "-triple=" + triple, "-mcpu=sh2",
			"-filetype=null"], unavailable_source)

boundary_source = []
boundary_words = []
for source_reg in (0, 15):
	for dest_reg in (0, 15):
		for mnemonic, suffix in (("shad", 0xc), ("shld", 0xd)):
			boundary_source.append(f"{mnemonic} r{source_reg},r{dest_reg}")
			boundary_words.append(0x4000 | (dest_reg << 8) | (source_reg << 4) | suffix)
for bank in range(8):
	base = 0 if bank % 2 == 0 else 15
	boundary_source.extend((
		f"ldc r{base},r{bank}_bank",
		f"ldc.l @r{base}+,r{bank}_bank",
		f"stc r{bank}_bank,r{base}",
		f"stc.l r{bank}_bank,@-r{base}",
	))
	boundary_words.extend((
		0x408e | (base << 8) | (bank << 4),
		0x4087 | (base << 8) | (bank << 4),
		0x0082 | (base << 8) | (bank << 4),
		0x4083 | (base << 8) | (bank << 4),
	))
boundary_source_text = "\n".join(boundary_source) + "\n"

for triple, byteorder, stem in (("sh-unknown-elf", "big", "be"), ("shle-unknown-elf", "little", "le")):
	expected = b"".join(word.to_bytes(2, byteorder) for word in boundary_words)
	assembled = assemble(boundary_source_text, triple, "sh3", f"sh3-boundaries-{stem}")
	assert assembled == expected, (stem, assembled.hex(), expected.hex())
	printed = disassemble(expected, triple, "sh3")
	roundtrip = assemble("\n".join(printed) + "\n", triple, "sh3", f"sh3-boundaries-{stem}-roundtrip")
	assert roundtrip == expected, (stem, roundtrip.hex(), expected.hex())

sh4_boundary_source = """\
fmov fr0,fr15
fmov fr15,fr0
fmov dr0,dr14
fmov dr14,dr0
fmov dr0,xd14
fmov xd14,dr0
fmov xd0,xd14
fmov xd14,xd0
fipr fv0,fv12
fipr fv12,fv0
ftrv xmtrx,fv0
ftrv xmtrx,fv12
"""
sh4_boundary_words = [
	0xff0c, 0xf0fc, 0xfe0c, 0xf0ec, 0xff0c, 0xf0fc,
	0xff1c, 0xf1fc, 0xfced, 0xf3ed, 0xf1fd, 0xfdfd,
]

for triple, byteorder, stem in (("sh-unknown-elf", "big", "be"), ("shle-unknown-elf", "little", "le")):
	expected = b"".join(word.to_bytes(2, byteorder) for word in sh4_boundary_words)
	assembled = assemble(sh4_boundary_source, triple, "sh4", f"sh4-boundaries-{stem}")
	assert assembled == expected, (stem, assembled.hex(), expected.hex())
	printed = disassemble(expected, triple, "sh4")
	roundtrip = assemble("\n".join(printed) + "\n", triple, "sh4", f"sh4-boundaries-{stem}-roundtrip")
	assert roundtrip == expected, (stem, roundtrip.hex(), expected.hex())

for triple in ("sh-unknown-elf", "shle-unknown-elf"):
	for cpu in ("sh2", "sh3"):
		for mnemonic, operands, pattern in sh4_delta_rows:
			source = "\n".join(instantiate(mnemonic, operands)) + "\n"
			run_failure([LLVM_MC, "-triple=" + triple, "-mcpu=" + cpu,
				"-filetype=null"], source, "instruction requires an unavailable feature")

		byteorder = "big" if triple.startswith("sh-") else "little"
		raw = b"".join(expected_word(pattern).to_bytes(2, byteorder)
			for mnemonic, operands, pattern in sh4_delta_rows)
		input_text = " ".join(f"0x{byte:02x}" for byte in raw) + "\n"
		result = subprocess.run([LLVM_MC, "-triple=" + triple, "-mcpu=" + cpu,
			"-disassemble", "-show-encoding"], input=input_text, text=True,
			stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		assert result.returncode == 0, (triple, cpu, result.stderr)
		assert "encoding:" not in result.stdout, (triple, cpu, result.stdout)
		assert result.stderr.count("warning: invalid instruction encoding") == 74, (
			triple, cpu, result.stderr)

invalid_sh4_operands = (
	"fadd dr1,dr2",
	"fadd xd3,xd4",
	"fipr fv2,fv4",
	"fadd dr2,fr4",
	"fadd fr2,dr4",
	"fmov xf2,fr3",
	"fmov fr2,xd4",
	"fmov.s dr2,@r3",
	"fmov.d fr2,@r3",
	"flds fpul,fr2",
	"fsts fr2,fpul",
	"fmac fr1,fr2,fr3",
	"ftrv fv0,fv4",
	"movca.l r1,@r2",
)
later_variant_instructions = (
	"fsrra fr0",
	"fsca fpul,dr0",
	"fpchg",
	"synco",
	"movli.l @r1,r0",
	"padd r1,r2",
)
for triple in ("sh-unknown-elf", "shle-unknown-elf"):
	for source in invalid_sh4_operands + later_variant_instructions:
		run_failure([LLVM_MC, "-triple=" + triple, "-mcpu=sh4",
			"-filetype=null"], source + "\n")
