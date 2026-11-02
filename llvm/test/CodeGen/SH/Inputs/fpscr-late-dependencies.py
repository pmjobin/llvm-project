#!/usr/bin/env python3
"""Independent pairwise reference for the actual late SH scheduler graph."""
import collections
import itertools
from pathlib import Path
import re
import sys

B = '$fr4 = FMUL_FR $fr4, $fr5, implicit-def $fpscr, implicit $fpscr'
S = '$fr6 = FADD_FR $fr6, $fr7, implicit-def dead $fpscr, implicit $fpscr'
C = '$fr0 = FLDI1 implicit $fpscr'
R = '$r2 = STS_FPSCR implicit $fpscr'
W = 'LDS_FPSCR $r3, implicit-def $fpscr'

def document(name, instructions, extra=''):
	return f'''---
name: {name}
tracksRegLiveness: true
noVRegs: true
frameInfo:
  isCalleeSavedInfoValid: true
{extra}body: |
  bb.0:
    liveins: $fr0, $fr1, $fr2, $fr3, $fr4, $fr5, $fr6, $fr7, $fpul, $fpscr, $pr, $r0, $r1, $r2, $r3, $r4, $r5, $r15
''' + ''.join('    ' + i + '\n' for i in instructions) + '    RTS implicit $pr, implicit $fr4, implicit $fr6, implicit $fr0, implicit $r2, implicit $fpscr\n...\n'

def generate(path):
	functions = {}
	for index, order in enumerate(itertools.permutations([B, S, C, R, W])):
		functions[f'order_{index}'] = list(order)
	functions.update({
		'hidden_control': [W, B, C, R],
		'fpr_reason': [B, '$fr0 = FMOV_FR $fr4, implicit $fpscr', R],
		'fpul_reason': ['FTRC_FR $fr4, implicit-def $fpul, implicit-def $fpscr, implicit $fpscr', '$fr0 = FSTS implicit $fpul, implicit $fpscr', R],
		'alias_reason': ['$dr4 = FMUL_DR $dr4, $dr6, implicit-def $fpscr, implicit $fpscr', '$fr0 = FMOV_FR $fr4, implicit $fpscr', R],
		'gpr_reason': [B, '$r0 = STS_FPSCR implicit $fpscr', '$fr0 = FMOV_S_LOAD_PC_ISLAND %const.0, -1, implicit-def dead $r0, implicit $fpscr :: (dereferenceable invariant load (s32) from constant-pool)', R],
		'faulting_memory': [B, '$fr0 = FMOV_S_LOAD $r4, implicit $fpscr :: (load (s32))', C, R],
		'volatile_memory': [B, '$fr0 = FMOV_S_LOAD $r4, implicit $fpscr :: (volatile load (s32))', C, R],
		'missing_mmo': [B, '$fr0 = FMOV_S_LOAD_PC_ISLAND %const.0, -1, implicit-def dead $r0, implicit $fpscr', C, R],
		'full_write': [B, W, C, R],
		'interval_before': [B, C, '$r0 = STS_FPSCR implicit $fpscr', '$r0 = XORrr $r0, $r1', 'LDS_FPSCR $r0, implicit-def $fpscr', R],
		'interval_after': ['$r0 = STS_FPSCR implicit $fpscr', '$r0 = XORrr $r0, $r1', 'LDS_FPSCR $r0, implicit-def $fpscr', C, B, R],
		'asm_boundary': [B, C, 'INLINEASM &"", sideeffect attdialect, clobber, implicit-def $fpscr', S, R],
		'call_boundary': [B, C, 'JSR $r5, csr_sh_hard, implicit-def $pr, implicit-def $r15', '$r3 = MOVri 0', 'LDS_FPSCR $r3, implicit-def $fpscr', '$fr0 = FLDI0 implicit $fpscr', '$fr4 = FLDI0 implicit $fpscr', '$fr6 = FLDI0 implicit $fpscr', '$fr7 = FLDI1 implicit $fpscr', S, R],
		'safe_load': [B, '$fr0 = FMOV_S_LOAD_PC_ISLAND %const.0, -1, implicit-def dead $r0, implicit $fpscr :: (dereferenceable invariant load (s32) from constant-pool)', '$fr0 = FADD_FR $fr0, $fr6, implicit-def $fpscr, implicit $fpscr', R],
	})
	text = ''
	for name, instructions in functions.items():
		extra = 'constants:\n  - id: 0\n    value: float 1.000000e+00\n    alignment: 4\n' if any('%const.' in i for i in instructions) else ''
		text += document(name, instructions, extra)
	text += document('boundary_liveout', [B, C]).replace('    RTS implicit', '    BRA %bb.1\n  bb.1:\n    liveins: $fr0, $fr4, $fr6, $r2, $fpscr, $pr\n    $r2 = STS_FPSCR implicit $fpscr\n    RTS implicit')
	Path(path).write_text(text)

