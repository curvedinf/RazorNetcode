#pragma once

#include <string>
#include <cstring>
#include <sstream>
#include <cmath>

namespace razor {
	inline constexpr auto VECTOR_MAX = 64;
	
	unsigned int copyIn(void* data, unsigned int position, auto in_value);
	unsigned int copyInArray(void *data, unsigned int position, auto* in_array, unsigned int length);
	unsigned int copyInCString(void *data, unsigned int position, const char* in);
	unsigned int copyInString(void *data, unsigned int position, std::string* in);
	unsigned int copyInBV(void *data, unsigned int position, bool* in, unsigned char bool_num);
	
	unsigned int copyOut(auto* out_value, void *data, unsigned int position);
	unsigned int copyOutArray(auto* out_array, void *data, unsigned int position, unsigned int length);
	unsigned int copyOutCString(char* out, void *data, unsigned int position);
	unsigned int copyOutString(std::string* out, void *data, unsigned int position);
	unsigned int copyOutBV(bool* out, unsigned char* bool_num, void *data, unsigned int position);
	
	int serializationUnitTest();
};