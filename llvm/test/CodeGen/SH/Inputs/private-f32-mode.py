#!/usr/bin/env python3
"""Legacy private-mode witnesses now retain ordinary ABI mode boundaries."""
import argparse
import pathlib
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--llc', required=True)
parser.add_argument('--tmp', required=True)
args = parser.parse_args()

base = r"""
define internal fastcc float @leaf(float %a, float %b) #0 {
	%v = fadd float %a, %b
	ret float %v
}
define i32 @caller(float %a, float %b) #1 {
	%v = call fastcc float @leaf(float %a, float %b)
	%bits = bitcast float %v to i32
	ret i32 %bits
}
attributes #0 = { noinline nounwind memory(none) "no-trapping-math"="true" }
attributes #1 = { noinline nounwind "no-trapping-math"="true" }
"""

cases = [('closed', base, True, False), ('fpclass-value-fact', base.replace('float %a, float %b) #0', 'float nofpclass(nan inf) %a, float %b) #0'), True, False), ('inreg-value-ABI', base.replace('float %a, float %b) #0', 'float inreg %a, float %b) #0'), False, False)]
cases += [('two-callers', base + r"""
define i32 @other(float %a, float %b) #1 {
	%v = call fastcc float @leaf(float %b, float %a)
	%bits = bitcast float %v to i32
	ret i32 %bits
}
""", True, False)]
cases += [('freeze', base.replace('%v = fadd float %a, %b', '%f = freeze float %a\n\t%v = fadd float %f, %b'), True, False)]
for attr in ['noipa', 'optnone', 'optsize', 'minsize', 'strictfp', 'sanitize_address', 'sspstrong']:
	cases.append((attr, base.replace('noinline nounwind memory(none)', 'noinline nounwind memory(none) ' + attr), False, False))
for scope in ['leaf', 'caller']:
	needle = 'define internal fastcc float @leaf(float %a, float %b) #0 {' if scope == 'leaf' else 'define i32 @caller(float %a, float %b) #1 {'
	cases.append((scope+'-asm', base.replace(needle, needle+'\n\tcall void asm sideeffect "nop", ""()'), False, False))
