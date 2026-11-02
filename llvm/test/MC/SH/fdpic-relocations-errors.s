! RUN: split-file %s %t
! RUN: not llvm-mc -triple=sh2-linux-musl -mcpu=sh2a -filetype=obj %t/ordinary.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=ORDINARY
! RUN: not llvm-mc -triple=sh2-linux-muslfdpic -mcpu=sh2a -filetype=obj %t/addend.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDEND
! RUN: not llvm-mc -triple=sh2-linux-muslfdpic -mcpu=sh2a -filetype=obj %t/width.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=WIDTH
! RUN: not llvm-mc -triple=sh2-linux-muslfdpic -mcpu=sh2a -filetype=obj %t/subtraction.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SUBTRACTION
! RUN: not llvm-mc -triple=sh2-linux-muslfdpic -mcpu=sh2a -filetype=obj %t/plain-movi20.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=PLAIN-MOVI20
! RUN: not llvm-mc -triple=sh2-linux-muslfdpic -mcpu=sh2a -filetype=obj %t/movi20-addend.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=MOVI20-ADDEND
! RUN: not llvm-mc -triple=sh2-linux-muslfdpic -mcpu=sh2a -filetype=obj %t/movi20-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=MOVI20-OVERFLOW

! ORDINARY: error: SH function-descriptor relocation requires FDPIC mode
! ADDEND: error: SH function-descriptor relocations do not support addends
! WIDTH: error: SH GOT and PLT modifiers require a four-byte value
! SUBTRACTION: error: SH GOT and PLT modifiers do not support symbol subtraction
! PLAIN-MOVI20: error: SH movi20 relocation requires @GOT, @GOTOFF, @GOTFUNCDESC, or @GOTOFFFUNCDESC
! MOVI20-ADDEND: error: only an SH movi20 @GOTOFF relocation supports an addend
! MOVI20-OVERFLOW: error: SH movi20 relocation addend is out of signed 20-bit range

!--- ordinary.s
	.long function@FUNCDESC

!--- addend.s
	.long function@GOTOFFFUNCDESC+4

!--- width.s
	.short function@GOTOFFFUNCDESC

!--- subtraction.s
	.long function@FUNCDESC-other

!--- plain-movi20.s
	.text
.Lplain:
	movi20 #.Lplain,r0

!--- movi20-addend.s
	movi20 #function@GOTFUNCDESC+4,r0

!--- movi20-overflow.s
	movi20 #object@GOTOFF+524288,r0
