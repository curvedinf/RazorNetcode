#include "rand.h"

unsigned long long inferno_rand(InfernoRandData *x) {
	unsigned long long e = x->a - rot(x->b, 27);
	x->a = x->b ^ rot(x->c, 17);
	x->b = x->c + x->d;
	x->c = x->d + e;
	x->d = e + x->a;
	return x->d;
}

void inferno_srand(InfernoRandData *x, unsigned long long seed ) {
	unsigned long long i;
	x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
	for (i=0; i<2; ++i) {
		(void)inferno_rand(x);
	}
}

unsigned long long inferno_hash(std::string& str) {
	unsigned long long a = 0;
	for(int i=0; i<str.size(); i++) {
		int rot = i % (64-8);
		a ^= str[i] << rot;
	}
	InfernoRandData rd;
	inferno_srand(&rd, a);
	return inferno_rand(&rd);
}