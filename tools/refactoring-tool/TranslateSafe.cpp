#include <algorithm>
#include <vector>
#include <sstream>
#include <string>
using namespace std;

#include "AnalysisVisitor.hpp"
#include "Translator.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/DeclarationName.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/AST/TypeLocVisitor.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "ASTVisitor.h"
#include "clang/AST/ParentMap.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Index/DeclReferenceMap.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/ASTConsumers.h"
#include "clang/Rewrite/Rewriter.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/DenseSet.h"

using namespace clang;
using namespace idx;

namespace {

class TranslateSafeConsumer : public ASTConsumer , public ASTVisitor<TranslateSafeConsumer>{

  Rewriter Rewrite;

  Translator * translator;
  AnalysisVisitor * analysis;

  ASTContext *Context;
  SourceManager *SM;
  VarDecl * previousVarDecl;
  Expr * currentLHS;
  string safePtrType;

  std::string InFileName;
  llvm::raw_ostream* OutFile;

  std::vector<FileID> filesToWrite;

public:
  std::vector<std::string> smartPointerTypes;

  virtual void Initialize(ASTContext &context) {
    previousVarDecl = NULL;
    Context = &context;
    SM = &Context->getSourceManager();
    currentLHS = NULL;

    Rewrite.setSourceMgr(Context->getSourceManager(), Context->getLangOptions());

    if(safePtrType.compare("") == 0){
      safePtrType = "aptr";
    }

    analysis = new AnalysisVisitor(SM,Rewrite);
    analysis->inputPointers();
    translator = new Translator(Rewrite,smartPointerTypes,"safe::ptr","safe::aptr",analysis); 
  }

  virtual bool HandleTopLevelDecl(DeclGroupRef D) {
    for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I){
      if(*I){
        HandleTopLevelSingleDecl(*I);
      }
    }
    return true;
  }

  void HandleTopLevelSingleDecl(Decl *D) {
    SourceLocation loc = D->getLocation();
    if(!loc.isInvalid() && !SM->isInSystemHeader(loc)){
      Visit(D);
    }
  }

  void HandleTranslationUnit(ASTContext &C) {
    // Required to get the last set of variable rewrites
    translator->translateLastDeclGroup();
    std::vector<FileID> filesToWrite = translator->getFilesToWrite();

    for(vector<FileID>::iterator i = filesToWrite.begin(), e = filesToWrite.end(); i != e; ++i){
      FileID id = *i;
      string fileName(SM->getBufferName(SM->getLocForStartOfFile(id)));
      if(SM->getLocForStartOfFile(id).isValid()){
	//llvm::outs() << fileName << "\n";
      }
      const RewriteBuffer *FileBuf = Rewrite.getRewriteBufferFor(id);
      if(FileBuf){
	fileName = fileName.append(".out");
	string errors;
	OutFile = new llvm::raw_fd_ostream(fileName.c_str(), errors);
	*OutFile << "#include <safe_cpp.hpp>\n";
	std::string output(FileBuf->begin(), FileBuf->end());
	fixOutput(output);
	*OutFile << output;
	OutFile->flush();
      }
    }
  }

  /*
   * Need to post-process output to fix any instances of new_safe_array() which tend 
   * to accidentally leave brackets, e.g. new_safe_array(new int[30])[30];
   */

  void fixOutput(std::string & output){
    size_t pos = output.find("new_array");
    while(pos != std::string::npos){
      size_t semiPos = output.find(';',pos);
      if(semiPos != std::string::npos){
        pos = output.find_last_of(')',semiPos);
        if(pos+1 < semiPos){
          output.erase(output.begin()+(pos+1),output.begin()+semiPos);
        }
      }
      pos = output.find("new_array",pos);
    }
  }

  void VisitDeclStmt(DeclStmt * declStmt){
    DeclGroupRef group = declStmt->getDeclGroup();
    stringstream ss;

    for(DeclGroupRef::iterator I = group.begin(), E = group.end(); I != E; ++I){
      Decl * decl = *I;
      if(isa<VarDecl>(decl)){
        VarDecl * varDecl = (VarDecl*)decl;
        QualType type = varDecl->getType();
        HandleVariable(varDecl);
        
        type = getDesugaredType(varDecl->getType());
        Type * typePtr = const_cast<Type*>(type.getTypePtr());
        if(TemplateSpecializationType * templateSpecType = dyn_cast<TemplateSpecializationType>(typePtr)){
          translator->translateTemplateArguments(varDecl,templateSpecType);
        }
      }
    }
  }

  void VisitVarDecl(VarDecl * varDecl){
    if(varDecl->hasGlobalStorage() && !varDecl->isStaticLocal()){
      HandleVariable(varDecl);
    }
  }

  void HandleVariable(ValueDecl * varDecl){
    string smartPtrType;
    QualType type = varDecl->getType();
    if(type->isFunctionPointerType()){
      translator->translateFunctionPointer(varDecl,type);
    }else if(type->isPointerType()){
      translator->translatePointer(varDecl,type);
    }else if(type->isArrayType()){
      translator->translatePointer(varDecl,type);
    }else if(translator->isSmartPointerType(type,smartPtrType)){
      translator->translateSmartPointer(varDecl,type,smartPtrType);
    }
  }

  void VisitFunctionDecl(FunctionDecl * functionDecl){
    if(shouldProcessFunction(functionDecl)){
      translator->translateFunction(functionDecl);
    }

    if(shouldProcessFunctionBody(functionDecl)){
      BaseDeclVisitor::VisitFunctionDecl(functionDecl);

      if (functionDecl->isThisDeclarationADefinition()){
        Visit(functionDecl->getBody());
      }
    }

    if(functionDecl->isMain()){
      translator->translateMain(functionDecl);
    }
  }

