// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include <set>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

#define ANY_POINTER_OPERAND anyOf(declRefExpr(to(valueDecl(hasType(isAnyPointer())).bind("pointer"))),implicitCastExpr(hasSourceExpression(declRefExpr(to(valueDecl(hasType(isAnyPointer())).bind("pointer"))))))

DeclarationMatcher PointerDeclMatcher =
  valueDecl(
	    hasType(isAnyPointer())
	    ).bind("pointerValue");

StatementMatcher ArraySubscriptMatcher =
  arraySubscriptExpr(hasBase(ANY_POINTER_OPERAND)).bind("subscriptExpr");

StatementMatcher PointerArithmeticMatcher =
  anyOf(
    binaryOperator(
      allOf(
        hasLHS(ANY_POINTER_OPERAND),
        hasRHS(binaryOperator(allOf(
          hasAnyOperatorName("+","-","*","/","<<",">>"),
	  hasEitherOperand(ANY_POINTER_OPERAND)))
        )
     )
  ),
    unaryOperator(
      allOf(
        hasAnyOperatorName("++","--"),
        hasUnaryOperand(ANY_POINTER_OPERAND)
      )
    )
);

DeclarationMatcher PointerArrayInitMatcher =
  varDecl(hasInitializer(cxxNewExpr(isArray()))).bind("pointer");

StatementMatcher CallMatcher =
  callExpr(
	   forEachArgumentWithParam(
				    ANY_POINTER_OPERAND,
				    parmVarDecl(hasType(isAnyPointer())).bind("paramPointer")
				    )
	   );

std::set<int64_t> arrayPointers;

class ArraySubscriptIdentifier : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result) override {
    ASTContext *Context = Result.Context;
    const ValueDecl *AD = Result.Nodes.getNodeAs<ValueDecl>("pointer");
    if(!AD || !Context->getSourceManager().isWrittenInMainFile(AD->getLocation()))
      return;

    arrayPointers.insert(AD->getID());
    //llvm::outs() << "Found array subscript: " << AD->getNameAsString() << "\n";
  } 
};

class PointerArithmeticIdentifier : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result) override {
    ASTContext *Context = Result.Context;
    const ValueDecl *AD = Result.Nodes.getNodeAs<ValueDecl>("pointer");
    if(!AD || !Context->getSourceManager().isWrittenInMainFile(AD->getLocation()))
      return;

    arrayPointers.insert(AD->getID());
    //llvm::outs() << "Found pointer arithmetic: " << AD->getNameAsString() << "\n";
  } 
};

class PointerArrayInitIdentifier : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result) override {
    ASTContext *Context = Result.Context;
    const VarDecl *AD = Result.Nodes.getNodeAs<VarDecl>("pointer");
    if(!AD || !Context->getSourceManager().isWrittenInMainFile(AD->getLocation()))
      return;

    arrayPointers.insert(AD->getID());
    llvm::outs() << "Found array init: " << AD->getNameAsString() << "\n";
  } 
};

class CallIdentifier : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result) override {
    const ValueDecl *Arg = Result.Nodes.getNodeAs<ValueDecl>("pointer");
    const ValueDecl *Param = Result.Nodes.getNodeAs<ValueDecl>("paramPointer");
    if(!Arg || !Param)
      return;

    if(arrayPointers.count(Param->getID()) > 0){    
      arrayPointers.insert(Arg->getID());
    }
    if(arrayPointers.count(Arg->getID()) > 0){
      arrayPointers.insert(Param->getID());
    }
    //llvm::outs() << "Found pointer arithmetic: " << AD->getNameAsString() << "\n";
  } 
};

class PointerRewriter : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result) override {
    ASTContext *Context = Result.Context;
    const ValueDecl *VD = Result.Nodes.getNodeAs<ValueDecl>("pointerValue");
    // We do not want to convert header files!
    if (!VD || !Context->getSourceManager().isWrittenInMainFile(VD->getLocation()))
      return;

    if(arrayPointers.count(VD->getID()) > 0){
        llvm::outs() << "Found array pointer: " << VD->getQualifiedNameAsString() << "\n";  
    }else{
        llvm::outs() << "Found singleton pointer: " << VD->getQualifiedNameAsString() << "\n";  
    }
  }
};

int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  // First, identify array and singleton pointers
  ArraySubscriptIdentifier SubID;
  PointerArithmeticIdentifier ArithID;
  PointerArrayInitIdentifier ArrayInitID;
  MatchFinder IdentifyFinder;
  IdentifyFinder.addMatcher(ArraySubscriptMatcher, &SubID);
  IdentifyFinder.addMatcher(PointerArithmeticMatcher, &ArithID);
  IdentifyFinder.addMatcher(PointerArrayInitMatcher, &ArrayInitID);
  Tool.run(newFrontendActionFactory(&IdentifyFinder).get());

  // Next, repeatedly check for pointers used as arguments to array parameters
  size_t oldSize;
  do{
    oldSize = arrayPointers.size();
    CallIdentifier CallID;
    MatchFinder CallFinder;
    CallFinder.addMatcher(CallMatcher, &CallID);
    Tool.run(newFrontendActionFactory(&CallFinder).get());
  }while(oldSize != arrayPointers.size());
  
  // Now, rewrite the pointer types and operations in this file
  PointerRewriter Rewriter;
  MatchFinder RewriteFinder;
  RewriteFinder.addMatcher(PointerDeclMatcher, &Rewriter);
  return Tool.run(newFrontendActionFactory(&RewriteFinder).get());
}
