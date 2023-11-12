#include "iospec.hpp"
#include "clang/AST/Decl.h"

#include <string>

std::string convert_type(const std::string& name) {
    if (name == "char" || name == "signed char") {
        return "int8";
    } else if (name == "short" || name == "short int" || name == "signed short" || name == "signed short int") {
        return "int16";
    } else if (name == "int" || name == "signed int" || name == "signed") {
        return "int32";
    } else if (name == "long int" || name == "long signed int" || name == "long" || name == "signed long" || name == "signed long int") {
        return "int64";
    } else if (name == "unsigned char") {
        return "uint8";
    } else if (name == "unsigned short" || name == "unsigned short int") {
        return "uint16";
    } else if (name == "unsigned" || name == "unsigned int") {
        return "uint32";
    } else if (name == "unsigned long" || name == "unsigned long int") {
        return "uint64";
    } else if (name == "float") {
        return "float32";
    } else if (name == "double") {
        return "float64";
    } else if (name == "bool") {
        return "bool";
    } else if (name == "char*" || name == "std::string" || name == "string") {
        return "string";
    } else if (name.find("*") != std::string::npos) {
        return "array(" + convert_type(name.substr(0, name.size() - 1)) + ")";
	} else if (false) {
		// TODO --- when to return pointer?
        return "pointer(<other type>)";
    }
    // Add more conditions as needed for other types

	// probably a struct --- do we need to strip the struct part?
    return name;
}

IOSpecType build_type_for(clang::RecordDecl *decl) {
	IOSpecType result;

	for (auto it = decl->field_begin(); it != decl->field_end(); ++it) {
		clang::FieldDecl *field = *it;
		std::string fieldName = field->getNameAsString();
		std::string field_type = convert_type(field->getType().getAsString());

		result.add_symbol(fieldName);
		result.set_type(fieldName, field_type);
	}

	return result;
}

void to_json(nlohmann::json &j, const IOSpecType &type) {
	j = nlohmann::json{{"type", type.type}, {"symbols", type.symbols}, {"typemap", type.typemap}};
}

void to_json(nlohmann::json &j, const IOSpecRange &range) {
	j = ""; // todo 
}

IOSpec build_iospec(clang::FunctionDecl *FD) {
	IOSpec result;

	// note that ordering here is important as the builder functions depend on it
	result.set_fname(FD);
	result.set_args(FD);
	result.set_livein();
	result.set_liveout();
	result.set_types(FD);
	result.set_classmap(FD);

	return result;
}
