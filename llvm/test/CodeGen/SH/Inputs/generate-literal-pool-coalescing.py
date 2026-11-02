print('--- |')
for name in ('at_limit', 'past_limit'):
	print('  define i32 @' + name + '() optsize { ret i32 0 }')
print('...')
for name, nops in (('at_limit', 502), ('past_limit', 504)):
	print('---\nname: ' + name + '\nalignment: 4\ntracksRegLiveness: true\nnoVRegs: true')
	print("constants:\n  - id: 0\n    value: 'i32 305419896'\n    alignment: 4")
	print('body: |')
	for line in ('bb.0:', '\tsuccessors: %bb.2', '\t$r0 = MOVL_load_pc_island %const.0, 0', '\tBRA %bb.2 {', '\t\tNOP', '\t}', 'bb.1 (align 4):', '\tsuccessors:', '\tSH_CONSTPOOL_ENTRY %const.0, 0, 4, 4', 'bb.2:'):
		print('  ' + line)
	for _ in range(nops):
		print('  \tNOP')
	for line in ('\t$r0 = MOVL_load_pc_island %const.0, 1', '\tRTS implicit $pr, implicit $r0 {', '\t\tNOP', '\t}', 'bb.3 (align 4):', '\tsuccessors:', '\tSH_CONSTPOOL_ENTRY %const.0, 1, 4, 4'):
		print('  ' + line)
	print('...')
