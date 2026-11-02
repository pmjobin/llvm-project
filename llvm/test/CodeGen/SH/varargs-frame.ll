; Current LLVM requires frontends to expand aggregate va_arg.
; The original cases now use legal per-endian IR in Inputs; scalar va_arg
; instructions and ABI/stack checks are retained. No verifier is disabled here.
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -stack-symbol-ordering=0 -verify-machineinstrs -stop-after=prolog-epilog %S/Inputs/varargs-frame-be.ll -o - | FileCheck %s --check-prefix=PEI
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -stack-symbol-ordering=0 -verify-machineinstrs -stop-after=prolog-epilog %S/Inputs/varargs-frame-le.ll -o - | FileCheck %s --check-prefix=PEI
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -stack-symbol-ordering=0 -verify-machineinstrs -asm-verbose=false %S/Inputs/varargs-frame-be.ll -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -stack-symbol-ordering=0 -verify-machineinstrs -asm-verbose=false %S/Inputs/varargs-frame-le.ll -o - | FileCheck %s --check-prefix=ASM
; PEI-LABEL: name:            nonleaf_save12
; PEI: stackSize:       20
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -16, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -12, size: 12, alignment: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -12
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -4
; PEI: JSR
; PEI-NEXT: $r15 = ADDri $r15, 4
; PEI: $r15 = frame-destroy ADDri $r15, 4
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 12
;
; ASM-LABEL: nonleaf_save12:
; ASM-NEXT: add	#-12,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-4,r15
; ASM: mov.l	r7,@(16,r15)
; ASM: mov.l	r6,@(12,r15)
; ASM: mov.l	r0,@(8,r15)
; ASM: add	#-4,r15
; ASM: jsr
; ASM-NEXT: {{nop|mov}}
; ASM-NEXT: add	#4,r15
; ASM: add	#4,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
; ASM-NEXT: rts
; ASM-NEXT: nop
; PEI-LABEL: name:            nonleaf_save8
; PEI: stackSize:       16
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -12, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -8, size: 8, alignment: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -8
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -4
; PEI: JSR
; PEI-NEXT: $r15 = ADDri $r15, 4
; PEI: $r15 = frame-destroy ADDri $r15, 4
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 8
;
; ASM-LABEL: nonleaf_save8:
; ASM-NEXT: add	#-8,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-4,r15
; ASM: mov.l	r7,@(12,r15)
; ASM: mov.l	r0,@(8,r15)
; ASM: add	#-4,r15
; ASM: jsr
; ASM-NEXT: {{nop|mov}}
; ASM-NEXT: add	#4,r15
; ASM: add	#4,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#8,r15
; ASM-NEXT: rts
; ASM-NEXT: nop
; PEI-LABEL: name:            nonleaf_save4
; PEI: stackSize:       12
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -8, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -4, size: 4, alignment: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -4
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -4
; PEI: JSR
; PEI-NEXT: $r15 = ADDri $r15, 4
; PEI: $r15 = frame-destroy ADDri $r15, 4
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 4
;
; ASM-LABEL: nonleaf_save4:
; ASM-NEXT: add	#-4,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-4,r15
; ASM: mov.l	r0,@(8,r15)
; ASM: add	#-4,r15
; ASM: jsr
; ASM-NEXT: {{nop|mov}}
; ASM-NEXT: add	#4,r15
; ASM: add	#4,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#4,r15
; ASM-NEXT: rts
; ASM-NEXT: nop
; PEI-LABEL: name:            nonleaf_save0
; PEI: stackSize:       8
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -4, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: 0, size: 4, alignment: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -4
; PEI: JSR
; PEI-NEXT: $r15 = ADDri $r15, 4
; PEI: $r15 = frame-destroy ADDri $r15, 4
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: RTS
;
; ASM-LABEL: nonleaf_save0:
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-4,r15
; ASM: add	#-4,r15
; ASM: jsr
; ASM-NEXT: {{nop|mov}}
; ASM-NEXT: add	#4,r15
; ASM: add	#4,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: rts
; ASM-NEXT: add	r1,r0
; PEI-LABEL: name:            leaf_save12
; PEI: stackSize:       16
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: default, offset: -12, size: 12, alignment: 4
; PEI-LABEL: body:
; PEI-NOT: STS_L_PR
; PEI-NOT: LDS_L_PR
; PEI: $r15 = frame-setup ADDri $r15, -16
; PEI: $r15 = frame-destroy ADDri $r15, 16
;
; ASM-LABEL: leaf_save12:
; ASM-NOT: sts.l
; ASM: add	#-16,r15
; ASM: mov.l	r7,@(12,r15)
; ASM: mov.l	r6,@(8,r15)
; ASM: mov.l	r0,@(4,r15)
; ASM-NOT: lds.l
; ASM: add	#16,r15
; ASM-NEXT: rts
; PEI-LABEL: name:            leaf_save0
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: default, offset: 0, size: 4, alignment: 4
; PEI-LABEL: body:
; PEI-NOT: STS_L_PR
; PEI-NOT: LDS_L_PR
;
; ASM-LABEL: leaf_save0:
; ASM-NOT: sts.l
; ASM-NOT: lds.l
; ASM: rts
; PEI-LABEL: name:            nonleaf_pressure
; PEI: stackSize:       60
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -16, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -12, size: 12, alignment: 4
; PEI: stack:
; PEI: { id: 0, name: slot, type: default, offset: -48, size: 4
; PEI: { id: 1, name: ap, type: default, offset: -52, size: 4
; PEI: { id: 2, name: '', type: spill-slot, offset: -56, size: 4
; PEI-DAG: callee-saved-register: '$r8'
; PEI-DAG: callee-saved-register: '$r9'
; PEI-DAG: callee-saved-register: '$r10'
; PEI-DAG: callee-saved-register: '$r11'
; PEI-DAG: callee-saved-register: '$r12'
; PEI-DAG: callee-saved-register: '$r13'
; PEI-DAG: callee-saved-register: '$r14'
; PEI: { id: 10, name: '', type: spill-slot, offset: -60, size: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -12
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r8, $r15
; PEI-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r9, $r15
; PEI-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r10, $r15
; PEI-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r11, $r15
; PEI-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r12, $r15
; PEI-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r13, $r15
; PEI-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r14, $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -16
; PEI: MOVL_store_disp killed $r0, $r15, 4 :: (store (s32) into %stack.2)
; PEI: $r15 = ADDri $r15, -16
; PEI: JSR
; PEI-NEXT: $r15 = ADDri $r15, 16
; PEI: $r15 = frame-destroy ADDri $r15, 16
; PEI-NEXT: early-clobber $r14, $r15 = frame-destroy MOVL_load_postinc $r15
; PEI: early-clobber $r8, $r15 = frame-destroy MOVL_load_postinc $r15
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 12
;
; ASM-LABEL: nonleaf_pressure:
; ASM-NEXT: add	#-12,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: mov.l	r8,@-r15
; ASM: mov.l	r14,@-r15
; ASM-NEXT: add	#-16,r15
; ASM: mov.l	r7,@(56,r15)
; ASM: mov.l	r6,@(52,r15)
; ASM: mov.l	r5,@(48,r15)
; ASM: mov.l	{{.*}},@(4,r15)
; ASM: add	#-16,r15
; ASM: jsr
; ASM-NEXT: {{nop|mov}}
; ASM-NEXT: add	#16,r15
; ASM: add	#16,r15
; ASM-NEXT: mov.l	@r15+,r14
; ASM: mov.l	@r15+,r8
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
; ASM-NEXT: rts
; PEI-LABEL: name:            nonleaf_frame_limit
; PEI: stackSize:       64
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -16, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -12, size: 12, alignment: 4
; PEI: stack:
; PEI: { id: 0, name: locals, type: default, offset: -56, size: 40
; PEI: { id: 1, name: ap, type: default, offset: -60, size: 4
; PEI: { id: 2, name: '', type: spill-slot, offset: -64, size: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -12
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -48
; PEI: $r15 = frame-destroy ADDri $r15, 48
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 12
;
; ASM-LABEL: nonleaf_frame_limit:
; ASM-NEXT: add	#-12,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-48,r15
; ASM: add	#48,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
; ASM-NEXT: rts
; PEI-LABEL: name:            nonleaf_i64_return
; PEI-LABEL: body:
; PEI: JSR
; PEI: $r15 = frame-destroy LDS_L_PR $r15
; PEI: RTS implicit $pr, implicit $r0, implicit $r1
;
; ASM-LABEL: nonleaf_i64_return:
; ASM: jsr
; ASM-NEXT: {{nop|mov}}
; ASM: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
; ASM-NEXT: rts
; PEI-LABEL: name:            nonleaf_aggregate_return
; PEI-LABEL: body:
; PEI: JSR
; PEI: $r1 = MOVL_load_reg $r15
; PEI-NEXT: renamable $r2 = COPY renamable $r1
; PEI-NEXT: $r2 = ADDri killed $r2, 4
; PEI-NEXT: MOVL_store_reg killed $r2, $r15
; PEI-NEXT: $r1 = MOVL_load_reg killed $r1
; PEI: $r15 = frame-destroy LDS_L_PR $r15
; PEI: RTS implicit $pr, implicit $r0, implicit $r1
;
; ASM-LABEL: nonleaf_aggregate_return:
; ASM: jsr
; ASM-NEXT: {{nop|mov}}
; ASM: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
; ASM-NEXT: rts
; PEI-LABEL: name:            nonleaf_named_byval
; PEI: stackSize:       24
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -12, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -8, size: 8, alignment: 4
; PEI: stack:
; PEI: { id: 0, name: ap, type: default, offset: -16, size: 4
; PEI: { id: 1, name: '', type: default, offset: -24, size: 8
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -8
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -12
; PEI: MOVL_store_disp killed $r5, $r15, 4 :: (store (s32) into %ir.named + 4)
; PEI-NEXT: MOVL_store_reg killed $r4, $r15 :: (store (s32) into %ir.named)
; PEI: JSR
; PEI: MOVB_load_frame $r15, 7
; PEI: $r15 = frame-destroy ADDri $r15, 12
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 8
;
; ASM-LABEL: nonleaf_named_byval:
; ASM-NEXT: add	#-8,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-12,r15
; ASM: mov.l	r5,@(4,r15)
; ASM-NEXT: mov.l	r4,@r15
; ASM: jsr
; ASM-NEXT: {{nop|mov}}
; ASM: mov.b	@(7,r15),r{{[0-9]+}}
; ASM: add	#12,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#8,r15
; PEI-LABEL: name:            nonleaf_packed_temporary
; PEI: stackSize:       40
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -16, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: -12, size: 12, alignment: 4
; PEI: stack:
; PEI: { id: 0, name: vaarg.tmp{{[0-9]*}}, type: default, offset: -24, size: 5, alignment: 4
; PEI: { id: 1, name: vaarg.tmp, type: default, offset: -32, size: 5, alignment: 4
; PEI: { id: 2, name: ap, type: default, offset: -36, size: 4, alignment: 4
; PEI: { id: 3, name: '', type: spill-slot, offset: -40, size: 4, alignment: 4
; PEI-LABEL: body:
; PEI: $r15 = frame-setup ADDri $r15, -12
; PEI-NEXT: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -24
; PEI: JSR
; PEI: $r15 = frame-destroy ADDri $r15, 24
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: $r15 = frame-destroy ADDri $r15, 12
;
; ASM-LABEL: nonleaf_packed_temporary:
; ASM-NEXT: add	#-12,r15
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-24,r15
; ASM: jsr
; ASM-NEXT: {{nop|mov}}
; ASM: add	#24,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: add	#12,r15
; PEI-LABEL: name:            nonleaf_named_byval_split
; PEI: stackSize:       16
; PEI: fixedStack:
; PEI-NEXT: - { id: 0, type: spill-slot, offset: -4, size: 4, alignment: 4
; PEI: - { id: 1, type: default, offset: 4, size: 4, alignment: 4
; PEI: - { id: 2, type: default, offset: 0, size: 4, alignment: 4
; PEI: stack:
; PEI: { id: 0, name: ap, type: default, offset: -8, size: 4
; PEI: { id: 1, name: '', type: default, offset: -16, size: 8
; PEI-LABEL: body:
; PEI: $r15 = frame-setup STS_L_PR $r15
; PEI-NEXT: $r15 = frame-setup ADDri $r15, -12
; PEI: $[[BYVAL:r[0-9]+]] = MOVL_load_disp $r15, 16 :: (load (s32) from %fixed-stack.2)
; PEI-DAG: MOVL_store_disp killed $[[BYVAL]], $r15, 4 :: (store (s32) into %ir.named + 4)
; PEI-DAG: MOVL_store_reg killed $r7, $r15 :: (store (s32) into %ir.named)
; PEI: JSR
; PEI: MOVB_load_frame $r15, 7
; PEI: $r15 = frame-destroy ADDri $r15, 12
; PEI-NEXT: $r15 = frame-destroy LDS_L_PR $r15
; PEI-NEXT: RTS
;
; ASM-LABEL: nonleaf_named_byval_split:
; ASM-NEXT: sts.l	pr,@-r15
; ASM-NEXT: add	#-12,r15
; ASM: mov.l	@(16,r15),r{{[0-9]+}}
; ASM-DAG: mov.l	{{.*}},@(4,r15)
; ASM-DAG: mov.l	r7,@r15
; ASM: jsr
; ASM-NEXT: {{nop|mov}}
; ASM: mov.b	@(7,r15),r{{[0-9]+}}
; ASM: add	#12,r15
; ASM-NEXT: lds.l	@r15+,pr
; ASM-NEXT: rts
