#pragma once

// Should probably switch to https://github.com/g-truc/glm or http://google.github.io/mathfu/

#define M_PI 3.14159265358979323846

#include <string>
#include <cstring>
#include <sstream>
#include <cmath>

class Vector3 {
public:
	double x, y, z;
	Vector3() {
		this->zero();
	}
	Vector3(double x, double y, double z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}
	Vector3(Vector3* o) {
		this->copy(o);
	}
	Vector3* zero() {
		this->x = 0;
		this->y = 0;
		this->z = 0;
		return this;
	}
	std::string str() {
		std::stringstream ss;
		ss << "[" << this->x << ", " << this->y << ", " << this->z << "]";
		return ss.str();
	}
	double magSq(){
		return this->x*this->x + this->y*this->y + this->z*this->z;
	}
	double mag(){
		return sqrt(this->x*this->x + this->y*this->y + this->z*this->z);
	}
	Vector3* normalize() {
		return this->mult(1.0f/this->mag());
	}
	Vector3* add(Vector3* o){
		this->x += o->x;
		this->y += o->y;
		this->z += o->z;
		return this;
	}
	Vector3* sub(Vector3* o){
		this->x -= o->x;
		this->y -= o->y;
		this->z -= o->z;
		return this;
	}
	Vector3* mult(double o){
		this->x *= o;
		this->y *= o;
		this->z *= o;
		return this;
	}
	Vector3* copy(Vector3* o){
		this->x = o->x;
		this->y = o->y;
		this->z = o->z;
		return this;
	}
	double dot(Vector3 *o) {
		return this->x * o->x + this->y * o->y + this->z * o->z;
	}
	Vector3* cross(Vector3* o) {
		Vector3 temp;
		temp.x = this->y * o->z - this->z * o->y;
		temp.y = this->z * o->x - this->x * o->z;
		temp.z = this->x * o->y - this->y * o->x;
		return this->copy(&temp);
	}
	Vector3* project(Vector3 *o) {
		return this->mult(o->dot(this)/this->dot(this));
	}
	Vector3* lerp(Vector3 *o, double t) {
		Vector3 temp;
		this->add(temp.copy(o)->sub(this)->mult(t));
		return this;
	}
	Vector3* randomUnit() { // DO NOT USE WITHIN A SERVER -- WILL NOT SYNCRONIZE
		this->x = rand() % 1000 - 500;
		this->y = rand() % 1000 - 500;
		this->z = rand() % 1000 - 500;
		this->normalize();
		return this;
	}
};

