// added 1/11/2024 by DD for file input
#ifndef _MATPARSER_H_
#define _MATPARSER_H_
#include <set>
#include <vector>
#include <string>
#include <sstream>

namespace H2B {
	class MatParser
	{
	public:
		char version[4];
		unsigned materialCount;
		std::vector<ATTRIBUTES> materials;
		bool Parse(const char* matPath)
		{
			Clear();
			std::ifstream file(matPath);// open file
			if (!file.is_open()) {// file isn't open
				return false;
			}
			
			ATTRIBUTES currMat = {};
			std::string line;
			bool startReadFile = true;
			while (std::getline(file, line)) {// loop until end of file
				std::istringstream stringStream(line);
				std::string token;
				stringStream >> token;
				if (token == "newmtl") {
					if (!startReadFile) {
						materials.push_back(currMat);
						currMat = ATTRIBUTES();
					}
					else {
						startReadFile = !startReadFile;
					}
				}
				else if (token == "Ns") {
					stringStream >> currMat.Ns;
				}
				else if (token == "Ka") {
					stringStream >> currMat.Ka.x >> currMat.Ka.y >> currMat.Ka.z;
				}
				else if (token == "Kd") {
					stringStream >> currMat.Kd.x >> currMat.Kd.y >> currMat.Kd.z;
				}
				else if (token == "Ks") {
					stringStream >> currMat.Ks.x >> currMat.Ks.y >> currMat.Ks.z;
				}
				else if (token == "Ke") {
					stringStream >> currMat.Ke.x >> currMat.Ke.y >> currMat.Ke.z;
				}
				else if (token == "Ni") {
					stringStream >> currMat.Ni;
				}
				else if (token == "d") {
					stringStream >> currMat.d;
				}
				else if (token == "illum") {
					stringStream >> currMat.illum;
				}
			}
			file.close();
			materials.push_back(currMat);

			return true;
		}
		void Clear()
		{
			*reinterpret_cast<unsigned*>(version) = 0;
			materials.clear();
		}
	};
}
#endif