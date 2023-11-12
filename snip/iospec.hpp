#ifndef IOSPEC_H
#define IOSPEC_H

#include "clang/AST/AST.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <nlohmann/json.hpp>
#include <set>
#include <map>
#include <list>

std::string convert_type(const std::string &name);

class IOSpecType {
	public:
		IOSpecType(): type("struct") {}

		void add_symbol(std::string symbol) {
			symbols.push_back(symbol);
		}

		void set_type(std::string name, std::string type) {
			typemap[name] = type;
		}

		nlohmann::json to_json() {
			nlohmann::json j = nlohmann::json{{"type", type}, {"symbols", symbols}, {"typemap", typemap}};
			return j;
		}

		std::string type;
		std::list<std::string> symbols;
		std::map<std::string, std::string> typemap;
};

IOSpecType build_type_for(clang::RecordDecl *decl);

class IOSpecRange {
	public:
		IOSpecRange() {}

		nlohmann::json to_json() {
			// TODO -- nee dto set this up
			return "";
		}
};

void to_json(nlohmann::json &j, const IOSpecType &type);
void to_json(nlohmann::json &j, const IOSpecRange &range);

class IOSpec {
	public:
		std::string fname;
		std::list<std::string> args;
		std::list<std::string> livein;
		std::list<std::string> liveout;
		std::map<std::string, std::string> types;
		std::map<std::string, IOSpecType> classmap;
		std::map<std::string, IOSpecRange> rangemap;

		IOSpec() {}

		nlohmann::json to_json() {
			nlohmann::json j;

			j["fname"] = fname;
			j["args"] = args;
			j["livein"] = livein;
			j["liveout"] = liveout;
			j["types"] = types;
			j["classmap"] = classmap;
			j["rangemap"] = rangemap;

			return j;
		}

		void set_fname(clang::FunctionDecl *FD) {
			fname = FD->getNameAsString();
		}

		void set_args(clang::FunctionDecl *FD) {
			for (unsigned i = 0; i < FD->getNumParams(); ++i) {
				clang::ParmVarDecl *param = FD->getParamDecl(i);
				args.push_back(param->getNameAsString());
			}
		}

		// initially, we are going to set everyting
		// as livein.
		void set_livein() {
			livein = args;
		}

		// we set all array/pointer types as liveout.
		void set_liveout() {
			for (auto it = args.begin(); it != args.end(); ++it) {
				std::string type = types[*it];

				if ((type.find("array") != std::string::npos) || (type.find("pointer") != std::string::npos)) {
					liveout.push_back(*it);
				}
			}
		}

		// Set the types from the function parameters
		void set_types(clang::FunctionDecl *FD) {
			for (unsigned i = 0; i < FD->getNumParams(); ++i) {
				clang::ParmVarDecl *param = FD->getParamDecl(i);
				types[param->getNameAsString()] = (convert_type(param->getType().getAsString()));
			}
		}

		// Go through the typemap, find every undefined type,
		// and get the struct def required for that type.
		void set_classmap(clang::FunctionDecl *FD) {
			for (unsigned i = 0; i < FD->getNumParams(); ++i) {
				clang::ParmVarDecl *param = FD->getParamDecl(i);
				clang::QualType paramType = param->getType();

				set_classmap_from_type(&paramType);
			}
		}

		void set_classmap_from_type(clang::QualType *type) {
			if (type->isNull())
				return;

			if (type->getTypePtr()->isRecordType()) {
				const clang::Type *typePtr = type->getTypePtr();
				if (const clang::RecordType *recordType = typePtr->getAs<clang::RecordType>()) {
					clang::RecordDecl *recordDecl = recordType->getDecl();
					std::string recordName = recordDecl->getNameAsString();
					if (classmap.find(recordName) == classmap.end()) { // Check if not already in map
						classmap[recordName] = build_type_for(recordDecl);
					} else {
						// Already insert, do not need to recurse.
						return;
					}
				}

				// Check for subtypes and recurse
				if (const clang::PointerType *pointerType = typePtr->getAs<clang::PointerType>()) {
					clang::QualType ptype = pointerType->getPointeeType();
					set_classmap_from_type(&ptype);
				} else if (typePtr->isArrayType()) {
					const clang::ArrayType *arrayType = clang::dyn_cast<clang::ArrayType>(type->getTypePtr());
					clang::QualType qtype = arrayType->getElementType();
					set_classmap_from_type(&qtype);
				} else if (const clang::FunctionProtoType *functionType = typePtr->getAs<clang::FunctionProtoType>()) {
					llvm::outs() << "Unsupported fnction proto type\n";
					// unsupoorted.
					/* for (unsigned i = 0; i < functionType->getNumParams(); ++i) { */
					/* 	set_classmap(functionType->getParamType(i)); */
					/* } */
					/* set_classmap(functionType->getReturnType()); */
				}
				// Add other composite types as needed
			}

		}
};

IOSpec build_iospec(clang::FunctionDecl *FD);

#endif