class Matrix44 {
public:
	double matrix[16];
	double& operator[] (const int index)
	{
		return matrix[index];
	}
	bool operator== (Matrix44& o)
	{
		if(
			this->matrix[0] == o[0] &&
			this->matrix[1] == o[1] &&
			this->matrix[2] == o[2] &&
			this->matrix[3] == o[3] &&
			this->matrix[4] == o[4] &&
			this->matrix[5] == o[5] &&
			this->matrix[6] == o[6] &&
			this->matrix[7] == o[7] &&
			this->matrix[8] == o[8] &&
			this->matrix[9] == o[9] &&
			this->matrix[10] == o[10] &&
			this->matrix[11] == o[11] &&
			this->matrix[12] == o[12] &&
			this->matrix[13] == o[13] &&
			this->matrix[14] == o[14] &&
			this->matrix[15] == o[15]
		) {
			return true;
		} 
		return false;
	}
	bool isZero() {
		return this->matrix[0]==0 &&
				this->matrix[1]==0 &&
				this->matrix[2]==0 &&
				this->matrix[3]==0 &&
				this->matrix[4]==0 &&
				this->matrix[5]==0 &&
				this->matrix[6]==0 &&
				this->matrix[7]==0 &&
				this->matrix[8]==0 &&
				this->matrix[9]==0 &&
				this->matrix[10]==0 &&
				this->matrix[11]==0 &&
				this->matrix[12]==0 &&
				this->matrix[13]==0 &&
				this->matrix[14]==0 &&
				this->matrix[15]==0;
	}
	Matrix44* copy(Matrix44* o) {
		std::memcpy(this->matrix,o->matrix,sizeof(Matrix44));
		return this;
	}
	Matrix44* identity() {
		this->matrix[0] = 1;
		this->matrix[1] = 0;
		this->matrix[2] = 0;
		this->matrix[3] = 0;
		this->matrix[4] = 0;
		this->matrix[5] = 1;
		this->matrix[6] = 0;
		this->matrix[7] = 0;
		this->matrix[8] = 0;
		this->matrix[9] = 0;
		this->matrix[10] = 1;
		this->matrix[11] = 0;
		this->matrix[12] = 0;
		this->matrix[13] = 0;
		this->matrix[14] = 0;
		this->matrix[15] = 1;
		return this;
	}
	Matrix44* lerp(Matrix44* o, double f) {
		this->matrix[0] = (o->matrix[0] - this->matrix[0]) * f + this->matrix[0];
		this->matrix[1] = (o->matrix[1] - this->matrix[1]) * f + this->matrix[1];
		this->matrix[2] = (o->matrix[2] - this->matrix[2]) * f + this->matrix[2];
		this->matrix[3] = (o->matrix[3] - this->matrix[3]) * f + this->matrix[3];
		this->matrix[4] = (o->matrix[4] - this->matrix[4]) * f + this->matrix[4];
		this->matrix[5] = (o->matrix[5] - this->matrix[5]) * f + this->matrix[5];
		this->matrix[6] = (o->matrix[6] - this->matrix[6]) * f + this->matrix[6];
		this->matrix[7] = (o->matrix[7] - this->matrix[7]) * f + this->matrix[7];
		this->matrix[8] = (o->matrix[8] - this->matrix[8]) * f + this->matrix[8];
		this->matrix[9] = (o->matrix[9] - this->matrix[9]) * f + this->matrix[9];
		this->matrix[10] = (o->matrix[10] - this->matrix[10]) * f + this->matrix[10];
		this->matrix[11] = (o->matrix[11] - this->matrix[11]) * f + this->matrix[11];
		this->matrix[12] = (o->matrix[12] - this->matrix[12]) * f + this->matrix[12];
		this->matrix[13] = (o->matrix[13] - this->matrix[13]) * f + this->matrix[13];
		this->matrix[14] = (o->matrix[14] - this->matrix[14]) * f + this->matrix[14];
		this->matrix[15] = (o->matrix[15] - this->matrix[15]) * f + this->matrix[15];
		return this;
	}
	Matrix44* multiply(Matrix44* o) {
		Matrix44 result;
		/*result[0] = this->matrix[0] * o->matrix[0] + this->matrix[1] * o->matrix[4] 
						+ this->matrix[2] * o->matrix[8] + this->matrix[3] * o->matrix[12];
		result[1] = this->matrix[0] * o->matrix[1] + this->matrix[1] * o->matrix[5] 
						+ this->matrix[2] * o->matrix[9] + this->matrix[3] * o->matrix[13];
		result[2] = this->matrix[0] * o->matrix[2] + this->matrix[1] * o->matrix[6] 
						+ this->matrix[2] * o->matrix[10] + this->matrix[3] * o->matrix[14];
		result[3] = this->matrix[0] * o->matrix[3] + this->matrix[1] * o->matrix[7] 
						+ this->matrix[2] * o->matrix[11] + this->matrix[3] * o->matrix[15];
		
		result[4] = this->matrix[4] * o->matrix[0] + this->matrix[5] * o->matrix[4] 
						+ this->matrix[6] * o->matrix[8] + this->matrix[7] * o->matrix[12];
		result[5] = this->matrix[4] * o->matrix[1] + this->matrix[5] * o->matrix[5] 
						+ this->matrix[6] * o->matrix[9] + this->matrix[7] * o->matrix[13];
		result[6] = this->matrix[4] * o->matrix[2] + this->matrix[5] * o->matrix[6] 
						+ this->matrix[6] * o->matrix[10] + this->matrix[7] * o->matrix[14];
		result[7] = this->matrix[4] * o->matrix[3] + this->matrix[5] * o->matrix[7] 
						+ this->matrix[6] * o->matrix[11] + this->matrix[7] * o->matrix[15];
		
		result[8] = this->matrix[8] * o->matrix[0] + this->matrix[9] * o->matrix[4] 
						+ this->matrix[10] * o->matrix[8] + this->matrix[11] * o->matrix[12];
		result[9] = this->matrix[8] * o->matrix[1] + this->matrix[9] * o->matrix[5] 
						+ this->matrix[10] * o->matrix[9] + this->matrix[11] * o->matrix[12];
		result[10] = this->matrix[8] * o->matrix[2] + this->matrix[9] * o->matrix[6] 
						+ this->matrix[10] * o->matrix[10] + this->matrix[11] * o->matrix[12];
		result[11] = this->matrix[8] * o->matrix[3] + this->matrix[9] * o->matrix[7] 
						+ this->matrix[10] * o->matrix[11] + this->matrix[11] * o->matrix[12];
		
		result[12] = this->matrix[12] * o->matrix[0] + this->matrix[13] * o->matrix[4] 
						+ this->matrix[14] * o->matrix[8] + this->matrix[15] * o->matrix[12];
		result[13] = this->matrix[12] * o->matrix[1] + this->matrix[13] * o->matrix[5] 
						+ this->matrix[14] * o->matrix[9] + this->matrix[15] * o->matrix[13];
		result[14] = this->matrix[12] * o->matrix[2] + this->matrix[13] * o->matrix[6] 
						+ this->matrix[14] * o->matrix[10] + this->matrix[15] * o->matrix[14];
		result[15] = this->matrix[12] * o->matrix[3] + this->matrix[13] * o->matrix[7] 
						+ this->matrix[14] * o->matrix[11] + this->matrix[15] * o->matrix[15];
		this->copy(&result);*/
		
		result[0] = this->matrix[0]*o->matrix[0]+this->matrix[4]*o->matrix[1]+this->matrix[8]*o->matrix[2]+this->matrix[12]*o->matrix[3];
		result[1] = this->matrix[1]*o->matrix[0]+this->matrix[5]*o->matrix[1]+this->matrix[9]*o->matrix[2]+this->matrix[13]*o->matrix[3];
		result[2] = this->matrix[2]*o->matrix[0]+this->matrix[6]*o->matrix[1]+this->matrix[10]*o->matrix[2]+this->matrix[14]*o->matrix[3];
		result[3] = this->matrix[3]*o->matrix[0]+this->matrix[7]*o->matrix[1]+this->matrix[11]*o->matrix[2]+this->matrix[15]*o->matrix[3];

		result[4] = this->matrix[0]*o->matrix[4]+this->matrix[4]*o->matrix[5]+this->matrix[8]*o->matrix[6]+this->matrix[12]*o->matrix[7];
		result[5] = this->matrix[1]*o->matrix[4]+this->matrix[5]*o->matrix[5]+this->matrix[9]*o->matrix[6]+this->matrix[13]*o->matrix[7];
		result[6] = this->matrix[2]*o->matrix[4]+this->matrix[6]*o->matrix[5]+this->matrix[10]*o->matrix[6]+this->matrix[14]*o->matrix[7];
		result[7] = this->matrix[3]*o->matrix[4]+this->matrix[7]*o->matrix[5]+this->matrix[11]*o->matrix[6]+this->matrix[15]*o->matrix[7];

		result[8] = this->matrix[0]*o->matrix[8]+this->matrix[4]*o->matrix[9]+this->matrix[8]*o->matrix[10]+this->matrix[12]*o->matrix[11];
		result[9] = this->matrix[1]*o->matrix[8]+this->matrix[5]*o->matrix[9]+this->matrix[9]*o->matrix[10]+this->matrix[13]*o->matrix[11];
		result[10] = this->matrix[2]*o->matrix[8]+this->matrix[6]*o->matrix[9]+this->matrix[10]*o->matrix[10]+this->matrix[14]*o->matrix[11];
		result[11] = this->matrix[3]*o->matrix[8]+this->matrix[7]*o->matrix[9]+this->matrix[11]*o->matrix[10]+this->matrix[15]*o->matrix[11];

		result[12] = this->matrix[0]*o->matrix[12]+this->matrix[4]*o->matrix[13]+this->matrix[8]*o->matrix[14]+this->matrix[12]*o->matrix[15];
		result[13] = this->matrix[1]*o->matrix[12]+this->matrix[5]*o->matrix[13]+this->matrix[9]*o->matrix[14]+this->matrix[13]*o->matrix[15];
		result[14] = this->matrix[2]*o->matrix[12]+this->matrix[6]*o->matrix[13]+this->matrix[10]*o->matrix[14]+this->matrix[14]*o->matrix[15];
		result[15] = this->matrix[3]*o->matrix[12]+this->matrix[7]*o->matrix[13]+this->matrix[11]*o->matrix[14]+this->matrix[15]*o->matrix[15];

		this->copy(&result);
		
		return this;
	}
	Matrix44* inverseNonScalingMatrix() {
		/*
			[ux vx wx tx]
			[uy vy wy ty]
			[uz vz wz tz]
			[ 0  0  0  1]
				==>
			[ux uy uz -dot(u,t)]
			[vx vy vz -dot(v,t)]
			[wx wy wz -dot(w,t)]
			[ 0  0  0	 1	]
		*/
		Vector3 u = Vector3(this->matrix[0],this->matrix[1],this->matrix[2]);
		Vector3 v = Vector3(this->matrix[4],this->matrix[5],this->matrix[6]);
		Vector3 w = Vector3(this->matrix[8],this->matrix[9],this->matrix[10]);
		Vector3 t = Vector3(this->matrix[12],this->matrix[13],this->matrix[14]);
		
		this->matrix[1] = v.x;
		this->matrix[2] = w.x;
		this->matrix[4] = u.y;
		this->matrix[8] = u.z;
		this->matrix[9] = v.z;
		this->matrix[6] = w.y;
		this->matrix[12] = -u.dot(&t);
		this->matrix[13] = -v.dot(&t);
		this->matrix[14] = -w.dot(&t);
		return this;
	}
	
