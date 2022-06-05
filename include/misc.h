#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <iostream>

#include <curl/curl.h>

#define NANOS_PER_MILLI 1000000LL
//#define NANOS_PER_SECOND NANOS_PER_MILLI * 1000ULL
#define NANOS_PER_SECOND 1000000000LL
#define MAX_NAME_LENGTH 63
#define INFERNO_TICK_RATE 150.0
#define TPS_NORM_MULTIPLIER (1000.0 / INFERNO_TICK_RATE)
#define TPS_NORM_RATIO (INFERNO_TICK_RATE / 1000.0)
#define SERIALIZATION_BUFFER_SIZE 1024*1024*1 // 5 MB

namespace misc {
	std::vector<std::string> split(std::string& s, std::string delim);
	std::vector<std::string> splitCommand(std::string command);
	std::string comGet(std::vector<std::string> command_parts, unsigned int part_number);
	
	unsigned long long nanoNow();
	
	std::string url_encode(std::string& s);
	std::string url_decode(std::string& s);
	
	bool download(const std::string& url, 
				std::string* output,
				std::vector<std::pair<std::string, std::string>>* get_args);
}
