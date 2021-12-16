#include "StdAfx.h"
#include <assert.h>										// assert()
#include "angularcubemapfilter.h"			// CAngularFilterSet

void CAngularFilterSet::FreeData()
{
	m_FilterKernelBlock.FreeData();
	m_pCubemap = NULL;
}

bool CAngularFilterSet::SetCubemap( const CSummedAreaFilterKernel &inFilter, const float inAngle, 
																	 const CSimpleBitmap<float>* pCubemap, const bool bCenter )
{
	FreeData();

	assert(pCubemap);
	assert(pCubemap->GetWidth() == pCubemap->GetHeight()*6);
	m_nCubeSideSize = pCubemap->GetHeight();
	m_pCubemap = pCubemap;

	float fCot = 1.f / tanf(inAngle);
	assert(fabs(fCot)>=0);	// check for NANs etc.
	float fRadius = fCot * sqrtf(3.f) * m_nCubeSideSize;

	inFilter.CreateWeightFilterBlock(m_FilterKernelBlock, 1, fRadius, bCenter);

	m_fAngle = inAngle;

	return true;
}

void CAngularFilterSet::GetBlockWithFilter( const Vec3& srcDir, float *outResult )
{
	assert(m_pCubemap);
	float fWeightSum=0.0f;
	const CSimpleBitmap<float> &rBitmap=m_FilterKernelBlock;

	int W=(int)rBitmap.GetWidth();
	int H=(int)rBitmap.GetHeight();

	int iSrcW=(int)m_pCubemap->GetWidth();
	int iSrcH=(int)m_pCubemap->GetHeight();

	float *pfWeights=rBitmap.GetPointer(0,0);

	//for(int y=0;y<H;y++)
	//{
	//	int iDestY=y/*+iniY*/-H/2;

	//	for(int x=0;x<W;x++,pfWeights++)
	//	{
	//		int iDestX=x/*+iniX*/-W/2;

	//		Vec3 destDir;

	//		float Value;
	//		m_pCubemap->GetForFiltering(destDir, Value);
	//		float fWeight=*pfWeights;
	//		(*outResult) += Value*fWeight;
	//		fWeightSum += fWeight;
	//	}
	//}
}