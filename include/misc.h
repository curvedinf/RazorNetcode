#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <curl/curl.h>
#include <SDL2/SDL.h>

namespace razor {
	typedef unsigned long long int nanotime;
	typedef unsigned int millitime;
	inline constexpr nanotime NANOS_PER_MILLI = 1'000'000ULL;
	inline constexpr nanotime NANOS_PER_SECOND = 1'000'000'000ULL;
	
	nanotime nanoNow();
	
	void sleep(millitime time);
	void busyWait(nanotime time);
	
	std::string urlEncode(std::string& s);
	std::string urlDecode(std::string& s);
	
	bool download(const std::string& url, std::string* output,
				std::vector<std::pair<std::string, std::string>>* get_args);
}
