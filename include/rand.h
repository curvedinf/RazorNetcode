#pragma once

#include <string>

// This is used a cross-platform predictable, captured PRNG for random numbers
// generated inside the Inferno Server

#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))

struct InfernoRandData {
	unsigned long long a;
	unsigned long long b;
	unsigned long long c;
	unsigned long long d; 
};

unsigned long long inferno_rand(InfernoRandData *x);
void inferno_srand(InfernoRandData *x, unsigned long long seed );

unsigned long long inferno_hash(std::string& str);