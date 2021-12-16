#include "StdAfx.h"
#include "../CCryDXPS.hpp"
#include "../../CCryTypes.hpp"
#include "CCryDXPSGCM_CMBMan.hpp"
#include "../../Device/Resource/Buffer/CCryDXPSBuffer.hpp"


extern "C" void *mwprivate2_memalign(size_t, size_t, ECryModule);

using namespace CRY_DXPS_GCMNAMESPACE;

static 	void*								g_pCmdBuffer;
static	CellGcmControl*			g_pControll;


namespace
{
	ILINE void Flush()
	{
		__sync();
		//faster way for: cellGcmAddressToOffset(gCellGcmCurrentContext->current, &Offset);
		uint32_t Offset = 
			tdLayer0::CMB().CmdBufferOffset() + 
			(uint32)gCellGcmCurrentContext->current -
			(uint32)g_pCmdBuffer;
		g_pControll->put = Offset;
		__sync();
	}
}

#if defined(CCRY_DXPS_UNSAFE)
int32_t DebugCallback(CellGcmContextData* pContext, uint32_t Id)
{
//	OutputDebugString("Callback\n");

	cellGcmSetJumpCommand(CELL_GCM_INIT_STATE_OFFSET);
	pContext->current	=	(uint32_t*)&reinterpret_cast<uint8*>(g_pCmdBuffer)[CELL_GCM_INIT_STATE_OFFSET];
//	gCellGcmCurrentContext->current	=	(uint32*)&reinterpret_cast<uint8*>(g_pCmdBuffer)[CELL_GCM_INIT_STATE_OFFSET];
	Flush();
	return CELL_OK;
}
#endif

void CCryDXPSGCMCMBMan::Init()
{
	m_Initialized	=	1;
	g_pCmdBuffer = mwprivate2_memalign(1024*1024, GCM_CMD_SIZE+tdLayer0::Sync().MemoryNeeded(), eCryM_Render);//64kb min size of default cmd buffer
	if(CELL_OK!=cellGcmInit(GCM_CMD_SIZE-16*1024, GCM_CMD_SIZE, g_pCmdBuffer))
	{
		CRY_DEBUGOUT(__FUNC__);
		CRY_DEBUGOUT_ALWAYS(" Failed to initialize GCM (cellGcmInit())\n");
		return;
	}
	else
	{
		CRY_DEBUGOUT(" successfully initialized GCM\n");
	}

//	cellGcmAddressToOffset(&reinterpret_cast<uint8*>(g_pCmdBuffer)[gCellGcmCurrentContext->current],&m_CmdBufferOffset);
	//regarding the doc, the CMDBuffer starts always at 0
	m_CmdBufferOffset=~0;
	if(CELL_OK!=cellGcmAddressToOffset(g_pCmdBuffer,&m_CmdBufferOffset))
	{
		CRY_DEBUGOUT_ALWAYS("Could not translate commandbuffer-address to offset\n");
		return;
	}

	g_pControll	=	cellGcmGetControlRegister();

#if defined(CCRY_DXPS_UNSAFE)
	gCellGcmCurrentContext->begin			=	(uint32*)&reinterpret_cast<uint8*>(g_pCmdBuffer)[CELL_GCM_INIT_STATE_OFFSET];
	gCellGcmCurrentContext->end				=	(uint32*)&reinterpret_cast<uint8*>(g_pCmdBuffer)[CELL_GCM_INIT_STATE_OFFSET+GCM_CMD_SIZE-GCM_CMD_SEGMENTSIZE];
	gCellGcmCurrentContext->callback	=	DebugCallback;
#endif

	for(uint32 a=0;a<GCM_CMD_SEGMENTCOUNT;a++)
		m_SegmentHandle[a]=TDRES_CREATE(0);
	const uint32 Current =	(uint32)gCellGcmCurrentContext->current-(uint32)g_pCmdBuffer;
	m_LastUsedSegment	=	Segment(Current);
}

void* CCryDXPSGCMCMBMan::SyncMemory()
{
	return reinterpret_cast<uint8*>(g_pCmdBuffer)+GCM_CMD_SIZE;
}

void CCryDXPSGCMCMBMan::FlushCMDs()
{
//	PROFILE_FRAME(Flush);
#if !defined(CCRY_DXPS_UNSAFE)
	cellGcmFlush();
#else
	Flush();

	//if here is something changed, please adapt in Tools/PS3JobManager/SPU/libDriverDMA/LibGCM_spu.cpp
	const uint32 Current =	(uint32)gCellGcmCurrentContext->current-(uint32)g_pCmdBuffer;
	const uint32 CurrentSeg	=	Segment(Current);
	//current crosses segments?
	if(CurrentSeg!=m_LastUsedSegment)
	{
		//then make sure next segment is not in use anymore
		const uint32 Next				=	Current+GCM_CMD_SEGMENTSIZE>=GCM_CMD_SIZE?CELL_GCM_INIT_STATE_OFFSET:Current+GCM_CMD_SEGMENTSIZE;
		const uint32 NextSeg		=	Segment(Next);
		uint32 zWriteCount;
		tdLayer0::Sync().SyncToRSX<true,false>(m_SegmentHandle[NextSeg], zWriteCount);

		//set most recent handle for last segment
		m_SegmentHandle[m_LastUsedSegment]	=	tdLayer0::Sync().HandleDeviceThread();
//		printf("0x%x 0x%x %d %d\n",Current,cellGcmGetControlRegister()->get,m_LastUsedSegment,CurrentSeg);

		m_LastUsedSegment=CurrentSeg;

		//loop?
		if(Current+GCM_CMD_SEGMENTSIZE>=GCM_CMD_SIZE)
		{
			cellGcmSetJumpCommand(CELL_GCM_INIT_STATE_OFFSET);
			gCellGcmCurrentContext->current	=	(uint32*)&reinterpret_cast<uint8*>(g_pCmdBuffer)[CELL_GCM_INIT_STATE_OFFSET];
	//		sys_timer_usleep(100000);
	//		printf("LOOOOP\n");
		}
		Flush();
	}

#if defined(CRY_DXPS_SINGLEFLUSHVALIDATE)
	while(cellGcmGetControlRegister()->get!=cellGcmGetControlRegister()->put)
	{
		sys_timer_usleep(10);
		int a=0;
	}
#endif
#endif
}

void CCryDXPSGCMCMBMan::Size(ICrySizer* Sizer)
{
	{
		SIZER_COMPONENT_NAME(Sizer,"DXPS Commandbuffer manager");		
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS RSX-Commandbuffer");
			Sizer->AddObject(g_pCmdBuffer,GCM_CMD_SIZE+tdLayer0::Sync().MemoryNeeded());
		}
	}
}


