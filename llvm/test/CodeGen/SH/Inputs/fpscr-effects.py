import json
import re
import sys


# Pseudo, explicit operands, live inputs, status-producing replacement.
EXPANSIONS = {
	'FIPR_VALUE': ('$fr0, $fv4 = FIPR_VALUE $fv4, $fv8', '$fv4, $fv8', 'FIPR'),
	'SET_FPSCR_FIELDS': ('$r1, $r2 = SET_FPSCR_FIELDS $r4', '$r4', 'LDS_FPSCR'),
	'FSCA_VALUE': ('$dr0 = FSCA_VALUE $r4', '$r4', 'FSCA'),
	'FTRV_MEM': ('$fv4, $r0, $r1, $r2 = FTRV_MEM $fv4, $r4, $r5', '$fv4, $r4, $r5, $xmtrx', 'FTRV'),
	'FMAC_FR_PSEUDO': ('$fr2 = FMAC_FR_PSEUDO $fr2, $fr4, $fr5', '$fr2, $fr4, $fr5', 'FMAC_FR'),
	'I32_TO_F32_PSEUDO': ('$fr0 = I32_TO_F32_PSEUDO $r4', '$r4', 'FLOAT_FR'),
	'I32_TO_F64_PSEUDO': ('$dr0 = I32_TO_F64_PSEUDO $r4', '$r4', 'FLOAT_DR'),
	'F32_TO_I32_PSEUDO': ('$r2 = F32_TO_I32_PSEUDO $fr4', '$fr4', 'FTRC_FR'),
	'F64_TO_I32_PSEUDO': ('$r2 = F64_TO_I32_PSEUDO $dr4', '$dr4', 'FTRC_DR'),
	'F32_TO_F64_PSEUDO': ('$dr0 = F32_TO_F64_PSEUDO $fr4', '$fr4', 'FCNVSD'),
	'F64_TO_F32_PSEUDO': ('$fr0 = F64_TO_F32_PSEUDO $dr4', '$dr4', 'FCNVDS'),
}
MEMORY = {
	'FTRV_MEM': ' :: (load (s512), align 8), (load store (s640), align 8)',
}
# These pseudos create CFG and virtual registers before allocation. Their
# signed-conversion replacement is covered again by EXPANSIONS after RA.
CUSTOM_EXPANSIONS = {
	'F32_TO_U32_PSEUDO': ('fr', ('FCMP_GT_FR', 'FSUB_FR', 'F32_TO_I32_PSEUDO')),
	'F64_TO_U32_PSEUDO': ('dr', ('FCMP_GT_DR', 'FSUB_DR', 'F64_TO_I32_PSEUDO')),
}
GRAPHICS_EXPANSIONS = {
	'FTRV4_MEM': '%0, %1, %2, %3',
	'FTRV_ARRAY': '%0, %1, %4, %2, %3',
	'FTRV_COMPOSE': '%0, %1, %4, %2, %5, %3',
}
STATUS_MNEMONICS = {'fadd', 'fsub', 'fmul', 'fdiv', 'fmac', 'fsqrt', 'float', 'ftrc', 'fcnvsd', 'fcnvds', 'fcmp/eq', 'fcmp/gt', 'fipr', 'ftrv', 'fsrra', 'fsca'}
NONSTATUS_MNEMONICS = {'fmov', 'fmov.s', 'fmov.d', 'fldi0', 'fldi1', 'flds', 'fsts'}
MODE_MNEMONICS = {'frchg', 'fschg', 'fpchg'}
MODE_PSEUDOS = {'SET_FPSCR_MODE', 'SET_FPSCR_MODE_TOGGLE', 'TRANSITION_FPSCR_MODE', 'TRANSITION_FPSCR_MODE_REG'}


def registers(record, kind):
	return [operand['def'] for operand in record[kind]]


