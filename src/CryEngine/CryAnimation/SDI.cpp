////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:  SDI.cpp
//  Version:    v1.00
//  Created:    22/7/2009 by Xiaomao Wu.
//  Compiler:   Visual Studio 2008 Professional
//  Description:

// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SDI.h"

const f32 kernelFuncCoeff = 1.0f/6.0f;

CSDI::~CSDI(void)
{
}

#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

//------------------------------------------------------------------------------
// The preprocess stage for Scattered Data Interpolation (SDI)
//------------------------------------------------------------------------------
void CSDI::Preprocess(int32& n, int32& iq, int32& m, f32&fb, f32& fl, f32& f, f32& fy, f32& fp, f32& norma, fMatrix& me, f32* mv, f32* w)
{
	fy = 0.0f;
	norma = 0.0f;
	fb = 0.0f;
	for (int32 i=0;i<n;++i) 
	{		
		mv[i] = fb*fy;
		fb = 0.0f;
		fy = 0.0f;
		fp  = 0.0f;
		iq=i+1;
		if (i < m) 
		{
			for (int32 k=i; k<m; ++k) 
				fb += fabs(me(k, i));

			if (fb) 
			{
				for (int32 k=i; k<m; ++k)
				{
					me(k, i) /= fb;
					fp += me(k, i)*me(k, i);
				}

				f = me(i, i);
				fy = -SIGN(sqrt(fp),f);
				fl = f*fy - fp;
				me(i, i) = f-fy;
				for (int32 j=iq; j<n; ++j)
				{
					f32 fh = 0.0f;
					for (int32 k=i; k<m; ++k) 
						fh += me(k, i)*me(k, j);
					f = fh/fl;
					for (int32 k=i; k<m; ++k) 
						me(k, j) += f*me(k, i);
				}
				for (int32 k=i; k<m; ++k) 
					me(k, i) *= fb;
			}
		}

		w[i] = fb *fy;
		fy= 0.0f;
		fp = 0.0f;
		fb = 0.0f;
		if (i < m && i != n-1)
		{
			for (int32 k=iq; k<n; ++k)
				fb += fabs(me(i, k));
			if (fb) 
			{
				for (int32 k=iq; k<n; ++k)
				{
					me(i, k) /= fb;
					fp += me(i, k)*me(i, k);
				}

				f=me(i, iq);
				fy = -SIGN(sqrt(fp),f);
				fl = f*fy-fp;
				me(i, iq) = f-fy;
				for (int32 k=iq; k<n; ++k) 
					mv[k]=me(i, k)/fl;

				for (int32 j=iq; j<m; ++j)
				{
					f32 fh = 0.0f;
					for (int32 k=iq; k<n; ++k) 
						fh += me(j, k)*me(i, k);

					for (int32 k=iq; k<n; ++k) 
						me(j, k) += fh*mv[k];
				}

				for (int32 k=iq; k<n; ++k) 
					me(i, k) *= fb;
			}
		} 

		norma=max(norma,(fabs(w[i])+fabs(mv[i])));
	}
}

//------------------------------------------------------------------------------
// The refinement stage for Scattered Data Interpolation (SDI)
//------------------------------------------------------------------------------
void CSDI::Refine(int32& m, int32& n, int32& iq, f32& f, f32& fy, f32& fp, fMatrix& me, fMatrix& mo, f32* mv, f32* w)
{

	for (int32 i=n-1; i>=0; --i) 
	{
		if (i < n-1) 
		{
			if (fy) 
			{
				for (int32 j=iq; j<n; ++j)
					mo(j, i)=(me(i, j)/me(i, iq))/fy;

				for (int32 j=iq; j<n; ++j) 
				{
					f32 fh = 0.0f;
					for (int32 k=iq; k<n; ++k) 
						fh += me(i, k)*mo(k, j);

					for (int32 k=iq; k<n; ++k) 
						mo(k, j) += fh*mo(k, i);
				}
			}

			for (int32 j=iq; j<n; ++j) 
			{
				mo(i, j) = 0.0f;
				mo(j, i) = 0.0f;
			}
		}

		fy=mv[i];
		mo(i, i)=1.0f;
		iq=i;
	}

	for (int32 i=min(m,n)-1; i>=0; --i) 
	{
		iq=i+1;
		fy=w[i];
		for (int32 j=iq; j<n; ++j) 
			me(i, j)=0.0f;

		if (fy) 
		{
			fy= (f32)(1.0/fy);
			for (int32 j=iq; j<n; ++j) 
			{
				f32 fh = 0.0f;
				for (int32 k=iq; k<m; ++k) 
					fh += me(k, i)*me(k, j);
				f=(fh/me(i, i))*fy;
				for (int32 k=i; k<m; ++k) 
					me(k, j) += f*me(k, i);
			}
			for (int32 j=i; j<m; ++j) 
				me(j, i) *= fy;
		} 
		else 
			for (int32 j=i;j<m;++j) 
				me(j, i)=0.0f;

		++me(i, i);
	}
}

