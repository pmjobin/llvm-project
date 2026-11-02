#!/usr/bin/env python3
"""T is a shared physical bit: every alternate consumer retains its definition."""
positive = ['direct', 'identity_copy', 'boolean_extension', 'other_boolean_use']
negative = ['dt', 'tst', 'cmp', 'addc', 'subc', 'rotcl', 'sett', 'clrt',
            'call', 'asm', 'boolean_overwritten', 'successor_t', 'later_movt',
            'later_addc', 'delayed_branch', 'no_liveness', 'sr_write', 'shift_t',
            'asm_after_test', 'join']
print('--- |\n  declare void @callee()')
for name in positive + negative:
    print('  define void @' + name + '(i32 %x, i32 %y, ptr %p) { ret void }')
print('...')
for name in positive + negative:
    if name == 'join':
        print('''---
name: join
tracksRegLiveness: true
noVRegs: true
body: |
  bb.0:
    successors: %bb.1, %bb.2
    liveins: $r4, $r5
    CMP_EQ $r4, $r5, implicit-def $tbit
    $r1 = MOVT implicit $tbit
    BT %bb.2, implicit killed $tbit
  bb.1:
    successors: %bb.2
    liveins: $r1, $r4, $r5
    CMP_HI $r4, $r5, implicit-def dead $tbit
    BRA %bb.2
  bb.2:
    successors: %bb.3, %bb.4
    liveins: $r1
    TST killed $r1, $r1, implicit-def $tbit
    BT %bb.4, implicit killed $tbit
  bb.3:
    RTS implicit $pr
  bb.4:
    RTS implicit $pr
...
# COMMON-LABEL: name: join
# COMMON: bb.2:
# COMMON: TST
# COMMON: BT %bb.4
''')
        continue
    live = '$r4, $r5, $r6, $r7'
    before = ['CMP_EQ $r4, $r5, implicit-def $tbit', '$r1 = MOVT implicit killed $tbit']
    reg = '$r1'
    if name == 'identity_copy':
        before.append('$r2 = COPY killed $r1')
        reg = '$r2'
    if name == 'boolean_extension':
        before.append('$r2 = EXTUB killed $r1')
        reg = '$r2'
    barriers = {
        'dt': '$r6 = DT $r6, implicit-def $tbit',
        'tst': 'TST $r6, $r7, implicit-def $tbit',
        'cmp': 'CMP_HI $r6, $r7, implicit-def $tbit',
        'addc': '$r6 = ADDC $r6, $r7, implicit-def $tbit, implicit $tbit',
        'subc': '$r6 = SUBC $r6, $r7, implicit-def $tbit, implicit $tbit',
        'rotcl': '$r6 = ROTCL $r6, implicit-def $tbit, implicit $tbit',
        'sett': 'SETT implicit-def $tbit',
        'clrt': 'CLRT implicit-def $tbit',
        'call': 'JSR $r4, csr_sh, implicit-def $pr',
        'asm': 'INLINEASM &"nop", sideeffect attdialect',
        'boolean_overwritten': '$r1 = MOVri 2',
        'sr_write': 'LDC_SR $r6, implicit-def $sr, implicit-def $sbit, implicit-def $mbit, implicit-def $qbit, implicit-def $tbit',
        'shift_t': '$r6 = SHLR $r6, implicit-def $tbit',
    }
    if name in barriers:
        before.append(barriers[name])
    if name in ('addc', 'subc', 'rotcl'):
        before[1] = '$r1 = MOVT implicit $tbit'
    if name == 'call':
        # The boolean must survive the actual GNU call-clobber mask.
        before[1] = '$r8 = MOVT implicit killed $tbit'
        reg = '$r8'
    delayed = name == 'delayed_branch'
    branch = 'BT_S %bb.2, implicit $tbit {\n      NOP\n    }' if delayed else 'BT %bb.2, implicit $tbit'
    # A visible store must keep the materialized boolean even after folding.
    if name == 'other_boolean_use':
        before.append('MOVL_store_reg $r1, $r6 :: (store (s32) into %ir.p)')
    body = before + [f'TST {reg}, {reg}, implicit-def $tbit', branch]
    if name == 'asm_after_test':
        body.insert(-1, 'INLINEASM &"nop", sideeffect attdialect')
    # An unequal predecessor T definition makes this a no-cross-edge witness.
    if name == 'join':
        body = ['CMP_EQ $r4, $r5, implicit-def $tbit', 'BT %bb.2, implicit $tbit']
    print(f'---\nname: {name}\ntracksRegLiveness: {"false" if name == "no_liveness" else "true"}\nnoVRegs: true\nbody: |\n  bb.0:\n    successors: %bb.1, %bb.2\n    liveins: {live}\n')
    for line in body:
        print('    ' + line)
    print('  bb.1:\n    successors: %bb.2')
    if name == 'join':
        print('    liveins: $r4, $r5\n    CMP_HI $r4, $r5, implicit-def $tbit')
    elif name == 'later_movt':
        print('    liveins: $tbit\n    $r2 = MOVT implicit killed $tbit')
    elif name == 'later_addc':
        print('    liveins: $tbit, $r6, $r7\n    $r6 = ADDC $r6, $r7, implicit-def dead $tbit, implicit killed $tbit')
    print('    BRA %bb.2')
    print('  bb.2:')
    if name == 'successor_t':
        print('    liveins: $tbit\n    $r0 = MOVT implicit killed $tbit')
    elif name == 'join':
        print('    liveins: $tbit\n    $r1 = MOVT implicit killed $tbit\n    TST $r1, $r1, implicit-def $tbit')
    print('    RTS implicit $pr\n...\n')
    if name in positive:
        print(f'# BASE-LABEL: name: {name}\n# BASE: TST\n# BASE: BT %bb.2')
        print(f'# OPT-LABEL: name: {name}\n# OPT-NOT: TST\n# OPT: BF %bb.2')
    elif name != 'join':
        print(f'# COMMON-LABEL: name: {name}\n# COMMON: TST\n# COMMON: {"BT_S" if delayed else "BT"} %bb.2')
    else:
        print('# COMMON-LABEL: name: join\n# COMMON: bb.2:\n# COMMON: TST')