	Vector3* multiplyVector(Vector3* v) {
		double temp[4], temp2[4];
		
		temp[0] = v->x;
		temp[1] = v->y;
		temp[2] = v->z;
		temp[3] = 1;
		
		temp2[0] = 0;
		temp2[1] = 0;
		temp2[2] = 0;
		temp2[3] = 0;
		
		for(int i=0;i<4;i++) {
			for(int j=0;j<4;j++) {
				 temp2[j]+= this->matrix[(i*4)+j] * temp[i];
			}
		}
		
		v->x = temp2[0] / temp2[3];
		v->y = temp2[1] / temp2[3];
		v->z = temp2[2] / temp2[3];
		
		return v;
	}
	
	std::string str() {
		std::stringstream s;
		s << "[[" << this->matrix[0] << ", " << this->matrix[1] << ", "
				<< this->matrix[2] << ", " << this->matrix[3] << "], ["
				<< this->matrix[4] << ", " << this->matrix[5] << ", "
				<< this->matrix[6] << ", " << this->matrix[7] << "], ["
				<< this->matrix[8] << ", " << this->matrix[9] << ", "
				<< this->matrix[10] << ", " << this->matrix[11] << "], ["
				<< this->matrix[12] << ", " << this->matrix[13] << ", "
				<< this->matrix[14] << ", " << this->matrix[15] << "]]";
		return s.str();
	}
	Matrix44() {
		this->identity();
	}
	Matrix44(const Matrix44& o) {
		this->copy((Matrix44*)(&o));
	}
	void copyToFloatVector(float* fv) {
		fv[0] = this->matrix[0];
		fv[1] = this->matrix[1];
		fv[2] = this->matrix[2];
		fv[3] = this->matrix[3];
		fv[4] = this->matrix[4];
		fv[5] = this->matrix[5];
		fv[6] = this->matrix[6];
		fv[7] = this->matrix[7];
		fv[8] = this->matrix[8];
		fv[9] = this->matrix[9];
		fv[10] = this->matrix[10];
		fv[11] = this->matrix[11];
		fv[12] = this->matrix[12];
		fv[13] = this->matrix[13];
		fv[14] = this->matrix[14];
		fv[15] = this->matrix[15];
	}
	