//------------------------------------------------------------------------------
// The update stage for Scattered Data Interpolation (SDI)
//------------------------------------------------------------------------------
void CSDI::Update(int32& m, int32& n, int32& numIters, int32& iq, int32& t, int32& flg, int32& maxIters, 
									f32& norma, f32& fz, f32& f, f32& fp, f32& x, f32& y, f32& fy, f32& fl, f32& z, fMatrix& me, fMatrix& mo, f32* mv, f32* w)
{
	for (int32 k=n-1; k>=0; --k)
	{
		for (int32 numItr=1; numItr<=maxIters; ++numItr)
		{
			flg=1;
			for (iq=k; iq>=0; --iq)
			{
				t=iq-1;
				if ((f32)(fabs(mv[iq])+norma) == norma)
				{
					flg=0;
					break;
				}
				if ((f32)(fabs(w[t])+norma) == norma) 
					break;
			}

			if (flg) 
			{
				fz=0.0;
				fp=1.0;
				for (int32 i=iq; i<=k; ++i) 
				{
					f=fp*mv[i];
					mv[i]=fz*mv[i];

					if ((f32)(fabs(f)+norma) == norma) 
						break;

					fy=w[i];
					fl=SqrtDist(f,fy);
					w[i]=fl;
					fl=(f32)(1.0/fl);
					fz=fy*fl;
					fp = -f*fl;

					for (int32 j=0;j<m;++j) 
					{
						y=me(j, t);
						z=me(j,i);
						me(j, t)=y*fz+z*fp;
						me(j, i)=z*fz-y*fp;
					}
				}
			}
			z=w[k];
			if (iq == k) 
			{
				if (z < 0.0) 
				{
					w[k] = -z;
					for (int32 j=0;j<n;++j) 
						mo(j, k) = -mo(j,k);
				}
				break;
			}
			if (numItr == maxIters) 
				CryLog("No convergence in maxIters iterations during arbitrary motion interpolation.");

			x = w[iq];
			t = k-1;
			y = w[t];
			fy = mv[t];
			fl = mv[k];
			f = (f32)( ((y-z)*(y+z)+(fy-fl)*(fy+fl))/(2.0*fl*y) );
			fy = SqrtDist(f,1.0);
			f = ((x-z)*(x+z)+fl*((y/(f+SIGN(fy,f)))-fl))/x;
			fz = 1.0f;
			fp = 1.0f;

			for (int32 j=iq;j<=t;++j) 
			{
				int32 i=j+1;
				fy=mv[i];
				y=w[i];
				fl=fp*fy;
				fy=fz*fy;
				z=SqrtDist(f,fl);
				mv[j]=z;
				fz=f/z;
				fp=fl/z;
				f=x*fz+fy*fp;
				fy = fy*fz-x*fp;
				fl=y*fp;
				y *= fz;
				for (int32 js=0;js<n;++js)
				{
					x=mo(js, j);
					z=mo(js, i);
					mo(js, j) = x*fz+z*fp;
					mo(js, i) = z*fz-x*fp;
				}
				z=SqrtDist(f,fl);
				w[j]=z;
				if (z) 
				{
					z=(f32) (1.0/z);
					fz=f*z;
					fp=fl*z;
				}
				f=fz*fy+fp*y;
				x=fz*y-fp*fy;
				for (int32 js=0;js<m;++js) 
				{
					y=me(js, j);
					z=me(js, i);
					me(js, j)=y*fz+z*fp;
					me(js, i)=z*fz-y*fp;
				}
			}
			mv[iq]=0.0;
			mv[k]=f;
			w[k]=x;
		}
	}
}

//------------------------------------------------------------------------------
// The matrix processing stage for Scattered Data Interpolation (SDI)
//------------------------------------------------------------------------------
void CSDI::ResolveMat(const fMatrix& a, fMatrix& me, f32* w, fMatrix& mo)
{
	int32 m = a.Rows();
	int32 n = a.Cols();

	me = a;

	int32 i=0, j=0, js=0, k=0, iq=0, t=0;
	int32 maxIters = 30;
	int32 flg=0;
	int32 numIters=0;
	f32 norma=.0f;
	f32 fz=.0f, f=0.f, fy=.0f, fl=.0f, fp=.0f;
	f32 fb=.0f;
	f32 x=.0f, y=.0f, z=.0f;

	f32* mv = (f32*)alloca(n*sizeof(f32));
	
	Preprocess(n, iq, m, fb, fl, f, fy, fp, norma, me, mv, w);

	Refine(m, n, iq, f, fy, fp, me, mo,  mv, w);

	Update(m, n, numIters, iq, t, flg, maxIters, norma, fz, f, fp, x, y, fy, fl, z, me, mo,mv, w);
}

