//
//  Vec3.h
//
//  Copyright (C) 2007-2008 Mikko Mononen
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
//  Mikko Mononen memon@inside.org
//

#ifndef VEC3_H
#define VEC3_H

#include <math.h>

namespace LayeredNavMesh {

template<class T> class Vec3Tpl
{
public:
	inline Vec3Tpl() {}
	inline Vec3Tpl(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}
	inline Vec3Tpl(const Vec3Tpl<T>& v) : x(v.x), y(v.y), z(v.z) {}

	inline void Set(T x_, T y_, T z_) { x=x_; y=y_; z=z_; }

	inline Vec3Tpl operator+(const Vec3Tpl& rhs) const { return Vec3Tpl(x+rhs.x, y+rhs.y, z+rhs.z); }
	inline Vec3Tpl operator-(const Vec3Tpl& rhs) const { return Vec3Tpl(x-rhs.x, y-rhs.y, z-rhs.z); }
	inline Vec3Tpl operator*(T s) const { return Vec3Tpl(x*s, y*s, z*s); }
	inline Vec3Tpl operator/(T s) const { return Vec3Tpl(x/s, y/s, z/s); }
	inline Vec3Tpl operator-() const { return Vec3Tpl(-x, -y, -z); }

	inline Vec3Tpl& operator+=(const Vec3Tpl& rhs) { x+=rhs.x; y+=rhs.y; z+=rhs.z; return *this; }
	inline Vec3Tpl& operator-=(const Vec3Tpl& rhs) { x-=rhs.x; y-=rhs.y; z-=rhs.z; return *this; }
	inline Vec3Tpl& operator*=(T s) { x*=s; y*=s; z*=s; return *this; }
	inline Vec3Tpl& operator/=(T s) { x/=s; y/=s; z/=s; return *this; }

	inline Vec3Tpl& operator=(const Vec3Tpl& rhs) { x=rhs.x; y=rhs.y; z=rhs.z; return *this; }
	
	inline operator T*() { return v; }
	inline operator const T*() const { return v; }

	inline bool	operator==(const Vec3Tpl& rhs) const { return x==rhs.x && y==rhs.y && z==rhs.z; }
	inline bool	operator!=(const Vec3Tpl& rhs) const { return x!=rhs.x || y!=rhs.y || z!=rhs.z; }

	inline T Length() const { return (T)sqrtf(x*x + y*y + z*z); }
	inline T LengthSqr() const { return x*x + y*y + z*z; }
	inline T Normalize()
	{
		T	len = Length();
		if(len > 0.0f)
		{
			x /= len;
			y /= len;
			z /= len;
		}
		return len;
	}

	inline Vec3Tpl& SetMin(const Vec3Tpl& a)
	{
		x = x < a.x ? x : a.x;
		y = y < a.y ? y : a.y;
		z = z < a.z ? z : a.z;
		return *this;
	}

	inline Vec3Tpl& SetMax(const Vec3Tpl& a)
	{
		x = x > a.x ? x : a.x;
		y = y > a.y ? y : a.y;
		z = z > a.z ? z : a.z;
		return *this;
	}

	union
	{
		struct
		{
			T	x, y ,z;
		};
		T	v[3];
	};
};

template<class T> inline Vec3Tpl<T> operator*(T s, const Vec3Tpl<T>& v)
{
	return Vec3Tpl<T>(v.x*s, v.y*s, v.z*s);
}

template<class T> inline T Dot(const Vec3Tpl<T>& lhs, const Vec3Tpl<T>& rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

template<class T> inline Vec3Tpl<T> Cross(const Vec3Tpl<T>& lhs, const Vec3Tpl<T>& rhs)
{
	return Vec3Tpl<T>(lhs.y*rhs.z - lhs.z*rhs.y,
										lhs.z*rhs.x - lhs.x*rhs.z,
										lhs.x*rhs.y - lhs.y*rhs.x);
}

template<class T> inline Vec3Tpl<T> Min(const Vec3Tpl<T>& a, const Vec3Tpl<T>& b)
{
	return Vec3Tpl<T>(a.x < b.x ? a.x : b.x,
										a.y < b.y ? a.y : b.y,
										a.z < b.z ? a.z : b.z);
}

template<class T> inline Vec3Tpl<T> Max(const Vec3Tpl<T>& a, const Vec3Tpl<T>& b)
{
	return Vec3Tpl<T>(a.x > b.x ? a.x : b.x,
										a.y > b.y ? a.y : b.y,
										a.z > b.z ? a.z : b.z);
}

typedef Vec3Tpl<float>	Vec3;
typedef Vec3Tpl<int>	Vec3i;

} // namespace LayeredNavMesh

#endif
