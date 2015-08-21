#ifndef ANALYSIS_VISITOR_H
#define ANALYSIS_VISITOR_H

#include <fstream>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <sstream>
#include <string>
using namespace std;

#include "Util.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/DeclarationName.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/AST/TypeLocVisitor.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
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

class PointerData{
public:
  std::vector<std::string> from;
  std::vector<std::string> to;
  bool singleton;

  PointerData() : singleton(true) {}
};

class AnalysisVisitor : public ASTVisitor<AnalysisVisitor>{
ASTContext *Context;
  SourceManager *SM;
  Rewriter & Rewrite;
  std::vector<CXXRecordDecl*> addConstructorList;
  std::vector<CXXRecordDecl*> addDummyMethodList;
  std::map<std::string,PointerData*> pointers;
  FunctionDecl * currentFunction;

public:

  AnalysisVisitor(SourceManager * _SM, Rewriter & _Rewrite) : SM(_SM),Rewrite(_Rewrite),currentFunction(NULL){}

  /*
   * This should propagate pointer/arrayref type data from a variable to 
   * the other variables, functions, and parameters that it interacts 
   * with during the program.
   */

  void finalizePointers(){
    for( std::map<std::string,PointerData*>::iterator I = pointers.begin(), E = pointers.end();
	 I != E; ++I){
      PointerData * ptrData = I->second;
      if(ptrData){
        updateDependentPointers(ptrData);
      }
    }
  }

  void printPointers(){
    for( std::map<std::string,PointerData*>::iterator I = pointers.begin(), E = pointers.end();
	 I != E; ++I){
      PointerData * ptrData = I->second;
      if(ptrData){
        llvm::outs() << I->first << " - " << (ptrData->singleton ? "ptr" : "arrayref") << "\n";
      }
    }
  }

  void outputPointers(){
    fstream file("safecpp_analysis.txt",fstream::out);
    for( std::map<std::string,PointerData*>::iterator I = pointers.begin(), E = pointers.end();
	 I != E; ++I){
      PointerData * ptrData = I->second;
      if(ptrData){
	file << I->first << ",";
        if(!ptrData->singleton){
	  file << "aptr";
	}else{
	  file << "ptr";
	}
	for( std::vector<std::string>::iterator I = ptrData->from.begin(), E = ptrData->from.end();
	   I != E; ++I){
	  std::string pointerName = *I;
	  file << "," << pointerName;
	}
	file << "\n";
      }
    }
    file.close();
  }

  void inputPointers(){
    fstream file("safecpp_analysis.txt",fstream::in);
    string line;
    string name = "";
    string ptrType = "aptr";
    if(file.is_open()){
      while(getline(file,line)){
	size_t index1 = 0;
	size_t index2 = line.find(",");
	string token = line.substr(index1,index2);
	name = token;
	PointerData * ptrData = InsertInMap(token);
	index1 = index2+1;
	index2 = line.find(",",index1);
	if(index2 != string::npos){
	  token = line.substr(index1,index2-index1);
	  ptrType = token;
	  while(index2 != string::npos){
	    index1 = index2+1;
	    index2 = line.find(",",index1);
	    if(index2 != string::npos){
	      token = line.substr(index1,index2-index1);
	    }else{
	      token = line.substr(index1,line.size()-index1);
	    }
	  }
	}else{
	  ptrType = line.substr(index1,line.size()-index1);
	}
	if(ptrType.compare("aptr") == 0){
	  ptrData->singleton = false;
	}else{
	  ptrData->singleton = true;
	}
      }
    }
    file.close();
  }

  void updateDependentPointers(PointerData * ptrData){
    for( std::vector<std::string>::iterator I = ptrData->from.begin(), E = ptrData->from.end();
	   I != E; ++I){
      std::string pointerName = *I;
      PointerData * otherData = pointers[pointerName];
      if(otherData){
	if(ptrData->singleton != otherData->singleton){
	  otherData->singleton = false;
	  updateDependentPointers(otherData);
	}
      }
    }
  }

