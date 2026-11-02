# RUN: llvm-mc -triple=sh-unknown-elf -filetype=null %s
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh1 -filetype=null %s
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2 -filetype=null %s
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2e -filetype=null %s
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2a -filetype=null %s
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh3 -filetype=null %s
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh3e -filetype=null %s
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -filetype=null %s
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4a -filetype=null %s
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4al-dsp -filetype=null %s
# RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=misspelled -filetype=null %s 2>&1 | FileCheck %s --check-prefix=MISSPELLED

# MISSPELLED: 'misspelled' is not a recognized processor for this target

nop
