include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_llvm_component_library(LLVMSHAsmParser
  SHAsmParser.cpp

  LINK_COMPONENTS
  MC
  MCParser
  SHDesc
  SHInfo
  Support

  ADD_TO_COMPONENT
  SH
  )

add_dependencies(LLVMSHAsmParser SHCommonTableGen)
