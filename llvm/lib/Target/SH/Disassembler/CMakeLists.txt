include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_llvm_component_library(LLVMSHDisassembler
  SHDisassembler.cpp

  LINK_COMPONENTS
  MC
  MCDisassembler
  SHDesc
  SHInfo
  Support

  ADD_TO_COMPONENT
  SH
  )

add_dependencies(LLVMSHDisassembler SHCommonTableGen)
