#include "StdAfx.h"
#include "../CCryDXPS.hpp"
#include "../../CCryTypes.hpp"
#include "CCryDXPSGCM_TiledRegionMan.hpp"

CCryDXPSGCMTiledRegionMan::CCryDXPSGCMTiledRegionMan()
{
	Init();
}

void CCryDXPSGCMTiledRegionMan::Init()
{
	for(uint32 a=0;a<CRY_TRM_COUNT;a++)
		m_Region[a].Init();
}

uint32 CCryDXPSGCMTiledRegionMan::Create(uint32 Offset,uint32 Pitch,uint32 Size,uint32 Compression)
{
	const uint32 ID	=	Alloc();
	if(ID>=CRY_TRM_COUNT)
		return ID;

#if defined(CRY_DXPS_DEVICETHREAD)
	tdLayer0::Device()->ThreadFinish();
#endif

	Size	=	(Size+0xffff)&~0xffff;

	CCryDXPSGCMRegion& rRegion	=	m_Region[ID];

	//seeking hole in local tag memory area
	int TagOffset	=	0;
	do
	{
		rRegion.Init(TagOffset,TagOffset+Size/0x10000);
		for(uint32 a=0;a<CRY_TRM_COUNT;a++)
			if(ID!=a && m_Region[a].Overlaps(rRegion))
			{
				TagOffset	=	m_Region[a].End();
				break;
			}
	}while(rRegion.Start()!=TagOffset && (TagOffset<CRY_TRM_MEMSIZE));//loop till neither the TagOffset was not moved
																																		//cause a free slot was found and we are not out
																																		//of the tag memory amount of 0x7ff

	if(TagOffset>=CRY_TRM_MEMSIZE)
	{
		rRegion.Free();
		return CRY_TRM_COUNT;
	}


	int Ret;
//	CRY_DEBUGOUT_ALWAYS("ID:%d,Offset:%d,Size:%d,End:%d,Pitch:%d,TagOffset:%d\n",ID,Offset,Size,Offset+Size,Pitch,rRegion.Start());
	if((Ret=cellGcmSetTileInfo(ID,CELL_GCM_LOCATION_LOCAL,Offset,Size,Pitch,Compression,TagOffset,ID&3))!=CELL_OK)
	{
		CRY_DEBUGOUT_ALWAYS("failed creating tile region %d\n",Ret);
		return CRY_TRM_COUNT;
	}

	if((Ret=cellGcmBindTile(ID))!=CELL_OK)
	{
		CRY_DEBUGOUT_ALWAYS("failed binding tile region %d\n",Ret);
		return CRY_TRM_COUNT;
	}
//	else
//	{
//		CRY_DEBUGOUT_ALWAYS("successfully binding tile region %d\n",ID);
//	}
//#else
//	cellGcmSetTile(ID,CELL_GCM_LOCATION_LOCAL,Offset,Size,Pitch,Compression,TagOffset,ID&3);
//#endif

#if defined(CRY_DXPS_DEVICETHREAD)
	tdLayer0::Device()->UpdateCmdBuffer();
#endif
	CELL_GCM_FLUSH;
	return ID;
}

void CCryDXPSGCMTiledRegionMan::Free(uint32 ID)
{
#if defined(CRY_DXPS_DEVICETHREAD)
	tdLayer0::Device()->ThreadFinish();
#endif
	if(ID>=CRY_TRM_COUNT)
		return;
	m_Region[ID].Free();
	cellGcmUnbindTile(ID);
#if defined(CRY_DXPS_DEVICETHREAD)
	tdLayer0::Device()->UpdateCmdBuffer();
#endif
	CELL_GCM_FLUSH;
}

void CCryDXPSGCMTiledRegionMan::Size(ICrySizer* Sizer)
{
	{
		SIZER_COMPONENT_NAME(Sizer,"DXPS Tiled region manager");		
	}

}