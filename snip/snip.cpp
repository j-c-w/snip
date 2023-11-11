//==============================================================================
// FILE:
//    FunctionSnip.cpp
//
// DESCRIPTION:
//    Counts the number of C++ record declarations in the input translation
//    unit. The results are printed on a file-by-file basis (i.e. for each
//    included header file separately).
//
//    Internally, this implementation leverages llvm::StringMap to map file
//    names to the corresponding #count of declarations.
//
// USAGE:
//   clang -cc1 -load <BUILD_DIR>/lib/libFunctionSnip.dylib '\'
//    -plugin hello-world test/FunctionSnip-basic.cpp
//
// License: The Unlicense
//==============================================================================
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
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/Preprocessor.h"


#include <unordered_set>
#include <set>
#include <string>
#include <sstream>

#include "synthesis.hpp"
#include "cache.hpp"
#include "string_utils.hpp"
#include "snip.hpp"
#include "includes.hpp"

using namespace clang;

std::string setToString(const std::set<std::string>& mySet) {
    std::ostringstream oss;
    for (auto it = mySet.begin(); it != mySet.end(); ) {
        oss << *it;
        if (++it != mySet.end()) {
            oss << ", ";
        }
    }
    return oss.str();
}

/* TODO --- Register PPCallbacks fro mincludes.hpp and then fill out this */
/* function to use it somehow. */
std::string getIncludes(CompilerInstance &CI) {
	std::string includes = "";
	std::ostringstream oss;
	llvm::outs() << "Includes are " << setToString(IncludeCollector::includes) << "\n";

	for (auto it = IncludeCollector::includes.begin(); it != IncludeCollector::includes.end(); it ++) {
		oss << "#include \"" << *it << "\"\n";
	}

	return oss.str();
}


class Dependencies {
	public:
		Dependencies() {};

		std::unordered_set<std::string> functionCalls;
		std::unordered_set<std::string> functionDefs;
		std::unordered_set<std::string> variableAccesses;
		std::unordered_set<std::string> typeUses;

		std::unordered_set<std::string> typeDefs;

		void clear() {
			functionCalls.clear();
			variableAccesses.clear();
			typeUses.clear();
			typeDefs.clear();
		}
};

class FunctionDependencyVisitor : public RecursiveASTVisitor<FunctionDependencyVisitor> {
public:
  explicit FunctionDependencyVisitor(ASTContext *Context) : Context(Context) {}
  Dependencies currentFunctionDependencies;

  // Visit FunctionDecl to initiate the dependency analysis for each function
  bool VisitFunctionDecl(FunctionDecl *FD) {
    if (FD->hasBody()) {
      // Clear previous function's dependencies
      currentFunctionDependencies.clear();

      // Traverse the function's body to accumulate dependencies
      TraverseStmt(FD->getBody());
    }
    return true;
  }

  // Visit CallExpr to find function calls
  bool VisitCallExpr(CallExpr *CE) {
    if (FunctionDecl *FD = CE->getDirectCallee()) {
      currentFunctionDependencies.functionCalls.insert(FD->getQualifiedNameAsString());
	  if (FD->hasBody()) {
		  /* llvm::outs() << "Def for " << FD->getQualifiedNameAsString() << "\n"; */
		  SourceManager &SM = Context->getSourceManager();
		  LangOptions LOpt = Context->getLangOpts();

		  SourceRange SR = FD->getSourceRange();
		  bool Invalid = false;
		  StringRef FuncStr = Lexer::getSourceText(CharSourceRange::getTokenRange(SR), SM, LOpt, &Invalid);

		  if (!Invalid) {
			  currentFunctionDependencies.functionDefs.insert(std::string(FuncStr));
		  }
	  }
    }
    return true;
  }

  // Visit DeclRefExpr to find variable accesses
  bool VisitDeclRefExpr(DeclRefExpr *DRE) {
    if (ValueDecl *VD = DRE->getDecl()) {
      // Check if the variable has external linkage and is not a local static variable
	  if (isa<FunctionDecl>(VD)) {
		  return true;
	  }
      if (VD->hasLinkage() && !VD->getDeclContext()->isFunctionOrMethod()) {
		  /* llvm::outs() << "Def of " << VD->getQualifiedNameAsString() << " is " << Context->getFullLoc(VD->getLocation()).getSpellingLineNumber(); */
		  currentFunctionDependencies.variableAccesses.insert(VD->getType().getAsString() + " " + VD->getQualifiedNameAsString() + ";");
      }
    }
    return true;
  }

  // Visit TypeLoc to find type uses
  bool VisitTypeLoc(TypeLoc TL) {
    if (TL.getType()->isRecordType()) {
		  // llvm::outs() << "Def of " << TL->getQualifiedNameAsString() << " is " << Context->getFullLoc(TL->getLocation()).getSpellingLineNumber();
		// llvm::outs() << "Type is: " << getTypeStringFromTypeLoc(TL, *Context) << "\n";
	  currentFunctionDependencies.typeDefs.insert(getTypeStringFromTypeLoc(TL, *Context));
      currentFunctionDependencies.typeUses.insert(TL.getType().getAsString());
    }
    return true;
  }

private:
  ASTContext *Context;

};


//-----------------------------------------------------------------------------
// RecursiveASTVisitor
//-----------------------------------------------------------------------------
class FunctionSnip : public RecursiveASTVisitor<FunctionSnip> {
public:
  IdentitySynthesizer synthesizer;
  CompilerInstance &CI;

  explicit FunctionSnip(ASTContext *Context, std::string cache_location, CompilerInstance &CI) : Context(Context), cache(cache_location), CI(CI) {}
  bool VisitCXXRecordDecl(CXXRecordDecl *Decl);

