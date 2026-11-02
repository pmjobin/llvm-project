; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s
; CHECK: error: SH inline assembly size cannot be determined: unknown instruction or macro invocation 'invalid_sh_assembly_mnemonic'
define void @invalid_after_comment() {
  call void asm sideeffect "# accepted comment\0A\09invalid_sh_assembly_mnemonic", ""()
  ret void
}