  bool shouldProcessFunction(FunctionDecl * functionDecl){
    bool shouldProcess = !functionDecl->isMain();

    // getInstantiatedFromMemberFunction tells us whether the function is a template instantiated 
    // function or not
    shouldProcess = shouldProcess && 
      (functionDecl->getInstantiatedFromMemberFunction() == NULL);

    return shouldProcess;
  }

  bool shouldProcessFunctionBody(FunctionDecl * functionDecl){
    return (functionDecl->getInstantiatedFromMemberFunction() == NULL);
  }

  void VisitClassTemplateDecl(ClassTemplateDecl * templateDecl){
    if(templateDecl->getTemplatedDecl()){
      VisitRecordDecl(templateDecl->getTemplatedDecl());
    }
    BaseDeclVisitor::VisitTemplateDecl(templateDecl);
  }

  void VisitRecordDecl(RecordDecl * recordDecl){
    SourceLocation lastLoc;
    bool hasLoc = false;
    for(RecordDecl::field_iterator I = recordDecl->field_begin(), 
	  E = recordDecl->field_end(); I != E; ++I){
      FieldDecl * field = *I;
      QualType type = field->getType();
      HandleVariable(field);
      if(!hasLoc){
        lastLoc = field->getLocStart();
        hasLoc = true;
      }
    }
    BaseDeclVisitor::VisitRecordDecl(recordDecl);
  }

  void VisitCXXDeleteExpr(CXXDeleteExpr * deleteExpr){
    translator->translateDelete(deleteExpr);
  }

  void VisitCallExpr(CallExpr * callExpr){
    Decl * callee = callExpr->getCalleeDecl();
    bool wasTranslated = false;

    if(callee){
      // Handle system function calls that involve pointers
      if(FunctionDecl * calleeDecl = callExpr->getDirectCallee()){
        SourceLocation sourceLoc = calleeDecl->getLocStart();
        if(sourceLoc.isValid() && SM->isInSystemHeader(sourceLoc) && 
	   !shouldIgnoreFunctionCall(callExpr,callee)){
          translator->translateSystemCall(callExpr,calleeDecl);
	  //llvm::outs() << "System Function: " << calleeDecl->getNameAsString() << "\n";
          wasTranslated = true;
        }
      }
    }

    if(!wasTranslated){
      BaseStmtVisitor::VisitCallExpr(callExpr);
    }
  }

  bool shouldIgnoreFunctionCall(CallExpr * callExpr, Decl * callee){
    bool ignore = false;

    if(NamedDecl * namedDecl = dyn_cast<NamedDecl>(callee)){
      string functionName = namedDecl->getNameAsString();

      if(functionName.compare("free") == 0){
        ignore = true;
      }else if(functionName.compare("malloc") == 0){
        ignore = true;
      }else if(functionName.compare("operator<<") == 0){
        ignore = true;
      }
    }

    return ignore;
  }

  void VisitTypedefNameDecl(TypedefNameDecl * typedefDecl){
    QualType type = typedefDecl->getUnderlyingType();
    string smartPtrType;    

    if(type->isPointerType()){
      translator->translatePointerTypedef(typedefDecl,type);
    }else if(translator->isSmartPointerType(type,smartPtrType)){
      translator->translateSmartPointer(typedefDecl,type,smartPtrType);
    }
  }

  void VisitCXXNewExpr(CXXNewExpr * newExpr){
    translator->translateNewExpr(newExpr,currentLHS);
  }

  void VisitBinaryOperator(BinaryOperator * binaryOperator){
    currentLHS = binaryOperator->getLHS();
    BaseStmtVisitor::VisitBinaryOperator(binaryOperator);
  }

  void VisitExplicitCastExpr(ExplicitCastExpr * castExpr){
    QualType type = castExpr->getTypeAsWritten();

    if(type->isPointerType()){
      translator->translateCast(castExpr,type);
    }
  }

  void VisitFriendDecl(FriendDecl * friendDecl){
    NamedDecl * namedDecl = friendDecl->getFriendDecl();
    if(namedDecl){
      if(FunctionDecl * functionDecl = dyn_cast<FunctionDecl>(namedDecl)){
	translator->translateFunction(functionDecl,true);
      }
    }
  }

  QualType getDesugaredType(QualType qualType){
    Type * type = const_cast<Type*>(qualType.getTypePtr());

    if(ElaboratedType * elaboratedType = dyn_cast<ElaboratedType>(type)){
      return getDesugaredType(elaboratedType->desugar());
    }else{
      return qualType;
    }
  }
};

class TranslateSafeAction : public PluginASTAction {
protected:
  TranslateSafeConsumer * consumer;

  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) {
    if(consumer){
      return consumer;
    }else{
      return new TranslateSafeConsumer();
    }
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) {
    if (args.size() && args[0] == "help"){
      PrintHelp(llvm::errs());
    }

    consumer = new TranslateSafeConsumer();

    if(args.size() > 0){
      for(unsigned int c = 0; c < args.size(); c++){
        std::string arg = args[c];
        if(arg.find("-p") != std::string::npos){
          if(c+1 < args.size()){
            consumer->smartPointerTypes.push_back(args[c+1]);
            ++c;
          }else{
            llvm::errs() << "-p must be followed by a smart pointer type string\n";
          }
        }
      }
    }

    return true;
  }
  void PrintHelp(llvm::raw_ostream& ros) {
    ros << "Help for TranslateSafe plugin goes here\n";
  }

};

}

static FrontendPluginRegistry::Add<TranslateSafeAction>
X("translate-safe", "translate safe c++");
