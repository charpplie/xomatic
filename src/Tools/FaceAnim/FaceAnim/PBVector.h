
#ifndef _VECTOR_3_H_
#define _VECTOR_3_H_

#pragma once

#define USE_DOUBLE

#ifdef USE_DOUBLE
typedef	double		ftype;
#else
typedef	float			ftype;
#endif

#pragma once

#include <math.h>

#define YAW		0  
#define PITCH	1    
#define ROLL	2   

#define M_PI	CV_PI
#define PI2		(CV_PI*2.0)
#define TO_RAD	(M_PI/180.0)	
#define	VEC_EPSILON	0.01

#define DEG2RAD( a ) ( ( (a) * M_PI ) / 180.0 )
#define RAD2DEG( a ) ( ( (a) * 180.0 ) / M_PI )

template<class T> class vector3
{
public:

	////////////////////////////////////////////////////////////////	
	//! constructors	
	vector3() {}
	vector3(T fXval, T fYval, T fZval) { x=fXval; y=fYval; z=fZval; }
	vector3(T fXval, T fYval, T fZval, T wval) { x=fXval; y=fYval; z=fZval;w=wval; }		
	vector3(const vector3 &vSource) { x=vSource.x; y=vSource.y; z=vSource.z;w=vSource.w; }  	

	////////////////////////////////////////////////////////////////
	//common methods

	//! reset vector's members
	void		Clear() { x=y=z=w=0; }

	//! set and get a copy of this vector
	vector3&	Set(T xval, T yval, T zval) { x=xval; y=yval; z=zval; return *this; }
	//void			Set(const POINT &tPoint) { x=(ftype)(tPoint.x);y=(ftype)(tPoint.y);}

	//! set maximum bound-to be used with checkmin-checkmax
	void		SetMax() { Set(99999,99999,99999); }

	//! set minimum bound-to be used with checkmin-checkmax
	void		SetMin() { Set(-99999,-99999,-99999); }

	//! check for min bounds
	void	CheckMin(const vector3 &other) 
	{ 
		if (other.x<x) x=other.x;
		if (other.y<y) y=other.y;
		if (other.z<z) z=other.z;
	}			

	//! check for max bounds
	void	CheckMax(const vector3 &other)
	{ 
		if (other.x>x) x=other.x;
		if (other.y>y) y=other.y;
		if (other.z>z) z=other.z;
	}

	//! check for min bounds
	void	CheckMinRaw(const T *other) 
	{ 
		if (other[0]<x) x=other[0];
		if (other[1]<y) y=other[1];
		if (other[2]<z) z=other[2];
	}			

	//! check for max bounds
	void	CheckMaxRaw(const T *other)
	{ 
		if (other[0]>x) x=other[0];
		if (other[1]>y) y=other[1];
		if (other[2]>z) z=other[2];
	}

	//! cut to int
	void	IntValues()
	{
		x=(int)(x);
		y=(int)(y);
		z=(int)(z);
	}

	//! interpolate to some defined value
	void  Interpolate(const vector3 &other,T fValue)
	{
		*this += other;
		*this /= fValue;
	}

	//! force vector lentgh by normalizing it	
	void  SetLen(const T fLen)
	{ 
		Normalize();
		(*this) *= fLen;
	}

	//! magnitude of this vector by the other
	void	Magnitude(const vector3 &Mag)
	{
		x*=Mag.x;y*=Mag.y;z*=Mag.z;
	}

	//! calcultae the length of the vector
	T	Length() const { return (T)sqrt(x*x+y*y+z*z); }		

	//! calcultae the squared length of the vector
	T	Length2() const { return x*x+y*y+z*z; }

	//! normalize the vector
	T	Normalize() { T l = Length(); if (l>(T)(0.0001)) { T f=(T)(1.0)/l; x*=f; y*=f; z*=f; return l; } return (0); }

	//! normalize the vector
	T Normalize(T * pf) 
	{ 
		T len=(T)sqrt(pf[0]*pf[0]+pf[1]*pf[1]+pf[2]*pf[2]); 
		T inv_len=(T)(1.0)/len; 
		pf[0]*=inv_len; pf[1]*=inv_len; pf[2]*=inv_len; 
		return len;
	}

	//! return a normalized vector
	vector3	Normalized() const { T f=(T)(1.0)/Length(); return vector3(x*f, y*f, z*f); }

	//! negate the vector	
	void	Negate() { x=-x; y=-y; z=-z; }	

	//! return a negated vector	
	vector3	Negated() const { return vector3(-x, -y, -z); }

	//! invert the vector
	void	Invert() { x=(T)(1.0)/x; y=(T)(1.0)/y; z=(T)(1.0)/z; }	

	//! return an inverted vector
	vector3	Inverted() const { return vector3((T)(1.0)/x, (T)(1.0)/y, (T)(1.0)/z); }

	//! convert from radians to degrees
	void  Rad2Deg()
	{
		x=RAD2DEG(x);
		y=RAD2DEG(y);
		z=RAD2DEG(z);
	}

	//! convert from degrees to radians
	void  Deg2Rad()
	{
		x=DEG2RAD(x);
		y=DEG2RAD(y);
		z=DEG2RAD(z);
	}	

	////////////////////////////////////////////////////////////////		
	//! check if the point is inside an AABB
	bool	InsideBBox(const vector3 &mins,const vector3 &maxs) const
	{
		if ((x>=mins.x && x<=maxs.x) && (y>=mins.y && y<=maxs.y) && (z>=mins.z && z<=maxs.z))
			return (true);

		return (false);
	}

	void Abs(void) {x=(T)fabs(x); y=(T)fabs(y); z=(T)fabs(z);}

	void	PrintDebug()
	{
		MyOutputDebugString("x=%0.2f,y=%0.2f,z=%0.2f\n",x,y,z);
	}

	////////////////////////////////////////////////////////////////	
	//friend methods
	friend	void	Clear(vector3 &vec) { vec.Clear(); }
	friend	void	Set(vector3 &vec, T xval, T yval, T zval) { vec.x=xval; vec.y=yval; vec.z=zval; }
	friend	T	Length(const vector3 &vec)	{ return vec.Length(); }
	friend	T	FLength(const vector3 &vec) { return (T)sqrt(vec.Length2()); }
	friend	T	Length2(const vector3 &vec) { return vec.Length2(); }
	friend	T	Normalize(vector3 &vec)			{ return vec.Normalize(); }
	friend	vector3	Normalized(const vector3 &vec) { return vec.Normalized(); }	
	friend	vector3	CalcNormal(const vector3 &v0,const vector3 &v1,const vector3 &v2)
	{
		vector3 normal;
		normal = (v0-v1)^(v2-v1);
		normal.Normalize();	
		return (normal);
	}
	friend	void	Negate(vector3 &vec)					{ vec.x=-vec.x; vec.y=-vec.y; vec.z=-vec.z; }
	friend	vector3	Negated(const vector3 &vec)		{ return vector3(-vec.x, -vec.y, -vec.z); }
	friend	void	Invert(vector3 &vec)					{ vec.x=1/vec.x; vec.y=1/vec.y; vec.z=1/vec.z; }	
	friend	vector3	Inverted(const vector3 &vec)	{ return vector3(1/vec.x, 1/vec.y, 1/vec.z); }	
	friend	T	Distance(const vector3 &vec1, const vector3 &vec2) 
	{ 
		return (T) sqrt((vec2.x-vec1.x)*(vec2.x-vec1.x)+(vec2.y-vec1.y)*(vec2.y-vec1.y)+(vec2.z-vec1.z)*(vec2.z-vec1.z)); 
	}
	friend	T	Distance2(const vector3 &vec1, const vector3 &vec2)
	{		
		return (vec2.x-vec1.x)*(vec2.x-vec1.x)+(vec2.y-vec1.y)*(vec2.y-vec1.y)+(vec2.z-vec1.z)*(vec2.z-vec1.z);
	}
	inline	friend	T L1Norm(const vector3 &vec1,const vector3 &vec2)
	{
		return((T)Ffabs(vec2.x-vec1.x)+(T)Ffabs(vec2.y-vec1.y)+(T)Ffabs(vec2.z-vec1.z));
	}	
	T	Distance2(const vector3 &vec2) { return (T) (vec2.x-x)*(vec2.x-x)+(vec2.y-y)*(vec2.y-y)+(vec2.z-z)*(vec2.z-z); }
	T	GetDistance(const vector3 &vec2) { return Distance(vec2); }	
	T	Distance(const vector3 &vec2)
	{ 
		return (T) sqrt((vec2.x-x)*(vec2.x-x)+(vec2.y-y)*(vec2.y-y)+(vec2.z-z)*(vec2.z-z));
	}
	bool Is000(){ return ((((unsigned long*)&x)==0) && (((unsigned long*)&y)==0) && (((unsigned long*)&x)==0)); }

	////////////////////////////////////////////////////////////////			
	//convert in place to Euler angles	and rotation	
	T	QuatToEuler(T W)
	{
		//hepefully the quaternion is normalized already
		T fS=(x*x)+(y*y)+(z*z);

		x/=fS;
		y/=fS;
		z/=fS;
		T fAlpha=2*(T)acos(W);	//2arccos(w)
		return (fAlpha);
	}

	////////////////////////////////////////////////////////////////		
	//operators
	void operator += (T f) { x+=f;y+=f;z+=f;}
	void operator -= (T f) { x-=f;y-=f;z-=f;}

	vector3 &operator = (const vector3 &source)  { x=source.x; y=source.y; z=source.z; return *this; }
	vector3 &operator = (const T value)  { x=value; y=value; z=value; /*w=value*/; return *this; }
	vector3 &operator += (const vector3 &vec)	{ x+=vec.x; y+=vec.y; z+=vec.z; return *this; }
	vector3 &operator -= (const vector3 &vec)	{ x-=vec.x; y-=vec.y; z-=vec.z; return *this; }
	vector3 &operator *= (T f)				{ x*=f; y*=f; z*=f; return *this; }
	vector3 &operator /= (T f)				{ x/=f; y/=f; z/=f; return *this; }

	vector3 Cross (const vector3 &vec2)	{ return vector3(y*vec2.z-z*vec2.y, z*vec2.x-x*vec2.z, x*vec2.y-y*vec2.x); }	
	T Dot (const vector3 &vec2)	{ return x*vec2.x + y*vec2.y + z*vec2.z; }
	vector3 Scale (const vector3 &vec2)	{ return vector3(x*vec2.x, y*vec2.y, z*vec2.z); }	

	////////////////////////////////////////////////////////////////		
	//friend operators	
	friend bool operator ==(const vector3 &vec1, const vector3 &vec2)	{ return (Ffabs(vec1.x-vec2.x) <= VEC_EPSILON)&&(Ffabs(vec1.y-vec2.y) <= VEC_EPSILON)&&(Ffabs(vec1.z-vec2.z) <= VEC_EPSILON); }
	friend bool operator !=(const vector3 &vec1, const vector3 &vec2)	{ return !(vec1==vec2); }
	friend bool operator <=(const vector3 &vec1, const vector3 &vec2)	{ return (vec1.x <= vec2.x) && (vec1.y <= vec2.y) && (vec1.z <= vec2.z); }
	friend bool operator >=(const vector3 &vec1, const vector3 &vec2)	{ return (vec1.x >= vec2.x) && (vec1.y >= vec2.y) && (vec1.z >= vec2.z); }
	friend bool operator <(const vector3 &vec1, const vector3 &vec2)		{ return (vec1.x < vec2.x) && (vec1.y < vec2.y) && (vec1.z < vec2.z); }
	friend bool operator >(const vector3 &vec1, const vector3 &vec2)		{ return (vec1.x > vec2.x) && (vec1.y > vec2.y) && (vec1.z > vec2.z); }

	friend vector3 operator + (const vector3 &vec1, const vector3 &vec2)	{ return vector3(vec1.x+vec2.x, vec1.y+vec2.y, vec1.z+vec2.z); }
	friend vector3 operator + (const vector3 &vec1, const T*  vec2)  { return vec1+vector3(vec2); }
	friend vector3 operator + (const T*  vec1, const vector3 &vec2)  { return vector3(vec1)+vec2; }
	friend vector3 operator - (const vector3 &vec1, const vector3 &vec2)	{ return vector3(vec1.x-vec2.x, vec1.y-vec2.y, vec1.z-vec2.z); }
	friend vector3 operator - (const vector3 &vec1, const T*  vec2)  { return vec1-vector3(vec2); }
	friend vector3 operator - (const T*  vec1, const vector3 &vec2)  { return vector3(vec1)-vec2; }
	friend vector3 operator - (const vector3 &vec)						{ return vector3(-vec.x, -vec.y, -vec.z); }
	friend T operator * (const vector3 &vec1, const vector3 &vec2)	{ return vec1.x*vec2.x + vec1.y*vec2.y + vec1.z*vec2.z; }
	friend T operator | (const vector3 &vec1, const vector3 &vec2)	{ return vec1.x*vec2.x + vec1.y*vec2.y + vec1.z*vec2.z; }
	friend vector3 operator * (const vector3 &vec, T f)				{ return vector3(vec.x*f, vec.y*f, vec.z*f); }
	friend vector3 operator * (T f, const vector3 &vec)				{ return vector3(f*vec.x, f*vec.y, f*vec.z); }
	friend vector3 operator / (const vector3 &vec, T f)				{ return vector3(vec.x/f, vec.y/f, vec.z/f); }
	friend vector3 operator / (T f, const vector3 &vec)				{ return vector3(f/vec.x, f/vec.y, f/vec.z); }
	friend vector3 operator ^ (const vector3 &vec1, const vector3 &vec2)	{ return vector3(vec1.y*vec2.z-vec1.z*vec2.y, vec1.z*vec2.x-vec1.x*vec2.z, vec1.x*vec2.y-vec1.y*vec2.x); }	

	T &operator [] (int index)		  { /*assert(index>=0 && index<=2);*/ return ((T*)this)[index]; }
	T operator [] (int index) const { /*assert(index>=0 && index<=2);*/ return ((T*)this)[index]; }
	operator T* ()					{ return (T*)this; }

	////////////////////////////////////////////////////////////////		
	//misc functions
	bool	Nozero() const { return (Ffabs(x) > VEC_EPSILON)||(Ffabs(y) > VEC_EPSILON)||(Ffabs(z) > VEC_EPSILON); }	
	bool	IsEquivalent(const vector3 &vec,T epsilon)
	{
		return  ((Ffabs(x-vec.x) <= epsilon)&&
			(Ffabs(y-vec.y) <= epsilon)&&
			(Ffabs(z-vec.z) <= epsilon)); 		
	}

	void	VecFromAngles();
	void	VecFromCameraAngles();
	void	VecFromCameraAngles2();
	//void AnglesFromVec();
	friend	bool	Nozero(const vector3 &vec) { return vec.Nozero(); }

	// reflect this vector from a surface with the specified normal
	vector3 Reflect(const vector3 &vNormal)
	{
		vector3 vI = (*this).Normalized();
		vector3 vN = vNormal.Normalized();
		vector3 vZ = vI - 2 * vN * (vI * vN);

		return vZ;
	}


	//protected: 

	T x,y,z;
	T w; //for 16-byte alignement purposes 
};

