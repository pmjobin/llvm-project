add_llvm_component_group(SH)

set(LLVM_TARGET_DEFINITIONS SH.td)

tablegen(LLVM SHGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM SHGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM SHGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM SHGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM SHGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM SHGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM SHGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(SHCommonTableGen)

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
