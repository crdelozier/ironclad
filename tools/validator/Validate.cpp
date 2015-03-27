#include <algorithm>
#include <vector>
#include <sstream>
#include <string>

#include "Util.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include "clang/Frontend/FrontendPluginRegistry.h"
#include <clang/Tooling/Tooling.h>

using namespace clang;
using namespace std;

class ValidateVisitor : public RecursiveASTVisitor<ValidateVisitor>{
  ASTContext *Context;
  SourceManager *SM;

  std::vector<std::string> warnings;

public:
  static const int TOTAL_SYSTEM_FILES = 16;

  ValidateVisitor(ASTContext *Context) : Context(Context), SM(&Context->getSourceManager()) {
  }

  virtual bool HandleTopLevelDecl(DeclGroupRef D) {
    llvm::outs() << "Here\n";
    for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I){
      if(*I){
        HandleTopLevelSingleDecl(*I);
      }
    }
    return true;
  }

  void HandleTopLevelSingleDecl(Decl *D) {
    SourceLocation loc = D->getLocation();
    if(!loc.isInvalid() && !SM->isInSystemHeader(loc) && processFile(SM->getBufferName(loc))){
      VisitDecl(D);
    }
  }

  const std::string systemFiles [TOTAL_SYSTEM_FILES] = {
    "ptr.hpp",
    "aptr.hpp",
    "array.hpp",
    "util.hpp",
    "ref.hpp",
    "safe_debug.hpp",
    "range_table/LockTree.hpp",
    "range_table/LockTree.cpp",
    "safe_string.hpp",
    "safe_mem.hpp",
    "gc_allocator.h",
    "gc.h",
    "co_iterator.hpp",
    "matrix.hpp",
    "Collector.hpp",
    "addr.c",
  };

  bool processFile(std::string fileName){
    for(int c = 0; c < TOTAL_SYSTEM_FILES; ++c){
      if(fileName.find(systemFiles[c]) != std::string::npos){
	return false;
      }
    }

    return true;
  }

  void addWarning(std::string warning){
    warnings.push_back(warning);
  }

  std::string getSrcLocation(SourceLocation loc){
    stringstream ss;
    ss << SM->getSpellingLineNumber(loc);
    ss << ":" << SM->getSpellingColumnNumber(loc);
    return ss.str();
  }

  std::string getSrcFileName(SourceLocation loc){
    return SM->getBufferName(loc);
  }

  std::string getDeclIdentifier(Decl * decl){
    stringstream ss;
    if(NamedDecl * namedDecl = dyn_cast<NamedDecl>(decl)){
      ss << "\"" << namedDecl->getQualifiedNameAsString() << "\" ";
    }
    ss << "at line " << getSrcLocation(decl->getLocStart());
    ss << " in file " << getSrcFileName(decl->getLocStart());
    return ss.str();
  }

  std::string getStmtIdentifier(Stmt * stmt){
    stringstream ss;
    ss << "at line " << getSrcLocation(stmt->getLocStart());
    ss << " in file " << getSrcFileName(stmt->getLocStart());
    ss << "\n\n";
    // TODO: Find another way to output the statement
    // ss << " in statement: " << Rewrite.ConvertToString(stmt) << "";
    return ss.str();
  }

  bool isSafeCPP(NamedDecl * namedDecl){
    if(namedDecl){
      DeclContext * context = namedDecl->getLexicalDeclContext();

      do{
	if(context->isTranslationUnit()){
	  break;
	}
	if(NamespaceDecl * namespaceDecl = dyn_cast<NamespaceDecl>(context)){
	  std::string name = namespaceDecl->getNameAsString();
	  if(name.compare("safe") == 0){
	    return true;
	  }
	}
      }while((context = context->getParent()) != NULL);
    }
    return false;
  }

  /*
   * Use this to remove temporary and constructor wrapped around address
   * of pointer arguments
   */

  Expr * stripTemporary(Expr * expr){
    expr = expr->IgnoreParenCasts();
    bool stripped;
    do{
      stripped = false;
      if(CXXBindTemporaryExpr * bindTemp = dyn_cast<CXXBindTemporaryExpr>(expr)){
	expr = bindTemp->getSubExpr();
	stripped = true;
      }
      expr = expr->IgnoreParenCasts();
      if(CXXConstructExpr * constructExpr = dyn_cast<CXXConstructExpr>(expr)){
	if(constructExpr->getNumArgs() > 0){
	  expr = constructExpr->getArg(0);
	  stripped = true;
	}
      }
      expr = expr->IgnoreParenCasts();
    }while(stripped);
    return expr;
  }

  bool isAddressOf(Expr * expr){
    expr = stripTemporary(expr);
    if(UnaryOperator * unaryOp = dyn_cast<UnaryOperator>(expr)){
      if(unaryOp->getOpcode() == UO_AddrOf){
	return true;
      }
    }
    return false;
  }

  NamedDecl * extractAddressOf(Expr * expr){
    expr = stripTemporary(expr);
    if(UnaryOperator * unaryOp = dyn_cast<UnaryOperator>(expr)){
      if(unaryOp->getOpcode() == UO_AddrOf){
	return extractDecl(unaryOp->getSubExpr());
      }
    }
    return NULL;
  }

  bool isThis(Expr * expr){
    expr = stripTemporary(expr);
    return isa<CXXThisExpr>(expr);
  }

  bool isArray(Expr * expr){
    expr = stripTemporary(expr);
    if(DeclRefExpr * refExpr = dyn_cast<DeclRefExpr>(expr)){
      ValueDecl * valueDecl = refExpr->getDecl();
      if(valueDecl){
	QualType type = valueDecl->getType();
	std::string typeString = type.getAsString();
	if(typeString.find("safe::array") != std::string::npos){
	  return true;
	}
      }
    }
    return false;
  }

  bool VisitDeclStmt(DeclStmt * declStmt){
    DeclGroupRef group = declStmt->getDeclGroup();
    stringstream ss;

    for(DeclGroupRef::iterator I = group.begin(), E = group.end(); I != E; ++I){
      Decl * decl = *I;
      if(isa<ValueDecl>(decl)){
        ValueDecl * valueDecl = (ValueDecl*)decl;
	HandleVariable(valueDecl);
      }
    }

    return true;
  }

  bool VisitVarDecl(VarDecl * varDecl){
    HandleVariable(varDecl);
    return true;
  }

  bool isArgv(ValueDecl * valueDecl){
    std::string name = valueDecl->getNameAsString();
    return name.compare("argv") == 0;
  }

  void HandleVariable(ValueDecl * valueDecl){
    QualType type = valueDecl->getType();
    if(type->isFunctionPointerType()){
      stringstream ss;
      ss << "Cannot declare function pointer " << getDeclIdentifier(valueDecl);
      //addWarning(ss.str());
    }else if(type->isPointerType()){
      if(!isArgv(valueDecl)){
	stringstream ss;
	ss << "Cannot declare bare pointer " << getDeclIdentifier(valueDecl);
	addWarning(ss.str());
      }
    }else if(type->isArrayType()){
      stringstream ss;
      ss << "Cannot declare array " << getDeclIdentifier(valueDecl);
      addWarning(ss.str());
    }else{
      if(VarDecl * varDecl = dyn_cast<VarDecl>(valueDecl)){
	if(varDecl->hasInit()){
	  Expr * init = varDecl->getInit();
	  if(init){
	    if(isAddressOf(init)){
	      if(isLocalScoped(varDecl)){
		checkLocalPointer(varDecl,init);
	      }
	    }
	  }
	}
      }
    }
  }

  bool VisitRecordDecl(RecordDecl * recordDecl){
    for(RecordDecl::field_iterator I = recordDecl->field_begin(), E = recordDecl->field_end(); I != E; ++I){
      FieldDecl * field = *I;
      HandleVariable(field);
    }
    return true;
  }

  bool VisitCallExpr(CallExpr * callExpr){
    FunctionDecl * calleeDecl = callExpr->getDirectCallee();

    if(calleeDecl){
      if(!isSafeCPP(calleeDecl)){
      std::string name = calleeDecl->getNameAsString();
      if(name.compare("malloc") == 0 || 
	 name.compare("free") == 0 ||
	 name.compare("calloc") == 0 ||
	 name.compare("realloc") == 0 ||
         name.compare("gets") == 0 ||
	 name.compare("memset") == 0 ||
	 name.compare("memcpy") == 0 ||
	 name.compare("memmove") == 0
	 ){
	if(SM->isInSystemHeader(calleeDecl->getLocStart())){
	  stringstream ss;
	  ss << "Use of function \"" << name << "\" is not permitted ";
	  ss << getStmtIdentifier(callExpr);
	  addWarning(ss.str());
	}
      }

      if(!SM->isInSystemHeader(calleeDecl->getLocStart())){
	for(unsigned int c = 0; c < callExpr->getNumArgs(); c++){
	  Expr * arg = callExpr->getArg(c);
	  if(isAddressOf(arg)){
	    if(c < calleeDecl->getNumParams()){
	      NamedDecl * namedDecl = extractAddressOf(arg);
	      if(namedDecl != NULL && isLocalScoped(namedDecl)){
		ParmVarDecl * param = calleeDecl->getParamDecl(c);
		checkLocalPointer(param,callExpr);
	      }
	    }
	  }else if(isThis(arg)){
	    if(c < calleeDecl->getNumParams()){
	      ParmVarDecl * param = calleeDecl->getParamDecl(c);
	      checkLocalPointer(param,callExpr,true);
	    }
	  }else if(isArray(arg)){
	    if(c < calleeDecl->getNumParams()){
	      ParmVarDecl * param = calleeDecl->getParamDecl(c);
	      checkLocalPointer(param,callExpr);
	    }
	  }
	}
      }
      }
    }
    return true;
  }

  bool VisitTypedefNameDecl(TypedefNameDecl * typedefDecl){
    QualType type = typedefDecl->getUnderlyingType();

    if(type->isPointerType()){
      stringstream ss;
      ss << "Cannot use a typedef for a bare pointer ";
      ss << getDeclIdentifier(typedefDecl);
      addWarning(ss.str());
    }else if(type->isArrayType()){
      stringstream ss;
      ss << "Cannot use a typedef for a static sized array ";
      ss << getDeclIdentifier(typedefDecl);
      addWarning(ss.str());
    }
    return true;
  }

  bool VisitCXXNewExpr(CXXNewExpr * newExpr){
    stringstream ss;
    ss << "Calls to \"new\" must be replaced with \"new_obj\" or \"new_array\" ";
    ss << getStmtIdentifier(newExpr);
    addWarning(ss.str());
    return true;
  }

  bool VisitCXXDeleteExpr(CXXDeleteExpr * deleteExpr){
    stringstream ss;
    ss << "Calls to \"delete\" must be replaced with \"ptr.destroy()\" ";
    ss << getStmtIdentifier(deleteExpr);
    addWarning(ss.str());
    return true;
  }

  bool VisitExplicitCastExpr(ExplicitCastExpr * castExpr){
    llvm::outs() << castExpr->getCastKind() << "\n";

    // Expr * subExpr = castExpr->getSubExpr();
    // if(subExpr){
    //   QualType type = subExpr->getType();

    //   if(type->isPointerType() || type->isReferenceType()){
    //     stringstream ss;
    //     ss << "Pointer casts must use \"safe::cast\" ";
    //     ss << getStmtIdentifier(castExpr);
    //     addWarning(ss.str());
    //   }
    // }
    return true;
  }

  bool VisitBinaryOperator(BinaryOperator * op){
    NamedDecl * namedDecl = NULL;
    if(op->isAssignmentOp() && op->getLHS()->getType()->isPointerType()){
      namedDecl = extractDecl(op->getLHS());
	  
      if(isAddressOf(op->getRHS())){
	checkLocalPointer(namedDecl,op);
      }
    }
    return true;
  }

  bool VisitCXXConstructorDecl(CXXConstructorDecl * constructorDecl){
    for(CXXConstructorDecl::init_iterator I = constructorDecl->init_begin(), E = constructorDecl->init_end(); I != E; ++I){
      FieldDecl * field = (*I)->getMember();
      if(field != NULL && field->getType()->isReferenceType()){
	if(!isAllowedReferenceInitializer((*I)->getInit())){
	  stringstream ss;
	  ss << "Class member reference initialization must come from the dereference of an aptr<T> or a ptr<T> ";
	  ss << getStmtIdentifier((*I)->getInit());
	  addWarning(ss.str());
	}
      }
    }
    return true;
  }

  bool VisitReturnStmt(ReturnStmt * returnStmt){
    if(returnStmt->getRetValue() != NULL && returnStmt->getRetValue()->getType()->isReferenceType()){
      if(!isAllowedReferenceInitializer(returnStmt->getRetValue(),true)){
	stringstream ss;
	ss << "Reference return values may only come from the dereference of an aptr<T>, the dereference of a ptr<T>,";
	ss << " the dereference of the \"this\" pointer, or a reference parameter ";
	ss << getStmtIdentifier(returnStmt);
	addWarning(ss.str());
      }
    }
    return true;
  }

  /*
   * This does the check to see if expr is either:
   * 1) The dereference of a ptr<T>
   * 2) The dereference or index of an aptr<T>
   * 3) Optional: The dereference of the "this" pointer
   * 4) Optional: A reference parameter to the function
   */

  bool isAllowedReferenceInitializer(Expr * expr, bool isReturn = false){
    bool allowed = false;

    expr = expr->IgnoreParenImpCasts();
    
    if(CXXOperatorCallExpr * operatorExpr = dyn_cast<CXXOperatorCallExpr>(expr)){
      Decl * decl = operatorExpr->getCalleeDecl();
      NamedDecl * calleeDecl = NULL;
      if(decl != NULL){
	calleeDecl = dyn_cast<NamedDecl>(decl);
      }

      if(calleeDecl != NULL && 
	 (calleeDecl->getNameAsString().compare("operator*") == 0 ||
	  calleeDecl->getNameAsString().compare("operator[]") == 0)){
	DeclContext * context = calleeDecl->getLexicalDeclContext();

	if(RecordDecl * recordDecl = dyn_cast<RecordDecl>(context)){
	  if(recordDecl->getNameAsString().compare("ptr") == 0 ||
	     recordDecl->getNameAsString().compare("aptr") == 0){
	    allowed = true;
	  }
	}
      }
    }

    // Check for the optional allowed cases
    if(!allowed && isReturn){
      // Check for dereference of "this"
      if(UnaryOperator * unaryOp = dyn_cast<UnaryOperator>(expr)){
	if(unaryOp->getOpcode() == UO_Deref){
	  Expr * subExpr = unaryOp->getSubExpr()->IgnoreParenImpCasts();
	  if(isa<CXXThisExpr>(subExpr)){
	    allowed = true;
	  }
	}
      }

      if(DeclRefExpr * refExpr = dyn_cast<DeclRefExpr>(expr)){
	ValueDecl * valueDecl = refExpr->getDecl();
	if(isa<ParmVarDecl>(valueDecl)){
	  if(valueDecl->getType()->isReferenceType()){
	    allowed = true;
	  }
	}
      }
    }

    return allowed;
  }

  /*
   * Checks if a declaration is locally scoped
   */

  bool isLocalScoped(NamedDecl * namedDecl){
    if(namedDecl){
      DeclContext * context = namedDecl->getLexicalDeclContext();

      do{
	if(context->isTranslationUnit()){
	  break;
	}
	if(isa<FunctionDecl>(context)/* ||
					isa<RecordDecl>(context)*/){
	  return true;
	}
      }while((context = context->getParent()) != NULL);
    }
    return false;
  }

  /*
   * Checks that a pointer previously identified as being 
   * assigned from an address of operation is a local pointer.
   */

  void checkLocalPointer(NamedDecl * decl, Stmt * stmt, bool thisPtr = false){
    ValueDecl * valueDecl = dyn_cast<ValueDecl>(decl);
    if(valueDecl != NULL){
      QualType type = valueDecl->getType();
      std::string typeString = type.getAsString();
      if(typeString.find("lptr") == std::string::npos &&
	 typeString.find("laptr") == std::string::npos){
	stringstream ss;
	ss << "Pointers initialized or assigned from an address of must be local ";
	ss << getStmtIdentifier(stmt);
	addWarning(ss.str());
      }
    }
  }

  /*
   * Checks where a local pointer is declared.
   * Local pointers may not be declared in classes, structs, 
   * or globally.  They must be either parameters or otherwise
   * local to some function.
   */

  void checkLocalPointerDecl(ValueDecl * namedDecl){
    if(namedDecl){
      DeclContext * context = namedDecl->getLexicalDeclContext();

      do{
	if(context->isTranslationUnit()){
	  break;
	}
	if(isa<FunctionDecl>(context)){
	  return;
	}
      }while((context = context->getParent()) != NULL);
    }
    stringstream ss;
    ss << "Local pointers may not be declared outside of functions ";
    ss << getDeclIdentifier(namedDecl);
    addWarning(ss.str());
  }
};

class ValidateConsumer : public ASTConsumer {
  ValidateVisitor Visitor;

public:
  explicit ValidateConsumer(ASTContext *Context) : Visitor(Context) {}
  
  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
};

class ValidateAction : public PluginASTAction {
public:
  virtual ASTConsumer *CreateASTConsumer(clang::CompilerInstance &Compiler, 
                                                llvm::StringRef InFile) {
    return new ValidateConsumer(&Compiler.getASTContext());
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) {
    if (args.size() && args[0] == "help"){
      llvm::outs() << "Used to validate that code conforms to the Ironclad C++ subset.\n";
    }

    return true;
  }
};

static FrontendPluginRegistry::Add<ValidateAction>
X("validate", "validate safe c++");
