#!/usr/bin/env python3
"""Emit generic conditional-copy cases, including branch reach boundaries."""
import sys

cases = ['equal_copy','biased_copy','inverted_copy','multiple_t_consumers','memory_on_fallthrough','copy_from_defined_value','cold_taken_edge','destination_live_on_other_edge','multiple_predecessors','load_on_target','volatile_on_target','atomic_on_target','asm_on_target','t_clobber_on_target','copy_not_first','frame_copy','undefined_copy','reserved_destination','reserved_source','no_merge_copy','asm_in_interval','literal_in_interval','aligned_target','short_reach','long_reach','optsize_copy','minsize_copy','optnone_copy','explicit_fallthrough_branch']
cases.remove('no_merge_copy')  # Current MIR parser cannot read its printer's nomerge flag.
cases.append('copy_can_fill_return')
positive = set(cases[:6]+['short_reach'])
result = '# A taken-successor copy may execute on the other edge only when its result is\n# dead there. Memory operations and T definitions stay on their original paths.\n--- |\n'
for name in cases:
	attr = {'optsize_copy':'optsize','minsize_copy':'minsize','optnone_copy':'optnone noinline'}.get(name,'')
	result += f'  define i32 @{name}(ptr %p) {attr} {{ ret i32 0 }}\n'
result += '...\n'
for name in cases:
	branch = 'BF' if name=='inverted_copy' else 'BT'
	probs = ('0x20000000','0x60000000') if name=='biased_copy' else ('0x60000000','0x20000000') if name=='cold_taken_edge' else ('0x40000000','0x40000000')
	entry_live = '$r4, $r5, $r6, $r7'+(', $r0' if name=='destination_live_on_other_edge' else '')
	preamble = '\t$r4 = ADDri killed $r4, 1\n' if name=='copy_from_defined_value' else ''
	tail = '\tBRA %bb.1\n' if name=='explicit_fallthrough_branch' else ''
	alive = '$r4, $r7'+(', $r0' if name=='destination_live_on_other_edge' else '')
	alt = '\t$r0 = MOVrr $r4\n\tRTS implicit $pr, implicit $r0\n'
	if name=='destination_live_on_other_edge': alt='\tRTS implicit $pr, implicit $r0\n'
	if name=='multiple_predecessors': alt='\tBRA %bb.2\n'; alive='$r4, $r6'
	if name=='memory_on_fallthrough': alt='\tMOVL_store_reg $r4, $r7 :: (volatile store (s32) into %ir.p)\n'+alt
	if name=='asm_in_interval': alt='\tINLINEASM &"nop", sideeffect attdialect\n'+alt
	if name=='literal_in_interval': alt='\t$r0 = MOVL_load_pc_island %const.0, 0\n\tRTS implicit $pr, implicit $r0\n'
	if name in ('short_reach','long_reach'): alt='\tNOP\n'*(124 if name=='short_reach' else 127)+alt
	first = '\t$r0 = MOVrr $r4\n'
	if name in ('load_on_target','volatile_on_target','atomic_on_target'):
		memory = {'load_on_target':'load','volatile_on_target':'volatile load','atomic_on_target':'load acquire'}[name]
		first = f'\t$r0 = MOVL_load_reg $r7 :: ({memory} (s32) from %ir.p)\n'
	if name=='asm_on_target': first='\tINLINEASM &"nop", sideeffect attdialect\n'+first
	if name=='t_clobber_on_target': first='\tCLRT implicit-def $tbit\n'+first
	if name=='copy_not_first': first='\tNOP\n'+first
	if name=='frame_copy': first='\t$r0 = frame-setup MOVrr $r4\n'
	if name=='undefined_copy': first='\t$r0 = MOVrr undef $r4\n'
	if name=='reserved_destination': first='\t$r15 = MOVrr $r4\n\t$r0 = MOVrr $r4\n'
	if name=='reserved_source': first='\t$r0 = MOVrr $r15\n'
	if name=='no_merge_copy': first='\t$r0 = nomerge MOVrr $r4\n'
	tlive = '$r4, $r6, $r7'+(', $tbit' if name=='multiple_t_consumers' else '')
	operation = '\t$r0 = ADDrr killed $r0, $r6\n'
	if name=='copy_can_fill_return': operation=''
	if name=='multiple_t_consumers': operation='\t$r0 = ADDC killed $r0, $r6, implicit-def $tbit, implicit $tbit\n\t$r0 = SUBC killed $r0, $r6, implicit-def dead $tbit, implicit $tbit\n'
	align = ' (align 4)' if name=='aligned_target' else ''
	body = f'bb.0:\n\tsuccessors: %bb.1({probs[0]}), %bb.2({probs[1]})\n\tliveins: {entry_live}\n\n{preamble}\tCMP_HI $r5, $r4, implicit-def $tbit\n\t{branch} %bb.2, implicit $tbit\n{tail}bb.1:\n'+('\tsuccessors: %bb.2\n' if name=='multiple_predecessors' else '')+f'\tliveins: {alive}\n\n{alt}bb.2{align}:\n\tliveins: {tlive}\n\n'+first+operation+'\tRTS implicit $pr, implicit $r0\n'
	if name=='literal_in_interval': body += 'bb.3 (align 4):\n\tsuccessors:\n\tSH_CONSTPOOL_ENTRY %const.0, 0, 4, 4\n'
	result += '\n---\nname: '+name+'\nalignment: 4\ntracksRegLiveness: true\nnoVRegs: true\n'
	if name=='literal_in_interval': result += "constants:\n  - { id: 0, value: 'i32 65536', alignment: 4 }\n"
	result += 'body: |\n'+''.join('  '+line+'\n' for line in body.splitlines())+'...\n'
	if name in positive:
		result += f'\n# OPT-LABEL: name: {name}\n# OPT: CMP_HI $r5, $r4, implicit-def $tbit\n# OPT-NEXT: {branch}_S %bb.2, implicit $tbit {{\n# OPT-NEXT: $r0 = MOVrr $r4\n# OPT-NEXT: }}\n# BASE-LABEL: name: {name}\n# BASE: CMP_HI $r5, $r4, implicit-def $tbit\n# BASE-NEXT: {branch} %bb.2, implicit $tbit\n'
	else:
		result += f'\n# COMMON-LABEL: name: {name}\n# COMMON: CMP_HI $r5, $r4, implicit-def $tbit\n# COMMON-NEXT: {branch} %bb.2, implicit $tbit\n'
sys.stdout.write(result)
