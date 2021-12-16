//
//  Matrix4.h
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

#ifndef MATRIX4_H
#define MATRIX4_H

#include "Vec3.h"

namespace LayeredNavMesh {

template<class T> class Matrix4Tpl
{
public:

	//! Default constructor
	inline Matrix4Tpl() {}

	//! Default constructor
	inline Matrix4Tpl(T m00, T m01, T m02, T m03,
										T m10, T m11, T m12, T m13,
										T m20, T m21, T m22, T m23,
										T m30, T m31, T m32, T m33)
	{
		mtx[0] = m00;		mtx[1] = m01;		mtx[2] = m02;		mtx[3] = m03;
		mtx[4] = m10;		mtx[5] = m11;		mtx[6] = m12;		mtx[7] = m13;
		mtx[8] = m20;		mtx[9] = m21;		mtx[10] = m22;	mtx[11] = m23;
		mtx[12] = m30;	mtx[13] = m31;	mtx[14] = m32;	mtx[15] = m33;
	}

	//! Copy constructor
	inline Matrix4Tpl(const Matrix4Tpl& m)
	{
		for(unsigned i = 0; i < 16; ++i)
			mtx[i] = m.mtx[i]; 
	}

	inline Matrix4Tpl& operator=(const Matrix4Tpl& mat)
	{
		for(unsigned i = 0; i < 16; i++) mtx[i] = mat.mtx[i];
		return *this;
	}

	//! Default destructor.
	inline ~Matrix4Tpl() {}

	//! Constant float pointer cast operator.
	inline operator const T*() const
	{
		return mtx;
	}

	//! Multiplies two matrices and returns the result.
	inline Matrix4Tpl operator*(const Matrix4Tpl& m) const
	{
		Matrix4Tpl	res;

		for(int row = 0; row < 4; row++)
		{
			for(int col = 0; col < 4; col++)
			{
				res.mtx[row * 4 + col] =	mtx[row * 4 + 0] * m.mtx[0 * 4 + col] +
																	mtx[row * 4 + 1] * m.mtx[1 * 4 + col] +
																	mtx[row * 4 + 2] * m.mtx[2 * 4 + col] + 
																	mtx[row * 4 + 3] * m.mtx[3 * 4 + col];
			}
		}
		return res;
	}

	inline void Set(int row, int col, T val)
	{
		mtx[col * 4 + row] = val;
	}
	
	inline float Get(int row, int col) const
	{
		return mtx[col * 4 + row];
	}

	//! Multiplies the specified vector by the specified matrix and returns the result.
	inline Vec3Tpl<T> TransformVector(const Vec3Tpl<T>& v) const
	{
		return Vec3Tpl<T>(v[0] * mtx[0] + v[1] * mtx[4] + v[2] * mtx[8],
								v[0] * mtx[1] + v[1] * mtx[5] + v[2] * mtx[9],
								v[0] * mtx[2] + v[1] * mtx[6] + v[2] * mtx[10]);
	}
	
	//! Multiplies the specified vector by the specified matrix and returns the result.
	inline Vec3Tpl<T> TransformPoint(const Vec3Tpl<T>& v) const
	{
		return Vec3Tpl<T>(v[0] * mtx[0] + v[1] * mtx[4] + v[2] * mtx[8] + mtx[12],
								v[0] * mtx[1] + v[1] * mtx[5] + v[2] * mtx[9] + mtx[13],
								v[0] * mtx[2] + v[1] * mtx[6] + v[2] * mtx[10] + mtx[14]);
	}

	//! Sets the matrix as identity matrix.
	inline Matrix4Tpl&	SetIdentity()
	{
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				mtx[i * 4 + j] = i == j ? (T)1.0 : (T)0.0;

		return *this;
	}

private:
	T	mtx[16];
};


typedef Matrix4Tpl<float>	Matrix4;

} // namespace LayeredNavMesh

#endif
