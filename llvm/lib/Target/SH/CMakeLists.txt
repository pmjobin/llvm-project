add_llvm_component_group(SH)

set(LLVM_TARGET_DEFINITIONS SH.td)

tablegen(LLVM SHGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM SHGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM SHGenCallingConv.inc -gen-callingconv)
tablegen(LLVM SHGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM SHGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM SHGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM SHGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM SHGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM SHGenSDNodeInfo.inc -gen-sd-node-info)
tablegen(LLVM SHGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(SHCommonTableGen)

add_llvm_target(SHCodeGen
  SHAsmPrinter.cpp
  SHBranchRange.cpp
  SHCodeGenPassBuilder.cpp
  SHConstantPoolValue.cpp
  SHDelaySlotFiller.cpp
  SHFrameLowering.cpp
  SHFPUState.cpp
  SHISelDAGToDAG.cpp
  SHISelLowering.cpp
  SHWideShift.cpp
  SHIRValidation.cpp
  SHInlineAsmSize.cpp
  SHGraphics.cpp
  SHInstrInfo.cpp
  SHJumpTable.cpp
  SHLiteralIsland.cpp
  SHLiteralPool.cpp
  SHLiteralPoolRangeCheck.cpp
  SHLoweringPolicy.cpp
  SHPrepareIR.cpp
  SHPostRAPeephole.cpp
  SHMCInstLower.cpp
  SHRegisterInfo.cpp
  SHSelectionDAGInfo.cpp
  SHSubtarget.cpp
  SHTargetMachine.cpp
  SHTargetTransformInfo.cpp

  LINK_COMPONENTS
  AsmPrinter
  Analysis
  CodeGen
  CodeGenTypes
  Core
  IPO
  MC
  MCParser
  Passes
  SelectionDAG
  SHDesc
  SHInfo
  Support
  Target
  TargetParser
  TransformUtils

  ADD_TO_COMPONENT
  SH
  )

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
