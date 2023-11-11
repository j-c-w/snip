#ifndef SYNTHESIS_H
#define SYNTHESIS_H

class SynthesisResult {
	public:
		bool changed;
		std::string new_function;
};

class Synthesizer {
	public:
		SynthesisResult Synthesize(std::string existing_program);
};

class IdentitySynthesizer: Synthesizer {
	public:
		IdentitySynthesizer() {}
		SynthesisResult Synthesize(std::string existing_program) {
			SynthesisResult result;
			result.changed = true;
			result.new_function = existing_program;
			return result;
		}
};


#endif
