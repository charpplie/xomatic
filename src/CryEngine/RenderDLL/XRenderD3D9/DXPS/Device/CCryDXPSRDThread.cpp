#include "StdAfx.h"
#include "../Layer0/CCryDXPS.hpp"
#include "CCryDXPSRDThread.hpp"

#if !defined(__SPU__)
	extern volatile int g_FlipSpinLock;

//DECLARE_SPU_JOB("MemsetSPULarge", TMemsetJob)

inline CDXPSRDThread::~CDXPSRDThread()
{
  Cancel();
	WaitForThread();
	READ_WRITE_BARRIER
}

extern NPPU::SFrameProfileRSXData& GetFrameStatsSPUThread();
static uint32 g_RSXWaitTime		= 0;
static uint32 g_RSXStallTime	= 0;
static uint32 g_DeviceStallTime	= 0;
uint64 g_StallRecords[32] = {0};//one record per thread
extern uint32 MapThreadIDToIndex(uint32);

void cellGcmAddRSXWaitTicks(const uint64 cStart, const uint64 cEnd)
{
	uint64 diff = (cEnd - cStart);
	g_RSXWaitTime += (uint32)diff;
}

void cellGcmAddRSXStallTicks(const unsigned long long cStart, const unsigned long long cEnd)
{
	uint64 diff = (cEnd - cStart);
	if(g_bProfilerEnabled)
		g_StallRecords[MapThreadIDToIndex((uint32)GetCurrentThreadId())] = diff;
	g_RSXStallTime += (uint32)diff;
}

void cellGcmAddDeviceStallTicks(const unsigned long long cStart, const unsigned long long cEnd)
{
	uint64 diff = (cEnd - cStart);
	if(g_bProfilerEnabled)
		g_StallRecords[MapThreadIDToIndex((uint32)GetCurrentThreadId())] = diff;
	g_DeviceStallTime += (uint32)diff;
}

void cellGcmGetAndResetRSXWaitTicks()
{
	NPPU::SFrameProfileRSXData& rProfData = GetFrameStatsSPUThread();
	const float cInvTB		= 1000.f / 79800.f;//inverse ticks per usec
	rProfData.rsxWaitTime = (uint32)((float)g_RSXWaitTime * cInvTB);
	rProfData.flushTime		= (uint32)((float)g_RSXStallTime * cInvTB);
	g_RSXWaitTime		= 0;
	g_RSXStallTime	= 0;
	g_DeviceStallTime = 0;
//		memset(g_StallRecords, 0, sizeof(g_StallRecords));
}

//	#define DABR_PPU
#if defined(DABR_PPU)
	uint32 g_DABRPPU = 0;//use debugger to set dma control check ad hoc
#endif

#include <sys/gpio.h>
//return DIP value for switch 0
static inline uint64 GetDIPValue0()
{
	uint64 dipVal = 0;
	sys_gpio_get(SYS_GPIO_DIP_SWITCH_DEVICE_ID, &dipVal);
	return dipVal&1;
}

static inline bool EnableVSync()
{
	//expect vsync to be false initially
	static const uint64 scInitialDipVal = GetDIPValue0();
	return scInitialDipVal == GetDIPValue0();
}

#if defined(CRY_DXPS_PERFORMANCECOUNTING)
CellGcmReportData*		g_pReport;
#endif

static void DXPSFlipIssued(const uint32_t/* head*/)//param is always one without any meaning, regarding doc sdk220
{
	tdLayer0::Sync().SwapRSX();
	g_FlipVars.pFlipLockedTarget	=	0;
	SNSTOPMARKER(SNTM_FLIPPING);
}

