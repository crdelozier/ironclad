#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <algorithm>
#include <sstream>
#include <string>
using namespace std;

#include "Util.hpp"
#include "BuilderVisitor.hpp"
#include "AnalysisVisitor.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/DeclarationName.h"
#include "clang/AST/AST.h"
#include "clang/AST/ParentMap.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Index/DeclReferenceMap.h"
#include "clang/Rewrite/ASTConsumers.h"
#include "clang/Rewrite/Rewriter.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace idx;

#define DEBUG 0
#define CHECK_EMPTY_PTR 1

class Translator{
private:
  Rewriter & Rewrite;
  SourceManager & SM;

  std::vector<std::string> smartPointerTypes;

  std::vector<FileID> filesToWrite;

  string safePtrType;
  string safeArrayType;

  AnalysisVisitor * analysis;

  Builder * builder;

  unsigned long modifiedLines;

  // Needed to translate groups of VarDecls since we can only rewrite once per line
  ValueDecl * lastValueDecl;
  string declGroupString;

  Translator();

public:
  Translator(Rewriter & New_Rewrite, std::vector<std::string> ptrTypes, string newPtrType, 
             string newArrayType, AnalysisVisitor * _analysis) : 
    Rewrite(New_Rewrite), SM(New_Rewrite.getSourceMgr()), 
    smartPointerTypes(ptrTypes), safePtrType(newPtrType), 
    safeArrayType(newArrayType), analysis(_analysis),
    builder(new Builder(Rewrite,SM,safePtrType,safeArrayType,analysis)),
    modifiedLines(0), lastValueDecl(NULL){}

void RewriteSource(SourceRange range, string replacement){
  ++modifiedLines;
  Rewrite.ReplaceText(range,replacement);
}

unsigned long getNumModifiedLines(){
  return modifiedLines;
}

/*
 * We can only rewrite each statement once.  With a DeclStmt, this isn't a problem, 
 * but global variables, object members, etc. that are declared as "int * a, * b, * c;"
 * aren't grouped together in the AST except by a common starting location.  So, 
 * we build a string of each of these declarations and rewrite only when we've visited
 * the last variable declaration in a group.  In this case, we would build as follows:
 * 1: safe_ptr<int> a (Not rewritten yet)
 * 2: safe_ptr<int> a, b (Not rewritten yet)
 * 3: safe_ptr<int> a, b, c (Not rewritten yet)
 * 4: Rewrite either at the next separate variable declaration or at the end of translation
 */

void translatePointer(ValueDecl * valueDecl, QualType & type){
  if(DEBUG){
    llvm::outs() << "Translating Variable - " << valueDecl->getNameAsString() << "\n";
  }

  if(!checkSourceRange(valueDecl->getSourceRange())){
    return;
  }

  stringstream ss;

  if(lastValueDecl != NULL && lastValueDecl->getLocStart() == valueDecl->getLocStart()){
    // Just appending the variable name to the list of declarations
    string varString = valueDecl->getNameAsString();
    if(VarDecl * varDecl = dyn_cast<VarDecl>(valueDecl)){
      if(varDecl->isStaticDataMember()){
	if(varDecl->isOutOfLine()){
	  varString = varDecl->getQualifiedNameAsString();
	}
      }
    }
    ss << "," << varString;
  }else{
    // On another line, rewrite now
    translateLastDeclGroup();

    // Start the translation for the next group of decls
    declGroupString = "";

    if(VarDecl * varDecl = dyn_cast<VarDecl>(valueDecl)){
      if(varDecl->isExternC()){
	ss << "extern ";
      }
    }

    ss << builder->buildVariable(valueDecl,type);
  }

  // Translate either default parameter or initializer by traversing the expression
  HandleInit(valueDecl,ss);

  declGroupString.append(ss.str());
  lastValueDecl = valueDecl;

  if(DEBUG){
    llvm::outs() << "Done\n";
  }
}

void HandleInit(ValueDecl * valueDecl, stringstream & ss){
  if(ParmVarDecl * param = dyn_cast<ParmVarDecl>(valueDecl)){
    if(param->hasDefaultArg() && !param->hasUninstantiatedDefaultArg()){
      builder->traverseStmt(param->getDefaultArg());
      if(builder->hasExpression()){
	ss << " = " << builder->getExpression();
      }else{
	ss << " = " << Rewrite.ConvertToString(param->getDefaultArg());
      }
    }
  }else if(VarDecl * varDecl = dyn_cast<VarDecl>(valueDecl)){
    QualType type = varDecl->getType();
    if(varDecl->hasInit()){
      builder->traverseStmt(varDecl->getInit());
      if(builder->hasExpression()){
	ss << " = " << builder->getExpression();
      }else{
	string initString = Rewrite.ConvertToString(varDecl->getInit());
	if(initString.compare("") != 0){
	  ss << " = " << Rewrite.ConvertToString(varDecl->getInit());
	}
      }
    }else{
      if(varDecl->hasInit()){
        builder->traverseStmt(varDecl->getInit());
        if(builder->hasExpression()){
	  ss << "(" << builder->getExpression() << ")";
        }else{
          string initString = Rewrite.ConvertToString(varDecl->getInit());
          if(initString.compare("") != 0){
	    ss << "(" << Rewrite.ConvertToString(varDecl->getInit()) << ")";
          }
        }
      }
    }
  }
}

void translateLastDeclGroup(){
  if(lastValueDecl){
    if(CHECK_EMPTY_PTR && 
       Rewrite.getRewrittenText(lastValueDecl->getSourceRange()).compare("") == 0){
      /* 
       * Clang cannot translate some pointers, usually because a declaration 
       * exists in a macro.  In these cases, we do not rewrite it and just warn
       * instead.
       */
      llvm::outs() << "Failed to rewrite pointer \"" << lastValueDecl->getNameAsString() << "\"\n";
    }else{
      RewriteSource(lastValueDecl->getSourceRange(),declGroupString);
    }
  }
}

void translateSmartPointer(Decl * decl, QualType & type, string & smartPtrType){
  if(DEBUG){
    llvm::outs() << "Translating Uninitialized Smart Pointer\n";
  }

  if(!checkSourceRange(decl->getSourceRange())){
    return;
  }

  stringstream ss;
  string currentSource = Rewrite.getRewrittenText(decl->getSourceRange());

  if(currentSource.find(smartPtrType) != string::npos){
    int position = currentSource.find(smartPtrType);
    currentSource = currentSource.replace(position,smartPtrType.size(),safePtrType);
  }

  RewriteSource(decl->getSourceRange(),currentSource);

  if(DEBUG){
    llvm::outs() << "Done\n";
  }
}

void translateFunctionPointer(ValueDecl * valueDecl, QualType & type){
  if(!checkSourceRange(valueDecl->getSourceRange())){
    return;
  }
  string ptrString = builder->buildFunctionPointer(valueDecl,type);
  if(ptrString.compare("") != 0){
    RewriteSource(valueDecl->getSourceRange(),ptrString);
  }
}

int findFunctionDefinitionLength(FunctionDecl * functionDecl){
  string currentSource = Rewrite.getRewrittenText(functionDecl->getSourceRange());
  int size = 0;

  for(size_t i = 0; i < currentSource.length(); ++i){
    if(currentSource[i] == ';' || currentSource[i] == '{'){
      break;
    }
    ++size;
  }

  return size;
}

void translateFunction(FunctionDecl * functionDecl, bool isFriend = false){
  if(DEBUG){
    llvm::outs() << "Translating Function\n";
  }

  if(!checkSourceRange(functionDecl->getSourceRange())){
    return;
  }

  stringstream ss;
  QualType type = functionDecl->getResultType();
  DeclContext * lexicalContext = functionDecl->getLexicalDeclContext();
  bool noReturn = false;
  bool modified = false;

  string templateParamString("");

  if(functionDecl->isExternC()){
    ss << "extern ";
  }

  if(isFriend){
    ss << "friend ";
  }

  if(functionDecl->getNumTemplateParameterLists() > 0){
    TemplateParameterList * paramList = functionDecl->getTemplateParameterList(0);
    if(paramList){
      bool needComma = false;
      ss << "template<";
      templateParamString.append("<");
      for(TemplateParameterList::iterator I = paramList->begin(),
	    E = paramList->end(); I != E; ++I){
        NamedDecl * namedDecl = *I;
       
        if(needComma){
          templateParamString.append(",");
          ss << ",";
        }

        if(dyn_cast<TemplateTypeParmDecl>(namedDecl)){
          templateParamString.append(namedDecl->getNameAsString());
          ss << "class " << namedDecl->getNameAsString();
        }
        needComma = true;
      }
      templateParamString.append(">");
      ss << ">\n";
    }
  }

  CXXMethodDecl * methodDecl = dyn_cast<CXXMethodDecl>(functionDecl);
  CXXRecordDecl * recordDecl = NULL;
  CXXConstructorDecl * consDecl = NULL;

  if(methodDecl){
    recordDecl = methodDecl->getParent();

    if(methodDecl->isStatic() && !methodDecl->isOutOfLine()){
      ss << "static ";
    }

    if(functionDecl->isVirtualAsWritten() && !methodDecl->isOutOfLine()){
      ss << "virtual ";
    }
  }
  if(functionDecl->isInlineSpecified()){
    ss << "inline ";
  }

  /*
   * Need to avoid generating code for default constructors and destructors
   * Otherwise, the class definition will be accidentally over-written
   */

  if((consDecl = dyn_cast<CXXConstructorDecl>(functionDecl))){
    noReturn = true;
    if(recordDecl){
      if(consDecl->isDefaultConstructor() && 
	 !recordDecl->hasUserProvidedDefaultConstructor()){
        return;
      }else if(consDecl->isCopyConstructor() 
	       && !methodDecl->isUserProvided()){
        return;
      }else if(consDecl->isMoveConstructor() && 
	       !recordDecl->hasUserDeclaredMoveConstructor()){
        return;
      }
    }
  }else if(dyn_cast<CXXDestructorDecl>(functionDecl)){
    noReturn = true;
    if(recordDecl){
      if(!recordDecl->hasUserDeclaredDestructor()){
        return;
      }
    }
  }else if(functionDecl->getNameAsString().compare("operator=") == 0 
	   && !methodDecl->isUserProvided()){
    return;
  }

  if(!noReturn){
    if(type->isPointerType()){
      modified = true;
      if(methodDecl){
        if(methodDecl->size_overridden_methods() > 0){
          type = getInheritedReturnType(methodDecl);
        }
      }
      ss << builder->buildType(type,functionDecl);
    }else if(type->isBooleanType()){
      ss << "bool";
    }else{
      ss << getTypeWithoutClass(type.getAsString());
    }
    ss << " ";
  }
  
  bool addClass = true;

  if(lexicalContext){
    DeclContext * parentContext = lexicalContext->getParent();
    if(parentContext){
      string contextKind(lexicalContext->getDeclKindName());
      if(contextKind.compare("CXXRecord") == 0 || 
	 contextKind.compare("ClassTemplateSpecialization") == 0){
        addClass = false;
      }
    }
  }

  if(!addClass){
    ss << functionDecl->getNameAsString() << "(";
  }else{
    ss << getTypeWithoutClass(functionDecl->getQualifiedNameAsString(),functionDecl) << "(";
  }

  bool addComma = false;
  bool hasProto = functionDecl->hasWrittenPrototype();
  bool isProto = !functionDecl->isThisDeclarationADefinition();

  for(FunctionDecl::param_iterator I = functionDecl->param_begin(), 
	E = functionDecl->param_end(); I != E; ++I){
    ParmVarDecl * param = *I;

    if(param->getType()->isPointerType()){
      modified = true;
    }

    if(addComma){
      ss << ", ";
    }

    ss << builder->buildParameter(param,!hasProto || !isProto);

    addComma = true;
  }

  if(functionDecl->isVariadic()){
    ss << ", ...";
  }

  ss << ")";

  addComma = false;

  // Handle initializer list for constructors
  if(consDecl != NULL){
    ss << builder->buildInitializerList(consDecl);
  }

  QualType functionType = functionDecl->getType();
  if(const FunctionProtoType * protoType = 
     dyn_cast<const FunctionProtoType>(functionType.getTypePtr())){
    if(protoType->getTypeQuals() & Qualifiers::Const){
      ss << " const ";
    }
  }

  if(functionDecl->isVirtualAsWritten() && functionDecl->isPure()){
    ss << " = 0";
  }

  if(modified){
    Rewrite.ReplaceText(functionDecl->getLocStart(),findFunctionDefinitionLength(functionDecl),ss.str());
    ++modifiedLines;
  }

  if(DEBUG){
    llvm::outs() << "Done\n";
  }
}
  /*
/*
 * Translates "delete expr" to "(expr).free()"
 */

void translateDelete(CXXDeleteExpr * deleteExpr){
  if(DEBUG){
    llvm::outs() << "Translating Delete Expression\n";
  }

  if(!checkSourceRange(deleteExpr->getSourceRange())){
    return;
  }

  Expr * argument = deleteExpr->getArgument();
  stringstream ss;

  ss << "(" << Rewrite.ConvertToString(argument) << ").free()";
  RewriteSource(deleteExpr->getSourceRange(),ss.str());

  if(DEBUG){
    llvm::outs() << "Done\n";
  }
}

/*
 * Translates "typedef T * typename" to "typedef ptr<T> typename" 
 */

void translatePointerTypedef(TypedefNameDecl * typedefDecl, QualType type){
  if(DEBUG){
    llvm::outs() << "Translating Pointer Typedef\n";
  }

  if(!checkSourceRange(typedefDecl->getSourceRange())){
    return;
  }

  string oldText = Rewrite.getRewrittenText(typedefDecl->getSourceRange());
  QualType ptrType = type->getPointeeType();
  Type * typePtr = const_cast<Type*>(ptrType.getTypePtr());
  RecordDecl * recordDecl = NULL;

  if(ElaboratedType * elabType = dyn_cast<ElaboratedType>(typePtr)){
    typePtr = const_cast<Type*>(elabType->getNamedType().getTypePtr());
    if(RecordType * recordType = dyn_cast<RecordType>(typePtr)){
      recordDecl = recordType->getDecl();
    }
  }

  stringstream ss;
 
  if(recordDecl != NULL && oldText.find("{") != string::npos){
    // Special case for typedef struct foo { ... } * Identifier;
    ss << Rewrite.getRewrittenText(recordDecl->getSourceRange());
    ss << ";\n";
  }

  ss << "typedef " << safePtrType << "< " << ptrType.getAsString() 
     << " > " << typedefDecl->getNameAsString();

  RewriteSource(typedefDecl->getSourceRange(),ss.str());

  if(DEBUG){
    llvm::outs() << "Done\n";
  }
}

/*
 * This method is really ugly because Clang doesn't provide a good mechanism for 
 * specifying the Source Range of a template parameter in a variable declaration
 *
 * It should be replaced by a more robust method as soon as Clang provides a method to do this.  
 * For now, the method takes the current, rewritten variable declaration and manually replaces 
 * the pointer types in the string, then overwrites the entire variable declaration 
 * with the new string.
 *
 */

void translateTemplateArguments(VarDecl * varDecl, TemplateSpecializationType * templateSpecType){
  if(DEBUG){
    llvm::outs() << "Translating Template Parameter\n";
  }

  if(!checkSourceRange(varDecl->getSourceRange())){
    return;
  }

  stringstream ss;
  string smartPtrType;
  for(TemplateSpecializationType::iterator I = templateSpecType->begin(), 
	E = templateSpecType->end(); I != E; ++I){
    const TemplateArgument templateArg = *I;
    QualType type = templateArg.getAsType();
    
    if(type->isPointerType()){
      string typeString = getTypeWithoutClass(type.getAsString());
      QualType ptrType = type->getPointeeType();
      string currentSource = Rewrite.getRewrittenText(varDecl->getSourceRange());
      int position = currentSource.find(typeString);
      if(currentSource.find(typeString) == string::npos){
        // Try without a space between T * (T*)
        position = typeString.find(" ");
        if(typeString.find(" ") != string::npos){
          typeString = typeString.erase(position,1);
          position = currentSource.find(typeString);
        }
      }
      if(currentSource.find(typeString) != string::npos){
        string newType = safePtrType + " < " + 
	  getTypeWithoutClass(ptrType.getAsString()) + " > ";
        currentSource.replace(position,typeString.size(),newType);
        ss << currentSource;
        RewriteSource(varDecl->getSourceRange(),ss.str());
      }
    }
  }

  if(DEBUG){
    llvm::outs() << "Done\n";
  }
}


bool isSmartPointerType(QualType type, string & smartPtrType){
  string typeString = getTypeWithoutClass(type.getAsString());
  for(std::vector<std::string>::iterator I = smartPointerTypes.begin(), 
	E = smartPointerTypes.end(); I != E; ++I){
    if(typeString.find(*I) != std::string::npos){
      smartPtrType = *I;
      return true;
    }
  }
  return false;
}

void translateMallocCall(CallExpr * callExpr, Expr *LHS){
  llvm::outs() << "Error: Remove all calls to malloc before translating to Safe C++\n";
}

void translateCast(ExplicitCastExpr * castExpr, QualType type){
  if(DEBUG){
    llvm::outs() << "Translating Cast\n";
  }

  stringstream ss;
  ss << builder->buildCast(castExpr,type);

  RewriteSource(castExpr->getSourceRange(),ss.str());

  if(DEBUG){
    llvm::outs() << "Done\n";
  }
}

void translateNewExpr(CXXNewExpr * newExpr, Expr * expr = NULL){
  if(DEBUG){
    llvm::outs() << "Translating New Expression\n";
  }

  if(!checkSourceRange(newExpr->getSourceRange())){
    return;
  }

  string newExprString = builder->buildCXXNewExpr(newExpr,expr);

  RewriteSource(newExpr->getSourceRange(),newExprString);

  cleanArrayCode(newExpr);

  if(DEBUG){
    llvm::outs() << "Done\n";
  }
}

void cleanArrayCode(Expr * newExpr){
  SourceLocation endLoc(newExpr->getLocEnd());
  endLoc = endLoc.getLocWithOffset(Rewrite.getRangeSize(newExpr->getSourceRange()));
  SourceRange editRange(newExpr->getLocStart(), endLoc);

  string currentSource = Rewrite.getRewrittenText(editRange);

  size_t i;
  int parens = 0;
  size_t startOffset = 0;
  size_t endOffset = 0;

  for(i = 0; i < currentSource.length(); i++){
    if(currentSource[i] == '('){
      ++parens;
    }else if(currentSource[i] == ')'){
      --parens;
    }else if(currentSource[i] == '['){
      if(parens == 0){
        startOffset = i;
      }
    }else if(currentSource[i] == ']'){
      if(parens == 0){
        endOffset = i + 1;
      }
    }else if(currentSource[i] == ';'){
      if(startOffset != 0 && endOffset != 0){
        currentSource = currentSource.erase(startOffset,endOffset-startOffset);
        Rewrite.ReplaceText(editRange,currentSource);
      }
      break;
    }
  }
}

void translateSystemCall(CallExpr * callExpr, FunctionDecl * calleeDecl){
  if(DEBUG){
    llvm::outs() << "Translating System Call\n";
  }

  string systemCall = builder->buildSystemCall(callExpr);

  if(systemCall.compare("") != 0){
    RewriteSource(callExpr->getSourceRange(),systemCall);
  }

  if(DEBUG){
    llvm::outs() << "Done\n";
  }
}
  /*
  /*
   * Handle the argv parameter to the main function
   */

