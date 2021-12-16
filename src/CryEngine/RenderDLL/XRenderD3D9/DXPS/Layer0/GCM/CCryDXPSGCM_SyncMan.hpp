#ifndef __CRYDXPSGCM_SYNCMAN__
#define __CRYDXPSGCM_SYNCMAN__

#include "CryThread.h"
#include <IJobManSPU.h>//for definition of SUPP_SPU_FRAME_STATS
#include <SPU/gcm_mapping.h>

//this type is used for resources to memorize the
//last Drawcall at which the resource was used, usefull
// for synchronisation via waitlabel
//typedef uint64	tdResHandle;
//typedef uint32	tdResHandle;

extern void cellGcmAddDeviceStallTicks(const unsigned long long, const unsigned long long);
extern void StopSPUs();

#if defined(CRY_DXPS_RESCOUNTERDEBUG)
class tdResHandle
{
	uint64			m_Counter;
public:
							tdResHandle(){}
	explicit		tdResHandle(uint64 C):m_Counter(C){}
							tdResHandle(tdResHandle& C):m_Counter(C.m_Counter){}
							tdResHandle(const tdResHandle& C):m_Counter(C.m_Counter){}
							tdResHandle(volatile tdResHandle& C):m_Counter(C.m_Counter){}
							tdResHandle(const volatile tdResHandle& C):m_Counter(C.m_Counter){}

	tdResHandle	operator=(tdResHandle& C){m_Counter=C.m_Counter;return *this;}
	tdResHandle	operator=(const tdResHandle& C){m_Counter=C.m_Counter;return *this;}
	tdResHandle	operator=(volatile tdResHandle& C){m_Counter=C.m_Counter;return *this;}
	tdResHandle	operator=(const volatile tdResHandle& C){m_Counter=C.m_Counter;return *this;}

	tdResHandle	operator+(uint64 C)const{tdResHandle V;V.m_Counter=m_Counter+C;return V;}
	tdResHandle	operator-(uint64 C)const{tdResHandle V;V.m_Counter=m_Counter-C;return V;}
	tdResHandle	operator+(tdResHandle C)const{tdResHandle V;V.m_Counter=m_Counter+C.m_Counter;return V;}
	tdResHandle	operator-(tdResHandle C)const{tdResHandle V;V.m_Counter=m_Counter-C.m_Counter;return V;}
	tdResHandle	operator&(uint64 C)const{tdResHandle V;V.m_Counter=m_Counter&C;return V;}

	bool				operator==(tdResHandle C)const{return m_Counter==C.m_Counter;}
	bool				operator!=(tdResHandle C)const{return m_Counter!=C.m_Counter;}
	bool				operator>=(tdResHandle C)const{return m_Counter>=C.m_Counter;}
	bool				operator<=(tdResHandle C)const{return m_Counter<=C.m_Counter;}
	bool				operator>(tdResHandle C)const{return m_Counter>C.m_Counter;}
	bool				operator<(tdResHandle C)const{return m_Counter<C.m_Counter;}

	uint64			Counter()const{return m_Counter;}
	void				Counter(uint64 C){m_Counter=C;}
};
#define TDRES_READ(X) (X).Counter()
#define TDRES_WRITE(X,Y) (X).Counter(Y)
#define TDRES_CREATE(X) tdResHandle(X)
#else
typedef uint64			tdResHandle;
#define	TDRES_MAX		(((tdResHandle)0)-1)
#define TDRES_READ(X) (X)
#define TDRES_WRITE(X,Y) ((X)=(Y))
#define TDRES_CREATE(X) tdResHandle(X)
#endif

ILINE tdResHandle StampOffset(tdResHandle Counter)
{
	Counter	=	Counter&((ECDXPSSL_BUFFERCOUNTMASK<<ECDXPSSL_BUFFERBITS)|ECDXPSSL_BUFFERSIZEMASK);
	return TDRES_CREATE(TDRES_READ(Counter)+ECDXPSSLR_RESERVED);
}


struct SRegisteredZWrite
{
	tdResHandle						registeredZWriteCountHandle;
	uint32								registeredZWriteCount;
};

