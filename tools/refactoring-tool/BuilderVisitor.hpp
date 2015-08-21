#ifndef BUILDER_H
#define BUILDER_H

#include <algorithm>
#include <vector>
#include <sstream>
#include <string>
using namespace std;

#include "Translator.hpp"
#include "AnalysisVisitor.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/DeclarationName.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/AST/TypeLocVisitor.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "ASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/ASTConsumers.h"
#include "clang/Rewrite/Rewriter.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace idx;

class Builder : public ASTVisitor<Builder>{
  Rewriter & Rewrite;
  SourceManager & SM;

  // Types for singleton and array pointers (ptr and aptr currently)
  string safePtrType;
  string safeArrayType;

  // Stores array vs. ptr analysis data
  AnalysisVisitor * analysis;

  string expression;
  bool arrayNew;

public:
  Builder(Rewriter & newRewrite, SourceManager & newSM, string newPtrType, 
	  string newArrayType, AnalysisVisitor * _analysis) : 
	Rewrite(newRewrite), SM(newSM), safePtrType(newPtrType), 
	safeArrayType(newArrayType), analysis(_analysis){
  }

  void resetExpression(){
    expression = "";
  }

  bool hasExpression(){
    return expression.compare("");
  }

  string getExpression(){
    return expression;
  }

  bool wasArrayNew(){
    return arrayNew;
  }

  void traverseStmt(Stmt * stmt){
    resetExpression();
    arrayNew = false;
    Visit(stmt);
  }

  void VisitImplicitCastExpr(ImplicitCastExpr * castExpr){
    Visit(castExpr->getSubExpr());
  }

  void VisitCallExpr(CallExpr * callExpr){
    expression.append(buildSystemCall(callExpr));
  }

  void VisitCXXNewExpr(CXXNewExpr * newExpr){
    expression.append(buildCXXNewExpr(newExpr));
  }

  void VisitExplicitCastExpr(ExplicitCastExpr * castExpr){
    expression.append(buildCast(castExpr,castExpr->getTypeAsWritten()));
  }

  string getQualifierString(QualType & type){
    stringstream ss;
    if(type.isLocalConstQualified()){
      ss << "const ";
    }
    if(type.isLocalRestrictQualified()){
      ss << "restrict ";
    }
    if(type.isLocalVolatileQualified()){
      ss << "volatile ";
    }
    return ss.str();
  }

  string buildType(QualType & type, NamedDecl * namedDecl = NULL, int level = 0){
    stringstream ss;
    Type * typePtr = const_cast<Type*>(type.getTypePtr());

    if(dyn_cast<TypedefType>(typePtr)){
      ss << getTypeWithoutClass(type.getAsString());
    }else if(type->isPointerType()){
      QualType pointeeType = type->getPointeeType();
      if(namedDecl != NULL && analysis->isSingleton(namedDecl,level)){
	ss << safePtrType << "< ";
      }else{
	ss << safeArrayType << "< ";
      }
      ss << buildType(pointeeType,namedDecl,level+1);
      ss << " >";
    }else if(type->isArrayType()){
      ArrayType * arrayType = const_cast<ArrayType*>(type->getAsArrayTypeUnsafe());
      QualType eleType = arrayType->getElementType();
      
      ss << "safe::array< ";
      ss << buildType(eleType,namedDecl,level+1);
      ss << ",";

      if(ConstantArrayType * actualType = 
	 dyn_cast<ConstantArrayType>(arrayType)){
        ss << actualType->getSize().toString(10,false);
      }else if(DependentSizedArrayType * actualType = 
	       dyn_cast<DependentSizedArrayType>(arrayType)){
        ss << Rewrite.ConvertToString(actualType->getSizeExpr());
      }else if(dyn_cast<IncompleteArrayType>(arrayType)){
	llvm::outs() << "Incomplete Array Type not handled\n";
      }else if(VariableArrayType * actualType = 
	       dyn_cast<VariableArrayType>(arrayType)){
	ss << Rewrite.ConvertToString(actualType->getSizeExpr());
      }

      ss << ">";
    }else{
      if(type->isBooleanType()){
	ss << getQualifierString(type);
	ss << "bool";
      }else{
	ss << getTypeWithoutClass(type.getAsString(),namedDecl);
      }
    }

    return ss.str();
  }

  string buildVariable(ValueDecl * valueDecl, QualType & type){
    stringstream ss;
    Type * typePtr = const_cast<Type*>(type.getTypePtr());

    string nameString = valueDecl->getNameAsString();

    return ss.str();
  }

  string buildVariable(ValueDecl * valueDecl, QualType & type){
    stringstream ss;

    string nameString = valueDecl->getNameAsString();

    if(VarDecl * varDecl = dyn_cast<VarDecl>(valueDecl)){
      if(varDecl->isStaticLocal()){
        ss << "static ";
      }else if(varDecl->isStaticDataMember()){
        if(varDecl->isOutOfLine()){
          nameString = valueDecl->getQualifiedNameAsString();
        }else{
          ss << "static ";
        }
      }
    }

    ss << getQualifierString(type);

    ss << buildType(type,valueDecl);
    ss << " " << nameString;

    return ss.str();
  }

