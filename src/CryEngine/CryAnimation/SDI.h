
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:  SDI.h
//  Version:    v1.00
//  Created:    22/7/2009 by Xiaomao Wu.
//  Compiler:   Visual Studio 2008 Professional
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SDI_H__
#define __SDI_H__

#include <vector>
#include "platform.h"
#include "Matrix.h"

class CSDI
{
public:
	// Memory should be allocated outside of SDI class
	// in order to improve efficiency (allocate on the stack)
	CSDI(const int N, const int D, f32* params, f32* pA, f32* pR, f32* fb):
			m_N(N), m_D(D), m_params(params), m_pA(pA), m_pR(pR), m_scale(fb)
	{
		assert(m_params && m_pA && m_pR && m_scale);
	}
	~CSDI(void);
	int GetTotalSamples() const {return m_N;}
	int GetParamDimension() const {return m_D;}
	void ConstructSDI();
	void GetWeights(f32* params, f32* weights);
	
private:
	int m_N; // Number of examples
	int m_D; // Number of parameter dimension
	f32* m_params; // NxD
	f32* m_pA; // DxN: Polynomial coefficients
	f32* m_pR; // NxN: SDI coefficients
	f32* m_scale; // N

	inline f32 SplineFunc(const f32 x);
	f32 SqrtDist(const f32 a, const f32 b) {return sqrt(a*a + b*b);}
	void ResolveMat(const fMatrix& a, fMatrix& me, f32* w, fMatrix& mo);
	void LinearSys(const fMatrix& A, const fMatrix& B, fMatrix& X);
	void Preprocess(int32& n, int32& iq, int32& m, f32&fb, f32& fl, f32& f, f32& fy, f32& fp, f32& norma, fMatrix& me, f32* mv, f32* w);
	void Refine(int32& m, int32& n, int32& iq, f32& f, f32& fy, f32& fp, fMatrix& me, fMatrix& mo, f32* mv, f32* w);
	void Update(int32& m, int32& n, int32& numIters, int32& iq, int32& t, int32& flg, int32& maxIters, 
		f32& norma, f32& fz, f32& f, f32& fp, f32& x, f32& y, f32& fy, f32& fl, f32& z, fMatrix& me, fMatrix& mo, f32* mv, f32* w);
};

#endif
