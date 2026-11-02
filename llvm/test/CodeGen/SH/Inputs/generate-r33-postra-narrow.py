#!/usr/bin/env python3
"""Allocated-register witnesses: real instruction savings and ISA/liveness vetoes."""
cases = ['direct_byte', 'direct_word', 'copy_pays_for_result', 'preserve_t',
         'r0_live', 'address_live', 'base_clobbered', 'no_saving', 'range_byte',
         'odd_word', 'volatile', 'atomic', 'frame_setup', 'inline_barrier',
         'intervening_address_read', 'no_liveness']
print('--- |')
for name in cases:
    print('  define void @' + name + '(ptr %p) { ret void }')
print('...')
for name in cases:
    word = name in ('direct_word', 'odd_word')
    offset = 30 if name == 'direct_word' else 3
    if name == 'range_byte':
        offset = 16
    dest = '$r3' if name in ('copy_pays_for_result', 'r0_live', 'no_saving') else '$r0'
    copy = name in ('copy_pays_for_result', 'r0_live', 'base_clobbered')
    address = '$r2' if copy else '$r4'
    flags = 'frame-setup ' if name == 'frame_setup' else 'nuw '
    before = [f'{address} = COPY killed $r4'] if copy else []
    before.append(f'{address} = {flags}ADDri {address}, {offset}')
    between = ['$r1 = MOVri 42']
    if name == 'base_clobbered':
        between = ['$r4 = MOVri 99']
    if name == 'inline_barrier':
        between = ['INLINEASM &"nop", sideeffect attdialect']
    if name == 'intervening_address_read':
        between = [f'$r1 = MOVrr {address}']
    kill = '' if name == 'address_live' else 'killed '
    memory = 'volatile load' if name == 'volatile' else 'load monotonic' if name == 'atomic' else 'load'
    opcode = 'MOVW_load_reg' if word else 'MOVB_load_reg'
    load = f'{dest} = {opcode} {kill}{address} :: ({memory} (s{16 if word else 8}) from %ir.p)'
    after = []
    if name in ('copy_pays_for_result', 'no_saving'):
        after = ['$r0 = MOVri 17']
    if name == 'r0_live':
        after = ['$r0 = ADDrr $r0, killed $r3']
    if name == 'address_live':
        after = ['$r1 = MOVrr killed $r4']
    if name == 'preserve_t':
        after = ['$r1 = MOVT implicit killed $tbit']
    uses = ', implicit $r3' if name in ('copy_pays_for_result', 'no_saving') else ', implicit $r0'
    if name in ('intervening_address_read', 'address_live', 'preserve_t'):
        uses += ', implicit $r1'
    body = before + between + [load] + after + ['RTS implicit $pr' + uses]
    liveins = '$r4' + (', $r0' if name == 'r0_live' else '') + (', $tbit' if name == 'preserve_t' else '')
    print(f'---\nname: {name}\ntracksRegLiveness: {"false" if name == "no_liveness" else "true"}\nnoVRegs: true\nbody: |\n  bb.0:\n    liveins: {liveins}\n')
    for line in body:
        print('    ' + line)
    print('...\n')
    if name in ('direct_byte', 'direct_word', 'copy_pays_for_result', 'preserve_t'):
        print(f'# BASE-LABEL: name: {name}\n# BASE: ADDri\n# BASE: {opcode}')
        print(f'# OPT-LABEL: name: {name}\n# OPT-NOT: ADDri\n# OPT: $r0 = {"MOVW_load_disp" if word else "MOVB_load_disp"} killed $r4, {offset}')
        if name == 'copy_pays_for_result':
            print('# OPT-NEXT: $r3 = MOVrr killed $r0')
        if name == 'preserve_t':
            print('# OPT: $r1 = MOVT implicit killed $tbit')
    else:
        print(f'# COMMON-LABEL: name: {name}\n# COMMON: ADDri\n# COMMON: {opcode}')
