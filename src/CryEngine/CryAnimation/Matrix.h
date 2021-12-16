////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:  Matrix.h
//  Version:    v1.00
//  Created:    22/7/2009 by Xiaomao Wu.
//  Compiler:   Visual Studio 2008 Professional
//  Description:

// -------------------------------------------------------------------------
//  History:
//  Sept. 3 2009: Changed allocator: from heap to stack
////////////////////////////////////////////////////////////////////////////

#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "math.h"
#include "platform.h"
#define dMatrix CMatrix<double>
#define fMatrix CMatrix<float>

template <class T>

class CMatrix
{
public:

	CMatrix(const int rows, const int cols, T* data)
	{
		assert(data);

		m_rows = rows;
		m_cols = cols;	
		m_size = m_rows*m_cols;
		m_data = data;
	}

	CMatrix(const CMatrix& rhs)
	{
		assert(m_data && rhs.GetData());

		m_rows = rhs.Rows();
		m_cols = rhs.Cols();
		m_size = rhs.GetSize();
		if(m_data)
			memcpy(m_data, rhs.GetData(), sizeof(T)*m_size);
	}

	~CMatrix(void){};

	T& operator() (const int row, const int col) 
	{
		assert(row >=0 && row <m_rows && col>=0 && col<m_cols);
		return m_data[col*m_rows +row];
	}

	const T& operator() (const int row, const int col) const 
	{
		assert(row >=0 && row <m_rows && col>=0 && col<m_cols);
		return m_data[col*m_rows +row];
	}

	CMatrix<T>& operator = (const CMatrix<T>& rh)
	{
		if(&rh == this)
			return *this;

	 assert(m_data && rh.GetData());

		m_rows = rh.Rows();
		m_cols = rh.Cols();
		m_size = rh.GetSize();
		memcpy(m_data, rh.GetData(), sizeof(T)*m_size);

		return *this;
	}

	CMatrix<T>& operator = (const T& rh)
	{
		for(int i=0; i<m_size; ++i)
			m_data[i] = rh;
		return *this;
	}

	CMatrix<T>& operator += (const CMatrix<T>& rhs)
	{
		assert(rhs.Rows() == m_rows && rhs.Cols() == m_cols);

		for(int i=0; i<m_rows; ++i)
		{
			for(int j=0; j<m_cols; ++j)
			{
				(*this)(i,j) += rhs(i,j);
			}
		}
		return *this;
	}

	CMatrix<T>& operator -= (const CMatrix<T>& rhs)
	{
		assert(rhs.Rows() == m_rows && rhs.Cols() == m_cols);

		for(int i=0; i<m_rows; ++i)
		{
			for(int j=0; j<m_cols; ++j)
			{
				(*this)(i,j) -= rhs(i,j);
			}
		}
		return *this;
	}

	CMatrix<T>& operator - (void)
	{
		(*this) *= -1;

		return *this;
	}

	CMatrix<T>& operator += (const T& v)
	{
		for(int i=0; i<m_rows; ++i)
		{
			for(int j=0; j<m_cols; ++j)
			{
				(*this)(i,j) += v;
			}
		}
		return *this;
	}

	//------------------------------------------------------------------------------
	// Matrix multiplication
	//------------------------------------------------------------------------------
	static void MultiMatrix (const CMatrix<T>& op1, const CMatrix<T>& op2, CMatrix<T>& res)
	{
		assert(op1.Cols() == op2.Rows());
	
		for(int i=0; i<op1.Rows(); ++i)
		{
			for(int j=0; j<op2.Cols(); ++j)
			{
				res(i, j) =0;
				for(int k=0; k<op1.Cols(); ++k)
					res(i,j) += op1(i, k) * op2(k, j);
			}
		}
	}

	CMatrix<T>& operator -= (const T& v)
	{
		(*this) += -v;
		return *this;
	}

	CMatrix<T>& operator *= (const T& v)
	{
		for(int i=0; i<m_rows; ++i)
		{
			for(int j=0; j<m_cols; ++j)
			{
				(*this)(i,j) *= v;
			}
		}
		return *this;
	}

	CMatrix<T>& operator /= (const T& v)
	{
		for(int i=0; i<m_rows; ++i)
		{
			for(int j=0; j<m_cols; ++j)
			{
				(*this)(i,j) /= v;
			}
		}
		return *this;
	}

	//------------------------------------------------------------------------------
	// sqrt(M.*M) for one-row matrix
T SqrtDist() const
	{
		assert(m_rows == 1);
		T dist = 0;
		for(int i=0; i<m_cols; ++i)
		{
			dist += (*this)(0, i)*(*this)(0, i);
		}

		dist = sqrt(dist);
		return dist;
	}

	void Resize(const int rows, const int cols)
	{
		if(m_data)
			delete[] m_data;
		m_rows = rows;
		m_cols = cols;
		m_size = rows*cols;
		m_data = new T[m_size];
	}

	int Rows() const {return m_rows;}
	int Cols() const {return m_cols;}
	T*  GetData()const {return m_data;}
	int GetSize() const{return m_size;}

private:
	int m_rows;
	int m_cols;
	int m_size;
	T* m_data;
};

#endif
