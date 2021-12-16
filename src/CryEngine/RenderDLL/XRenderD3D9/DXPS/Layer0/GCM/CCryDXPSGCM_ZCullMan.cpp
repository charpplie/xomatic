#include "StdAfx.h"
#include "../CCryDXPS.hpp"
#include "../../CCryTypes.hpp"
#include "CCryDXPSGCM_ZCullMan.hpp"

CCryDXPSGCMZCullMan::CCryDXPSGCMZCullMan()
{
	Init();
}

void CCryDXPSGCMZCullMan::Init()
{
	for(uint32 a=0;a<CRY_ZCM_COUNT;a++)
		m_ZCull[a].Init();
}

uint32 CCryDXPSGCMZCullMan::Create(uint32 Offset,uint32 SizeX,uint32 SizeY)
{
	const uint32 ID	=	Alloc();
	if(ID>=CRY_ZCM_COUNT)
		return ID;

#if defined(CRY_DXPS_DEVICETHREAD)
	tdLayer0::Device()->ThreadFinish();
#endif

	const uint32 Size	=	(SizeX*SizeY+0xfff)&~0xfff;

	CCryDXPSGCMZCull& rZCull	=	m_ZCull[ID];

	//seeking hole in local tag memory area
	int TagOffset	=	0;
	do
	{
		rZCull.Init(TagOffset,TagOffset+Size);
		for(uint32 a=0;a<CRY_TRM_COUNT;a++)
			if(ID!=a && m_ZCull[a].Overlaps(rZCull))
			{
				TagOffset	=	m_ZCull[a].End();
				break;
			}
	}while(rZCull.Start()!=TagOffset && (TagOffset<CRY_ZCM_MEMSIZE));//loop till neither the TagOffset was not moved
																																		//cause a free slot was found and we are not out
																																		//of the tag memory amount of 0x7ff

	if(TagOffset>=CRY_ZCM_MEMSIZE)
	{
		rZCull.Free();
		return CRY_TRM_COUNT;
	}

	cellGcmSetZcull(ID,Offset,SizeX,SizeY,TagOffset,CELL_GCM_ZCULL_Z24S8,CELL_GCM_SURFACE_CENTER_1,
									CELL_GCM_ZCULL_LESS,CELL_GCM_ZCULL_MSB,CELL_GCM_SCULL_SFUNC_EQUAL,0x01,0xff);

#if defined(CRY_DXPS_DEVICETHREAD)
	tdLayer0::Device()->UpdateCmdBuffer();
#endif
	CELL_GCM_FLUSH;
	return ID;
}

void CCryDXPSGCMZCullMan::Free(uint32 ID)
{
	if(ID>=CRY_ZCM_COUNT)
		return;
	m_Count--;
	m_ZCull[ID].Free();
//	cellGcmSetInvalidateTile(ID);
}

void CCryDXPSGCMZCullMan::RecalcHeuristic()
{
/*	//we cannot handle it right now on SPU
	using namespace CRY_DXPS_GCMNAMESPACE;

	cellGcmSetReport(CELL_GCM_ZCULL_STATS,  REPORT_ZCULL_STATS0);
	cellGcmSetReport(CELL_GCM_ZCULL_STATS1, REPORT_ZCULL_STATS1);
	int32	maxSlope = cellGcmGetReport(CELL_GCM_ZCULL_STATS,  REPORT_ZCULL_STATS0);
	int32	sumSlope = cellGcmGetReport(CELL_GCM_ZCULL_STATS1, REPORT_ZCULL_STATS1);
	int32	numTiles, avgSlope, moveForward, pushBack;

	numTiles = maxSlope & 0xffff;
	maxSlope = (maxSlope & 0xFFFF0000) >> 16;
	avgSlope = numTiles ? sumSlope / numTiles : 0;

	moveForward = (avgSlope + maxSlope) / 2;
	pushBack = moveForward / 2;

	if (moveForward < 1)
		moveForward = 1;

	if (pushBack < 1)
		pushBack = 1;

	cellGcmSetZcullLimit(moveForward, pushBack);
*/
}

void CCryDXPSGCMZCullMan::Size(ICrySizer* Sizer)
{
	{
		SIZER_COMPONENT_NAME(Sizer,"DXPS ZCull manager");		
	}
}