


#ifndef PBMATRIX_H
#define PBMATRIX_H

#pragma once

#include "PBvector.h"

////////////////////////////////////////////////////////////////	
template<class T> class CMatrix
{

public:
	CMatrix() {};     

	//! apply translation to the matrix, as from Mesa 
	void	TranslateMatrix(const vector3<T> &trans)
	{
		T *m_pValues=&m_Tvalues[0][0];		
		m_pValues[12] = m_pValues[0] * trans.x + m_pValues[4] * trans.y + m_pValues[8]  * trans.z + m_pValues[12];
		m_pValues[13] = m_pValues[1] * trans.x + m_pValues[5] * trans.y + m_pValues[9]  * trans.z + m_pValues[13];
		m_pValues[14] = m_pValues[2] * trans.x + m_pValues[6] * trans.y + m_pValues[10] * trans.z + m_pValues[14];
		m_pValues[15] = m_pValues[3] * trans.x + m_pValues[7] * trans.y + m_pValues[11] * trans.z + m_pValues[15];		
	}

	//! apply rotation to the matrix, as from Mesa 
	void RotationMatrix(T angle,T x,T y,T z,T m[])
	{
		/* This function contributed by Erich Boleyn (erich@uruk.org) */
		T mag, s, c;
		T xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

		mag = (T)sqrt(x*x+y*y+z*z);

		if (mag == 0.0) 
		{
			CMatrix<T> mtx;
			mtx.Identity();
			/* generate an identity matrix and return */
			memcpy(m,&mtx,sizeof(T)*16);
			return;
		}

		s=(T)sin(DEG2RAD(angle));
		c=(T)cos(DEG2RAD(angle));

		x /= mag;
		y /= mag;
		z /= mag;

#define M(row,col)  m[col*4+row]

		xx = x * x;
		yy = y * y;
		zz = z * z;
		xy = x * y;
		yz = y * z;
		zx = z * x;
		xs = x * s;
		ys = y * s;
		zs = z * s;
		one_c = 1.0 - c;

		M(0,0) = (one_c * xx) + c;
		M(0,1) = (one_c * xy) - zs;
		M(0,2) = (one_c * zx) + ys;
		M(0,3) = 0.0;

		M(1,0) = (one_c * xy) + zs;
		M(1,1) = (one_c * yy) + c;
		M(1,2) = (one_c * yz) - xs;
		M(1,3) = 0.0;

		M(2,0) = (one_c * zx) - ys;
		M(2,1) = (one_c * yz) + xs;
		M(2,2) = (one_c * zz) + c;
		M(2,3) = 0.0;

		M(3,0) = 0.0;
		M(3,1) = 0.0;
		M(3,2) = 0.0;
		M(3,3) = 1.0;

#undef M		
	}

	//! multiply 2 matrices, as from Mesa 
	void matmul(T *product,const T *a,const T *b)
	{
		/* This matmul was contributed by Thomas Malik */		

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  product[(col<<2)+row]

		for (int i = 0;i<4;i++) 
		{
			T ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
			P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
			P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
			P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
			P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
		}

#undef A
#undef B
#undef P
	}

	//! multiply 2 matrices, as from Mesa 
	void MultMatrixf(const T *m)
	{		
		T *m_pValues=&m_Tvalues[0][0];		
		matmul(m_pValues,m_pValues,m);		
	}

	//! apply rotation to the matrix, in degrees
	void RotateMatrixf(T angle,T x,T y,T z)
	{
		T m[16];
		RotationMatrix(angle,x,y,z,m);
		MultMatrixf(m);
	}

	//! apply rotation to the matrix, in degrees
	void RotateMatrix(const vector3<T> &angles)
	{
		RotateMatrixf(angles.z,0,0,1);
		RotateMatrixf(angles.y,0,1,0);
		RotateMatrixf(angles.x,1,0,0);
	}

	void BuildFromVectors(const vector3f &vright, const vector3f &vup, const vector3f &vpn, const vector3f &pos)
	{
		m_Tvalues[0][0] = vright[0];
		m_Tvalues[0][1] = vright[1];
		m_Tvalues[0][2] = vright[2];
		m_Tvalues[0][3] = 0;

		m_Tvalues[1][0] = vup[0];
		m_Tvalues[1][1] = vup[1];
		m_Tvalues[1][2] = vup[2];
		m_Tvalues[1][3] = 0;

		m_Tvalues[2][0] = vpn[0];
		m_Tvalues[2][1] = vpn[1];
		m_Tvalues[2][2] = vpn[2];
		m_Tvalues[2][3] = 0;

		m_Tvalues[3][0] = pos[0];
		m_Tvalues[3][1] = pos[1];
		m_Tvalues[3][2] = pos[2];
		m_Tvalues[3][3] = 1;
	}

	//! set matrix to identity
	void	Identity()
	{
		T identity[4][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
		memcpy(m_Tvalues, &identity, sizeof(identity)); 
	}

	//! multiplication between 2 matrices
	friend	CMatrix<T> operator * (const CMatrix<T> &in1, const CMatrix<T> &in2)
	{
		CMatrix<T> out;
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				out[i][j] = in1[i][0] * in2[0][j] + in1[i][1] * in2[1][j] + in1[i][2] * in2[2][j] + in1[i][3] * in2[3][j];

		return out;
	}

	//! transform a point by a matrix
	vector3<T> TransformPoint(const vector3<T> &b) const
	{
		vector3<T> v;
		v.x = (m_Tvalues[0][0] * b.x) + (m_Tvalues[1][0] * b.y) + (m_Tvalues[2][0] * b.z) + m_Tvalues[3][0];
		v.y = (m_Tvalues[0][1] * b.x) + (m_Tvalues[1][1] * b.y) + (m_Tvalues[2][1] * b.z) + m_Tvalues[3][1];
		v.z = (m_Tvalues[0][2] * b.x) + (m_Tvalues[1][2] * b.y) + (m_Tvalues[2][2] * b.z) + m_Tvalues[3][2];

		return v;
	}	

	//! scale the matrix
	void ScaleMatrix(T x,T y,T z)  
	{

		T *m=&m_Tvalues[0][0];

		m[0] *= x;   m[4] *= y;   m[8]  *= z; 
		m[1] *= x;   m[5] *= y;   m[9]  *= z;
		m[2] *= x;   m[6] *= y;   m[10] *= z;
		m[3] *= x;   m[7] *= y;   m[11] *= z;		
	}

	CMatrix<T> GetRotation(const vector3<T> &axis, T degrees)
	{
		CMatrix<T> mat;
		vector3<T> ax = axis.Normalized();

		T rad = degrees * (T)(3.14159) / (T)(180.0);
		T c = (T)(cos(rad));
		T s = (T)(sin(rad));
		T t = (T)(1.0) - c;

		mat.m_Tvalues[0][0] = t*ax.x*ax.x+c; 			mat.m_Tvalues[0][1] = t*ax.x*ax.y-s*ax.z;	mat.m_Tvalues[0][2] = t*ax.x*ax.z+s*ax.y;	mat.m_Tvalues[0][3] = (T)(0.0);
		mat.m_Tvalues[1][0] = t*ax.x*ax.y+s*ax.z; mat.m_Tvalues[1][1] = t*ax.y*ax.y+c; 			mat.m_Tvalues[1][2] = t*ax.y*ax.z-s*ax.x;	mat.m_Tvalues[1][3] = (T)(0.0);
		mat.m_Tvalues[2][0] = t*ax.x*ax.z-s*ax.y;	mat.m_Tvalues[2][1] = t*ax.y*ax.z+s*ax.x;	mat.m_Tvalues[2][2] = t*ax.z*ax.z+c;			mat.m_Tvalues[2][3] = (T)(0.0);
		mat.m_Tvalues[3][0] = (T)(0.0);						mat.m_Tvalues[3][1] = 0.0;						    mat.m_Tvalues[3][2] = (T)(0.0);						mat.m_Tvalues[3][3] = (T)(1.0);

		return mat;
	}

	void Transpose()
	{
		ftype from[16];
		memcpy(from,&m_Tvalues[0][0],sizeof(CMatrixf));
		ftype *to=&m_Tvalues[0][0];
		to[0] = from[0];
		to[1] = from[4];
		to[2] = from[8];
		to[3] = from[12];
		to[4] = from[1];
		to[5] = from[5];
		to[6] = from[9];
		to[7] = from[13];
		to[8] = from[2];
		to[9] = from[6];
		to[10] = from[10];
		to[11] = from[14];
		to[12] = from[3];
		to[13] = from[7];
		to[14] = from[11];
		to[15] = from[15];
	}


	inline	T* operator [] (int index)							{ return m_Tvalues[index]; }	
	inline	const T* operator [] (int index) const	{ return m_Tvalues[index]; }	
	inline	T& cell(int index) const { return ((T*)m_Tvalues)[index]; }

	T m_Tvalues[4][4];
};

typedef CMatrix<ftype> CMatrixf;

/*
* Perform a 4x4 matrix multiplication  (product = a x b).
* Input:  a, b - matrices to multiply
* Output:  product - product of a and b
*/
inline void matmul2(double * product, const double * a, const double * b)
{
	// This matmul was contributed by Thomas Malik 
	double temp[16];
	int i;

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define T(row,col)  temp[(col<<2)+row]

	// i-te Zeile 
	for (i = 0; i < 4; i++) {
		T(i, 0) =
			A(i, 0) * B(0, 0) + A(i, 1) * B(1, 0) + A(i, 2) * B(2, 0) + A(i,
			3) *
			B(3, 0);
		T(i, 1) =
			A(i, 0) * B(0, 1) + A(i, 1) * B(1, 1) + A(i, 2) * B(2, 1) + A(i,
			3) *
			B(3, 1);
		T(i, 2) =
			A(i, 0) * B(0, 2) + A(i, 1) * B(1, 2) + A(i, 2) * B(2, 2) + A(i,
			3) *
			B(3, 2);
		T(i, 3) =
			A(i, 0) * B(0, 3) + A(i, 1) * B(1, 3) + A(i, 2) * B(2, 3) + A(i,
			3) *
			B(3, 3);
	}

#undef A
#undef B
#undef T
	memcpy(product, temp, 16 * sizeof(double));
}

#endif