	void getRight(Vector3* right) {
		right->x = this->matrix[0];
		right->y = this->matrix[1];
		right->z = this->matrix[2];
	}
	
	void getUp(Vector3* up) {
		up->x = this->matrix[4];
		up->y = this->matrix[5];
		up->z = this->matrix[6];
	}
	
	void getForward(Vector3* forward) {
		forward->x = -this->matrix[8];
		forward->y = -this->matrix[9];
		forward->z = -this->matrix[10];
	}
	
	void getVectors(Vector3* right, Vector3* up, Vector3* forward) {
		this->getRight(right);
		this->getUp(up);
		this->getForward(forward);
	}
	
	void getPosition(Vector3* pos) {
		pos->x = this->matrix[12];
		pos->y = this->matrix[13];
		pos->z = this->matrix[14];
	}
	
	void setPosition(double x, double y, double z) {
		this->matrix[12] = x;
		this->matrix[13] = y;
		this->matrix[14] = z;
	}
};

class Quaternion {
public:
  double W, X, Y, Z;	  // components of a quaternion

  // default constructor
  Quaternion() {
	W = 1.0;
	X = 0.0;
	Y = 0.0;
	Z = 0.0;
  }
  
  // copy constructor
  Quaternion(const Quaternion& o) {
	W = o.W;
	X = o.X;
	Y = o.Y;
	Z = o.Z;
  }
  
