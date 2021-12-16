#pragma once


#include "SimpleBitmap.h"							// SimpleBitmap<>
#include <vector>											// STL vector<>
#include "SummedAreaFilterKernel.h"		// CSummedAreaFilterKernel

class CWeightFilterSet
{
public:

	// Arguments:
	//   bCenter - true=center is in mid pixel, false=center is at a pixel corner
	bool Create( const CSummedAreaFilterKernel &inFilter, const float infR, const bool bCenter );

	// 3x3
	bool CreateSharpen( const float fStrength );

	//!
	void FreeData();

	bool IsValid() const
	{
		return m_FilterKernelBlock.IsValid();
	}

	//! optimizable
	//! weight is 1.0
	//! /param iniX x position in inoutDest
	//! /param iniY y position in inoutDest
	//! /param TInputImage typically CSimpleBitmap<TElement>
	//! /return weight
	template <class TElement, class TInputImage >
	float GetBlockWithFilter( const TInputImage &inSrc, const int iniX, const int iniY, TElement &outResult, const bool isCubemap, uint32 dwXMask=0xffffffff, uint32 dwYMask=0xffffffff )
	{
		float fWeightSum=0.0f;
		const CSimpleBitmap<float> &rBitmap=m_FilterKernelBlock;
		
		int W=(int)rBitmap.GetWidth();
		int H=(int)rBitmap.GetHeight();

		int iSrcW=(int)inSrc.GetWidth();
		int iSrcH=(int)inSrc.GetHeight();

		assert(dwXMask);
		assert(dwYMask);

		float *pfWeights=rBitmap.GetPointer(0,0);

		const EImageFilteringMode filteringMode = isCubemap?eifmCubemapFilter:eifm2DBorder;
		if(isCubemap)	// for cubemap mode we don't need tiling
		{
			dwXMask = 0xffffffff;
			dwYMask = 0xffffffff;
		}

		for(int y=0;y<H;y++)
		{
			int iDestY=y+iniY-H/2;

			for(int x=0;x<W;x++,pfWeights++)
			{
				int iDestX=x+iniX-W/2;

				TElement Value;

				if(inSrc.GetForFiltering(((uint32)iDestX)%dwXMask,((uint32)iDestY)%dwYMask, iniX, iniY, Value, filteringMode))			// tiled
				{
					float fWeight=*pfWeights;
	
					outResult += Value*fWeight;
					fWeightSum += fWeight;
				}
				else assert(0);
			}
		}

//		assert(fWeightSum<=1.0f);		// less is better - more can introduce in amplifying the data
		return fWeightSum;
	}

	// should be 1.0f
	float ComputeSum() const
	{
		float fWeightSum=0.0f;

		int W=(int)m_FilterKernelBlock.GetWidth();
		int H=(int)m_FilterKernelBlock.GetHeight();

		float *pfWeights=m_FilterKernelBlock.GetPointer(0,0);

		for(int i=0;i<H*W;i++)
			fWeightSum += *pfWeights++;

		return fWeightSum;
	}

	int GetBorderSize()
	{
		int W=(int)m_FilterKernelBlock.GetWidth();

		return (W-1)/2;
	}
	
	void Debug( const char *szName );

private: // -------------------------------------------------------------

	CSimpleBitmap<float>										m_FilterKernelBlock;			//!< weight = 1
};