// 2 ms inner wait loop




	#define SYNC_LOOP_INIT const uint64 timeoutTime = rdtsc() + 40*1024/*0.5ms*/;uint32 iterCount = 0;uint32 sleepCount = 0;
	#define SYNC_LOOP {\
		++iterCount;\
		if(rdtsc() > timeoutTime)\
		{\
			++sleepCount;\
			if(IsSPUEnabled())\
				StopSPUs();\
			else\
				Sleep(2);\
			if(sleepCount>500/*~1sec*/){printf("DXPS sync timeout in %s:%d\n",__FUNC__,__LINE__);break;}\
		}\
		else\
			sys_timer_subusleep_inldb16cyc(iterCount);}


enum{EDXPS_FREELIST_COUNT	=	4};
enum{EDXPS_FREELIST_MASK	=	EDXPS_FREELIST_COUNT-1};

class CCryDXPSGCMSyncMan
{
private:
	//both are assumed to lie consecutively within the same cache line
	ppu_volatile tdResHandle				m_CountDeviceThread _ALIGN(128);
	ppu_volatile tdResHandle				m_SwapRSX;
	volatile	tdResHandle						m_CountRenderer;
	CellGcmReportData*							m_pReportArea;
	//to be registered by RegisterZWrite valid for upcoming draw call
	SRegisteredZWrite								m_RegisteredZWrite;
	volatile uint32_t*							m_pRSXVSyncLabel[2];

//#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	uint64													m_RefTime;
	uint32													m_RSXIdleTime;
	mutable uint32									m_RSXStallTime[2];
	uint32													m_RSXStallTimeIndex : 1;
//#endif
	bool														m_RSXProfInUse;
#if defined(CRY_DXPS_SINGLEFLUSHVALIDATE)
	CryMutex												m_Lock;
#endif

#ifndef __SPU__
	std::vector<CCryDXPSResource*>	m_FreeList[EDXPS_FREELIST_COUNT];
#endif
	tdResHandle											BufferID(tdResHandle Counter)
																	{
																		return TDRES_CREATE((TDRES_READ(Counter)>>ECDXPSSL_BUFFERBITS)&ECDXPSSL_BUFFERCOUNTMASK);
																	}
	//TODOMK REMOVE UGLY workaround
public:
	tdResHandle											FrameFlipID(tdResHandle Counter)
																	{
																		return TDRES_CREATE(ECDXPSSLR_VSYNC0+(TDRES_READ(BufferID(Counter))&1));
																	}
	ILINE	tdResHandle								FrameBase(tdResHandle Counter)	const
																	{
																		return TDRES_CREATE(TDRES_READ(Counter)&~ECDXPSSL_BUFFERSIZEMASK);
																	}
public:

	void														Lock()
																	{
#if defined(CRY_DXPS_SINGLEFLUSHVALIDATE)
																		m_Lock.Lock();
#endif
																	}
	void														Unlock()
																	{
#if defined(CRY_DXPS_SINGLEFLUSHVALIDATE)
																		m_Lock.Unlock();
#endif
																	}
	void														Init(void*& rpReport);

	void														ClearFreeList(const uint32 Idx);
	void														AddToFreeList(CCryDXPSResource* pResource);

	ILINE void											RegisterZWrite(const uint32 cZWriteCount, tdResHandle H)
																	{
																		m_RegisteredZWrite.registeredZWriteCount = cZWriteCount;
																		m_RegisteredZWrite.registeredZWriteCountHandle = H;
																	}

	//sync handles
	ILINE	tdResHandle								HandleRenderer()const
																	{
																		return *(tdResHandle*)&m_CountRenderer;//read non volatile
																	}
	ILINE	tdResHandle								HandleDeviceThread()const
																	{
																		return *(tdResHandle*)&m_CountDeviceThread;		
																	}

	ILINE	tdResHandle								FrameBaseRenderer()	const
																	{
																		return FrameBase(HandleRenderer());
																	}

	ILINE	tdResHandle								FrameBaseDeviceThread()	const
																	{
																		return FrameBase(HandleDeviceThread());
																	}

