#ifndef INCLUDES_H
#define INCLUDES_H

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include <sstream>
#include "clang/Basic/SourceManager.h"

using namespace clang;

// A PPCallbacks subclass to collect include files
class IncludeCollector : public clang::PPCallbacks {
	private:
		clang::SourceManager &sm;
		static std::set<std::string> filteredIncludes;

	public:
		static std::set<std::string> includes;

		IncludeCollector(clang::SourceManager &SM) : sm(SM) {}

		virtual void InclusionDirective(clang::SourceLocation hash_loc,
                        const clang::Token &includeTok,
                        clang::StringRef fileName,
                        bool isAngled,
                        clang::CharSourceRange filenameRange,
                        const clang::OptionalFileEntryRef file,
                        clang::StringRef searchPath,
                        clang::StringRef relativePath,
                        const clang::Module *imported,
                        clang::SrcMgr::CharacteristicKind FileType) override {
			if (file) {
				clang::FullSourceLoc fullLoc(hash_loc, sm);
				if (fullLoc.isValid() && isIncludable(file->getName().data())) {
					includes.insert(file->getName().data());
				}
			}
		}

		bool isIncludable(const std::string & fileName) {
			for (const auto& filter : filteredIncludes) {
				if (fileName.find(filter) != std::string::npos) {
					return false;
				}
			}
			return true;
		}

		std::set<std::string> getIncludes() const {
			return includes;
		}
};

// Wrap the PPCallback in a plugin
class IncludeCollectorPlugin : public PluginASTAction {
	protected:
		std::unique_ptr<ASTConsumer> 
 CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) override {
			return std::make_unique<ASTConsumer>();
		}

		bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string> &args) override {
			return true;
		}

		void ExecuteAction() override {
			Preprocessor &PP = getCompilerInstance().getPreprocessor();
			llvm::outs() << "Setting up include collector\n";
			PP.addPPCallbacks(std::make_unique<IncludeCollector>(getCompilerInstance().getSourceManager()));
		}
};

#endif