static void DXPSVBIssued(const uint32_t Flag)//param is always one without any meaning, regarding doc sdk220
{
	//flip triggered?
	if(g_FlipVars.flipDrawCallID==TDRES_CREATE(0))
		return;
	uint32 Dummy;
	if(EnableVSync()!=(CRenderer::CV_r_vsync==Flag) || !tdLayer0::Sync().SyncToRSX<false,false>(g_FlipVars.flipDrawCallID,Dummy))
		return;

	SNSTOPMARKER(SNTM_FLIPREQUEST);
	SNSTARTMARKER(SNTM_FLIPPING,"FlippingInVBIssued");
	
	//there is a pending cellGcmSetPrepareFlip
	//execute on a dummy context to make the rsx control register frame id set correctly (only way to get working from spu)
	if(g_FlipVars.flipModeUsed == 0)
	{
		#define DUMMY_CONTEXT_SIZE (128)
		uint32_t dummyArea[DUMMY_CONTEXT_SIZE] _ALIGN(16);
		CellGcmContextData dummyContext = {dummyArea, &dummyArea[DUMMY_CONTEXT_SIZE], dummyArea, NULL};
		using namespace CRY_DXPS_GCMNAMESPACE;
		const int cFlipBufID = g_FlipVars.flipBufID;
		if(CELL_GCM_ERROR_FAILURE == cellGcmSetPrepareFlip(&dummyContext,cFlipBufID))
			printf("cellGcmSetPrepareFlip failed for ID=%d\n",cFlipBufID);
		#undef DUMMY_CONTEXT_SIZE
	}

	{
		CrySpinLock(&g_FlipSpinLock, 0, 1);		
		const int32_t cFrameID = g_FlipVars.flipFrameID;
	#if defined(CRY_MM_DEBUG_DEFRAG)
		if(CRenderer::CV_r_PS3VMemDefragDebug>0)
			tdLayer0::Memory().DrawDebug(CRenderer::CV_r_PS3VMemDefragDebug,g_FlipLockedTarget);
	#endif
		
		if(CELL_GCM_ERROR_FAILURE == cellGcmSetFlipImmediate(cFrameID))
		{
			if(g_FlipVars.flipFrameID != cFrameID)
				printf("cellGcmSetFlipImmediate flip frame mismatch(%d != %d)\n",g_FlipVars.flipFrameID,cFrameID);
			printf("cellGcmSetFlipImmediate failed for ID=%d\n",cFrameID);
			snPause();
		}
		g_FlipVars.flipDrawCallID =	TDRES_CREATE(0);
		READ_WRITE_BARRIER;
		__sync();
		g_FlipSpinLock = 0;
	}
}

void CDXPSRDThread::Init()
{
	m_CMDBCurrent	=	0;
	m_CMDBPut	=	0;
	m_CMDBGet	=	0;
	#if defined(CRY_DXPS_DEVICETHREAD) && !defined(_RELEASE)
		m_Waiting = 1;
		Start(0, "DXPSThread", THREAD_PRIORITY_NORMAL, SIMPLE_THREAD_STACK_SIZE_KB*1024);
	#endif//CRY_DXPS_DEVICETHREAD
	cellGcmSetFlipHandler(DXPSFlipIssued);
	cellGcmSetVBlankHandler(DXPSVBIssued);
	cellGcmSetUserHandler(DXPSVBIssued);
	m_Worker.InitPSCache();
}

#if defined(CRY_DXPS_DEVICETHREAD)
void CDXPSRDThread::Run()
{
#if !defined(_RELEASE)
	CryThreadSetName( -1, "DXPSRenderDevice" );
	CDXPSRDJobSPU* pJob;
	while(true)
	{
		volatile uint32 LoopCount=0;
		pJob	=	reinterpret_cast<CDXPSRDJobSPU*>(&m_CMDBuffer[m_CMDBGet]);
		while(m_CMDBGet==m_CMDBPut || (pJob->IsSPUJob() && (gPS3Env->spuEnabled != -1)))
		{
			if(LoopCount==512)
			{
				m_LockNotify.Lock();
				if(m_CMDBGet==m_CMDBPut || (pJob->IsSPUJob() && (gPS3Env->spuEnabled != -1)))
				{
					m_Waiting = 1;
					READ_WRITE_BARRIER
					m_Condition.Wait(m_LockNotify);
				}
				m_LockNotify.Unlock();
			}
			else
			{
				++LoopCount;
				__db16cycl__
				__db16cycl__
				__db16cycl__
				__db16cycl__
			}
			pJob	=	reinterpret_cast<CDXPSRDJobSPU*>(&m_CMDBuffer[m_CMDBGet]);
		}
		m_Waiting = 0;
		const uint32 jobType		= pJob->FullTypeVolatile();
		const uint32 maskedType = jobType & EDXPSJOB_MASK;
		if((EDXPSJob)maskedType == EDXPSJ_EXIT)
		{
			NextJob<false>((uint32&)m_CMDBGet,pJob->Size());
			break;//enable possible exit of thread
		}
		m_Worker.WorkOn(pJob, (EDXPSJob)maskedType);
		NextJob<false>((uint32&)m_CMDBGet,pJob->Size());
	}
  // Done - has exited 
  m_Finished = 1; 
#endif
}
#endif//CRY_DXPS_DEVICETHREAD
#endif//__SPU__

#if !defined(_RELEASE) || defined(__SPU__)
	uint8 g_LocalPSBuf[LOCAL_PS_BUFFER_SIZE+LOCAL_SHADER_GUARD_BUFFER_SIZE] _ALIGN(128) SPU_LOCAL;//ps buffer, also used for constant copying in SetVertexShader
	uint8 g_LocalVSBuf[LOCAL_VS_BUFFER_SIZE+LOCAL_SHADER_GUARD_BUFFER_SIZE] _ALIGN(128) SPU_LOCAL;//vs buffer
#endif






























































































































































