//------------------------------------------------------------------------------
// The linear system solver for Scattered Data Interpolation (SDI)
//------------------------------------------------------------------------------
void CSDI::LinearSys(const fMatrix& A, const fMatrix& B, fMatrix& X)
{
	f32* pU = (f32*)alloca(A.Rows()*A.Cols()*sizeof(f32));
	fMatrix U(A.Rows(), A.Cols(), pU);

	f32* pV = (f32*)alloca(A.Cols()*A.Cols()*sizeof(f32));
	fMatrix V(A.Cols(), A.Cols(), pV);

	f32* W = (f32*)alloca(A.Cols()*sizeof(f32));

	ResolveMat(A, U, W, V);

	f32 fmax=0.0;
	for (size_t i=0;i<A.Cols(); ++i)
		if (W[i] > fmax) fmax=W[i];
	f32 fmin= (f32)( fmax*(1.0e-6) );
	for (size_t k=0;k<A.Cols(); ++k)
		if (W[k] < fmin) W[k]=.0f;

	f32* p_ut = (f32*)alloca(U.Cols()*U.Rows()*sizeof(f32));
	fMatrix ut(U.Cols(), U.Rows(), p_ut);

	for(int i=0; i<ut.Rows(); ++i)
		for(int j=0; j<ut.Cols(); ++j)
			ut(i, j) = U(j, i);

	f32* pDialgW = (f32*)alloca( V.Cols()*ut.Rows()*sizeof(f32) );
	fMatrix diagW(V.Cols(), ut.Rows(), pDialgW);
	assert(diagW.Rows() == diagW.Cols());
	diagW = .0f;
	for(int i=0; i<(int)A.Cols(); ++i)
	{
		if(fabs(W[i]) < 1e-20)
			diagW(i, i) = 1e+20f;
		else
			diagW(i, i) = (f32)(1.0/W[i]);
	}

	f32* pTa = (f32*)alloca( V.Rows()*diagW.Cols()*sizeof(f32) );
	fMatrix Ta(V.Rows(), diagW.Cols(), pTa);
	CMatrix<f32>::MultiMatrix(V, diagW, Ta);

	f32* pTb = (f32*)alloca( Ta.Rows()*ut.Cols()*sizeof(f32) );
	fMatrix Tb(Ta.Rows(), ut.Cols(), pTb);
	CMatrix<f32>::MultiMatrix(Ta, ut, Tb);
	CMatrix<f32>::MultiMatrix(Tb, B, X);
}

inline f32 CSDI::SplineFunc(const f32 x)
{
	return (f32)exp(-1.0 * x *x);
}

void CSDI::ConstructSDI()
{
	int N = m_N;
	int D = m_D;

	//------------------------------------------------------------------------------
	// scaling factors
	{	
	
		f32* pPdiff = (f32*)alloca(D*sizeof(f32));
		fMatrix Pdiff(1, D, pPdiff);
			
		for(int i=0; i<N; ++i)
		{
			double distance = 99999.0f;;
			for(int j=0; j<N; ++j) 
			{
				if (j!=i)
				{
					fMatrix ti(1, D, m_params+i*D);
					fMatrix tj(1, D, m_params+j*D);

					Pdiff = ti;
					Pdiff -= tj;
					double d = Pdiff.SqrtDist();
					assert(d > 1e-20);
					if (d > 1e-20)
					{
						distance = min(distance, d);
					}
				}
			}

			m_scale[i] = (f32)(1.45f/(distance));
		}
	}


	//------------------------------------------------------------------------------
	// First try least square fitting

	f32* pP = (f32*)alloca(N*D*sizeof(f32));
	fMatrix P(N, D, pP);
	f32 jitter = 0.001f;
	for(int i =0; i<N; ++i)
	{
		for(int j=0; j<D; ++j)
		{
			P(i,j) = m_params[i*D + j];
			if(j == i)
				P(i, j) += jitter;
		}
	}


	f32* pI = (f32*)alloca(N*N*sizeof(f32));
	fMatrix I(N, N, pI);
	I = 0.0;

	for(int i=0; i<N; ++i)
		I(i, i) = 1.0;

	fMatrix A(D, N, m_pA);

	LinearSys(P, I, A);

	//------------------------------------------------------------------------------
	// Calculate error with least square fitting

	f32* pPA = (f32*)alloca(P.Rows()*A.Cols()*sizeof(f32));
	fMatrix PA(P.Rows(), A.Cols(), pPA);
	CMatrix<f32>::MultiMatrix(P, A, PA);

	f32* pQ = (f32*)alloca(N*N*sizeof(f32));
	fMatrix Q(N, N, pQ);
	Q = I;
	Q -= PA;

	//------------------------------------------------------------------------------
	// Fitting with Mixture of Gaussian model

	fMatrix R(N, N, m_pR);

	f32* pH = (f32*)alloca(N*N*sizeof(f32));
	fMatrix H(N, N, pH);

	f32* p_Diff = (f32*)alloca(D*sizeof(f32));
	fMatrix Pdiff(1, D, p_Diff);
	for(int i=0; i<N; ++i)
	{
		for(int j=0; j<N; ++j)
		{
			fMatrix ti(1, D, m_params+i*D);
			fMatrix tj(1, D, m_params+j*D);

			Pdiff = ti;
			Pdiff -= tj;

			f32 d = (f32)(Pdiff.SqrtDist()*m_scale[j]);
			H(i, j) = SplineFunc(d);
		}
	}

	LinearSys(H, Q, R);
}

