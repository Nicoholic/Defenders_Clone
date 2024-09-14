// added 1/11/2024 by DD for file input
#ifndef _LIGHTDATALOAD_H_
#define _LIGHTDATALOAD_H_
#include <set>
#include <vector>
#include <string>
#include <sstream>

namespace H2B {
	class LightDataLoad
	{
	public:
		char version[4];
		std::vector<GW::MATH::GVECTORF> lightData;
		bool LoadLights(const char* _lightDataPath)
		{
			Clear();
			std::ifstream file(_lightDataPath);// open file
			if (!file.is_open()) {// file isn't open
				return false;
			}
			
			GW::MATH::GVECTORF currLight = {};
			std::string line;
			bool startReadFile = true;
			while (std::getline(file, line)) {// loop until end of file
				if (line == "LIGHT") {
					std::getline(file, line);
					if (line == "Point") {
						std::getline(file, line);
						std::istringstream stringStream(line);
						stringStream >> currLight.x >> currLight.y >> currLight.z >> currLight.w;
						lightData.push_back(currLight);
					}
				}
			}
			file.close();

			return true;
		}
		void Clear()
		{
			*reinterpret_cast<unsigned*>(version) = 0;
			lightData.clear();
		}
	};
}
#endif