def normalize(instruction):
	instruction = re.sub(r'/\*.*?\*/', '', instruction)
	instruction = re.sub(r'\b(killed|dead|renamable) ', '', instruction)
	instruction = re.sub(r'\(tied-def \d+\)', '', instruction)
	return re.sub(r'\s+', ' ', instruction).strip()

def opcode(instruction):
	return re.search(r'\b([A-Z][A-Za-z0-9_]+)(?: |$)', instruction)[1]

def domains(instruction):
	# This is an independent semantic reference, not the implementation's
	# last-definition frontier or its edge-deletion predicate.
	op = opcode(instruction)
	read = set()
	write = set()
	if re.search(r'implicit \$fpscr', instruction):
		read = {'control', 'status'}
	if re.search(r'implicit-def (?:dead )?\$fpscr', instruction):
		write = {'control', 'status'}
	if op in {'FMUL_FR', 'FMUL_DR', 'FADD_FR', 'FTRC_FR'}:
		write = {'status'}
	elif op in {'FLDI0', 'FLDI1', 'FMOV_FR', 'FSTS'}:
		read = {'control'}
	elif op == 'FMOV_S_LOAD_PC_ISLAND' and 'dereferenceable invariant load' in instruction:
		read = {'control'}
	return read, write

def machine_functions(path):
	result = {}
	for doc in Path(path).read_text().split('---\n'):
		name = re.search(r'^name:\s*(\S+)', doc, re.M)
		if not name:
			continue
		body = doc.split('body:', 1)[1]
		result[name[1]] = [normalize(l) for l in body.splitlines()[1:] if l.startswith('    ') and not l.strip().startswith(('liveins:', 'successors:')) and l.strip()]
	return result

def graphs(path):
	for part in Path(path).read_text().split('SHFPSCR begin ')[1:]:
		name, bb, count = part.splitlines()[0].split()
		nodes = {}
		edges = {'before': [], 'after': [], 'metadata': {'before': {}, 'after': {}}}
		for line in part.split('SHFPSCR end')[0].splitlines()[1:]:
			if line.startswith('SHFPSCR node '):
				prefix, instruction = line.split(' | ', 1)
				nodes[int(prefix.split()[2])] = normalize(instruction)
			elif line.startswith('SHFPSCR edge '):
				_, _, stage, a, b, kind, reg, latency, weak = line.split()
				edges[stage].append((int(a), int(b), int(kind), reg, int(latency), int(weak)))
			elif line.startswith('SHFPSCR metadata '):
				_, _, stage, n, depth, height = line.split()
				edges['metadata'][stage][int(n)] = (int(depth), int(height))
		assert len(nodes) == int(count), name
		yield name, nodes, edges

def verify_metadata(name, edges):
	for stage in ('before', 'after'):
		metadata = edges['metadata'][stage]
		assert metadata, ('missing complete graph metadata', name, stage)
		preds = {n: [] for n in metadata}
		succs = {n: [] for n in metadata}
		for a, b, kind, reg, latency, weak in edges[stage]:
			preds[b].append((a, latency))
			succs[a].append((b, latency))
		remaining = {n: len(preds[n]) for n in metadata}
		ready = collections.deque(n for n in metadata if not remaining[n])
		order = []
		while ready:
			n = ready.popleft()
			order.append(n)
			for successor, latency in succs[n]:
				remaining[successor] -= 1
				if remaining[successor] == 0:
					ready.append(successor)
		assert len(order) == len(metadata), ('cyclic graph', name, stage)
		depth, height = {}, {}
		for n in order:
			depth[n] = max((depth[p] + latency for p, latency in preds[n]), default=0)
		for n in reversed(order):
			height[n] = max((height[s] + latency for s, latency in succs[n]), default=0)
		assert all(metadata[n] == (depth[n], height[n]) for n in metadata), ('stale cached depth/height', name, stage)

