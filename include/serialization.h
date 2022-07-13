#pragma once

#include <string>
#include <cstring>
#include <sstream>
#include <cmath>
#include <iostream>
#include <cstddef>

namespace razor {
	inline constexpr auto BOOL_VECTOR_MAX = 64;
	
	// Note: template functions must be in the header file due to how 
	// they must be compiled after a translation unit (cpp file) has declared
	// the types that will be used.
	
	// Copy in a fixed-length datatype
	template<class T> inline unsigned int copyIn(void* data, unsigned int position, T in_value) {
		auto address = (uint8_t*)data+position;
		std::memcpy(address, &in_value, sizeof(in_value));
		return sizeof(T);
	}
	
	// Copy in a fixed-length array of a single fixed-length datatype
	template<class T> inline unsigned int copyInArray(void *data, unsigned int position, 
			T* in_array, unsigned int length)	{
		auto copy_len = length * sizeof(T);
		auto address = (uint8_t*)data+position;
		std::memcpy(address, in_array, copy_len);
		return copy_len;
	}
	
	unsigned int copyInCString(void *data, unsigned int position, const char* in);
	unsigned int copyInString(void *data, unsigned int position, std::string* in);
	unsigned int copyInBV(void *data, unsigned int position, bool* in, unsigned char bool_num);
	
	template<class T> inline unsigned int copyOut(T* out_value, void *data, unsigned int position) {
		auto address = (uint8_t*)data+position;
		std::memcpy(out_value, address, sizeof(T));
		return sizeof(T);
	}
	
	template<class T> inline unsigned int copyOutArray(T* out_array, void *data, 
			unsigned int position, unsigned int length) {
		auto copy_len = length * sizeof(T);
		auto address = (uint8_t*)data+position;
		std::memcpy(out_array, address, copy_len);
		return copy_len;
	}
	
	unsigned int copyOutCString(char* out, void *data, unsigned int position);
	unsigned int copyOutString(std::string* out, void *data, unsigned int position);
	unsigned int copyOutBV(bool* out, unsigned char* bool_num, void *data, unsigned int position);
	
	int serializationUnitTest();
};