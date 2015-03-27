set(MODULE TRUE)

set( LLVM_USED_LIBS
  clangFrontend
  clangAST
  )

set( LLVM_LINK_COMPONENTS support mc)

add_clang_library(validate Validate.cpp)

set_target_properties(validate
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "")