  string buildParameter(ParmVarDecl * param, bool addDefault = true){
    stringstream ss;

    QualType type = param->getType();
    Type * typePtr = const_cast<Type*>(type.getTypePtr());

    if((type->isPointerType() || type->isReferenceType()) && 
	     dyn_cast<TypedefType>(typePtr) == NULL){
      ss << getQualifierString(type);

      if(type->isFunctionPointerType()){
        ss << buildFunctionPointer(param,type);
      }else{
	ss << buildType(type,param) << " ";
      }
    }else{
      ss << buildType(type,param) << " ";
    }

    if(!type->isFunctionPointerType()){
      ss << param->getNameAsString();
    }

    if(addDefault && param->hasDefaultArg() && !param->hasUninstantiatedDefaultArg()){
      ss << " = " << Rewrite.ConvertToString(param->getDefaultArg());
    }

    return ss.str();
  }

  string buildFunctionPointer(ValueDecl * valueDecl, QualType & type){
    stringstream ss;
    FunctionProtoType * functionType = NULL;
    bool isParam = false;

    if(ParenType * parenType = 
       dyn_cast<ParenType>(const_cast<Type*>(type->getPointeeType().getTypePtr()))){
      functionType = 
	dyn_cast<FunctionProtoType>(
          const_cast<Type*>(parenType->getInnerType().getTypePtr()));
    }else{
      functionType = 
	dyn_cast<FunctionProtoType>(
	  const_cast<Type*>(type->getPointeeType().getTypePtr()));
      isParam = true;
    }

    if(functionType){
      QualType resultType = functionType->getResultType();
      if(resultType->isPointerType()){
        ss << safePtrType << "< ";
        ss << getTypeWithoutClass(resultType->getPointeeType().getAsString()) << " > ";
      }else{
        ss << getTypeWithoutClass(resultType.getAsString()) << " ";
      }

      bool isQualified = false;
      if(VarDecl * varDecl = dyn_cast<VarDecl>(valueDecl)){
        if(varDecl->isThisDeclarationADefinition() && varDecl->isOutOfLine()){
          isQualified = true;
        }else{
          if(varDecl->isStaticDataMember()){
            ss << "static ";
          }
        }
      }

      if(!isParam){
        ss << "(*";
      }

      if(isQualified){
        ss << valueDecl->getQualifiedNameAsString();
      }else{
        ss << valueDecl->getNameAsString();
      }

      if(!isParam){
        ss << ") ";
      }

      bool firstArg = true;
      ss << "(";
      for(FunctionProtoType::arg_type_iterator I = 
	    functionType->arg_type_begin(), E = functionType->arg_type_end();
	  I != E; ++I){
        const QualType argType = *I;

        if(!firstArg){
          ss << ", ";
        }

        if(argType->isPointerType()){
          ss << safePtrType << "< " << 
	    getTypeWithoutClass(argType->getPointeeType().getAsString()) << " >";
        }else{
          ss << getTypeWithoutClass(argType.getAsString());
        }
        firstArg = false;
      }
      ss << ")";
    }
    return ss.str();
  }

  string buildFunctionReturnType(FunctionDecl * functionDecl, QualType type){
    QualType ptrType = type->getPointeeType();
    stringstream ss;

    ss << getQualifierString(type);

    if(type->isPointerType()){
      ss << safePtrType << "< " << ptrType.getAsString() << " > ";
    }else if(type->isReferenceType()){
      ss << "ref< " << ptrType.getAsString() << " > ";
    }

    return ss.str();
  }

  string buildCast(ExplicitCastExpr * castExpr, QualType type){
    stringstream ss;

    traverseStmt(castExpr->getSubExpr());
    string subExprString;

    if(hasExpression()){
      subExprString = getExpression();
    }else{
      subExprString = Rewrite.ConvertToString(castExpr->getSubExpr());
    }

    // Need to handle const_cast separately
    if(isa<CXXConstCastExpr>(castExpr)){
      ss << "safe::c_cast< " << 
	getTypeWithoutClass(type->getPointeeType().getAsString()) << 
	" >(" << subExprString << ")";
    }else{
      ss << "safe::cast< " << 
	getTypeWithoutClass(type->getPointeeType().getAsString()) << 
	" >(" << subExprString << ")";
    }

    return ss.str();
  }