  bool isSingleton(NamedDecl * namedDecl, int level = 0){
    bool singleton = false;
    if(namedDecl){
      if(PointerData * ptrData = pointers[convertToUniqueString(namedDecl,level)]){
	singleton = ptrData->singleton;
      }
    }
    return singleton;
  }

  void Analyze(Decl * decl){
    Visit(decl);
  }

  virtual void VisitDeclStmt(DeclStmt * declStmt){
    DeclGroupRef group = declStmt->getDeclGroup();

    // Need to handle multiple declarations in a single decl statement

    for(DeclGroupRef::iterator I = group.begin(), E = group.end(); I != E; ++I){
      Decl * decl = *I;
      if(isa<VarDecl>(decl)){
        VarDecl * varDecl = (VarDecl*)decl;
        if(varDecl->getType()->isPointerType() || varDecl->getType()->isArrayType()){
          PointerData * ptrData = InsertInMap(varDecl);
	  if(varDecl->hasInit()){
	    HandleInit(ptrData, varDecl->getInit());    
	  }
        }
      }
    }
  }

  virtual void VisitVarDecl(VarDecl * varDecl){
    if(varDecl->getType()->isPointerType() || varDecl->getType()->isArrayType()){
      PointerData * ptrData = InsertInMap(varDecl);
      if(varDecl->hasInit()){
        HandleInit(ptrData, varDecl->getInit());
      }
    }
  }

  void HandleInit(PointerData * newData, Expr * init){
    init = init->IgnoreParenCasts();

    if(DeclRefExpr * declRefExpr = dyn_cast<DeclRefExpr>(init)){
      ValueDecl * valueDecl = declRefExpr->getDecl();
      newData->from.push_back(convertToUniqueString(valueDecl));
    }else if(CXXNewExpr * newExpr = dyn_cast<CXXNewExpr>(init)){
      /*
       * This is our base case for whether a pointer is initialized as an array or not
       * All array pointers to be used with delete[] must at some point link back to a new[] call
       */
      if(newExpr->isArray()){
        newData->singleton = false;
      }
    }else if(CallExpr * callExpr = dyn_cast<CallExpr>(init)){
      FunctionDecl * calleeDecl = callExpr->getDirectCallee();
      if(calleeDecl != NULL){
        newData->from.push_back(convertToUniqueString(calleeDecl));
      }
    }else if(MemberExpr * memberExpr = dyn_cast<MemberExpr>(init)){
      ValueDecl * memberDecl = memberExpr->getMemberDecl();
      newData->from.push_back(convertToUniqueString(memberDecl));
    }else if(isa<GNUNullExpr>(init)){
      // Do nothing
    }else if(isa<IntegerLiteral>(init)){
      // Do nothing
    }else if(isa<CXXDependentScopeMemberExpr>(init)){
      // Not sure if we can or need to do anything here
    }else{
      //llvm::outs() << "Unhandled Init Expression Type: " << init->getStmtClassName() << "\n";
    }
  }

  virtual void VisitFunctionDecl(FunctionDecl * functionDecl){
    BaseDeclVisitor::VisitFunctionDecl(functionDecl);

    if(functionDecl->getResultType()->isPointerType()){
      InsertInMap(functionDecl);
    }

    for(FunctionDecl::param_iterator I = functionDecl->param_begin(), E = functionDecl->param_end(); I != E; ++I){
      ParmVarDecl * param = *I;
      if(param->getType()->isPointerType() || param->getType()->isArrayType()){
        InsertInMap(param);
      }
    }

    if (functionDecl->isThisDeclarationADefinition()){
      if(functionDecl->getLocStart().isValid() && !SM->isInSystemHeader(functionDecl->getLocStart())){
	currentFunction = functionDecl;
        VisitStmt(functionDecl->getBody());
      }
    }
  }

