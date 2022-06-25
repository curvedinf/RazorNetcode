#pragma once

#include <string>
#include <cstring>
#include <sstream>
#include <cmath>
#include <iostream>

namespace razor {
	inline constexpr auto VECTOR_MAX = 64;
	
	// Note: auto parameter functions must be in the header file due to how 
	// they must be compiled after a translation unit (cpp file) has declared
	// the types that will be used.
	
	// Copy in a fixed-length datatype
	inline unsigned int copyIn(void* data, unsigned int position, auto in_value) {
		// Get the memory address
		auto address = (uint8_t*)data+position;
		// 1) Get the type of in_value and remove any reference from the type
		// 2) Get the pointer type of that type
		// 3) Cast address to the pointer type
		// 4) Dereference address
		// 5) Assign in_value to dereferenced address
		*(std::remove_reference_t<decltype(in_value)>*)address = in_value;
		return sizeof(std::remove_reference_t<decltype(in_value)>);
	}
	// Copy in a fixed-length array of a single fixed-length datatype
	inline unsigned int copyInArray(void *data, unsigned int position, 
			auto* in_array, unsigned int length)	{
		auto copy_len = length * sizeof(std::remove_reference_t<decltype(*in_array)>);
		std::memcpy((uint8_t*)data+position, in_array, copy_len);
		return copy_len;
	}
	unsigned int copyInCString(void *data, unsigned int position, const char* in);
	unsigned int copyInString(void *data, unsigned int position, std::string* in);
	unsigned int copyInBV(void *data, unsigned int position, bool* in, unsigned char bool_num);
	
	inline unsigned int copyOut(auto* out_value, void *data, unsigned int position) {
		*out_value = *((std::remove_reference_t<decltype(*out_value)>*)((uint8_t*)data+position));
		return sizeof(std::remove_reference_t<decltype(*out_value)>);
	}
	inline unsigned int copyOutArray(auto* out_array, void *data, 
			unsigned int position, unsigned int length) {
		auto copy_len = length * sizeof(std::remove_reference_t<decltype(*out_array)>);
		std::memcpy(out_array, (uint8_t*)data+position, copy_len);
		return copy_len;
	}
	unsigned int copyOutCString(char* out, void *data, unsigned int position);
	unsigned int copyOutString(std::string* out, void *data, unsigned int position);
	unsigned int copyOutBV(bool* out, unsigned char* bool_num, void *data, unsigned int position);
	
	int serializationUnitTest();
};