include_directories(
  ${LLVM_MAIN_SRC_DIR}/lib/Target/SH
  ${LLVM_BINARY_DIR}/lib/Target/SH
  )

set(LLVM_LINK_COMPONENTS
  CodeGen
  Core
  MC
  Passes
  SHCodeGen
  SHDesc
  SHInfo
  Support
  Target
  TargetParser
  )

add_llvm_target_unittest(SHTests
  SHInstrInfoTest.cpp
  )
