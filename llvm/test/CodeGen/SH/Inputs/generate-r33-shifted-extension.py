#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Allocated-register extension identities with precise observation barriers."""
positive = ['byte_inplace', 'word_inplace', 'byte_order8_16', 'byte_three8',
            'byte_copy_load', 'word_copy_load', 'r0_disp_inplace',
            'unrelated_instructions', 't_live', 'byte_poison_flags']
negative = ['byte_too_few', 'word_too_few', 'other_use', 'clobber',
            'r16_multiply', 'source_not_killed', 'r0_disp_retarget',
            'volatile_retarget', 'atomic_retarget', 'not_a_load',
            'call_gap', 'asm_gap', 'branch_gap', 'delay_gap', 'frame_gap',
            'no_liveness', 'too_far', 'no_mmo', 'frame_destroy', 'byte_too_few_poison']
print('--- |')
for name in positive + negative:
    print('  define i32 @' + name + '(ptr %p) { ret i32 0 }')
print('...')
for name in positive + negative:
    word = name.startswith('word')
    copied = name in ('byte_copy_load', 'word_copy_load', 'source_not_killed',
                      'r0_disp_retarget', 'volatile_retarget', 'atomic_retarget', 'not_a_load', 'no_mmo')
    dest = '$r2' if copied else '$r8' if name == 'call_gap' else '$r0'
    source = '$r0' if name == 'r0_disp_retarget' else '$r1' if copied else dest
    liveins = ['$r4', '$r5']
    if name in ('t_live', 'delay_gap'):
        liveins.append('$tbit')
    opcode = 'EXTUW' if word else 'EXTUB'
    kind = 'W' if word else 'B'
    memory = 'volatile load' if name == 'volatile_retarget' else 'load monotonic' if name == 'atomic_retarget' else 'load'
    if name in ('r0_disp_inplace', 'r0_disp_retarget'):
        load = f'{source} = MOVB_load_disp $r4, 15 :: (load (s8) from %ir.p + 15)'
    elif name == 'not_a_load':
        load = f'{source} = MOVri 255'
    else:
        load = f'{source} = MOV{kind}_load_reg $r4 :: ({memory} (s{16 if word else 8}) from %ir.p)'
    if name == 'no_mmo':
        load = load.split(' :: ')[0]
    kill = 'killed ' if copied and name != 'source_not_killed' else ''
    body = [load, f'{dest} = {opcode} {kill}{source}']
    if name == 'unrelated_instructions':
        body += ['$r6 = MOVri 7']
    if name == 'other_use':
        body += [f'MOVL_store_reg {dest}, $r4 :: (store (s32) into %ir.p)']
    if name == 'clobber':
        body += [f'{dest} = MOVri 17']
    if name == 'r16_multiply':
        body += [f'MULU_W {dest}, $r5, implicit-def $macl', '$r3 = STS_MACL implicit $macl']
    if name == 'source_not_killed':
        body += ['$r3 = MOVrr killed $r1']
    if name == 'too_far':
        body += ['$r6 = MOVri 7'] * 17
    shifts = [16] if word or name in ('byte_too_few', 'byte_too_few_poison') else [8] if name == 'word_too_few' else [8, 16] if name == 'byte_order8_16' else [8, 8, 8] if name == 'byte_three8' else [16, 8]
    if name == 'word_too_few':
        shifts = [8]
    for i, amount in enumerate(shifts):
        flags = 'frame-setup ' if name == 'frame_gap' and i == 0 else 'frame-destroy ' if name == 'frame_destroy' and i == 0 else 'nuw nsw ' if name in ('byte_poison_flags', 'byte_too_few_poison') else ''
        body += [f'{dest} = {flags}SHLL{amount} {dest}']
        if i == 0 and name == 'call_gap':
            body += ['JSR $r5, csr_sh, implicit-def $pr']
        if i == 0 and name == 'asm_gap':
            body += ['INLINEASM &"nop", sideeffect attdialect']
    trailing = []
    if name in ('branch_gap', 'delay_gap'):
        # The second shift belongs to another block: no cross-edge proof.
        body = body[:-1]
        body += ['BRA %bb.1'] if name == 'branch_gap' else ['BT_S %bb.1, implicit killed $tbit {\n      NOP\n    }']
        trailing = [f'{dest} = SHLL8 {dest}']
    if name == 't_live':
        trailing += ['$r3 = MOVT implicit killed $tbit']
    if dest != '$r0':
        trailing += [f'$r0 = MOVrr killed {dest}']
    uses = ', implicit $r0'
    if name in ('t_live', 'r16_multiply', 'source_not_killed'):
        uses += ', implicit $r3'
    if name == 'unrelated_instructions':
        uses += ', implicit $r6'
    trailing += ['RTS implicit $pr' + uses]
    print(f'---\nname: {name}\ntracksRegLiveness: {"false" if name == "no_liveness" else "true"}\nnoVRegs: true\nbody: |\n  bb.0:')
    if name in ('branch_gap', 'delay_gap'):
        print('    successors: %bb.1')
    print('    liveins: ' + ', '.join(liveins))
    for line in body:
        print('    ' + line)
    if name in ('branch_gap', 'delay_gap'):
        print('  bb.1:\n    liveins: ' + dest)
    for line in trailing:
        print('    ' + line)
    print('...\n')
    if name in positive:
        print(f'# BASE-LABEL: name: {name}\n# BASE: {opcode}')
        print(f'# OPT-LABEL: name: {name}\n# OPT-NOT: {opcode}')
        if name in ('byte_copy_load', 'word_copy_load'):
            print(f'# OPT: $r2 = MOV{kind}_load_reg\n# OPT-NOT: MOVrr\n# OPT-NOT: {opcode}')
        if name == 'byte_poison_flags':
            print('# BASE: $r0 = nuw nsw SHLL16\n# BASE: $r0 = nuw nsw SHLL8')
            print('# OPT: $r0 = SHLL16\n# OPT: $r0 = SHLL8')
        else:
            print('# OPT: SHLL')
    else:
        print(f'# COMMON-LABEL: name: {name}\n# COMMON: {opcode}')
        if name == 'byte_too_few_poison':
            print('# COMMON: $r0 = nuw nsw SHLL16')
