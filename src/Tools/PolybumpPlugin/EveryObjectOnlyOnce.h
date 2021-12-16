#pragma once

#include <float.h>												// FLT_MAX
#include "RasterCube.h"										// CPossibleIntersectionSink<>

#define ALREADY_TESTED_ARRARY_SIZE	1024



template <class TElement, class TTri>
class CEveryObjectOnlyOnce :public CPossibleIntersectionSink<TTri *>
{
public:

	//! constructor
	//! /param invFrom
	//! /param invDir
	//! /param infRayLength
	//! /param inpIgnore for bias free rayshooting from object surface, may be 0
	CEveryObjectOnlyOnce( const Vec3 &invFrom, const Vec3 &invDir, float infRayLength )
	{
		m_vFrom=invFrom;
		m_vDir=invDir;
		m_fMaxRayLength=infRayLength;
		m_dwAlreadyTested=0;
	}

	//! constructor
	//! /param invFrom
	//! /param invDir
	//! /param infRayLength
	//! /param inpIgnore for bias free rayshooting from object surface, may be 0
	CEveryObjectOnlyOnce( const Vec3 &invFrom, const Vec3 &invDir, float infRayLength, const TElement inIgnore )
	{
		m_vFrom=invFrom;
		m_vDir=invDir;
		m_fMaxRayLength=infRayLength;

		assert(inIgnore!=0xffffffff);
		m_arrAlreadyTested[0]=inIgnore;
		m_dwAlreadyTested=1;
	}


protected:	// --------------------------------------------

	Vec3							m_vFrom;																					//!< position in object space 
	Vec3							m_vDir;																						//!< direction in object space
	TElement					m_arrAlreadyTested[ALREADY_TESTED_ARRARY_SIZE];		//!< [0..ALREADY_TESTED_ARRARY_SIZE-1], this is faster than a set<> at least for typical amounts
	DWORD							m_dwAlreadyTested;																//!< 0..ALREADY_TESTED_ARRARY_SIZE
	float							m_fMaxRayLength;																	//!<

	bool InsertAlreadyTested( const TElement &inObject )
	{
		const TElement *ptr=m_arrAlreadyTested;

		for(uint32 i=0;i<m_dwAlreadyTested;i++)
			if(*ptr++==inObject)
				return true;

		if(m_dwAlreadyTested<ALREADY_TESTED_ARRARY_SIZE-1) 
			m_arrAlreadyTested[m_dwAlreadyTested++]=inObject;
//			else assert(0);		// ALREADY_TESTED_ARRARY_SIZE is not big enough

		return false;
	}
};



// find all intersection within the range
template <class TProvider, class TElement, class TTri>
class CEveryObjectOnlyOnceSTLVector :public CEveryObjectOnlyOnce<TElement,TTri>
{
public:

	//! constructor
	//! /param invFrom
	//! /param invDir
	//! /param infRayLength
	//! /param inIgnore for bias free ray casting from object surface, may be 0
	CEveryObjectOnlyOnceSTLVector( TProvider &rProvider, const Vec3 &invFrom, const Vec3 &invDir, float infRayLength,
		std::vector<CIntersInfo<TTri> > &inOutput, const TElement inIgnore )
		:CEveryObjectOnlyOnce<TElement,TTri>(invFrom,invDir,infRayLength,inIgnore), m_FoundIntersections(inOutput), m_rProvider(rProvider)
	{
	}

	CEveryObjectOnlyOnceSTLVector( TProvider &rProvider, const Vec3 &invFrom, const Vec3 &invDir, float infRayLength,
		std::vector<CIntersInfo<TTri> > &inOutput)
		:CEveryObjectOnlyOnce<TElement,TTri>(invFrom,invDir,infRayLength), m_FoundIntersections(inOutput), m_rProvider(rProvider)
	{
	}

	//! /param inObject 
	//! /param inoutfRayMaxDist in and out value for max. shooting distance or FLT_MAX, if you found a intersection you may update this because further intersections are no longer interesting
	//! /return true=do further processing, false=stop I found what I need (e.g. any intersection for shadow testing)
	bool ReturnElement( const TElement dwIndex, float &inoutfRayMaxDist )
	{
//		TTri *inObject = m_rProvider.GetTrianglePtr(dwIndex);

		if(!InsertAlreadyTested(dwIndex))
		{
			TTri *pTri;

			float fPos = m_rProvider.CalcTriIntersectionFromDir(dwIndex,m_vFrom,m_vDir,pTri);

//			if(fPos!=FLT_MAX)
			{
				assert(fPos>=0);

				if(fPos<m_fMaxRayLength)
				{
					CIntersInfo<TTri> inters;

					inters.m_fDist=fPos;
//					inters.m_Point=m_vFrom+inters.m_fDist*m_vDir;
					inters.m_dwObjectIndex=dwIndex;
					inters.m_pTri=pTri;

					m_FoundIntersections.push_back(inters);
				}
			}
		}
		return true;	// do further intersections
	}

private:	// --------------------------------------------

