#pragma once

#include "SimpleBitmap.h"							// SimpleBitmap<>
#include <vector>											// STL vector<>
#include "SummedAreaFilterKernel.h"		// CSummedAreaFilterKernel
#include "Cry_Math.h"							// Vec3

class CAngularFilterSet
{
public:

	// Arguments:
	//   bCenter - true=center is in mid pixel, false=center is at a pixel corner
	bool SetCubemap( const CSummedAreaFilterKernel &inFilter, const float inAngle, const CSimpleBitmap<float>* pCubemap, const bool bCenter );

	//! optimizable
	//! weight is 1.0
	//! /param iniX x position in inoutDest
	//! /param iniY y position in inoutDest
	//! /param TInputImage typically CSimpleBitmap<TElement>
	//! /return weight
	void GetBlockWithFilter( const Vec3& srcDir, float *outResult );
	void FreeData();

private: // -------------------------------------------------------------

	CSimpleBitmap<float>										m_FilterKernelBlock;			//!< weight = 1
	const CSimpleBitmap<float>*							m_pCubemap;								//!< src cubemap
	float																		m_fAngle;									//!< angle of cone
	uint32																	m_nCubeSideSize;					//!< angle of cone
};
