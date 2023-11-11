#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Parse/Parser.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/Preprocessor.h>


using namespace clang;
std::string getStringFromRange(SourceRange Range, ASTContext &Ctx) {
  SourceManager &sourceManager = Ctx.getSourceManager();

  if (Range.isValid()) {
	  clang::SourceLocation b(Range.getBegin()), _e(Range.getEnd());
	  clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, sourceManager, Ctx.getLangOpts()));

	  bool invalidTemp = false;
	  const char* begin = sourceManager.getCharacterData(b, &invalidTemp);
	  if (invalidTemp) return "";

	  const char* end = sourceManager.getCharacterData(e, &invalidTemp);
	  if (invalidTemp) return "";

	  if (begin && end && begin < end) {
		  // The length of the definition string
		  size_t length = end - begin;
		  return std::string(begin, length);
	  }
  }

  return ""; // Return an empty string if the range is not valid
}

std::string GetFunctionDeclarationLocationOrInclude(const clang::FunctionDecl *FD, const clang::SourceManager &SM) {
    if (!FD)
        return "";

	const clang::FunctionDecl *Def = FD->getDefinition();
	if (!Def) {
		// This is a use of an imported function --- just skip
		return "";
	}

	return "TODO --- set up handling of functions" ;
}

std::string getTypeStringFromFD(const FunctionDecl *FD, ASTContext &Ctx) {
  SourceManager &SM = Ctx.getSourceManager();
  std::string FunctionTypeStr;

  // Get the return type as a string.
  QualType ReturnType = FD->getReturnType();
  FunctionTypeStr += ReturnType.getAsString(Ctx.getPrintingPolicy()) + " ";

  // Get the function name.
  FunctionTypeStr += FD->getNameInfo().getName().getAsString() + "(";

  // Iterate over the parameters and get their types as strings.
  for (unsigned i = 0, e = FD->getNumParams(); i != e; ++i) {
    if (i > 0) FunctionTypeStr += ", "; // Add a comma between parameters.

    const ParmVarDecl *Param = FD->getParamDecl(i);
    assert(Param && "Param decl cannot be null");

    // Add the parameter type string.
    QualType ParamType = Param->getType();
    FunctionTypeStr += ParamType.getAsString(Ctx.getPrintingPolicy());

    // If the parameter has a name, add it to the signature.
    if (!Param->getName().empty()) {
      FunctionTypeStr += " " + Param->getName().str();
    }
  }

  // If the function is a variadic function, add '...'.
  if (FD->isVariadic()) {
    FunctionTypeStr += ", ...";
  }

  FunctionTypeStr += ")";

  // Retrieve exception specification.
  if (FD->hasWrittenPrototype() && FD->getType()->isFunctionProtoType()) {
    const FunctionProtoType *FPT = FD->getType()->getAs<FunctionProtoType>();
    if (FPT && FPT->hasExceptionSpec() && FPT->getExceptionSpecType() == EST_Dynamic) {
      FunctionTypeStr += " throw(";
      for (FunctionProtoType::exception_iterator EI = FPT->exception_begin(), EE = FPT->exception_end(); EI != EE; ++EI) {
        if (EI != FPT->exception_begin()) FunctionTypeStr += ", ";
        FunctionTypeStr += EI->getAsString(Ctx.getPrintingPolicy());
      }
      FunctionTypeStr += ")";
    }
  }

  if (FD->hasBody()) {
	  const Stmt *Body = FD->getBody();

	  // Convert the body statement into a string.
	  bool Invalid = false;
	  StringRef BodyText = Lexer::getSourceText(
			  CharSourceRange::getTokenRange(Body->getSourceRange()), SM, Ctx.getLangOpts(), &Invalid);

	  if (!Invalid) {
		  FunctionTypeStr += "" + BodyText.str() + "";
	  } else {
		  llvm::errs() << "The source text for the function body could not be retrieved.\n";
	  }
  }

  return FunctionTypeStr;
}

std::string getTypeStringFromTypeLoc(const TypeLoc &TL, ASTContext &Ctx) {
  // Get the source range that covers the TypeLoc.
  QualType qualType = TL.getType();
  const Type* type = qualType.getTypePtr();

  SourceRange Range;

  if (const TagType* tagType = type->getAs<TagType>()) {
	  TagDecl* tagDecl = tagType->getDecl();
	  if (tagDecl) {
		  Range = tagDecl->getSourceRange();
	  }
  } else if (const TypedefType* typedefType = type->getAs<TypedefType>()) {
	  TypedefNameDecl* typedefDecl = typedefType->getDecl();
	  if (typedefDecl) {
		  Range = typedefDecl->getSourceRange();
	  }
  }

  return getStringFromRange(Range, Ctx);
}