  string buildInitializerList(CXXConstructorDecl * consDecl){
    stringstream ss;
    bool addComma = false;

    for(CXXConstructorDecl::init_iterator I = consDecl->init_begin(), E = consDecl->init_end();
	I != E; ++I){
      CXXCtorInitializer * initializer = *I;
      if(initializer->isWritten()){
        FieldDecl * field = initializer->getMember();
        Expr * initExpr = initializer->getInit();

        if(field != NULL && initExpr != NULL){
          if(!addComma){
            ss << " : ";
          }else{
            ss << ", ";
          }
          ss << field->getNameAsString() << "(";
          ss << Rewrite.ConvertToString(initExpr);
          ss << ")";
          addComma = true;
        }
      }
    }

    return ss.str();
  }

  bool isImplemented(string functionName){
    if(functionName.compare("memcpy") == 0 || 
       functionName.compare("memset") == 0 ||
       functionName.compare("memmove") == 0 || 
       functionName.compare("calloc") == 0 ||
       functionName.compare("realloc") == 0 || 
       functionName.compare("mmap") == 0 ||
       functionName.compare("pthread_create") == 0 || 
       functionName.compare("atoi") == 0 || 
       functionName.compare("atol") == 0 || 
       functionName.compare("atof") == 0 || 
       functionName.compare("fclose") == 0 ||
       functionName.compare("ftell") == 0 || 
       functionName.compare("fseek") == 0 ||
       functionName.compare("getc") == 0 ||
       functionName.compare("gets") == 0 ||
       functionName.compare("feof") == 0 ||
       functionName.compare("rewind") == 0 ||
       functionName.compare("stat") == 0 || 
       functionName.compare("perror") == 0 ||
       functionName.compare("clock_gettime") == 0 ||
       functionName.compare("strcmp") == 0 ||
       functionName.compare("strcpy") == 0 ||
       functionName.compare("strncpy") == 0 ||
       functionName.compare("strcat") == 0 ||
       functionName.compare("strncat") == 0 ||
       functionName.compare("strtok") == 0 ||
       functionName.compare("strlen") == 0 ||
       functionName.compare("strchr") == 0){
      return true;
    }

    return false;
  }

  string buildSystemCall(CallExpr * callExpr){
    stringstream ss;
    stringstream ss2;
    bool needsRewrite = false;
    bool pointerRewrite = false;

    FunctionDecl * calleeDecl = callExpr->getDirectCallee();

    if(calleeDecl == NULL || dyn_cast<CXXMemberCallExpr>(callExpr) != NULL){
      return "";
    }

    SourceLocation sourceLoc = calleeDecl->getLocStart();

    string prefix = "";
    
    if(sourceLoc.isValid() && SM.isInSystemHeader(sourceLoc)){
      prefix = "safe::safe_";
    }

    ss << prefix << calleeDecl->getNameAsString() << "(";
    ss2 << calleeDecl->getQualifiedNameAsString() << "(";

    for(unsigned int c = 0; c < callExpr->getNumArgs(); c++){
      Expr * arg = callExpr->getArg(c);

      if(c > 0){
        ss << ", ";
	ss2 << ", ";
      }

      Expr * bare = arg->IgnoreImplicit();
      QualType type = bare->getType();
      string typeString = getTypeWithoutClass(type.getAsString());

      
      if(type->isPointerType() && typeString.compare("__va_list_tag *") != 0 
	 && !isa<StringLiteral>(bare)){
	needsRewrite = true;
	pointerRewrite = true;
      }

      traverseStmt(arg);
      if(hasExpression()){
	needsRewrite = true;
	pointerRewrite = true;
      }

      traverseStmt(arg);
      if(hasExpression()){
        needsRewrite = true;
        ss << getExpression();
	ss2 << getExpression();
      }else{
        ss << Rewrite.ConvertToString(arg);
	ss2 << Rewrite.ConvertToString(arg);
      }
    }

    ss << ")";
    ss2 << ")";

    if(needsRewrite && pointerRewrite){
      return ss.str();
    }else if(needsRewrite){
      return ss2.str();
    }else{
      return "";
    }
  }

  string buildCXXNewExpr(CXXNewExpr * newExpr,Expr * expr = NULL){
    stringstream ss;

    QualType newType = newExpr->getAllocatedType();
    string typeString = buildType(newType);

    if(expr){
      NamedDecl * namedDecl = extractDecl(expr);
      if(namedDecl){
	typeString = buildType(newType,namedDecl,1);
      }
    }

    if(newExpr->isArray()){
      ss << "safe::new_array< " << typeString << 
	" >(" << Rewrite.ConvertToString(newExpr->getArraySize()) << ")";
    }else{
      bool addComma = false;
      ss << "safe::new_ptr< " << typeString << " >(";
      for(CXXNewExpr::arg_iterator I = newExpr->constructor_arg_begin(), 
	    E = newExpr->constructor_arg_end(); I != E; ++I){
	if(!I->isDefaultArgument()){
	  if(addComma){
	    ss << ",";
	  }
	  ss << Rewrite.ConvertToString(*I);
	  addComma = true;
	}
      }
      
      ss << ")";
    }

    return ss.str();
  }
};

#endif
