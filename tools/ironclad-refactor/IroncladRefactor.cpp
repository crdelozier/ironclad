#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <sstream>

// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

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

using namespace clang;

class IroncladHelper{
public:
  static std::string buildType(const QualType& qt){
    std::stringstream ss;
    if(qt->isPointerType()){
      const PointerType *pt = dyn_cast<const PointerType>(qt.getTypePtr());
      ss << "ironclad::aptr< " << buildType(pt->getPointeeType()) << " >";
    }else if(qt->isDependentType()){
      // Translate template parameters that are pointers
      // TODO
    }else{
      // Base type, return full qualified name
      ss << qt.getAsString();
    }
    return ss.str();
  }

  static std::string StmtToString(Stmt *S){
    std::string str;
    raw_string_ostream stream(str);
    S->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
    stream.flush();
    return str;
  }

  static std::string OptExprToString(Optional<Expr*> OE){
    std::string str;
    raw_string_ostream stream(str);
    (*OE)->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
    stream.flush();
    return str;
  }
};

class IroncladRefactorVisitor
  : public RecursiveASTVisitor<IroncladRefactorVisitor> {
public:
  explicit IroncladRefactorVisitor(ASTContext *Context, Rewriter *Rewrite)
    : Context(Context), Rewrite(Rewrite) {}

  bool doRewrite(Decl *D){
    return Context->getSourceManager().isWrittenInMainFile(D->getLocation());
  }
  
  bool VisitVarDecl(VarDecl *Var) {
    std::stringstream ss;

    if(doRewrite(Var)){
      if(Var->getType()->isPointerType()){
	ss << IroncladHelper::buildType(Var->getType());
	ss << " ";
	ss << Var->getNameAsString();

	if(Var->hasInit()){
	  Expr *InitExpr = Var->getInit();
	  if(isa<CXXNewExpr>(InitExpr)){
	    CXXNewExpr *NewExpr = dyn_cast<CXXNewExpr>(InitExpr);
	    if(NewExpr->isArray()){
	      Optional<Expr*> ArraySize = NewExpr->getArraySize();
	      if(ArraySize){
		ss << " = ironclad::new_array< " << IroncladHelper::buildType(NewExpr->getAllocatedType())
		   << " >(" << IroncladHelper::OptExprToString(ArraySize) << ")";
	      }else{
		llvm::errs() << "????NO ARRAY SIZE????\n";
	      }
	    }else{

	    }
	  }else{
	    llvm::errs() << Var->getInit()->getStmtClassName() << "\n";
	    //llvm::errs() << IroncladHelper::StmtToString(Var->getInit()) << "\n";
	  }
	}
	
	Rewrite->ReplaceText(Var->getSourceRange(),ss.str());
      }
    }
    
    return true;
  }

private:
  ASTContext *Context;
  Rewriter *Rewrite;
};

class IroncladRefactorConsumer : public clang::ASTConsumer {
public:
  explicit IroncladRefactorConsumer(ASTContext *Context)
    : Rewrite(Context->getSourceManager(),Context->getLangOpts()), Visitor(Context,&Rewrite) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    Rewrite.overwriteChangedFiles();
  }
private:
  Rewriter Rewrite;
  IroncladRefactorVisitor Visitor;
};

class IroncladRefactorAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer (
								clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
    return std::make_unique<IroncladRefactorConsumer>(&Compiler.getASTContext());
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
  
  return Tool.run(newFrontendActionFactory<IroncladRefactorAction>().get());
}
