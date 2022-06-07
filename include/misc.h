#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <iostream>
#include <curl/curl.h>

#define RAZOR_NANOS_PER_MILLI 1000000LL
#define RAZOR_NANOS_PER_SECOND 1000000000LL

namespace razor {
	unsigned long long nanoNow();
	
	std::string urlEncode(std::string& s);
	
	std::string urlDecode(std::string& s);
	
	bool download(const std::string& url, 
				std::string* output,
				std::vector<std::pair<std::string, std::string>>* get_args);
}