////////////////////////////////////////////////////////////////		
typedef vector3<ftype> point3f;
typedef vector3<ftype> vector3f;
typedef vector3<ftype> color3f;

typedef vector3<float> vector3float;

////////////////////////////////////////////////////////////////		
union CCol
{
	CCol() {};
	CCol(unsigned int color)
	{
		m_color=color;
	}

	unsigned int m_color;
	unsigned char r,g,b,a;
};

////////////////////////////////////////////////////////////////		
inline int		isqr(int iOp)	{ return(iOp*iOp); }
inline long		lsqr(long lOp)	{ return(lOp*lOp); }
inline ftype	sqr(ftype fOp)	{ return(fOp*fOp); }
inline double dsqr(double dOp){ return(dOp*dOp); }

////////////////////////////////////////////////////////////////		
//misc functions
//! check if a ray hit an AABB
bool	RayHitBBox(const vector3f &start,const vector3f &end,const vector3f &mins,const vector3f &maxs);

////////////////////////////////////////////////////////////////		
//! check if a Ray hit a sphere
bool	RayHitSphere(const vector3f &start,const vector3f &end,const vector3f &center, ftype radius2);

// Given a normalized forward vector, create two
// other perpendicular vectors
/////////////////////////////////////////////////////////////////
void MakeNormalVectors(const vector3f &vForward,vector3f &vRight,vector3f &vUp);

// calculate the angle between two vectors in 3d 
// /param vA direction of vector a (don't have to be normalized)
// /param vB direction of vector b (don't have to be normalized)
// /return angle in the range 0 to p radians. 
inline ftype CalcAngleBetween(const vector3f &vA,const vector3f &vB)
{
	ftype fLength=vA.Length()*vB.Length();

	if (fLength<0.01f)
		fLength=0.01f;
	return((ftype)(acos((vA*vB)/fLength)));
}

//////////////////////////////////////////////////////////////////////////	
typedef std::vector<vector3f> VectorVec;
typedef std::vector<vector3f>::iterator VectorVecIt;


#endif //vector