set(LLVM_LINK_COMPONENTS support)

add_clang_executable(ironclad-refactor
  IroncladRefactor.cpp
  )
target_link_libraries(ironclad-refactor
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  )
