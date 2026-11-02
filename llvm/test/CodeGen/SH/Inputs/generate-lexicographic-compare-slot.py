#!/usr/bin/env python3
"""Exercise old-T branch decisions, new-T consumers and conservative slot vetoes."""
import sys

positive = ['unsigned_hi','unsigned_hs','signed_gt','signed_ge','inverted_consumer','multiple_t_consumers','carry_after_branch','borrow_after_branch','low_non_strict','biased_edge','short_reach']
negative = ['cold_edge','other_live_t','target_live_t','other_movt','other_addc','other_subc','other_dt','other_tst','other_call','other_asm','other_load','other_volatile','other_atomic','target_load','target_volatile','target_atomic','target_asm','target_trap','target_call','target_copy','target_eq','target_dt','intervening_t_def','intervening_gpr_def','different_operands','undefined_operand','reserved_operand','multiple_predecessors','already_bundled','nested_conditional','asm_interval','literal_interval','aligned_target','long_reach','no_liveness','optsize','minsize','optnone']
cases = positive+negative
# Conditional bundles are formed after branch relaxation in the real pipeline.
if '--before-relax' in sys.argv:
	cases = [name for name in cases if name not in ('already_bundled','nested_conditional','no_liveness')]
out = '# The branch reads equality T; the slot writes ordering T. Every taken-path\n# consumer keeps its reaching definition. Low-word fallthrough kills new T.\n--- |\n  declare void @callee()\n'
for name in cases:
	attr = {'optsize':'optsize','minsize':'minsize','optnone':'optnone noinline'}.get(name,'')
	out += f'  define i32 @{name}(i32 %ah, i32 %bh, i32 %al, i32 %bl, ptr %p) {attr} {{ ret i32 0 }}\n'