	std::vector<CIntersInfo<TTri> > &			m_FoundIntersections;															//!< reference to the output STL vector - optimizable
	TProvider &														m_rProvider;
};






//! find if there is a intersection in the range
template <class TProvider, class TElement, class TTri>
class CEveryObjectOnlyOnceBool :public CEveryObjectOnlyOnce<TElement,TTri>
{
public:
	//! constructor
	//! /param invFrom start position in worldspace
	//! /param invDir
	//! /param infRayLength
	//! /param inpIgnore for bias free ray shooting from object surface, may be 0
	CEveryObjectOnlyOnceBool( TProvider &rProvider, const Vec3 &invFrom, const Vec3 &invDir, float infRayLength, const TElement inIgnore )
		:CEveryObjectOnlyOnce<TElement,TTri>(invFrom,invDir,infRayLength,inIgnore), m_rProvider(rProvider)
	{
		m_bResult=false;
	}

	//! /param inObject 
	//! /param inoutfRayMaxDist in and out value for max. shooting distance or FLT_MAX, if you found a intersection you may update this because further intersections are no longer interesting
	//! /return true=do further processing, false=stop I found what I need (e.g. any intersection for shadow testing)
	bool ReturnElement( const uint32 dwIndex, float &inoutfRayMaxDist )
	{
//		TTri *inObject = m_rProvider.GetTrianglePtr(dwIndex);

		if(!InsertAlreadyTested(dwIndex))
		{
			TTri *pTri;
			float fPos = m_rProvider.CalcTriIntersectionFromDir(dwIndex,m_vFrom,m_vDir,pTri);

//			if(fPos!=FLT_MAX)
			{
				assert(fPos>=0);

				if(fPos<m_fMaxRayLength)
				{
					m_bResult=true;
					return false;		// no further intersection, one hit is enough
				}
			}
		}
		
		return true;	// try further intersections, this one was no real one
	}

	//!
	bool GetResult()
	{
		return m_bResult;
	}

private: // --------------------------------------------

	bool																	m_bResult;										//!< true=there was a hit, false otherwise
	TProvider &														m_rProvider;
};





// find nearest intersection to the midpoint of the ray
template <class TProvider, class TElement, class TTri>
class CEveryObjectOnlyOnce_Latest :public CEveryObjectOnlyOnce<TElement,TTri>
{
public:

	//! constructor
	//! /param vLowPolyNormal not necessarily normalized
	CEveryObjectOnlyOnce_Latest( TProvider &rProvider, const Vec3 &invFrom, const Vec3 &invDir, float infRayLength, const Vec3 vLowPolyNormal )
		:CEveryObjectOnlyOnce<TElement,TTri>(invFrom,invDir,infRayLength), m_rProvider(rProvider), m_fBestValue(-1.0f), m_vLowPolyNormal(vLowPolyNormal),
		m_fRayLength(infRayLength)
	{
		m_FoundIntersection.m_pTri=0;
	}

	//! /param inObject 
	//! /param inoutfRayMaxDist in and out value for max. shooting distance or FLT_MAX, if you found a intersection you may update this because further intersections are no longer interesting
	//! /return true=do further processing, false=stop I found what I need (e.g. any intersection for shadow testing)
	bool ReturnElement( const TElement dwIndex, float &inoutfRayMaxDist )
	{
		if(!InsertAlreadyTested(dwIndex))
		{
			TTri *pTri;

			float fPos = m_rProvider.CalcTriIntersectionFromDir(dwIndex,m_vFrom,m_vDir,pTri);

//			if(fPos==FLT_MAX)
			if(fPos>=m_fRayLength)
				return true;	// do further intersections
			
			assert(fPos>=0);

			if(fPos<=m_fBestValue)
				return true;	// do further intersections

			float factor=pTri->ComputeTriangleNormal()*m_vLowPolyNormal;

			if(factor <= 0)
				return true;	// do further intersections

			m_fBestValue=fPos;
			m_FoundIntersection.m_fDist=fPos;
//			m_FoundIntersection.m_Point=m_vFrom+m_FoundIntersection.m_fDist*m_vDir;
			m_FoundIntersection.m_dwObjectIndex=dwIndex;
			m_FoundIntersection.m_pTri=pTri;
		}
		return true;	// do further intersections
	}
	