def check_records(records):
	status_pseudos = set()
	for name in records['!instanceof']['Instruction']:
		record = records[name]
		if record.get('Namespace') != 'SH':
			continue
		uses, defs = registers(record, 'Uses'), registers(record, 'Defs')
		marked = record.get('MayModifyFPSCRStatus', 0)
		asm = record['AsmString'].split()
		mnemonic = asm[0] if asm else ''
		if marked:
			assert 'FPSCR' in uses and 'FPSCR' in defs, (name, 'status updates need both FPSCR operands', uses, defs)
			if record['isPseudo']:
				status_pseudos.add(name)
		if mnemonic in STATUS_MNEMONICS:
			assert marked, (name, 'architectural status writer missing marker')
		elif mnemonic in NONSTATUS_MNEMONICS:
			assert not marked and 'FPSCR' not in defs, (name, 'transport is not a status writer')
			assert 'FPSCR' in uses, (name, 'transport still depends on mode/bank')
		elif mnemonic in ('fabs', 'fneg'):
			# Shared FR descriptors conservatively include E-class status effects;
			# SH4 double sign-bit operations are not status writers.
			assert bool(marked) == name.endswith('_FR'), name
			assert ('FPSCR' in defs) == bool(marked) and 'FPSCR' in uses, name
		elif mnemonic in MODE_MNEMONICS or name in MODE_PSEUDOS:
			assert 'FPSCR' in uses and 'FPSCR' in defs, name
		elif mnemonic.startswith('f'):
			raise AssertionError((name, 'classify new FPU instruction effects'))
		if name in ('STS_FPSCR', 'STS_L_FPSCR'):
			assert 'FPSCR' in uses and 'FPSCR' not in defs, name
		if name in ('LDS_FPSCR', 'LDS_L_FPSCR'):
			assert 'FPSCR' in defs, name
		if mnemonic.startswith('fcmp/'):
			assert 'TBit' in defs, (name, 'comparison predicate must survive')
		if mnemonic in ('ftrc', 'fcnvds') or name in EXPANSIONS and name not in ('FMAC_FR_PSEUDO', 'FTRV_MEM', 'FTRV4_MEM', 'SET_FPSCR_FIELDS'):
			assert 'FPUL' in defs, (name, 'conversion scratch/result must survive')
		if name == 'FMAC_FR_PSEUDO':
			assert 'FR0' in defs, name
	covered = set(EXPANSIONS) | set(CUSTOM_EXPANSIONS) | set(GRAPHICS_EXPANSIONS)
	assert status_pseudos == covered, ('missing status-pseudo expansion coverage', status_pseudos ^ covered)
	for name, (_, _, replacement) in EXPANSIONS.items():
		if name == 'SET_FPSCR_FIELDS':
			# A full LDS writes FPSCR without reading it; the pseudo's STS
			# provides the read half of its mode-preserving update.
			assert 'FPSCR' in registers(records[replacement], 'Defs'), replacement
		else:
			assert records[replacement]['MayModifyFPSCRStatus'], replacement
	for name, (_, replacements) in CUSTOM_EXPANSIONS.items():
		assert records[name]['usesCustomInserter'], name
		assert {'TBit', 'FPUL', 'FPSCR'} <= set(registers(records[name], 'Defs')), name
		for replacement in replacements:
			assert records[replacement]['MayModifyFPSCRStatus'], replacement
	for name in GRAPHICS_EXPANSIONS:
		assert records[name]['usesCustomInserter'], name
		assert {'XMTRX', 'FPSCR'} <= set(registers(records[name], 'Defs')), name


def generate(records):
	for name, (instruction, liveins, _) in EXPANSIONS.items():
		implicit = ', '.join(['implicit-def $' + r.lower() for r in registers(records[name], 'Defs')] + ['implicit $' + r.lower() for r in registers(records[name], 'Uses')])
		# MIR's YAML block indentation follows the required YAML syntax.
		print('---\nname: ' + name.lower() + '\ntracksRegLiveness: true\nnoVRegs: true\nbody: |')
		print('  bb.0:\n    liveins: ' + liveins + ', $fpscr, $pr')
		print('    ' + instruction + ', ' + implicit + MEMORY.get(name, ''))
		print('    $r0 = STS_FPSCR implicit $fpscr\n    RTS implicit $pr, implicit $r0\n...')


