#ifndef SYNTHESIS_H
#define SYNTHESIS_H

#include <nlohmann/json.hpp>

class SynthesisResult {
	public:
		bool changed;
		std::string new_function;
};

class Synthesizer {
	public:
		SynthesisResult Synthesize(std::string existing_program, nlohmann::json metadata);
};

class IdentitySynthesizer: Synthesizer {
	public:
		IdentitySynthesizer() {}
		SynthesisResult Synthesize(std::string existing_program, nlohmann::json metadata) {
			SynthesisResult result;
			result.changed = true;
			result.new_function = existing_program;
			return result;
		}
};


#endif