	CellGcmReportData*							HandlePointerDeviceThread()
																	{
																		return &m_pReportArea[TDRES_READ(StampOffset(*(tdResHandle*)&m_CountDeviceThread))];
																	}
	template<bool Wait>
	uint32													ZWriteReport(tdResHandle Handle, bool recordWaitTime = true)
																	{







																		IF(TDRES_READ(*(tdResHandle*)&m_SwapRSX)>=TDRES_READ(Handle)+(ECDXPSSL_BUFFERCOUNT-1)*ECDXPSSL_BUFFERSIZE, 0)//is not overwritten?
																			return TOO_OLD_Z_COUNT;//sadly the drawcall is too old, the count was not preserved
																		//if draw call has not finished, INVALID_Z_COUNT is still set in the report location
																		uint32 zWriteCount;
																		SyncToRSX<Wait,false>(Handle, zWriteCount, recordWaitTime, 200/*timeout*/);
																		return zWriteCount;
																	}

	uint32													GetHandleDeviceThreadEA()const
																	{
																		return (uint32)&m_CountDeviceThread;
																	}

	const SRegisteredZWrite&				RegisterZWrite() const
																	{
																		return m_RegisteredZWrite;
																	}

	//increments
	void														IncRenderer()
																	{
#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
																		int Th=GetCurrentThreadId();
																		if(g_MAINTHREADID!=Th && g_MAINTHREADID!=-1)
																		{
																			snPause();
																		}
#endif
#if defined(CRY_DXPS_SINGLEFLUSHVALIDATE)
																		SyncThreads<false>();
#endif
																		const tdResHandle cCountRenderer = m_CountRenderer;
																		m_pReportArea[TDRES_READ(StampOffset(cCountRenderer))].value = INVALID_Z_COUNT;
																		m_CountRenderer = cCountRenderer + 1;
																		m_RegisteredZWrite.registeredZWriteCount = 0;//reset, has become invalid
																		READ_WRITE_BARRIER
																	}

	SPU_ILINE
	void														IncDeviceThread(bool PrepareFlip=false)
																	{
																		using namespace CRY_DXPS_GCMNAMESPACE;
																		//on spu this is done on very first invocation
#if !defined(__SPU__)
																		IF(TDRES_READ(*(tdResHandle*)&m_SwapRSX)<ECDXPSSL_BUFFERSIZE, 0)//before the first flip is issued always set the report location
																		{
																			cellGcmSetZpassPixelCountEnable(CELL_GCM_TRUE);
																			cellGcmSetReportLocation(CELL_GCM_LOCATION_MAIN);
																		}
#endif
																		if(PrepareFlip)
																		{
																			cellGcmSetWriteCommandLabel(TDRES_READ(FrameFlipID(m_CountDeviceThread)),1);
																			cellGcmSetZpassPixelCountEnable(CELL_GCM_TRUE);
																			cellGcmSetReportLocation(CELL_GCM_LOCATION_MAIN);
																		}

																		tdResHandle *const pCountDeviceThread = (tdResHandle*)&m_CountDeviceThread;
																		const uint64 Idx	=	TDRES_READ(StampOffset(*pCountDeviceThread));
//																		if(!(Idx&ECDXPSSL_BUFFERSIZEMASK))		//needed after frame swap
//																			cellGcmSetReportLocation(CELL_GCM_LOCATION_MAIN);
																		m_pReportArea[Idx].value = INVALID_Z_COUNT;
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
																		cellGcmSetTimeStamp(Idx);
#endif
																		cellGcmSetReport(CELL_GCM_ZPASS_PIXEL_CNT,Idx);

//																		if(!(Idx&ECDXPSSL_BUFFERSIZEMASK))		//just needed after frame swap
//																			cellGcmSetZpassPixelCountEnable(CELL_GCM_TRUE);
																		cellGcmSetClearReport(CELL_GCM_ZPASS_PIXEL_CNT);
																		*pCountDeviceThread	=	TDRES_CREATE(TDRES_READ(*pCountDeviceThread) + 1);
																	}