  void translateMain(FunctionDecl * functionDecl){
    if(functionDecl->getNumParams() == 2){
      ParmVarDecl * argV = functionDecl->getParamDecl(1);
      string replaceArgV("char ** old_argv");
      RewriteSource(argV->getSourceRange(),replaceArgV);

      if(Stmt * body = functionDecl->getBody()){
        if(CompoundStmt * compoundStmt = dyn_cast<CompoundStmt>(body)){
          string handleArgV("\n  " + safeArrayType + " < " + 
			    safePtrType + 
			    " < char > > argv = safe::handleArgv(argc,old_argv);\n");
          Rewrite.InsertTextAfterToken(compoundStmt->getLBracLoc(),handleArgV);
	}
      }
    }
  }

std::vector<FileID> getFilesToWrite(){
  return filesToWrite;
}

private:

/*
 * Helper method for return type translation that calculates the string length
 * of the return type
 */

int getReturnTypeLength(FunctionDecl * functionDecl){
  int length = 0;
  int classLength = 0;
  string currentSource = Rewrite.getRewrittenText(functionDecl->getSourceRange());

  if(CXXMethodDecl * methodDecl = dyn_cast<CXXMethodDecl>(functionDecl)){
    if(CXXRecordDecl * recordDecl = methodDecl->getParent()){
      string recordName = getTypeWithoutClass(recordDecl->getNameAsString());
      if(currentSource.find(recordName) != string::npos){
        if(methodDecl->isVirtual()){
          classLength = currentSource.find(recordName,recordName.size()+7);
        }else{
          classLength = currentSource.find(recordName,recordName.size());
        }
      }
    }
  }

  length = currentSource.find(functionDecl->getNameAsString());

  if(classLength != 0 && classLength < length){
    length = classLength;
  }

  return length;
}

/*
 * Helper method that finds the return type of an overridden method in the deepest 
 * parent class that contains this method
 * 
 * This method is necessary for meeting C++ standards regarding covariant
 * return types (must be either:
 *   1) pointers or references to classes in the same hierarchy
 *   2) the same type (which is what we use)
 */

QualType getInheritedReturnType(CXXMethodDecl * methodDecl){
  if(methodDecl->size_overridden_methods() > 0){
    CXXMethodDecl::method_iterator iter = methodDecl->begin_overridden_methods();
    CXXMethodDecl * nextParentMethod = const_cast<CXXMethodDecl*>(*iter);
    return getInheritedReturnType(nextParentMethod);
  }else{
    return methodDecl->getResultType();
  }
}

bool checkSourceRange(SourceRange range){
 return checkSourceLocations(range.getBegin(), range.getEnd());
}

bool checkSourceLocations(SourceLocation begin, SourceLocation end){
  bool isValid = begin.isValid() && end.isValid();

  if(isValid){
    FileID id = SM.getFileID(begin);
    if(std::find(filesToWrite.begin(),filesToWrite.end(),id) == filesToWrite.end()){
      filesToWrite.push_back(id);
    }
  }

  return isValid;
}

bool checkSourceLocation(SourceLocation begin){
  bool isValid = begin.isValid();

  if(isValid){
    FileID id = SM.getFileID(begin);
    if(std::find(filesToWrite.begin(),filesToWrite.end(),id) == filesToWrite.end()){
      filesToWrite.push_back(id);
    }
  }

  return isValid;
}

};

#endif