def check(original, scheduled, graph, retained=False):
	before, after = machine_functions(original), machine_functions(scheduled)
	assert before.keys() == after.keys()
	for name in before:
		assert collections.Counter(before[name]) == collections.Counter(after[name]), ('inventory', name)
	checked = 0
	orders = 0
	projected = set()
	for name, nodes, edges in graphs(graph):
		checked += 1
		projected.add(name)
		verify_metadata(name, edges)
		ids = sorted(nodes)
		relation = set()
		for a, b in itertools.combinations(ids, 2):
			ra, wa = domains(nodes[a])
			rb, wb = domains(nodes[b])
			if wa & (rb | wb) or ra & wb:
				relation.add((a, b))
		adj = {i: set() for i in ids}
		for a, b, kind, reg, latency, weak in edges['after']:
			if a in adj and not weak:
				adj[a].add(b)
		for a, b in relation:
			seen, todo = set(), list(adj[a])
			while todo:
				n = todo.pop()
				assert n != a, ('cycle', name)
				if n not in seen:
					seen.add(n)
					todo.extend(adj.get(n, set()) - seen)
			assert b in seen, ('missing semantic dependency', name, a, b)
		for a, b, kind, reg, latency, weak in edges['before']:
			survivors = [e for e in edges['after'] if e[:4] == (a, b, kind, reg) and e[5] == weak]
			if survivors:
				assert max(e[4] for e in survivors) >= latency, ('latency weakened', name, a, b, reg)
			else:
				assert not retained, ('lost original coarse edge', name, a, b, kind, reg)
				assert reg == 'FPSCR' and kind in {0, 1}, ('lost other reason', name, a, b, kind, reg)
				assert (a, b) not in relation, ('lost true edge', name, a, b)
		positions = {}
		used = set()
		for i in ids:
			p = next(p for p, ins in enumerate(after[name]) if p not in used and ins == nodes[i])
			used.add(p)
			positions[i] = p
		assert all(positions[a] < positions[b] for a, b in relation), ('final semantic order', name)
		if retained:
			assert all(positions[a] < positions[b] for a, b, k, reg, lat, weak in edges['before']
				if a in positions and b in positions and not weak), ('final original-edge order', name)
		if len(ids) <= 5:
			for order in itertools.permutations(ids):
				pos = {n: p for p, n in enumerate(order)}
				if all(pos[a] < pos[b] for a in ids for b in adj[a] if b in pos):
					orders += 1
					assert all(pos[a] < pos[b] for a, b in relation), ('illegal exposed order', name, order)
		if name == 'hidden_control':
			assert (0, 2, 0, 'FPSCR', 4, 0) in edges['after'], 'hidden full definition latency'
		if name == 'fpr_reason':
			assert any(a == 0 and b == 1 and reg == 'FR4' and latency == 4 for a, b, k, reg, latency, w in edges['after'])
		if name == 'fpul_reason':
			assert any(a == 0 and b == 1 and reg == 'FPUL' for a, b, k, reg, latency, w in edges['after'])
		if name == 'alias_reason':
			assert any(a == 0 and b == 1 and reg == 'FR4' for a, b, k, reg, latency, w in edges['after'])
	for name, instructions in before.items():
		if not name.startswith('order_'):
			continue
		position = instructions.index(C)
		previous = [opcode(i) for i in instructions[:position] if domains(i)[1]]
		following = [opcode(i) for i in instructions[position + 1:] if domains(i)[1]]
		exposed = (previous and previous[-1] != 'LDS_FPSCR') or (following and following[0] != 'LDS_FPSCR')
		assert (name in projected) == bool(exposed), ('complete permutation coverage', name)
	assert orders >= checked and {'hidden_control', 'fpr_reason', 'fpul_reason', 'safe_load', 'interval_before', 'interval_after'} <= projected
	if not retained:
		assert after['safe_load'].index(normalize(before['safe_load'][1])) < after['safe_load'].index(normalize(B)), 'positive must actually use new motion'
	print(f'PASS: {checked} actual projected regions; {orders} enumerated legal orders; final inventories/orders; retained={retained}')

if sys.argv[1] == 'generate':
	generate(sys.argv[2])
elif sys.argv[1] == 'check':
	check(*sys.argv[2:])
elif sys.argv[1] == 'check-retained':
	check(*sys.argv[2:], retained=True)
elif sys.argv[1] == 'coarse':
	assert 'SHFPSCR begin' not in Path(sys.argv[2]).read_text()