out += '...\n'
for name in cases:
	cmp = {'unsigned_hs':'CMP_HS','signed_gt':'CMP_GT','signed_ge':'CMP_GE','target_eq':'CMP_EQ'}.get(name,'CMP_HI')
	consumer = 'BF' if name=='inverted_consumer' else 'BT'
	prob = ('0x20000000','0x60000000') if name=='biased_edge' else ('0x60000000','0x20000000') if name=='cold_edge' else ('0x40000000','0x40000000')
	low = 'CMP_HS' if name=='low_non_strict' else 'CMP_HI'
	pre = ''
	if name=='intervening_t_def': pre='\tTST $r6, $r7, implicit-def $tbit\n'
	if name=='intervening_gpr_def': pre='\t$r4 = ADDri $r4, 1\n'
	altpre = ''
	if name=='other_movt': altpre='\t$r0 = MOVT implicit $tbit\n'
	if name in ('other_addc','other_subc'): altpre=f'\t$r6 = {name[6:].upper()} $r6, $r7, implicit-def $tbit, implicit $tbit\n'
	if name=='other_dt': altpre='\t$r6 = DT $r6, implicit-def $tbit\n'
	if name=='other_tst': altpre='\tTST $r6, $r7, implicit-def $tbit\n'
	if name in ('other_asm','asm_interval'): altpre='\tINLINEASM &"nop", sideeffect attdialect\n'
	if name in ('other_load','other_volatile','other_atomic'):
		kind={'other_load':'load','other_volatile':'volatile load','other_atomic':'load acquire'}[name]
		altpre=f'\t$r0 = MOVL_load_reg $r6 :: ({kind} (s32) from %ir.p)\n'
	if name=='other_call': altpre='\tJSR $r4, csr_sh, implicit-def $pr, implicit-def $r6, implicit-def $r7\n'
	if name=='literal_interval': altpre='\t$r0 = MOVL_load_pc_island %const.0, 0\n'
	if name in ('short_reach','long_reach'): altpre='\tNOP\n'*(119 if name=='short_reach' else 127)
	# For interval-only tests the low compare remains first.
	altpost=altpre if name in ('asm_interval','literal_interval','short_reach','long_reach') else ''
	if altpost: altpre=''
	a,b='$r4','$r5'
	if name=='different_operands': a='$r6'
	if name=='undefined_operand': a='undef $r4'
	if name=='reserved_operand': a='$r15'
	target=f'\t{cmp} {a}, {b}, implicit-def $tbit\n'
	if name in ('target_load','target_volatile','target_atomic'):
		kind={'target_load':'load','target_volatile':'volatile load','target_atomic':'load acquire'}[name]
		target=f'\t$r0 = MOVL_load_reg $r6 :: ({kind} (s32) from %ir.p)\n'+target
	if name=='target_asm': target='\tINLINEASM &"nop", sideeffect attdialect\n'+target
	if name=='target_trap': target='\tTRAPA 0\n'+target
	if name=='target_call': target='\tJSR $r4, csr_sh, implicit-def $pr, implicit-def $r4, implicit-def $r5, implicit-def $r6, implicit-def $r7\n'+target
	if name=='target_copy': target='\t$r0 = MOVrr $r6\n'+target
	if name=='target_dt': target='\t$r6 = DT $r6, implicit-def $tbit\n'+target
	entry_branch='\tBF %bb.2, implicit $tbit\n'
	if name=='already_bundled': entry_branch='\tBF_S %bb.2, implicit $tbit {\n\t  NOP\n\t}\n'
	if name=='nested_conditional': target=target+f'\t{consumer}_S %bb.4, implicit $tbit {{\n\t  NOP\n\t}}\n'
	else: target+=f'\t{consumer} %bb.4, implicit $tbit\n'
	otherlive=', $tbit' if name in ('other_live_t','other_movt','other_addc','other_subc') else ''
	targetlive=', $tbit' if name=='target_live_t' else ''
	alt_succ='%bb.2, %bb.4' if name=='multiple_predecessors' else '%bb.3, %bb.4'
	alt_branch='\tBRA %bb.2\n' if name=='multiple_predecessors' else '\tBRA %bb.3\n'
	align=' (align 4)' if name=='aligned_target' else ''
	body=f'bb.0:\n\tsuccessors: %bb.1({prob[0]}), %bb.2({prob[1]})\n\tliveins: $r4, $r5, $r6, $r7\n\n\tCMP_EQ $r4, $r5, implicit-def $tbit\n{pre}{entry_branch}bb.1:\n\tsuccessors: {alt_succ}\n\tliveins: $r4, $r6, $r7{otherlive}\n\n{altpre}\t{low} $r6, $r7, implicit-def $tbit\n{altpost}\tBT %bb.4, implicit $tbit\n{alt_branch}bb.2{align}:\n\tsuccessors: %bb.3, %bb.4\n\tliveins: $r4, $r5, $r6, $r7{targetlive}\n\n{target}'
	for block in (3,4):
		live='\tliveins: $r6, $r7, $tbit\n' if name in ('multiple_t_consumers','carry_after_branch','borrow_after_branch') else ''
		ops=f'\t$r0 = MOVri {block-3}\n'
		if name=='multiple_t_consumers': ops='\t$r0 = MOVT implicit $tbit\n\t$r1 = MOVT implicit $tbit\n\t$r0 = ADDrr $r0, $r1\n'
		if name in ('carry_after_branch','borrow_after_branch'):
			op='ADDC' if name=='carry_after_branch' else 'SUBC'
			ops=f'\t$r6 = {op} $r6, $r7, implicit-def $tbit, implicit $tbit\n\t$r0 = MOVT implicit $tbit\n\t$r0 = ADDrr $r0, $r6\n'
		body+=f'bb.{block}:\n{live}\n{ops}\tRTS implicit $pr, implicit $r0\n'
	if name=='literal_interval': body+='bb.5 (align 4):\n\tsuccessors:\n\tSH_CONSTPOOL_ENTRY %const.0, 0, 4, 4\n'
	out+=f'\n---\nname: {name}\nalignment: 4\ntracksRegLiveness: '+('false' if name=='no_liveness' else 'true')+'\nnoVRegs: true\n'
	if name=='literal_interval': out+="constants:\n  - { id: 0, value: 'i32 65536', alignment: 4 }\n"
	out+='body: |\n'+''.join('  '+line+'\n' for line in body.splitlines())+'...\n'
	if name in positive:
		out+=f'\n# OPT-LABEL: name: {name}\n# OPT: CMP_EQ $r4, $r5, implicit-def $tbit\n# OPT-NEXT: BF_S %bb.2, implicit $tbit {{\n# OPT-NEXT: {cmp} $r4, $r5, implicit-def $tbit\n# OPT-NEXT: }}\n# OPT: bb.2\n# OPT: liveins: {{{{.*}}}}$tbit\n# OPT-NOT: {cmp} $r4, $r5\n# OPT: {consumer} %bb.4, implicit $tbit\n# BASE-LABEL: name: {name}\n# BASE: CMP_EQ $r4, $r5, implicit-def $tbit\n# BASE-NEXT: BF %bb.2, implicit $tbit\n'
	elif name=='intervening_gpr_def':
		# The pre-existing local scheduler may put ADDri in the branch slot.
		# The ordering compare must still remain on its original target.
		out+=f'\n# COMMON-LABEL: name: {name}\n# COMMON: bb.2\n# COMMON: CMP_HI $r4, $r5, implicit-def $tbit\n'
	elif name=='already_bundled':
		out+=f'\n# COMMON-LABEL: name: {name}\n# COMMON: BF_S %bb.2, implicit $tbit {{\n# COMMON-NEXT: NOP\n# COMMON-NEXT: }}\n'
	else:
		out+=f'\n# COMMON-LABEL: name: {name}\n# COMMON: BF %bb.2, implicit $tbit\n'
sys.stdout.write(out)
