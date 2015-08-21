set(MODULE TRUE)

set( LLVM_USED_LIBS
  clangFrontend
  clangAST
  )

set( LLVM_LINK_COMPONENTS support mc)

add_clang_library(TranslateSafe TranslateSafe.cpp Translator.cpp)

set_target_properties(TranslateSafe
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "")