	tdResHandle											RendererNextFrame()
																	{
																		return TDRES_CREATE((TDRES_READ(m_CountRenderer)+ECDXPSSL_BUFFERSIZE)&~ECDXPSSL_BUFFERSIZEMASK);
																	}
	void														SwapRenderer()
																	{
																		//align buffer for zero based indexing
																		m_CountRenderer=RendererNextFrame();
																		//buffer still in use
																		const uint64 timeoutTime = rdtsc() + CRY_DXPS_SPU_TIME_OUT_ITER;
																		CRY_DXPS_STALL_SCOPE
																		while(TDRES_READ(BufferID(m_CountRenderer))==TDRES_READ(BufferID(m_SwapRSX)))
																		{
																			READ_WRITE_BARRIER
																			if(IsSPUEnabled() && (rdtsc() > timeoutTime)) 
																				StopSPUs();
																			CRY_DXPS_STALL
																		}
																		ClearFreeList((TDRES_READ(m_CountRenderer)>>ECDXPSSL_BUFFERBITS)&EDXPS_FREELIST_MASK);
																	}
	void														SwapDeviceThread()
																	{
																		//allign buffer for zero based indexing
																		m_CountDeviceThread=(m_CountDeviceThread+ECDXPSSL_BUFFERSIZE)&~ECDXPSSL_BUFFERSIZEMASK;
																		//buffer still in use
																		CRY_DXPS_STALL_SCOPE
																		while(BufferID(m_CountDeviceThread)==m_SwapRSX)
																		{
																			READ_WRITE_BARRIER
																			CRY_DXPS_STALL
																		}
																	}
	tdResHandle											GetSwapRSX() const
																	{
																		return m_SwapRSX;
																	}
	void														SwapRSX()
																	{
																		CalcIdleTime();
																		*m_pRSXVSyncLabel[(TDRES_READ(m_SwapRSX)>>ECDXPSSL_BUFFERBITS)&1]	=	0;
//																		char Text[1024];
//																		sprintf(Text,"SwapRSX:%d\n",(uint32)((m_SwapRSX>>ECDXPSSL_BUFFERBITS)&1));
//																		OutputDebugString(Text);
																		//next buffer
																		m_SwapRSX=m_SwapRSX+ECDXPSSL_BUFFERSIZE;
																		READ_WRITE_BARRIER
																	}

	void														SyncRSXToFrame()
																	{
																		using namespace CRY_DXPS_GCMNAMESPACE;
																		cellGcmSetWaitLabel(TDRES_READ(FrameFlipID(m_SwapRSX)),0);
																	}

