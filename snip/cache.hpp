#ifndef CACHE_H
#define CACHE_H

#include <filesystem>
#include <iostream>
#include <functional>
#include <fstream>

class CacheManager {
	public:
		std::string location;

		CacheManager(std::string location) {
			//Make the location into a directory if it doesn't exist
			if (!std::filesystem::exists(location)) {
				std::filesystem::create_directory(location);
			}
			this->location = location;
		}

		void write(std::string ast, std::string contents) {
			std::string hash = this->hash(ast);

			// write contents to location/hash.c, where
			std::ofstream outfile;
			outfile.open(this->location + "/" + hash + ".c");
			outfile << contents;
			outfile.close();
		}

		std::string read(std::string ast) {
			std::string hash = this->hash(ast);

			// read contents from location/hash.c, where hash is the hash of the ast
			std::ifstream infile;
			infile.open(this->location + "/" + hash + ".c");
			std::string contents;
			std::string line;
			while (std::getline(infile, line)) {
				contents += line + "\n";
			}
			infile.close();
			return contents;
		}
	private:
		std::string hash(std::string clang_ast) {
			std::hash<std::string> hasher;
			return std::to_string(hasher(clang_ast));
		}
};

#endif
