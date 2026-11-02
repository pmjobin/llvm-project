include_directories(
  ${LLVM_MAIN_SRC_DIR}/lib/Target/SH
  ${LLVM_BINARY_DIR}/lib/Target/SH
  )

set(LLVM_LINK_COMPONENTS
  SHCodeGen
  SHDesc
  SHInfo
  CodeGen
  Core
  MC
  Passes
  Support
  Target
  TargetParser
  )

add_llvm_target_unittest(SHTests
  SHRintTest.cpp
  SHInlineAsmSizeTest.cpp
  SHInstrInfoTest.cpp
  )