  llvm::StringMap<unsigned> getDeclMap() { return DeclMap; }

  bool VisitFunctionDecl(clang::FunctionDecl *FD) {
	  if (!FD->hasBody()) {
		  // If the FD has no body, we can't really
		  // lift here -- -this ends up pointing
		  // to a lot of library functions.
		  return true;
	  }
	  llvm::outs() << "Visiting a function: " << FD->getNameAsString() << "\n";
	  FunctionDependencyVisitor visitor(Context);
	  visitor.TraverseDecl(FD);

	  std::string original_function = getTypeStringFromFD(FD, *Context);

	  // combine all these into a single rnnable string with the function
	  // body also
	  std::string runnable_string = "";
	  runnable_string += getIncludes(CI);
	  for (const auto &func: visitor.currentFunctionDependencies.functionCalls) {
		  runnable_string += "// function: " + func + "\n";
	  }
	  for (const auto &func : visitor.currentFunctionDependencies.functionDefs) {
		  runnable_string += func + "\n";
	  }
	  for (const auto &var : visitor.currentFunctionDependencies.variableAccesses) {
		  runnable_string += var + "\n";
	  }
	  for (const auto &type : visitor.currentFunctionDependencies.typeDefs) {
		  runnable_string += type + "\n";
	  }
	  runnable_string += "\n\n";
	  runnable_string += original_function;
	  runnable_string += "\n";

	  SynthesisResult result = synthesizer.Synthesize(runnable_string);
	  llvm::outs() << "Runnable is " << runnable_string << "\n";
	  llvm::outs() << "Result is " << result.new_function << "\n";


	  // Now, do the replacement of this fuction body if the code
	  // was updated.
	  if (result.changed) {
		  llvm::outs() << "Updated: replacing the function " << "\n";

		  std::string NewBodyCode = result.new_function;
		  // TODO --- Manage this better.
		  cache.write(original_function, result.new_function);
	  }

	  return true;
  }

private:
  CacheManager cache;
  ASTContext *Context;
  // Map that contains the count of declaration in every input file
  llvm::StringMap<unsigned> DeclMap;
};


bool FunctionSnip::VisitCXXRecordDecl(CXXRecordDecl *Declaration) {
  FullSourceLoc FullLocation = Context->getFullLoc(Declaration->getBeginLoc());

  // Basic sanity checking
  if (!FullLocation.isValid())
    return true;

  // There are 2 types of source locations: in a file or a macro expansion. The
  // latter contains the spelling location and the expansion location (both are
  // file locations), but only the latter is needed here (i.e. where the macro
  // is expanded). File locations are just that - file locations.
  if (FullLocation.isMacroID())
    FullLocation = FullLocation.getExpansionLoc();

  SourceManager &SrcMgr = Context->getSourceManager();
  const FileEntry *Entry =
      SrcMgr.getFileEntryForID(SrcMgr.getFileID(FullLocation));
  DeclMap[Entry->getName()]++;

  return true;
}

void setNewBodyForFunction(FunctionDecl *FD, Stmt *NewBody, ASTContext &Ctx) {
	if (FD == nullptr || NewBody == nullptr) {
		return;
	}

	FD->setBody(NewBody);
}

//-----------------------------------------------------------------------------
// ASTConsumer
//-----------------------------------------------------------------------------
class FunctionSnipASTConsumer : public clang::ASTConsumer {
public:
  explicit FunctionSnipASTConsumer(ASTContext *Ctx, std::string cache_location, CompilerInstance &Compiler) : Visitor(Ctx, cache_location, Compiler) {}

  void HandleTranslationUnit(clang::ASTContext &Ctx) override {
    Visitor.TraverseDecl(Ctx.getTranslationUnitDecl());

    if (Visitor.getDeclMap().empty()) {
      llvm::outs() << "(clang-tutor)  no declarations found "
                   << "\n";
      return;
    }

    for (auto &Element : Visitor.getDeclMap()) {
      llvm::outs() << "(clang-tutor)  file: " << Element.first() << "\n";
      llvm::outs() << "(clang-tutor)  count: " << Element.second << "\n";
    }
  }

private:
  FunctionSnip Visitor;
};

static llvm::cl::opt<std::string> FunctionCache(
		"snip-cache",
		llvm::cl::desc("Folder where the function lifting cache is stored"),
		llvm::cl::value_desc("foldername"),
		llvm::cl::init("snip_cache"),
		llvm::cl::Optional
);

//-----------------------------------------------------------------------------
// FrontendAction for FunctionSnip
//-----------------------------------------------------------------------------
class FindNamedClassAction : public clang::PluginASTAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    llvm::StringRef InFile) override {
	  // llvm::outs() << "setting up ast consumer\n";
    return std::unique_ptr<clang::ASTConsumer>(
        std::make_unique<FunctionSnipASTConsumer>(&Compiler.getASTContext(), FunctionCache, Compiler));
  }
  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }

  void ExecuteAction() override {
	  Preprocessor &PP = getCompilerInstance().getPreprocessor();
	  // llvm::outs() << "Setting up include collector\n";
	  PP.addPPCallbacks(std::make_unique<IncludeCollector>(getCompilerInstance().getSourceManager()));

	  PluginASTAction::ExecuteAction();
  }
};


//-----------------------------------------------------------------------------
// Registration
//-----------------------------------------------------------------------------

static FrontendPluginRegistry::Add<FindNamedClassAction>
    X(/*Name=*/"snipper", /*Description=*/"The FunctionSnip plugin");

// Regisre the include collector
static FrontendPluginRegistry::Add<IncludeCollectorPlugin>
	Y("include-collector", "Collect the includes required");
