#include <string>
#include <cmath>
#include <vector>
#include <sstream>

#include "serialization.h"

// all functions return how many bytes the position should be advanced
// for the next element in the data.

unsigned int copyInB(void *data, unsigned int position, bool in) {
	*((bool*)(data+position)) = in;
	return sizeof(bool);
}

unsigned int copyInC(void *data, unsigned int position, char in) {
	*((char*)(data+position)) = in;
	return sizeof(char);
}

unsigned int copyInS(void *data, unsigned int position, short in) {
	*((short*)(data+position)) = in;
	return sizeof(short);
}

unsigned int copyInI(void *data, unsigned int position, int in) {
	*((int*)(data+position)) = in;
	return sizeof(int);
}

unsigned int copyInF(void *data, unsigned int position, float in) {
	*((float*)(data+position)) = in;
	return sizeof(float);
}

unsigned int copyInLL(void *data, unsigned int position, long long in) {
	*((long long*)(data+position)) = in;
	return sizeof(long long);
}

unsigned int copyInD(void *data, unsigned int position, double in) {
	*((double*)(data+position)) = in;
	return sizeof(double);
}

unsigned int copyInData(void *data, unsigned int position, void* in, unsigned int length) {
	if(length>0)
		memcpy(data+position, in, length);
	return length;
}

unsigned int copyInCString(void *data, unsigned int position, char* in) {
	int length = 0;
	int sl = strlen(in);
	// TODO change ints to shorts
	length += copyInI(data, position, sl);
	if(sl!=0) {
		length += copyInData(data, position+length, (void*)in, sl);
	}
	return length;
}

unsigned int copyInString(void *data, unsigned int position, std::string* in) {
	short length = 0;
	// TODO control max string size
	int sl = in->size();
	length += copyInI(data, position, sl);
	if(sl!=0) {
		length += copyInData(data, position+length, (void*)in->c_str(), sl);
	}
	return length;
}

unsigned int copyInV3(void* data, unsigned int position, Vector3* v) {
	int ds = sizeof(double);
	copyInD(data, position, v->x);
	copyInD(data, position+ds, v->y);
	copyInD(data, position+ds*2, v->z);
	return ds*3;
}

unsigned int copyInQ4(void* data, unsigned int position, Quaternion* q) {
	int ds = sizeof(double);
	copyInD(data, position, q->X);
	copyInD(data, position+ds, q->Y);
	copyInD(data, position+ds*2, q->Z);
	copyInD(data, position+ds*3, q->W);
	return ds*4;
}

unsigned int copyInM44(void* data, unsigned int position, Matrix44* m) {
	int size = sizeof(double)*16;
	copyInData(data, position, m->matrix, size);
	return size;
}

unsigned int copyInBV(void *data, unsigned int position, bool* in, unsigned char bool_num) {
	int length = 0;
	
	if(bool_num > SERIALIZATION_VECTOR_MAX) {
		std::stringstream ss;
		ss << "Bool vector exceeded maximum size during serialization (" <<
			bool_num << ") out of range (" << SERIALIZATION_VECTOR_MAX << ")";
		throw std::range_error(ss.str());
	}
	length += copyInC(data, position, bool_num);
	
	char packed_bools[8];
	memset(packed_bools, 0, 8);
	
	int packed_bool_num = std::ceil(bool_num / 8.0f);
	int bool_i = 0;
	for(int packed_i = 0; packed_i<packed_bool_num; packed_i++) {
		for(int inner_i=0; inner_i<8 && bool_i<bool_num; inner_i++) {
			if(in[bool_i]) {
				packed_bools[packed_i] |= ((unsigned char)1) << inner_i;
			}
			bool_i++;
		}
	}
	
	length += copyInData(data, position+length, (void*)packed_bools, packed_bool_num);
	return length;
}

//

unsigned int copyOutB(bool* out, void *data, unsigned int position) {
	*out = *((bool*)(data+position));
	return sizeof(bool);
}

unsigned int copyOutC(char* out, void *data, unsigned int position) {
	*out = *((char*)(data+position));
	return sizeof(char);
}

