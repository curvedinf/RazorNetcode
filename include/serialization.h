#pragma once

#include <string>
#include <cmath>
#include <vector>
#include <sstream>

#include "math.h"

#define RAZOR_SERIALIZATION_VECTOR_MAX 64

namespace razor {
	// all functions return how many bytes the position should be advanced
	// for the next element in the data.

	unsigned int copyInB(void *data, unsigned int position, bool in);
	unsigned int copyInC(void *data, unsigned int position, char in);
	unsigned int copyInS(void *data, unsigned int position, short in);
	unsigned int copyInI(void *data, unsigned int position, int in);
	unsigned int copyInF(void *data, unsigned int position, float in);
	unsigned int copyInLL(void *data, unsigned int position, long long in);
	unsigned int copyInD(void *data, unsigned int position, double in);
	unsigned int copyInData(void *data, unsigned int position, void* in, unsigned int length);
	unsigned int copyInCString(void *data, unsigned int position, char* in);
	unsigned int copyInString(void *data, unsigned int position, std::string* in);
	unsigned int copyInV3(void* data, unsigned int position, Vector3* v);
	unsigned int copyInQ4(void* data, unsigned int position, Quaternion* q);
	unsigned int copyInM44(void* data, unsigned int position, Matrix44* m);
	unsigned int copyInBV(void *data, unsigned int position, bool* in, unsigned char bool_num); // supports up to 255 bools

	//

	unsigned int copyOutB(bool* out, void *data, unsigned int position);
	unsigned int copyOutC(char* out, void *data, unsigned int position);
	unsigned int copyOutS(short* out, void *data, unsigned int position);
	unsigned int copyOutI(int* out, void *data, unsigned int position);
	unsigned int copyOutF(float* out, void *data, unsigned int position);
	unsigned int copyOutLL(long long* out, void *data, unsigned int position);
	unsigned int copyOutD(double* out, void *data, unsigned int position);
	unsigned int copyOutData(void* out, void *data, unsigned int position, unsigned int length);
	unsigned int copyOutCString(char* out, void *data, unsigned int position);
	unsigned int copyOutString(std::string* out, void *data, unsigned int position);
	unsigned int copyOutV3(Vector3* out, void* data, unsigned int position);
	unsigned int copyOutQ4(Quaternion* out, void* data, unsigned int position);
	unsigned int copyOutM44(Matrix44* out, void* data, unsigned int position);
	unsigned int copyOutBV(bool* out, unsigned char* bool_num, void *data, unsigned int position); // supports up to 255 bools

	int serializationUnitTest();
};