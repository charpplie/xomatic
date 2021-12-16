#include "StdAfx.h"
#include <assert.h>										// assert()
#include "weightfilterset.h"					// CWeightFilterSet

bool CWeightFilterSet::CreateSharpen( const float fStrength )
{
	assert(fStrength>0.0f);
	assert(fStrength<=1.0f);

	float fNegStrength = -0.125f * fStrength;

	m_FilterKernelBlock.Alloc(3,3,&fNegStrength);

	m_FilterKernelBlock.Set(1,1,0.999999f+fStrength);

	assert(ComputeSum()<=1.0f);
	return true;
}


void CWeightFilterSet::FreeData()
{
	m_FilterKernelBlock.FreeData();
}


bool CWeightFilterSet::Create( const CSummedAreaFilterKernel &inFilter, const float infR, const bool bCenter )
{
	FreeData();

	inFilter.CreateWeightFilterBlock(m_FilterKernelBlock,1,infR,bCenter);

//	assert(ComputeSum()<=1.0f);

	return true;
}

void CWeightFilterSet::Debug( const char *szName )
{
	char str[256];

	sprintf(str,"kernel '%s'=\n{\n",szName);
	OutputDebugString(str);

	float fSum=0.0f;

	for(uint32 dwY=0;dwY<m_FilterKernelBlock.GetHeight();++dwY)
	{
		OutputDebugString("   ");

		for(uint32 dwX=0;dwX<m_FilterKernelBlock.GetWidth();++dwX)
		{
			float fValue = m_FilterKernelBlock.GetRef(dwX,dwY);

			sprintf(str,"%.2f ",fValue);
			OutputDebugString(str);

			fSum+=fValue;
		}
		OutputDebugString("\n");
	}

	sprintf(str,"}\n sum = %.2f\n",fSum);
	OutputDebugString(str);
}
