#ifndef __CRYDXPS__
#define __CRYDXPS__



#define CRY_DXPS_TILEDREGIONS
#define	CRY_DXPS_PIXELCACHE
//#define CRY_DXPS_PRECOMPILE_CMDBUFFER
//#define CRY_DXPS_VALIDATEWEAKPTR

//#define CRY_DXPS_NODRAW
//need vsync with SPUs currently (missing SPU side flip immediate

#define CRY_DXPS_DEVICETHREAD
#define CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING
//#define CRY_DXPS_SOFTWARE_TRANSFORMATIONS
//#define CRY_DXPS_RASTERTHREAD
#define CRY_DXPS_CACHESTATES
#define CRY_DXPS_LSCONSTANTS
//#define CRY_DXPS_LSCONSTANTS_PROFILE
#define CRY_DXPS_RESOURCE_OWNERSHIP_LOCK

//timeout 1 sec
#define CRY_DXPS_SPU_TIME_OUT_ITER (80*1024*1024)
#define SPU_FRAME_PROFILE_SECTION(name) 
#define SPU_ILINE inline

//#define CRY_DXPS_PROFILE
//increases a counter in DXPSWorker by one on every drawcall and deletes it on frameflip
//	#define DRAWCALLDEBUGGING
//flushes the job and stalls till it's done
//also validates the renderer and devicecounter in syncman
//#define CRY_DXPS_SINGLEFLUSHVALIDATE
//as we dont have any virtuals in dxps, deleting by parent pointers will not call the correct dtor
//this define checks if it's not done the correct way
//	#define CRY_DXPS_DESTRUCTORVALIDATION
//#define	CRY_DXPS_SINGLETHREAD_OWNERSHIP
//#define	CRY_DXPS_RESCOUNTERDEBUG
//outputs various stats
//	#define CRY_MM_DEBUG
//	#define CRY_MM_DEBUG_ITEMLIST
//checks bounds in vmem -> slow but nice to have a verification
//	#define CRY_MM_DEBUG_BOUNDCHECK
//	#define CRY_MM_DEBUG_DEFRAG
//	#define CRY_MM_PROFILE
//	#define CRY_MM_VALIDATE_DYN_ALLOC
//	#define CRY_DXPS_REPORTSTALLS
//passes random debugging data from main to the dxps-thread, defines size of buffer in uint32
//#define CRY_DXPS_THREAD_DEBUGDATA	4096
#if defined(SNTUNER)
	#define CRY_DXPS_SNMARKERS
#endif


#if !defined(_RELEASE)
//	#define CRY_DXPS_PERFORMANCECOUNTING
#endif

#if defined(CRY_DXPS_SNMARKERS)
	#include <lib/libsntuner.h>
	enum
	{
		SNTM_FLUSHING			=	0x08,
		SNTM_FLIPREQUEST	=	0x09,
		SNTM_FLIPPING			=	0x0A,
	};
	#define SNSTARTMARKER			snStartMarker
	#define SNSTOPMARKER			snStopMarker
#else
	#define SNSTARTMARKER(X,Y)
	#define SNSTOPMARKER(X)
#endif
 

//enum {EReportSize		=	1024*1024};
//enum {EZBufferSize	=	512*512*4};
#define EReportSize		(1024*1024)
#define EZBufferSize	(512*512*4)


#if defined(CRY_DXPS_REPORTSTALLS)
	class CCryStallCounter
	{
		uint32			m_Counter;
		uint32			m_Line;
		const char* m_pStr;
	public:
								CCryStallCounter(const char* pStr,uint32 Line):
								m_Counter(0),
								m_Line(Line),
								m_pStr(pStr){}
								~CCryStallCounter()
								{
									if(m_Counter>1)
									{
										char Text[1024];
										sprintf(Text,m_pStr,m_Line,m_Counter);
										OutputDebugString(Text);
									}
								}
		void				operator++(){m_Counter++;}
	};
	#define CRY_DXPS_STALL_SCOPE CCryStallCounter StallCounter("Stall in: "__FILE__" %d %d\n",__LINE__);
	#define CRY_DXPS_STALL ++StallCounter;