  void VisitClassTemplateDecl(ClassTemplateDecl * templateDecl){
    if(templateDecl->getTemplatedDecl()){
      VisitRecordDecl(templateDecl->getTemplatedDecl());
    }
    BaseDeclVisitor::VisitTemplateDecl(templateDecl);
  }

  void VisitRecordDecl(RecordDecl * recordDecl){
    for(RecordDecl::field_iterator I = recordDecl->field_begin(), E = recordDecl->field_end(); I != E; ++I){
      FieldDecl * field = *I;
      if(field->getType()->isPointerType() || field->getType()->isArrayType()){
        InsertInMap(field);
      }
    }

    BaseDeclVisitor::VisitRecordDecl(recordDecl);
  }

  /*
   * Expression Visitors
   * Used to determine if a pointer is used as an array or not
   */

  void VisitUnaryOperator(UnaryOperator * op){
    Expr * subExpr = op->getSubExpr();
    if(subExpr->getType()->isPointerType()){
      NamedDecl * namedDecl = extractDecl(subExpr);
      if(namedDecl){
	PointerData * ptrData = pointers[convertToUniqueString(namedDecl)];
	if(ptrData != NULL && op->isArithmeticOp()){
	  ptrData->singleton = false;
	}
      }
    }
    VisitStmt(op->getSubExpr());
  }

  void VisitBinaryOperator(BinaryOperator * op){
    NamedDecl * namedDecl = NULL;
    if(op->isAssignmentOp() && op->getLHS()->getType()->isPointerType()){
      namedDecl = extractDecl(op->getLHS());
      if(namedDecl){
        PointerData * ptrData = pointers[convertToUniqueString(namedDecl,
							     getSubscriptLevel(op->getLHS()))];
        if(ptrData != NULL){
          if(isNewExpr(op->getRHS())){
            if(namedDecl != NULL){
              CXXNewExpr * newExpr = dyn_cast<CXXNewExpr>(op->getRHS()->IgnoreParenCasts());
              if(newExpr != NULL && newExpr->isArray()){
                ptrData->singleton = false;
              }
            }
	  }else if(isCallExpr(op->getRHS())){
	    NamedDecl * calleeDecl = extractDecl(op->getRHS());
	    if(calleeDecl){
	      PointerData * calleeData = pointers[convertToUniqueString(calleeDecl)];
	      if(calleeData){
	        if(namedDecl){
	          calleeData->from.push_back(convertToUniqueString(namedDecl));
		  ptrData->from.push_back(convertToUniqueString(calleeDecl));
	        }
	      }
	    }
	    VisitStmt(op->getRHS());
          }else{
            NamedDecl * assignedFromDecl = extractDecl(op->getRHS());
            if(namedDecl != NULL && assignedFromDecl != NULL){
	      ptrData->from.push_back(convertToUniqueString(assignedFromDecl));
            }
          }
	}
      }
      VisitStmt(op->getLHS()->IgnoreParenCasts());
      VisitStmt(op->getRHS()->IgnoreParenCasts());
    }else{
      VisitStmt(op->getLHS()->IgnoreParenCasts());
      VisitStmt(op->getRHS()->IgnoreParenCasts());
    }
  }

  void VisitArraySubscriptExpr(ArraySubscriptExpr * subscriptExpr){
    Expr * base = subscriptExpr->getBase();
    if(base->getType()->isPointerType()){
      NamedDecl * namedDecl = extractDecl(base);
      PointerData * ptrData = pointers[convertToUniqueString(namedDecl)];
      /*
      if(namedDecl){
	llvm::outs() << "Extracted: " << convertToUniqueString(namedDecl) << " from: " << Rewrite.ConvertToString(subscriptExpr) << "\n";
      }else{
	llvm::outs() << "Couldn't extract from: " << Rewrite.ConvertToString(subscriptExpr) << "\n";
      }
      */
      if(ptrData){
	ptrData->singleton = false;
      }
    }

    VisitStmt(subscriptExpr->getBase());
    VisitStmt(subscriptExpr->getIdx());
  }