  std::string str() {
	std::stringstream ss;
	ss << "[" << this->W << ", " << this->X << ", " << this->Y << ", " << this->Z << "]";
	return ss.str();
  }

  // initialized constructor

  Quaternion(double w, double x, double y, double z) {
	W = w;
	X = x;
	Y = y;
	Z = z;
  }
  
  Quaternion* scale(double f) {
	W *= f;
	return this;
  }
  
  Quaternion* copy(Quaternion& o) {
	W = o.W;
	X = o.X;
	Y = o.Y;
	Z = o.Z;
	return this;
  }
  
  // quaternion multiplication
  Quaternion* mult (Quaternion& q) {
	double w = W*q.W - (X*q.X + Y*q.Y + Z*q.Z);

	double x = W*q.X + q.W*X + Y*q.Z - Z*q.Y;
	double y = W*q.Y + q.W*Y + Z*q.X - X*q.Z;
	double z = W*q.Z + q.W*Z + X*q.Y - Y*q.X;

	W = w;
	X = x;
	Y = y;
	Z = z;
	return this;
  }

  // conjugates the quaternion
  Quaternion* conjugate () {
	X = -X;
	Y = -Y;
	Z = -Z;
	return this;
  }

  // inverts the quaternion
  Quaternion* reciprical () {
	double norme = sqrt(W*W + X*X + Y*Y + Z*Z);
	if (norme == 0.0)
	  norme = 1.0;

	double recip = 1.0 / norme;

	W =  W * recip;
	X = -X * recip;
	Y = -Y * recip;
	Z = -Z * recip;

	return this;
  }

  // sets to unit quaternion
  Quaternion* normalize() {
	double norme = sqrt(W*W + X*X + Y*Y + Z*Z);
	if (norme == 0.0)
	{
	  W = 1.0; 
	  X = Y = Z = 0.0;
	}
	else
	{
	  double recip = 1.0/norme;

	  W *= recip;
	  X *= recip;
	  Y *= recip;
	  Z *= recip;
	}
	return this;
  }

  // Makes quaternion from axis
  Quaternion* fromAxis(double Angle, double x, double y, double z) { 
	double omega, s, c;

	s = sqrt(x*x + y*y + z*z);

	if (abs(s) > std::numeric_limits<double>::min())
	{
	  c = 1.0/s;

	  x *= c;
	  y *= c;
	  z *= c;

	  omega = -0.5 * Angle;
	  s = (double)sin(omega);

	  X = s*x;
	  Y = s*y;
	  Z = s*z;
	  W = (double)cos(omega);
	}
	else
	{
	  X = Y = 0.0;
	  Z = 0.0;
	  W = 1.0;
	}
	normalize();
	return this;
  }

