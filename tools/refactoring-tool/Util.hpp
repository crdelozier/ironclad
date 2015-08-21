#ifndef UTIL_H
#define UTIL_H

#include <string>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/DeclarationName.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/AST/TypeLocVisitor.h"

using namespace clang;

namespace{

string cleanContext(string current, NamedDecl * namedDecl = NULL){
  size_t start;
  if(namedDecl){
    /*
    if(ValueDecl * valueDecl = dyn_cast<ValueDecl>(namedDecl)){
      LangOptions options;
      PrintingPolicy policy(options);
      policy.Dump = 0;
      policy.Bool = 1;
      policy.SuppressUnwrittenScope = 1;
      policy.SuppressTagKeyword = 1;
      current = valueDecl->getType().getAsString(policy);
      }else{*/
      DeclContext * context = namedDecl->getLexicalDeclContext();
      string name = namedDecl->getNameAsString();

      if(context){
	do{
	  if(context->isTranslationUnit()){
	    break;
	  }
	  if(NamedDecl * parentDecl = dyn_cast<NamedDecl>(context)){
	    RecordDecl * parentRecordDecl = dyn_cast<RecordDecl>(parentDecl);
	    RecordDecl * recordDecl = dyn_cast<RecordDecl>(namedDecl);

	    if(recordDecl == NULL || parentRecordDecl == NULL || 
	       recordDecl->getNameAsString().compare(parentRecordDecl->getNameAsString()) != 0){
	  
	      string token = parentDecl->getNameAsString();
	      if(token.find("class ") != std::string::npos){
		start = token.find("class ");
		token = token.erase(start,6);
	      }
	      token = token.append("::");
	      if(current.find(token) != std::string::npos){
		start = current.find(token);
		current = current.erase(start,token.size());
	      }
	    }
	  }
	}while((context = context->getParent()) != NULL);
      }
      //}
  }
  return current;
 }

string getTypeWithoutClass(string oldType,NamedDecl * namedDecl = NULL){
  string ret = oldType;
  size_t start;

  while(ret.find("class ") != std::string::npos){
    start = oldType.find("class ");
    ret = ret.erase(start,6);
  }

  ret = cleanContext(ret,namedDecl);

  return ret;
}

  NamedDecl * extractDecl(Expr * expr){
    expr = expr->IgnoreParenCasts();
    NamedDecl * namedDecl = NULL;

    if(DeclRefExpr * declRefExpr = dyn_cast<DeclRefExpr>(expr)){
      namedDecl = declRefExpr->getDecl();
    }else if(CallExpr * callExpr = dyn_cast<CallExpr>(expr)){
      namedDecl = callExpr->getDirectCallee();
    }else if(MemberExpr * memberExpr = dyn_cast<MemberExpr>(expr)){
      namedDecl = memberExpr->getMemberDecl();
    }else if(isa<StringLiteral>(expr)){
      // Do Nothing
    }else if(isa<GNUNullExpr>(expr)){
      // Do Nothing
    }else if(isa<UnaryOperator>(expr)){
      // Do Nothing
    }else if(ArraySubscriptExpr * subscriptExpr = dyn_cast<ArraySubscriptExpr>(expr)){
      namedDecl = extractDecl(subscriptExpr->getBase());
    }else if(dyn_cast<CXXThisExpr>(expr)){
      // TODO: Figure out what to do here, if necessary
    }else if(dyn_cast<CXXBoolLiteralExpr>(expr)){
      // Do nothing
    }else if(dyn_cast<IntegerLiteral>(expr)){
      // Do nothing
    }else if(ConditionalOperator * condOp = dyn_cast<ConditionalOperator>(expr)){
      // For now, if either are pointer types, return the first one
      NamedDecl * trueDecl = extractDecl(condOp->getTrueExpr());
      NamedDecl * falseDecl = extractDecl(condOp->getFalseExpr());
      if(trueDecl){
        if(ValueDecl * vTrue = dyn_cast<ValueDecl>(trueDecl)){
	  if(vTrue->getType()->isPointerType()){
	    return trueDecl;
	  }
        }
      }
      if(falseDecl){
        if(ValueDecl * vFalse = dyn_cast<ValueDecl>(falseDecl)){
	  if(vFalse->getType()->isPointerType()){
	    return falseDecl;
	  }
        }
      }
    }else if(dyn_cast<BinaryOperator>(expr)){
      // Do nothing
    }else if(dyn_cast<CXXDependentScopeMemberExpr>(expr)){
      // Not sure if we can or need to do anything here
    }else{
      //llvm::outs() << "Unhandled Extract Expression Type: " << expr->getStmtClassName() << "\n";
    }
    return namedDecl;
  }

}

#endif