  void VisitCallExpr(CallExpr * callExpr){
    FunctionDecl * calleeDecl = callExpr->getDirectCallee();

    if(calleeDecl != NULL){
      //      if(!SM->isInSystemHeader(calleeDecl->getLocStart())){
        for(unsigned int c = 0; c < callExpr->getNumArgs(); ++c){
	  if(c >= calleeDecl->getNumParams()){
	    break;
	  }
	  if(callExpr->getArg(c)->getType()->isPointerType() && !callExpr->getArg(c)->getType()->isFunctionPointerType()){
	    NamedDecl * argDecl = extractDecl(callExpr->getArg(c));
	    NamedDecl * paramDecl = calleeDecl->getParamDecl(c);
	    if(argDecl != NULL && paramDecl != NULL){
	      PointerData * argData = pointers[convertToUniqueString(argDecl)];
	      PointerData * paramData = pointers[convertToUniqueString(paramDecl)];
	      if(argData){
		argData->to.push_back(convertToUniqueString(paramDecl));
	      }
	      if(paramData){
		paramData->from.push_back(convertToUniqueString(argDecl));
	      }else{
		paramData = InsertInMap(argDecl);
		if(paramData){
		  paramData->from.push_back(convertToUniqueString(argDecl));
		}
	      }
	    }
	    if(callExpr->getArg(c)){
	      VisitStmt(callExpr->getArg(c));
	    }
	  }
	}
	//}
    }
  }

  void VisitReturnStmt(ReturnStmt * returnStmt){
    Expr * expr = returnStmt->getRetValue();
    if(expr){
      if(isNewExpr(expr)){
	CXXNewExpr * newExpr = dyn_cast<CXXNewExpr>(expr->IgnoreParenCasts());
	if(currentFunction){
	  PointerData * functionData = pointers[convertToUniqueString(currentFunction)];
	  if(functionData != NULL && newExpr->isArray()){
	    functionData->singleton = false;
	  }
	}
      }else{
        NamedDecl * namedDecl = extractDecl(expr);
        if(namedDecl != NULL){
          PointerData * ptrData = pointers[convertToUniqueString(namedDecl)];
          if(ptrData != NULL && currentFunction != NULL){
	    ptrData->from.push_back(convertToUniqueString(currentFunction));
	  }
        }
      }
    }
  }

  bool isNewExpr(Expr * expr){
    expr = expr->IgnoreParenCasts();
    return isa<CXXNewExpr>(expr);
  }

  bool isCallExpr(Expr * expr){
    expr = expr->IgnoreParenCasts();
    return isa<CallExpr>(expr);
  }

  bool isSubscriptExpr(Expr * expr){
    expr = expr->IgnoreParenCasts();
    return isa<ArraySubscriptExpr>(expr);
  }

  int getSubscriptLevel(Expr * expr){
    expr = expr->IgnoreParenCasts();
    if(ArraySubscriptExpr * subscriptExpr = dyn_cast<ArraySubscriptExpr>(expr)){
      return 1 + getSubscriptLevel(subscriptExpr->getBase());
    }else{
      return 0;
    }
  }

  PointerData * InsertInMap(NamedDecl * namedDecl){
    string key = convertToUniqueString(namedDecl);
    if(pointers.count(key) == 0){
      PointerData * ptrData = new PointerData;
      pointers[key] = ptrData;

      if(ValueDecl * valueDecl = dyn_cast<ValueDecl>(namedDecl)){
        QualType type = valueDecl->getType();
        if(type->isPointerType()){
	  InsertSubscriptsInMap(namedDecl,type->getPointeeType(),1);
        }else if(type->isArrayType()){
          ArrayType * arrayType = const_cast<ArrayType*>(type->getAsArrayTypeUnsafe());
          QualType eleType = arrayType->getElementType();
	  InsertSubscriptsInMap(namedDecl,eleType,1);
	}
      }

      return ptrData;
    }else{
      PointerData * ptrData = pointers[key];
      return ptrData;
    }
  }