#else
	#define CRY_DXPS_STALL_SCOPE
	#define CRY_DXPS_STALL 
#endif





	#define CELL_GCM_FLUSH tdLayer0::CMB().FlushCMDs()
	


#define CCRY_DXPS_UNSAFE

#if defined(CRY_DXPS_SINGLEFLUSHVALIDATE)
	#define CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_START(X) 	rSyncMan.Lock();\
																															rSyncMan.SyncThreads<X>();

	#define CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(X)	 	if(!X)\
																																rSyncMan.Unlock();\
																															rSyncMan.SyncThreads<false>();\
																															if(X)\
																																rSyncMan.Unlock();
#else
	#define CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_START(X)
	#define CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(X)
#endif

#if defined(CCRY_DXPS_UNSAFE)
	#define CRY_DXPS_GCMNAMESPACE	cell::Gcm::UnsafeInline
#else
	#define CRY_DXPS_GCMNAMESPACE	cell::Gcm
#endif

#if defined(CRY_DXPS_PROFILE)
	#define DXPS_PROFILE_FRAME	PROFILE_FRAME
#else
	#define DXPS_PROFILE_FRAME(X)
#endif


#if defined(CRY_DXPS_VSYNC)
	#define DXPS_FLIP(X,Y)	cellGcmSetFlip(X,Y)
#else
	#define DXPS_FLIP(X,Y)	cellGcmSetFlipImmediate(cellGcmSetPrepareFlip(X,Y))
#endif

#if defined(CRY_MM_DEBUG_ITEMLIST)
	#define CRY_MM_VALIDATE Validate()
	#define CRY_MM_VALIDATE_ITEM(it) it->Validate()
#else
	#define CRY_MM_VALIDATE 
	#define CRY_MM_VALIDATE_ITEM(it)
#endif


#include "../CCryTypes.hpp"


#ifdef 	WIN32
	#include "CCryDXPS_PC.hpp"
#else
	#include "CCryDXPS_PS3.hpp"
#endif

#if CRY_USE_GCM
	#define CRY_DXPS_CREATE(T,X) tdLayer0::Create ## T X
	#define CRY_DXPS_NEW(T) tdLayer0::New<T>()
	#define CRY_DXPS_NEWARRAY(T,S) tdLayer0::New<T>(S)

	#define CRY_DXPS_RELEASE(P)	tdLayer0::Release(P)
	#define CRY_DXPS_DELETE(X) tdLayer0::Delete(X)
	#define CRY_DXPS_DELETEARRAY(X) tdLayer0::DeleteArray(X)

	#include "CCryDXPSGCM.hpp"
#else
	#define CRY_DXPS_CREATE(T,X) new T X
	#define CRY_DXPS_NEW(T) new T
	#define CRY_DXPS_NEWARRAY(T,S) new T[S]

	#define CRY_DXPS_RELEASE(P)	P->Release()
	#define CRY_DXPS_DELETE(X) delete (X)
	#define CRY_DXPS_DELETEARRAY(X) delete[] (X)
#endif

#ifndef CRY_DXPS_CREATE
	#define CRY_DXPS_CREATE(T,X) new T X
	#define CRY_DXPS_NEW(T) new T
	#define CRY_DXPS_NEWARRAY(T,S) new T[S]

	#define CRY_DXPS_RELEASE(P)	delete P->Release()
	#define CRY_DXPS_DELETE(X) delete (X)
	#define CRY_DXPS_DELETEARRAY(X) delete[] (X)
#endif

#include "../CCryDXPSAPtr.hpp"

#if !defined(__SPU__)
//	#include <GcmReplay.h>
#endif
#endif