	template<bool Wait,bool isResourceSync>
  ILINE bool											SyncToRSX(tdResHandle Value, uint32& __restrict rZWriteCount, bool recordWaitTime = true, int timeOut = 500/*ms*/)
																	{












#define cellGcmGetReportDataLocation(cIndex, MAIN) ((uint32)*((volatile uint32*)(&m_pReportArea[TDRES_READ(cIndex)].value)))
																		using namespace CRY_DXPS_GCMNAMESPACE;
																		if(Value==TDRES_MAX)
																			Value=m_CountRenderer-1;
																		const tdResHandle Idx	=	StampOffset(Value);
																		rZWriteCount = cellGcmGetReportDataLocation(Idx,CELL_GCM_LOCATION_MAIN);

																		if(*(tdResHandle*)&m_SwapRSX >= Value || (Value<=m_CountDeviceThread && rZWriteCount!=INVALID_Z_COUNT))//avoid volatile lookup for 1st time
																			return true;

																		if(Wait)
																		{
																			const int64 cWaitStart = CryGetTicks();
																			CRY_DXPS_STALL_SCOPE
																			SYNC_LOOP_INIT
																			do
																			{
					//															READ_WRITE_BARRIER
																				rZWriteCount = cellGcmGetReportDataLocation(Idx,CELL_GCM_LOCATION_MAIN);
																				SYNC_LOOP
																				READ_WRITE_BARRIER
																				CRY_DXPS_STALL
																			}
																			while(rZWriteCount==INVALID_Z_COUNT && m_SwapRSX<Value);
																			const int64 cWaitEnd = CryGetTicks();
																			if(recordWaitTime && (iterCount > 1))
																			{
																				if(isResourceSync)
																					cellGcmAddRSXStallTicks(cWaitStart, cWaitEnd);
																				else
																					cellGcmAddRSXWaitTicks(cWaitStart, cWaitEnd);
																			}
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
																			m_RSXStallTime[m_RSXStallTimeIndex] += cWaitEnd - cWaitStart;
#endif
																			return true;
																		}
																		//can only reach here if Wait==false
																		if(rZWriteCount==INVALID_Z_COUNT)
																		{
																			rZWriteCount=PENDING_Z_COUNT;
																			return false;
																		}
																		return Value<=m_CountDeviceThread;
#undef cellGcmGetReportDataLocation

																	}
#if !defined(__SPU__)
	template<bool Wait>
	ILINE bool											SyncToDeviceThread(tdResHandle Value)	const
																	{
																		if(Value==TDRES_MAX)
																			Value=m_CountRenderer-1;
																		if(Wait)
																		{
																			const int64 cWaitStart = CryGetTicks();
																			CRY_DXPS_STALL_SCOPE
																			SYNC_LOOP_INIT
																			while(m_CountDeviceThread<=Value)
																			{
																				READ_WRITE_BARRIER
																				SYNC_LOOP
																				CRY_DXPS_STALL
																			}
																			if(iterCount > 10)
																				cellGcmAddDeviceStallTicks(cWaitStart, CryGetTicks());
																			const int64 cWaitEnd = CryGetTicks();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
																			m_RSXStallTime[m_RSXStallTimeIndex] += cWaitEnd - cWaitStart;
#endif
																			return true;
																		}
																		return m_CountDeviceThread>Value;
																	}
#endif//__SPU__
#if defined(CRY_DXPS_SINGLEFLUSHVALIDATE)
	template<bool ONEAHEAD>
	void														SyncThreads()
																	{
																		uint32 iterCount	=	0;
																		const uint32 Add	=	ONEAHEAD?1:0;
																		const int64 cWaitStart = CryGetTicks();
																		while(m_CountDeviceThread+Add!=m_CountRenderer)
																		{
																			if(++iterCount > CRY_DXPS_SPU_TIME_OUT_ITER*4)
																			{
																				sys_timer_usleep(iterCount/CRY_DXPS_SPU_TIME_OUT_ITER);
																				READ_WRITE_BARRIER
																				CRY_DXPS_STALL
																			}
																			CRY_DXPS_STALL
																		}
																		uint32 zWriteCount;
																		SyncToRSX<true,false>(m_CountRenderer,zWriteCount);
																	}
#endif
#ifndef __SPU__
	uint32													IdleTime()
																	{
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
																		m_RSXProfInUse	=	true;
																		return m_RSXIdleTime;
#else
																		return 0;
#endif
																	}
	float														StallTime()
																	{
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
																		m_RSXProfInUse	=	true;
																		uint32 index = m_RSXStallTimeIndex;
																		m_RSXStallTimeIndex ^= 1;
																		m_RSXStallTime[m_RSXStallTimeIndex] = 0;
																		static const float cFrequDivider = 1.f /(float)sys_time_get_timebase_frequency();
																		return (float)m_RSXStallTime[index] * cFrequDivider;
#else
																		return 0.f;
#endif
																	}
#endif
	void														CalcIdleTime()
																	{
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
																		if(!m_RSXProfInUse)
																			return;
																		m_RSXProfInUse	=	false;
																		uint64 Delta=0;
																		const uint32 IdxOffset	=	ECDXPSSLR_RESERVED;
																		const uint32	a=(BufferID((uint32)m_SwapRSX)-1)&3;
																		const uint64	RefTime	=	m_RefTime;
																		uint64	CurrentTime0,CurrentTime1;
																		uint64	T0,T1;
																		uint64	LastTime	=	RefTime;
																		for(uint32 b=a*ECDXPSSL_BUFFERSIZE+IdxOffset;b<(a+1)*ECDXPSSL_BUFFERSIZE;b+=2)
																		{
																			CurrentTime0	=	m_pReportArea[b].timer;
																			CurrentTime1	=	m_pReportArea[b+1].timer;
																			if(CurrentTime0<LastTime	||	CurrentTime1<LastTime)
																				break;
																			LastTime		=	CurrentTime0;
																			T0	=	CurrentTime0-RefTime;
																			T1	=	CurrentTime1-RefTime;
																			Delta+=T1-T0;
																			m_RefTime	=	CurrentTime1;
																		}


																		Delta	=	T1-Delta;
																		Delta	=	Delta*100/T1;
																		m_RSXIdleTime	= Delta;
#endif
																	}
	static uint32										MemoryNeeded();
	void														Size(class ICrySizer* Sizer);
} _ALIGN(128);

#endif