  PointerData * InsertInMap(string key){
    if(pointers.count(key) == 0){
      PointerData * ptrData = new PointerData;
      pointers[key] = ptrData;
      return ptrData;
    }else{
      PointerData * ptrData = pointers[key];
      return ptrData;
    }
  }

  void InsertSubscriptsInMap(NamedDecl * namedDecl, QualType type, int level){
    string key = convertToUniqueString(namedDecl);
    if(type->isPointerType() || type->isArrayType()){
      for(int c = 0; c < level; c++){
	key += "_sub";
      }
      if(pointers.count(key) == 0){
	PointerData * ptrData = new PointerData;
	pointers[key] = ptrData;
	if(type->isPointerType()){
	  InsertSubscriptsInMap(namedDecl, type->getPointeeType(), level+1);
	}else{
	  ArrayType * arrayType = const_cast<ArrayType*>(type->getAsArrayTypeUnsafe());
          QualType eleType = arrayType->getElementType();
	  InsertSubscriptsInMap(namedDecl,eleType,level+1);
	}
      }
    }
  }

  string convertToUniqueString(NamedDecl * namedDecl, int level = 0){
    string declString;
    DeclContext * context = namedDecl->getLexicalDeclContext();

    assert(namedDecl != NULL);

    do{
      if(context->isTranslationUnit()){
        break;
      }
      if(NamedDecl * parentDecl = dyn_cast<NamedDecl>(context)){
        declString = parentDecl->getNameAsString() + "::" + declString;
      }
    }while((context = context->getParent()) != NULL);

    declString = declString + namedDecl->getNameAsString();

    for(int c = 0; c < level; c++){
      declString += "_sub";
    }

    return declString;
  }

  bool shouldAddConstructor(CXXRecordDecl * recordDecl){
    if(std::find(addConstructorList.begin(),addConstructorList.end(),recordDecl) != addConstructorList.end()){
      return true;
    }else{
      return false;
    }
  }

  bool shouldAddDummyMethod(CXXRecordDecl * recordDecl){
    if(std::find(addDummyMethodList.begin(),addDummyMethodList.end(),recordDecl) != addDummyMethodList.end()){
      return true;
    }else{
      return false;
    }
  }

  CXXRecordDecl * getRecordDeclFromType(QualType type){
    Type * typePtr = const_cast<Type*>(type.getTypePtr());
    if(RecordType * recordType = dyn_cast<RecordType>(typePtr)){
      RecordDecl * recordDecl = recordType->getDecl();
      if(CXXRecordDecl * decl = dyn_cast<CXXRecordDecl>(recordDecl)){
	return decl;
      }
    }
    return NULL;
  }

  void VisitCXXNewExpr(CXXNewExpr * newExpr){
    if(newExpr->isArray()){
      CXXRecordDecl * decl = getRecordDeclFromType(newExpr->getAllocatedType());
      if(decl != NULL && decl->hasUserDeclaredConstructor() && !decl->hasDeclaredDefaultConstructor()){
        addConstructorList.push_back(decl);
      }
    }
  }

  void VisitExplicitCastExpr(ExplicitCastExpr * castExpr){
    QualType fromType = castExpr->getSubExpr()->getType();
    QualType toType = castExpr->getTypeAsWritten();

    if(fromType->isRecordType() && toType->isRecordType()){
      CXXRecordDecl * fromDecl = getRecordDeclFromType(fromType);
      CXXRecordDecl * toDecl = getRecordDeclFromType(toType);
 
      if(fromDecl != NULL && toDecl != NULL){
        if(fromDecl->isPolymorphic()){
          addDummyMethodList.push_back(fromDecl);
        }
        if(toDecl->isPolymorphic()){
          addDummyMethodList.push_back(toDecl);
        }
      }
    }
    VisitStmt(castExpr->getSubExpr());
  }
};

#endif