//------------------------------------------------------------------------------
// Getting weights for Scattered Data Interpolation (SDI), with user provide
// arbitrary parameters
//------------------------------------------------------------------------------
void CSDI::GetWeights(f32* params, f32* weights)
{	
	int D = GetParamDimension();
	int N = GetTotalSamples();

	fMatrix A(D, N, m_pA);

	for(int i=0; i<N; ++i)
	{
		f32 linearTerm = 0;
		for(int j=0; j<D; ++j)
		{
			linearTerm += A(j, i) * params[j];
		}

		f32 AmiTerm = 0;

		f32 pPdiff [MAX_PMG_PARAM_DIM+1];
		fMatrix Pdiff(1, D, pPdiff);
		fMatrix Mparams(1, D, params);

		fMatrix R(N, N, m_pR);
		for(int j=0; j<N; ++j)
		{
			fMatrix paramJ(1, D, m_params+j*D);
			Pdiff = Mparams;
			Pdiff -= paramJ;
			
			f32 d = (f32)(Pdiff.SqrtDist()*m_scale[j]);
			f32 f = SplineFunc(d);
			AmiTerm += R(i, j)* f;
		}

		weights[i] = linearTerm + AmiTerm;
	}


	//------------------------------------------------------------------------------
	// Please keep the following code for future test

	/*
	f32* pPdiff = (f32*)alloca(D*sizeof(f32));
	fMatrix Pdiff(1, D, pPdiff);
	fMatrix Mparams(1, D, params);
	f32 weightSum = 0.0f;

	std::map<f32, int32> distMap;

	for(int i=0; i<N; ++i)
	{		
	fMatrix paramJ(1, D, m_params+i*D);
	Pdiff = Mparams;
	Pdiff -= paramJ;
	f32 d = Pdiff.SqrtDist();
	distMap[d] = i;		
	}

	//std::map<f32, int32>::iterator ita = distMap.begin();
	//for(int i=0; i<N; ++i)
	//{
	//	CryLog("%f, %d", ita->first, ita->second);
	//	++ita;
	//}
	std::map<f32, int32>::iterator it = distMap.end();
	--it;

	//for(int i=0; i<N/2; ++i)
	//	--it;

	int32 minInd = it->second;
	assert(minInd>0 && minInd <N);
	//CryLog("selected: %d, %f", it->second, it->first);

	fMatrix tKnear(1, D, m_params+minInd*D);
	Pdiff = Mparams;
	Pdiff -= tKnear;
	f32 dKnear = Pdiff.SqrtDist();

	dKnear += 1.0f;


	for(int i=0; i<N; ++i)
	{
	fMatrix paramJ(1, D, m_params+i*D);
	Pdiff = Mparams;
	Pdiff -= paramJ;
	f32 dCurr = Pdiff.SqrtDist();

	f32 a = 0.0f;
	if(dCurr<1e-5f)
	a = 1e+5f;
	else
		a = 1.0f / dCurr;//(dCurr, 2);
	//	a = 1.0f / (pow(dCurr, 1));

	f32 b = 0.0f;
	if(dKnear<1e-5f)
	b = 1e+5f;
	else
	b = 1.0f / dKnear;

	weights[i] = a - b;
	weightSum += weights[i];
	}

	for(int i=0; i<N; ++i)
		weights[i] /= weightSum;

*/

	//------------------------------------------------------------------------------
	// test

	/*f32* pPdiff = (f32*)alloca(D*sizeof(f32));
	fMatrix Pdiff(1, D, pPdiff);
	fMatrix Mparams(1, D, params);
	f32 weightSum = 0.0f;

	std::map<f32, int32> distMap;

	for(int i=0; i<N; ++i)
	{		
		fMatrix paramJ(1, D, m_params+i*D);
		Pdiff = Mparams;
		Pdiff -= paramJ;
		f32 d = Pdiff.SqrtDist();
		distMap[d] = i;		
	}

  //std::map<f32, int32>::iterator ita = distMap.begin();
	//for(int i=0; i<N; ++i)
	//{
	//	CryLog("%f, %d", ita->first, ita->second);
	//	++ita;
	//}
	std::map<f32, int32>::iterator it = distMap.end();
	--it;

	//for(int i=0; i<N/2; ++i)
	//	--it;

	int32 minInd = it->second;
	assert(minInd>0 && minInd <N);
	//CryLog("selected: %d, %f", it->second, it->first);

	fMatrix tKnear(1, D, m_params+minInd*D);
	Pdiff = Mparams;
	Pdiff -= tKnear;
	f32 dKnear = Pdiff.SqrtDist();
	

	for(int i=0; i<N; ++i)
	{
		fMatrix paramJ(1, D, m_params+i*D);
		Pdiff = Mparams;
		Pdiff -= paramJ;
		f32 dCurr = Pdiff.SqrtDist();

		f32 a = 0.0f;
		if(dCurr<1e-5f)
			a = 1e+5f;
		else
			a = 1.0f / (pow(dCurr, 6));

		f32 b = 0.0f;
		if(dKnear<1e-5f)
			b = 1e+5f;
		else
			b = 1.0f / (pow(dKnear, 6) );

		weights[i] = a - b;
		weightSum += weights[i];
	}

	f32* synParam = (f32*) alloca(sizeof(f32)*D);
	memset(synParam, 0, sizeof(f32)*D);

	for(int i=0; i<N; ++i)
	{
		weights[i] /= weightSum;
		for(int j=0; j<D; ++j)
			synParam[j] += weights[i]*m_params[i*D + j];
	}

	std::vector<f32> paramDiff(D, 0.0f);

	for(int i=0; i<D; ++i)
		paramDiff[i] = synParam[i] - params[i];

	f32* newParam = (f32*) alloca(sizeof(f32)*D);
	for(int i=0; i<D; ++i)
		newParam[i] = 2*params[i] - synParam[i];

	fMatrix MNewParams(1, D, newParam);
		distMap.clear();
	for(int i=0; i<N; ++i)
	{		
		fMatrix paramJ(1, D, m_params+i*D);
		Pdiff = MNewParams;
		Pdiff -= paramJ;
		f32 d = Pdiff.SqrtDist();
		distMap[d] = i;		
	}

	std::map<f32, int32>::iterator it2 = distMap.end();
	--it2;

	//for(int i=0; i<N/2; ++i)
	//	--it;

	int32 minInd2 = it2->second;
	assert(minInd2>0 && minInd2 <N);
	fMatrix tKnear2(1, D, m_params+minInd2*D);
	Pdiff = MNewParams;
	Pdiff -= tKnear2;
	f32 dKnear2 = Pdiff.SqrtDist();


	std::vector<f32> weights2(N, 0.0f);
	weightSum = 0.0f;
	for(int i=0; i<N; ++i)
	{
		fMatrix paramJ(1, D, m_params+i*D);
		Pdiff = MNewParams;
		Pdiff -= paramJ;
		f32 dCurr = Pdiff.SqrtDist();

		f32 a = 0.0f;
		if(dCurr<1e-5f)
			a = 1e+5f;
		else
			a = 1.0f / (pow(dCurr, 6));

		f32 b = 0.0f;
		if(dKnear<1e-5f)
			b = 1e+5f;
		else
			b = 1.0f / (pow(dKnear2, 6) );

		weights2[i] = a - b;
		weightSum += weights[i];
	}

	memset(synParam, 0, sizeof(f32)*D);
	for(int i=0; i<N; ++i)
	{
		weights2[i] /= weightSum;

		for(int j=0; j<D; ++j)
			synParam[j] += weights2[i]*m_params[i*D + j];
	}

	for(int i=0; i<D; ++i)
		paramDiff[i] = synParam[i] - params[i];

	for(int i=0; i<N; ++i)
		weights[i] = (weights[i] + weights2[i]) / 2.0f;
		*/
}