	CIntersInfo<TTri>											m_FoundIntersection;

private:	// --------------------------------------------

	TProvider &														m_rProvider;
	Vec3																	m_vLowPolyNormal;
	float																	m_fBestValue;
	float																	m_fRayLength;
};



// find nearest intersection to the midpoint of the ray
template <class TProvider, class TElement, class TTri>
class CEveryObjectOnlyOnce_Nearest :public CEveryObjectOnlyOnce<TElement,TTri>
{
public:

	//! constructor
	//! /param vLowPolyNormal not necessarily normalized
	CEveryObjectOnlyOnce_Nearest( TProvider &rProvider, const Vec3 &invFrom, const Vec3 &invDir, float infRayLength, const Vec3 vLowPolyNormal )
		:CEveryObjectOnlyOnce<TElement,TTri>(invFrom,invDir,infRayLength), m_rProvider(rProvider), m_fBestValue(infRayLength/2.0f), m_vLowPolyNormal(vLowPolyNormal),
		m_fMid(infRayLength/2.0f)
	{
		m_FoundIntersection.m_pTri=0;
	}

	//! /param inObject 
	//! /param inoutfRayMaxDist in and out value for max. shooting distance or FLT_MAX, if you found a intersection you may update this because further intersections are no longer interesting
	//! /return true=do further processing, false=stop I found what I need (e.g. any intersection for shadow testing)
	bool ReturnElement( const TElement dwIndex, float &inoutfRayMaxDist )
	{
		if(!InsertAlreadyTested(dwIndex))
		{
			TTri *pTri;

			float fPos = m_rProvider.CalcTriIntersectionFromDir(dwIndex,m_vFrom,m_vDir,pTri);

			if(fPos==FLT_MAX)
				return true;	// do further intersections

			assert(fPos>=0);

			float fAbsPos = fabs(fPos-m_fMid);

			if(fAbsPos>=m_fBestValue)
				return true;	// do further intersections

			float factor = pTri->ComputeTriangleNormal()*m_vLowPolyNormal;

			if(factor<=0)
				return true;	// do further intersections

			m_fBestValue=fAbsPos;
			m_FoundIntersection.m_fDist=fPos;
//			m_FoundIntersection.m_Point=m_vFrom+m_FoundIntersection.m_fDist*m_vDir;
			m_FoundIntersection.m_dwObjectIndex=dwIndex;
			m_FoundIntersection.m_pTri=pTri;
		}
		return true;	// do further intersections
	}

	CIntersInfo<TTri>											m_FoundIntersection;

private:	// --------------------------------------------

	TProvider &														m_rProvider;
	Vec3																	m_vLowPolyNormal;
	float																	m_fBestValue;
	float																	m_fMid;
};



// find nearest intersection to the midpoint of the ray on the first half and on the second half of the ray
template <class TProvider, class TElement, class TTri>
class CEveryObjectOnlyOnce_Nearest2Sides :public CEveryObjectOnlyOnce<TElement,TTri>
{
public:

	//! constructor
	//! /param vLowPolyNormal not necessarily normalized
	CEveryObjectOnlyOnce_Nearest2Sides( TProvider &rProvider, const Vec3 &invFrom, const Vec3 &invDir, float infRayLength )
		:CEveryObjectOnlyOnce<TElement,TTri>(invFrom,invDir,infRayLength), m_rProvider(rProvider), m_fBestMin(-infRayLength/2), m_fBestMax(infRayLength/2),
		m_fMid(infRayLength/2.0f)
	{
	}

	//! /param inObject 
	//! /param inoutfRayMaxDist in and out value for max. shooting distance or FLT_MAX, if you found a intersection you may update this because further intersections are no longer interesting
	//! /return true=do further processing, false=stop I found what I need (e.g. any intersection for shadow testing)
	bool ReturnElement( const TElement dwIndex, float &inoutfRayMaxDist )
	{
		if(!InsertAlreadyTested(dwIndex))
		{
			TTri *pTri;

			float fPos = m_rProvider.CalcTriIntersectionFromDir(dwIndex,m_vFrom,m_vDir,pTri);

			if(fPos==FLT_MAX)
				return true;	// do further intersections

			assert(fPos>=0);

			float fPosNeg = fPos-m_fMid;

			if(fPosNeg>=0)
			{
				if(fPosNeg<m_fBestMax)
					m_fBestMax=fPosNeg;
			}
			else
			{
				if(fPosNeg>m_fBestMin)
					m_fBestMin=fPosNeg;
			}
		}
		return true;	// do further intersections
	}

	float																	m_fBestMin;
	float																	m_fBestMax;

private:	// --------------------------------------------

	TProvider &														m_rProvider;
	float																	m_fMid;
};