  Quaternion* fromAxis(double Angle, Vector3* axis) {
	return (this->fromAxis(Angle, axis->x, axis->y, axis->z));
  }
  
	Quaternion* toAxis(double* angle, Vector3* axis) { // must be normalized
		*angle = 2 * acos(this->W);
		double s = sqrt(1-this->W*this->W);
		if (s < 0.001) {
			axis->x = 1;
			axis->y = 0;
			axis->z = 0;
		} else {
			axis->x = this->X / s;
			axis->y = this->Y / s;
			axis->z = this->Z / s;
		}
	}

  // Rotates towards other quaternion
  Quaternion* slerp(Quaternion& b, double t)
  {
	Quaternion& a = *this;
	double omega, cosom, sinom, sclp, sclq;

	cosom = a.X*b.X + a.Y*b.Y + a.Z*b.Z + a.W*b.W;

	if ((1.0+cosom) > std::numeric_limits<double>::min())
	{
	  if ((1.0-cosom) > std::numeric_limits<double>::min())
	  {
		omega = acos(cosom);
		sinom = sin(omega);
		sclp = sin((1.0-t)*omega) / sinom;
		sclq = sin(t*omega) / sinom;
	  }
	  else
	  {
		sclp = 1.0 - t;
		sclq = t;
	  }

	  X = sclp*a.X + sclq*b.X;
	  Y = sclp*a.Y + sclq*b.Y;
	  Z = sclp*a.Z + sclq*b.Z;
	  W = sclp*a.W + sclq*b.W;
	}
	else
	{
	  X =-a.Y;
	  Y = a.X;
	  Z =-a.W;
	  W = a.Z;

	  sclp = sin((1.0-t) * M_PI * 0.5);
	  sclq = sin(t * M_PI * 0.5);

	  X = sclp*a.X + sclq*b.X;
	  Y = sclp*a.Y + sclq*b.Y;
	  Z = sclp*a.Z + sclq*b.Z;
	}
	return this;
  }

  Quaternion* exp()
  {							   
	double Mul;
	double Length = sqrt(X*X + Y*Y + Z*Z);

	if (Length > 1.0e-4)
	  Mul = sin(Length)/Length;
	else
	  Mul = 1.0;

	W = cos(Length);

	X *= Mul;
	Y *= Mul;
	Z *= Mul; 

	return this;
  }

  Quaternion* log()
  {
	double Length;

	Length = sqrt(X*X + Y*Y + Z*Z);
	Length = atan(Length/W);

	W = 0.0;

	X *= Length;
	Y *= Length;
	Z *= Length;

	return this;
  }
  
  Quaternion* toMatrix(Matrix44* matrix) {
	double xx	  = this->X * this->X;
	double xy	  = this->X * this->Y;
	double xz	  = this->X * this->Z;
	double xw	  = this->X * this->W;
	double yy	  = this->Y * this->Y;
	double yz	  = this->Y * this->Z;
	double yw	  = this->Y * this->W;
	double zz	  = this->Z * this->Z;
	double zw	  = this->Z * this->W;
	
	(*matrix)[0] = 1 - 2 * ( yy + zz );
	(*matrix)[1] = 2 * ( xy - zw );
	(*matrix)[2] = 2 * ( xz + yw );
	(*matrix)[3] = 0;
	(*matrix)[4] = 2 * ( xy + zw );
	(*matrix)[5] = 1 - 2 * ( xx + zz );
	(*matrix)[6] = 2 * ( yz - xw );
	(*matrix)[7] = 0;
	(*matrix)[8] = 2 * ( xz - yw );
	(*matrix)[9] = 2 * ( yz + xw );
	(*matrix)[10] = 1 - 2 * ( xx + yy );
	(*matrix)[11] = 0;
	(*matrix)[12] = 0;
	(*matrix)[13] = 0;
	(*matrix)[14] = 0;
	(*matrix)[15] = 1;
	
	return this;
  }
};