unsigned int copyOutS(short* out, void *data, unsigned int position) {
	*out = *((short*)(data+position));
	return sizeof(short);
}

unsigned int copyOutI(int* out, void *data, unsigned int position) {
	*out = *((int*)(data+position));
	return sizeof(int);
}

unsigned int copyOutF(float* out, void *data, unsigned int position) {
	*out = *((float*)(data+position));
	return sizeof(float);
}

unsigned int copyOutLL(long long* out, void *data, unsigned int position) {
	*out = *((long long*)(data+position));
	return sizeof(long long);
}

unsigned int copyOutD(double* out, void *data, unsigned int position) {
	*out = *((double*)(data+position));
	return sizeof(double);
}

unsigned int copyOutData(void* out, void *data, unsigned int position, unsigned int length) {
	if(length>0)
		memcpy(out, data+position, length);
	return length;
}

unsigned int copyOutCString(char* out, void *data, unsigned int position) {
	int str_len = 0;
	int data_copied = 0;
	data_copied += copyOutI(&str_len, data, position);
	
	if(str_len!=0) {
		memcpy(out, data+position+data_copied, str_len);
		out[str_len] = 0;
		data_copied += str_len;
	}
	
	return data_copied;
}

unsigned int copyOutString(std::string* out, void *data, unsigned int position) {
	int str_len = 0;
	int data_copied = 0;
	data_copied += copyOutI(&str_len, data, position);
	
	if(str_len!=0) {
		out->resize(str_len);
		out->assign((char *)(data+position+data_copied), str_len);
		data_copied += str_len;
	}
	
	return data_copied;
}

unsigned int copyOutV3(Vector3* out, void* data, unsigned int position) {
	int ds = sizeof(double);
	copyOutD(&(out->x), data, position);
	copyOutD(&(out->y), data, position+ds);
	copyOutD(&(out->z), data, position+ds*2);
	return ds*3;
}

unsigned int copyOutQ4(Quaternion* out, void* data, unsigned int position) {
	int ds = sizeof(double);
	copyOutD(&(out->X), data, position);
	copyOutD(&(out->Y), data, position+ds);
	copyOutD(&(out->Z), data, position+ds*2);
	copyOutD(&(out->W), data, position+ds*3);
	return ds*4;
}

unsigned int copyOutM44(Matrix44* out, void* data, unsigned int position) {
	int size = sizeof(double)*16;
	copyOutData(&(out->matrix), data, position, size);
	return size;
}

unsigned int copyOutBV(bool* out, unsigned char* bool_num, void *data, unsigned int position) {
	int length = 0;
	length += copyOutC((char*)bool_num, data, position);
	// TODO check sanity of bool num size
	
	char packed_bools[8];
	memset(packed_bools, 0, 8);
	int packed_bool_num = std::ceil(*bool_num / 8.0f);
	
	length += copyOutData((void*)packed_bools, data, position+length, packed_bool_num);
	
	int bool_i = 0;
	for(int packed_i = 0; packed_i<packed_bool_num; packed_i++) {
		for(int inner_i=0; inner_i<8 && bool_i<*bool_num; inner_i++) {
			if(packed_bools[packed_i] & (((unsigned char)1) << inner_i))
				out[bool_i] = true;
			else
				out[bool_i] = false;
			bool_i++;
		}
	}
	
	return length;
}

