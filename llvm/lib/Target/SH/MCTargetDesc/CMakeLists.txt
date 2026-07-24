include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_llvm_component_library(LLVMSHDesc
  SHAsmBackend.cpp
  SHInstPrinter.cpp
  SHMCAsmInfo.cpp
  SHMCCodeEmitter.cpp
  SHMCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  SHInfo
  Support
  TargetParser

  ADD_TO_COMPONENT
  SH
  )

add_dependencies(LLVMSHDesc SHCommonTableGen)