cases += [
	('module-asm', 'module asm "nop"\n'+base, False, False),
	('trapping', base.replace('"no-trapping-math"="true"', '"no-trapping-math"="false"'), False, False),
	('missing-trap-contract', base.replace('"no-trapping-math"="true"', ''), False, False),
	('forged-input-marker', base.replace('noinline nounwind memory(none)', 'noinline nounwind memory(none) "sh-private-f32-mode"="true"'), False, False),
	('escape', '@address = global i32 0\n'+base.replace('%bits = bitcast float %v to i32', 'store i32 ptrtoint (ptr @leaf to i32), ptr @address\n\t%bits = bitcast float %v to i32'), False, False),
	('public-C', base.replace('internal fastcc', 'internal').replace('call fastcc', 'call'), False, False),
	('memory', '@source = global float 1.0\n'+base.replace('%v = fadd float %a, %b', '%x = load float, ptr @source\n\t%v = fadd float %a, %x').replace('memory(none)', 'memory(read)'), False, False),
	('f64-body', base.replace('%v = fadd float %a, %b', '%d = fpext float %a to double\n\t%q = fadd double %d, 1.0\n\t%v = fptrunc double %q to float'), False, False),
	('unknown-helper', 'declare float @helper(float)\n'+base.replace('%v = fadd float %a, %b', '%v = call float @helper(float %a)').replace('memory(none)', ''), False, False),
	('recursion', base.replace('%v = fadd float %a, %b', '%v = call fastcc float @leaf(float %a, float %b)'), False, False),
	('native-but-unadmitted-divide', base.replace('fadd float', 'fdiv float'), False, False),
	('actual-tail', base.replace('define i32 @caller', 'define float @caller').replace('%v = call fastcc', '%v = tail call fastcc').replace('%bits = bitcast float %v to i32\n\tret i32 %bits', 'ret float %v'), False, False),
	('musttail', base.replace('define i32 @caller', 'define float @caller').replace('%v = call fastcc', '%v = musttail call fastcc').replace('%bits = bitcast float %v to i32\n\tret i32 %bits', 'ret float %v'), False, True),
	('hidden-fastcc', base.replace('internal fastcc', 'hidden fastcc'), False, True),
	('wrong-convention', base.replace('call fastcc', 'call'), False, True),
	('wrong-prototype', base.replace('call fastcc float @leaf(float %a, float %b)', 'call fastcc float @leaf(float %a)'), False, True),
	('varargs', base.replace('@leaf(float %a, float %b)', '@leaf(float %a, float %b, ...)').replace('call fastcc float @leaf', 'call fastcc float (float, float, ...) @leaf'), False, True),
	('incompatible-features', base.replace('attributes #1 = {', 'attributes #1 = { "target-features"="-fsqrt"'), False, False),
]
cases += [
	('public-interposable-C', base.replace('internal fastcc', '').replace('call fastcc', 'call'), False, False),
	('alias-escape', '@alias = internal alias float (float, float), ptr @leaf\n'+base, False, None),
	('llvm-used', '@llvm.used = appending global [1 x ptr] [ptr @leaf], section "llvm.metadata"\n'+base, False, None),
	('callback-table', '@callbacks = global [1 x ptr] [ptr @leaf]\n'+base, False, None),
	('vector-body', base.replace('%v = fadd float %a, %b', '%q = insertelement <2 x float> zeroinitializer, float %a, i32 0\n\t%v = extractelement <2 x float> %q, i32 0'), False, False),
	('unknown-intrinsic', 'declare float @llvm.sqrt.f32(float)\n'+base.replace('%v = fadd float %a, %b', '%v = call float @llvm.sqrt.f32(float %a)'), False, False),
	('caller-register-observation', 'declare i32 @llvm.read_register.i32(metadata)\n'+base.replace('%bits = bitcast float %v to i32', '%fpscr = call i32 @llvm.read_register.i32(metadata !0)\n\t%bits = bitcast float %v to i32')+'\n!0 = !{!"fpscr"}\n', False, None),
	('caller-opaque-intrinsic', 'declare float @llvm.sqrt.f32(float)\n'+base.replace('%bits = bitcast float %v to i32', '%q = call float @llvm.sqrt.f32(float %v)\n\t%bits = bitcast float %q to i32'), False, False),
	('mixed-ordinary-leaf', base+"\ndefine float @ordinary(float %x) #1 { ret float %x }\n", True, False),
]
count = 0
for endian in ['shle', 'sh']:
	for pm in ['0', '1']:
		for name, text, legacy_private, reject in cases:
			path = pathlib.Path(args.tmp + '.ll')
			path.write_text(text)
			command = [args.llc, '-mtriple='+endian+'-unknown-none-elf', '-mcpu=sh4', '-target-abi=gnu-hard-float', '-O3', '-enable-new-pm='+pm, '-verify-machineinstrs', '-stop-after=sh-fpu-state', str(path), '-o', '-']
			p = subprocess.run(command, capture_output=True)
			assert reject is None or (p.returncode != 0) == reject, (name, endian, pm, p.stderr.decode())
			if p.returncode == 0:
				out = p.stdout.decode()
				assert 'JSR_PRIVATE_F32' not in out, (name, endian, pm, out)
				if name == 'closed':
					leaf = out.split('name:            leaf', 1)[1].split('body:', 1)[1].split('...', 1)[0]
					assert 'SET_FPSCR_MODE' in leaf or 'TRANSITION_FPSCR_MODE' in leaf, leaf
				if name == 'closed':
					mir = pathlib.Path(args.tmp + '.mir')
					# A state-pass-only fixture avoids SH's unsupported target CP parser.
					# Its placeholder callee address is never emitted or executed.
					out = re.sub(r'constants:\n(?:  .*\n)+(?=machineFunctionInfo:)', 'constants: []\n', out)
					out = re.sub(r'MOVL_load_pc_island %const\.0, -1 :: .*', 'MOVri 0', out)
					mir.write_text(out)
					replay = [args.llc, '-mtriple='+endian+'-unknown-none-elf', '-mcpu=sh4', '-target-abi=gnu-hard-float', '-O3', '-run-pass=sh-fpu-state', '-verify-machineinstrs', str(mir), '-o', '-']
					q = subprocess.run(replay, capture_output=True)
					assert q.returncode == 0 and b'JSR_PRIVATE_F32' not in q.stdout, q.stderr
					assert q.stdout.count(b'SET_FPSCR_MODE') == p.stdout.count(b'SET_FPSCR_MODE'), q.stdout.decode()
					mir.write_text(out.replace('attributes #0 = { noinline', 'attributes #0 = { noinline "noipa"', 1))
					q = subprocess.run(replay, capture_output=True)
					assert q.returncode == 0 and b'JSR_PRIVATE_F32' not in q.stdout, q.stderr
					mir.unlink()
					count += 2
			count += 1
		# Independently supported target profiles and PIC retain ordinary boundaries.
		path.write_text(base)
		for cpu, abi, extra in [('sh2', 'gnu', []), ('sh3', 'gnu', []), ('sh4a', 'gnu-hard-float', []), ('sh4', 'gnu', []), ('sh4', 'gnu-hard-float-single', ['-mattr=+fpu-single-mode']), ('sh4', 'gnu-hard-float', ['-relocation-model=pic'])]:
			p = subprocess.run([args.llc, '-mtriple='+endian+'-unknown-none-elf', '-mcpu='+cpu, '-target-abi='+abi, '-O3', '-enable-new-pm='+pm, '-verify-machineinstrs', '-stop-after=sh-fpu-state', *extra, str(path), '-o', '-'], capture_output=True)
			assert p.returncode == 0 and b'JSR_PRIVATE_F32' not in p.stdout, (cpu, abi, p.stderr)
			count += 1
		# Protected unoptimized route declines, even on the same valid source.
		path.write_text(base)
		p = subprocess.run([args.llc, '-mtriple='+endian+'-unknown-none-elf', '-mcpu=sh4', '-target-abi=gnu-hard-float', '-O0', '-enable-new-pm='+pm, '-verify-machineinstrs', '-stop-after=sh-fpu-state', str(path), '-o', '-'], capture_output=True)
		assert p.returncode == 0 and b'JSR_PRIVATE_F32' not in p.stdout, p.stderr
		count += 1
path.unlink()
print('PASS', count, 'ordinary ABI mode controls (legacy private cases retained); both endian and codegen PM routes')
