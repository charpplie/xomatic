#include "StdAfx.h"
#include "PolybumpProgress.h"

CPolybumpProgress::CPolybumpProgress() :m_NextIdToCheckIfPropagated(0)
{
}



void CPolybumpProgress::Init( const uint32 dwIdCount )
{
	m_ProgressBlockmap.resize(dwIdCount);
	Clear();
}


void CPolybumpProgress::Clear()
{
	memset(&m_ProgressBlockmap[0],EPPS_NotStarted,sizeof(EPolybumpProgressState)*m_ProgressBlockmap.size());
	m_NextIdToCheckIfPropagated=0;
}


void CPolybumpProgress::DeInit()
{
	m_ProgressBlockmap.resize(0);			// should not reallocate
	m_NextIdToCheckIfPropagated=0;
}

uint32 CPolybumpProgress::PropagateTo( CPolybumpProgress &rDest ) const
{
	if(m_ProgressBlockmap.empty())
		return 0xffffffff;		// good?

	assert(!m_ProgressBlockmap.empty());			// call Init() before

	uint32 dwI=rDest.m_NextIdToCheckIfPropagated;
	uint32 dwWarpId = (uint32)m_ProgressBlockmap.size();

	do
	{
		uint32 dwRet=dwI;

		EPolybumpProgressState &refOldVal = rDest.m_ProgressBlockmap[dwI];
		EPolybumpProgressState newVal = m_ProgressBlockmap[dwI];

		++dwI;

		// wrap around
		if(dwI==dwWarpId)
			dwI=0;

		if(refOldVal!=newVal)
		{
			refOldVal=newVal;
			rDest.m_NextIdToCheckIfPropagated=dwI;
			return dwRet;
		}

	} while(dwI!=rDest.m_NextIdToCheckIfPropagated);

	return 0xffffffff;
}

CPolybumpProgress::EPolybumpProgressState CPolybumpProgress::GetState( const uint32 dwId ) const
{
	assert(!m_ProgressBlockmap.empty());			// call Init() before
	assert(dwId<m_ProgressBlockmap.size());

	return m_ProgressBlockmap[dwId];
}

void CPolybumpProgress::SetState( const uint32 dwId, const CPolybumpProgress::EPolybumpProgressState state )
{
	assert(!m_ProgressBlockmap.empty());			// call Init() before
	assert(dwId<m_ProgressBlockmap.size());

	m_ProgressBlockmap[dwId]=state;
}
