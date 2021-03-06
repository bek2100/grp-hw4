#include "vec4.h"


vec4::vec4()
{
	x = 0.0;
	y = 0.0;
	z = 0.0;
	p = 0.0;
}

vec4::vec4(double _x, double _y, double _z, double _p)
{
	x = _x;
	y = _y;
	z = _z;
	p = _p;
}


vec4::vec4(double xp, double yp, double zp, double d, double alpha, double scale){
	// get the true depth
	z = (-scale * d * d * alpha) / (zp * (d - alpha) - scale * d * d);
	
	// get the true x, y, z 
	y = yp * z / d;
	x = xp * z / d;
	p = z / d;

	// get the "dolly" prespective z
	z = zp *  z / d;
}

vec4::~vec4()
{
}

vec4& vec4::operator=(vec4 l){
	x = l.x;
	y = l.y;
	z = l.z;
	p = l.p;
	return *this;
}

bool operator==(vec4 l, vec4 r){
	if (l.x == r.x &&
		l.y == r.y &&
		l.z == r.z &&
		l.p == r.p)
		return true;
	else
		return false;
}

vec4 operator+(vec4 lhs, vec4 rhs){
	vec4 res;

	res.x = lhs.x + rhs.x;
	res.y = lhs.y + rhs.y;
	res.z = lhs.z + rhs.z;
	res.p = lhs.p + rhs.p;
	return res;
};

vec4 operator-(vec4 lhs, vec4 rhs){
	vec4 res;

	res.x = lhs.x - rhs.x;
	res.y = lhs.y - rhs.y;
	res.z = lhs.z - rhs.z;
	res.p = lhs.p - rhs.p;
	return res;
};

vec4 operator/(vec4 lhs, double x){
	vec4 res;

	res.x = lhs.x / x;
	res.y = lhs.y / x;
	res.z = lhs.z / x;
	res.p = lhs.p / x;
	return res;
};

vec4 operator*(vec4 lhs, double x){
	vec4 res;

	res.x = lhs.x * x;
	res.y = lhs.y * x;
	res.z = lhs.z * x;
	res.p = lhs.p * x;
	return res;
};


vec4 operator*(double x, vec4 lhs){
	vec4 res;

	res.x = lhs.x * x;
	res.y = lhs.y * x;
	res.z = lhs.z * x;
	res.p = lhs.p * x;
	return res;
};


double& vec4::operator[](int indx){
	if (indx == 0)
		return x;
	else if (indx == 1)
		return y;
	else if (indx == 2)
		return z;
	else if (indx == 3)
		return p;
	else
		throw;
};

// return the product of (lhx X rhs)
vec4 cross(vec4 lhs, vec4 rhs){
	vec4 res;
	lhs = lhs / lhs.p;
	rhs = rhs / rhs.p;

	res.x = (lhs.y * rhs.z - lhs.z * rhs.y);
	res.y = (lhs.z * rhs.x - lhs.x * rhs.z);
	res.z = (lhs.x * rhs.y - lhs.y * rhs.x);
	res.p = 1;

	return res;
};