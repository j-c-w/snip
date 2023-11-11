#ifndef STRING_UTILS_H
#define STRING_UTILS_H

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

std::string getStringFromRange(SourceRange Range, ASTContext &Ctx);
std::string GetFunctionDeclarationLocationOrInclude(const clang::FunctionDecl *FD, const clang::SourceManager &SM);
std::string getTypeStringFromFD(const FunctionDecl *FD, ASTContext &Ctx);
std::string getTypeStringFromTypeLoc(const TypeLoc &TL, ASTContext &Ctx);

#endif
