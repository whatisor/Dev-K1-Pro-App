
#ifndef COMMONUTILS_H
#define COMMONUTILS_H
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <string>
#include <chrono>
#include <cstdint>


class CommonUtils
{
public:
	static std::unordered_map<std::string, uint64_t> fpsTimeStart;
	static std::unordered_map<std::string, uint32_t> fpsTime;
	static std::unordered_map<std::string, uint32_t> fpsTimeCounter;

	static void timerStart(std::string key)
	{
		fpsTimeStart[key] = getCurrentMilliseconds();
	}

	static void timerStop(std::string key, int max = 1)
	{
		fpsTime[key] = getCurrentMilliseconds() - fpsTimeStart[key];
		fpsTimeCounter[key]++;
		if (fpsTimeCounter[key] == max)
		{
			std::stringstream str;
			str << "\t\t\t[PROFILE]" << key << ": " << fpsTime[key] * 1.0 / fpsTimeCounter[key] << std::endl;
			std::cout << str.str();
			//writeToFile(str.str());
			fpsTimeCounter[key] = 0;
			fpsTime[key] = 0;
		}
	}

	static void writeToFile(std::string content, std::string fileName = "profile.txt")
	{
		std::ofstream file;
		file.open(fileName, std::ofstream::app);
		file << content;
		file.close();
	}
	static uint64_t getCurrentMilliseconds()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
#define _timerS(key) CommonUtils::timerStart(key);
#define _timerE(key) CommonUtils::timerStop(key);
#define _timerDataInit std::unordered_map<std::string, uint64_t> CommonUtils::fpsTimeStart;\
					   std::unordered_map<std::string, uint32_t> CommonUtils::fpsTime;\
					   std::unordered_map<std::string, uint32_t> CommonUtils::fpsTimeCounter;
};

#endif