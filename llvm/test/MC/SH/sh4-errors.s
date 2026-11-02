# RUN: split-file %s %t
# RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -filetype=null %t/invalid.s 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -filetype=null %t/invalid.s 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -filetype=null %t/later.s 2>&1 | FileCheck %s --check-prefix=LATER
# RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -filetype=null %t/later.s 2>&1 | FileCheck %s --check-prefix=LATER

# INVALID-COUNT-3: error: invalid register name
# INVALID-COUNT-11: error: invalid operand for instruction
# LATER-COUNT-5: error: instruction requires an unavailable feature
# LATER: error: unrecognized instruction mnemonic

#--- invalid.s
fadd dr1,dr2
fadd xd3,xd4
fipr fv2,fv4
fadd dr2,fr4
fadd fr2,dr4
fmov xf2,fr3
fmov fr2,xd4
fmov.s dr2,@r3
fmov.d fr2,@r3
flds fpul,fr2
fsts fr2,fpul
fmac fr1,fr2,fr3
ftrv fv0,fv4
movca.l r1,@r2

#--- later.s
fsrra fr0
fsca fpul,dr0
fpchg
synco
movli.l @r1,r0
padd r1,r2
