// RUN: %clang --target=sh-unknown-linux-gnu -m4-nofpu -O2 -fno-pic -fno-pie -fstack-protector-all -S -o - %s | FileCheck %s --check-prefix=STATIC
// RUN: %clang --target=shle-unknown-linux-gnu -m4-nofpu -O2 -fPIC -fstack-protector-all -S -o - %s | FileCheck %s --check-prefix=PIC

__thread int le_tls __attribute__((tls_model("local-exec")));
__thread int ie_tls __attribute__((tls_model("initial-exec")));

__attribute__((interrupt_handler, nosave_low_regs, trap_exit(11))) void advanced(void) {
	volatile char buffer[32];
	buffer[0] = 1;
	++le_tls;
	++ie_tls;
}

// STATIC-LABEL: advanced:
// STATIC: sts.l pr,@-r15
// STATIC: sts.l mach,@-r15
// STATIC: sts.l macl,@-r15
// STATIC-NOT: mov.l r{{[0-7]}},{{.*}}Folded Spill
// STATIC: stc gbr,
// STATIC: lds.l @r15+,macl
// STATIC-NEXT: lds.l @r15+,mach
// STATIC-NEXT: lds.l @r15+,pr
// STATIC-NEXT: trapa #11
// STATIC-NOT: rte
// STATIC-DAG: le_tls@TPOFF
// STATIC-DAG: ie_tls@TPOFF
// STATIC-DAG: __stack_chk_guard
// STATIC-DAG: __stack_chk_fail
// STATIC-NOT: @GOTTPOFF

// PIC-LABEL: advanced:
// PIC: sts.l pr,@-r15
// PIC: sts.l mach,@-r15
// PIC: sts.l macl,@-r15
// PIC-NOT: mov.l r{{[0-7]}},{{.*}}Folded Spill
// PIC: mov.l r12,{{.*}}Folded Spill
// PIC: mova [[GOT:.LCPI[0-9]+_[0-9]+_[0-9]+]],r0
// PIC-NEXT: mov.l [[GOT]],r12
// PIC-NEXT: add r0,r12
// PIC: ie_tls@GOTTPOFF
// PIC: mov.l {{.*}},r12{{.*}}Folded Reload
// PIC: lds.l @r15+,macl
// PIC-NEXT: lds.l @r15+,mach
// PIC-NEXT: lds.l @r15+,pr
// PIC-NEXT: trapa #11
// PIC-NOT: rte
// PIC-DAG: le_tls@TPOFF
// PIC-DAG: __stack_chk_guard@GOT
// PIC-DAG: __stack_chk_fail@PLT