def check_expanded(records, text):
	for name, (_, _, replacement) in EXPANSIONS.items():
		body = re.search(r'name:\s+' + name.lower() + r'\n(.*?)(?=\n\.\.\.)', text, re.S).group(1).split('body:')[1]
		assert not re.search(r'\b' + name + r'\b', body), name
		writer = next(line for line in body.splitlines() if re.search(r'\b' + replacement + r'\b', line))
		assert re.search(r'implicit-def(?: dead)? \$fpscr', writer), (name, writer)
		if name == 'SET_FPSCR_FIELDS':
			reads = [m.start() for m in re.finditer(r'\bSTS_FPSCR\b', body)]
			assert len(reads) == 2 and reads[0] < body.index(replacement) < reads[1], (name, 'ordered read/update/read')
		else:
			assert re.search(r'implicit(?: killed)? \$fpscr', writer), (name, writer)
			assert body.index(replacement) < body.index('STS_FPSCR'), (name, 'full read moved over status writer')
		for line in body.splitlines():
			for opcode in re.findall(r'\b[A-Z][A-Za-z0-9_]+\b', line):
				if opcode in records and 'FPSCR' in registers(records[opcode], 'Uses'):
					assert re.search(r'implicit(?: killed)? \$fpscr', line), line


def generate_custom(records):
	for name, (rc, _) in CUSTOM_EXPANSIONS.items():
		implicit = ', '.join(['implicit-def $' + r.lower() for r in registers(records[name], 'Defs')] + ['implicit $' + r.lower() for r in registers(records[name], 'Uses')])
		print('---\nname: ' + name.lower() + '\ntracksRegLiveness: true\nbody: |')
		print('  bb.0:\n    liveins: $' + rc + '4, $' + rc + '6, $fpscr, $pr')
		print('    %0:' + rc + ' = COPY $' + rc + '4\n    %1:' + rc + ' = COPY $' + rc + '6')
		print('    %2:gpr = ' + name + ' %0, %1, ' + implicit)
		print('    $r0 = COPY %2\n    $r1 = STS_FPSCR implicit $fpscr\n    RTS implicit $pr, implicit $r0, implicit $r1\n...')
	for name, operands in GRAPHICS_EXPANSIONS.items():
		implicit = ', '.join(['implicit-def $' + r.lower() for r in registers(records[name], 'Defs')] + ['implicit $' + r.lower() for r in registers(records[name], 'Uses')])
		memory = ' :: (store (s512), align 8), (load (s512), align 8), (load (s512), align 8)'
		if name == 'FTRV_COMPOSE':
			memory += ', (load (s512), align 8)'
		memory += ', (load store (s128), align 8)'
		print('---\nname: ' + name.lower() + '\ntracksRegLiveness: true\nbody: |')
		print('  bb.0:\n    liveins: $r4, $r5, $r6, $r7, $r8, $xmtrx, $fpscr, $pr')
		for n, reg in ((0,4),(1,5),(2,6),(3,7),(5,8)):
			print('    %' + str(n) + ':gpr = COPY $r' + str(reg))
		print('    %4:gpr = MOVri 7')
		print('    ' + name + ' ' + operands + ', ' + implicit + memory)
		print('    $r1 = STS_FPSCR implicit $fpscr\n    RTS implicit $pr, implicit $r1\n...')


def check_custom_expanded(records, text):
	custom = {name: replacements for name, (_, replacements) in CUSTOM_EXPANSIONS.items()}
	custom.update({name: ('FTRV',) for name in GRAPHICS_EXPANSIONS})
	for name, replacements in custom.items():
		body = re.search(r'name:\s+' + name.lower() + r'\n(.*?)(?=\n\.\.\.)', text, re.S).group(1).split('body:')[1]
		assert not re.search(r'\b' + name + r'\b', body), name
		for replacement in replacements:
			writer = next(line for line in body.splitlines() if re.search(r'\b' + replacement + r'\b', line))
			assert re.search(r'implicit-def(?: dead)? \$fpscr', writer), (name, writer)
			assert re.search(r'implicit(?: killed)? \$fpscr', writer), (name, writer)
			assert body.index(replacement) < body.index('STS_FPSCR'), (name, 'full read moved over status writer')
		for line in body.splitlines():
			for opcode in re.findall(r'\b[A-Z][A-Za-z0-9_]+\b', line):
				if opcode in records and 'FPSCR' in registers(records[opcode], 'Uses'):
					assert re.search(r'implicit(?: killed)? \$fpscr', line), line


if __name__ == '__main__':
	mode, filename = sys.argv[1:3]
	records = json.load(open(filename))
	if mode == 'records':
		check_records(records)
	elif mode == 'generate':
		generate(records)
	elif mode == 'generate-custom':
		generate_custom(records)
	elif mode == 'custom-expanded':
		check_custom_expanded(records, open(sys.argv[3]).read())
	else:
		assert mode == 'expanded'
		check_expanded(records, open(sys.argv[3]).read())