int serialization_unitTest() {
	
	// Test serialization
	bool bin = true;
	char chin = 15;
	short sin = 16;
	int iin = 17;
	long long llin = 18;
	float fin = 19.5;
	double din = 20.5;
	Vector3 v3in = Vector3(100,100,100);
	Quaternion qin = Quaternion(100,100,100,1);
	Matrix44 m44in;
	m44in.identity();
	std::string strin(R"(
But I must explain to you how all this mistaken idea of denouncing pleasure and praising pain 
was born and I will give you a complete account of the system, and expound the actual teachings 
of the great explorer of the truth, the master-builder of human happiness. No one rejects, 
dislikes, or avoids pleasure itself, because it is pleasure, but because those who do not know 
how to pursue pleasure rationally encounter consequences that are extremely painful. Nor again 
is there anyone who loves or pursues or desires to obtain pain of itself, because it is pain, 
but because occasionally circumstances occur in which toil and pain can procure him some great 
pleasure. To take a trivial example, which of us ever undertakes laborious physical exercise, 
except to obtain some advantage from it? But who has any right to find fault with a man who 
chooses to enjoy a pleasure that has no annoying consequences, or one who avoids a pain that 
produces no resultant pleasure?
)");
	bool bvin[9]; // vector of 9 bools
	bvin[0] = true;
	bvin[1] = true;
	bvin[2] = true;
	bvin[3] = false;
	bvin[4] = true;
	bvin[5] = true;
	bvin[6] = false;
	bvin[7] = true;
	bvin[8] = true;
	
	char data[5000];
	int p = 0;
	int out = 0;
	
	out = copyInC(data, p, chin);
	if(out != 1) return 1;
	p += out;
	
	out = copyInS(data, p, sin);
	if(out != 2) return 2;
	p += out;
	
	out = copyInI(data, p, iin);
	if(out != 4) return 3;
	p += out;
	
	out = copyInLL(data, p, llin);
	if(out != 8) return 4;
	p += out;
	
	out = copyInF(data, p, fin);
	if(out != 4) return 5;
	p += out;
	
	out = copyInD(data, p, din);
	if(out != 8) return 6;
	p += out;
	
	out = copyInV3(data, p, &v3in);
	if(out != 3*8) return 7;
	p += out;
	
	out = copyInQ4(data, p, &qin);
	if(out != 4*8) return 8;
	p += out;
	
	out = copyInM44(data, p, &m44in);
	if(out != 16*8) return 9;
	p += out;
	
	out = copyInString(data, p, &strin);
	if(out != strin.size() + 4) return 10;
	p += out;
	
	out = copyInB(data, p, bin);
	if(out != 1) return 11;
	p += out;
	
	out = copyInBV(data, p, bvin, 9);
	if(out != 3) return 12;
	p += out;
	
	// Test derialization
	
	bool bout = false;
	char chout = 0;
	short sout = 0;
	int iout = 0;
	long long llout = 0;
	float fout = 0;
	double dout = 0;
	Vector3 v3out;
	Quaternion qout;
	Matrix44 m44out;
	std::string strout;
	bool bvout[9];
	unsigned char bvoutc;
	
	int in_len = p;
	p = 0;
	out = 0;
	
	out = copyOutC(&chout, data, p);
	if(out != 1 || chout != chin) return 100;
	p += out;
	
	out = copyOutS(&sout, data, p);
	if(out != 2 || sout != sin) return 101;
	p += out;
	
	out = copyOutI(&iout, data, p);
	if(out != 4 || iout != iin) return 102;
	p += out;
	
	out = copyOutLL(&llout, data, p);
	if(out != 8 || llout != llin) return 103;
	p += out;
	
	out = copyOutF(&fout, data, p);
	if(out != 4 || fout != fin) return 104;
	p += out;
	
	out = copyOutD(&dout, data, p);
	if(out != 8 || dout != din) return 105;
	p += out;
	
	out = copyOutV3(&v3out, data, p);
	if(out != 3*8 || v3out.x != v3in.x || v3out.y != v3in.y || v3out.z != v3in.z) return 106;
	p += out;
	
	out = copyOutQ4(&qout, data, p);
	if(out != 4*8 || qout.W != qin.W || qout.X != qin.X || qout.Y != qin.Y || qout.Z != qin.Z) return 107;
	p += out;
	
	out = copyOutM44(&m44out, data, p);
	if(out != 16*8 || !(m44out == m44in)) return 108;
	p += out;
	
	out = copyOutString(&strout, data, p);
	if(out != strin.size() + 4 || !(strout == strin)) return 109;
	p += out;
	
	out = copyOutB(&bout, data, p);
	if(out != 1 || bout != bin) return 110;
	p += out;
	
	out = copyOutBV(bvout, &bvoutc, data, p);
	if(out != 3 || bvoutc != 9) return 111;
	p += out;
	
	if(in_len != p) return 200;
	
	return 0;
}