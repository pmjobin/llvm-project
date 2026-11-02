# RUN: split-file %s %t
# RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh2 %t/sh3.s 2>&1 | FileCheck %s --check-prefix=SH2
# RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh2 %t/sh3.s 2>&1 | FileCheck %s --check-prefix=SH2
# RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh3 %t/invalid.s 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh3 %t/later.s 2>&1 | FileCheck %s --check-prefix=LATER

# SH2-COUNT-18: error: instruction requires an unavailable feature
# INVALID-COUNT-9: error:
# LATER-COUNT-4: error:

#--- sh3.s
shad r2,r3
shld r2,r3
clrs
sets
ldc r2,ssr
ldc.l @r2+,ssr
stc ssr,r3
stc.l ssr,@-r3
ldc r2,spc
ldc.l @r2+,spc
stc spc,r3
stc.l spc,@-r3
ldc r2,r3_bank
ldc.l @r2+,r3_bank
stc r2_bank,r3
stc.l r2_bank,@-r3
ldtlb
pref @r3

#--- invalid.s
shad ssr,r0
shad r0,ssr
shld r0
ldc r0,r8_bank
ldc r0,r1
ldc.l @-r0,ssr
stc r0_bank,r1_bank
stc.l r0_bank,@r1+
pref r0

#--- later.s
fadd fr1,fr2
movi20 #1,r1
ocbi @r1
padd r1,r2
