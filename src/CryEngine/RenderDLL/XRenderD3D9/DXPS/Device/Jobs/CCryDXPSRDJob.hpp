#ifndef __CRYDXPSRDJOB__
#define __CRYDXPSRDJOB__

#if defined(CRY_DXPS_LSCONSTANTS)
enum{	DXPS_DEVICECMDBSIZE				=	1<<22};//in bytes, default 256KB,1<<19
enum{	DXPS_DEVICECMDBPREFETCH		=	1<<18};//need to be bigger due to embeding of constbuffers
#elif defined(CRY_DXPS_DEVICETHREAD)
enum{	DXPS_DEVICECMDBSIZE				=	1<<18};//in bytes, default 256KB,1<<19
enum{	DXPS_DEVICECMDBPREFETCH		=	1<<12};
#else
enum{	DXPS_DEVICECMDBSIZE				=	1<<12};//in bytes, default 4KB,1<<12
enum{	DXPS_DEVICECMDBPREFETCH		=	1<<12};
#endif
enum{	DXPS_DEVICECMDBMASK				=DXPS_DEVICECMDBSIZE-1};
//used during compiletime, forced to be non-enum
#define	DXPS_DEVICECMDBALLIGN			16
enum{	DXPS_DEVICECMDBALLIGNMASK	=	DXPS_DEVICECMDBALLIGN-1};

enum EDXPSJob
{
	EDXPSJ_SWAP = 0,
	EDXPSJ_DRAWINDEXED,
	EDXPSJ_COPYRESOURCESCALED,
	EDXPSJ_COPYRESOURCE,
	EDXPSJ_COPY,
	EDXPSJ_EXIT,								//must be flagged as flushing to execute it on SPU
	EDXPSJ_RENDERTARGET1X,
	EDXPSJ_RENDERTARGET2X,
	EDXPSJ_RENDERTARGET4X,
	EDXPSJ_RENDERTARGET1XATC,		//AlphaToCoverage	//10
	EDXPSJ_RENDERTARGET2XATC,		//AlphaToCoverage	
	EDXPSJ_RENDERTARGET4XATC,		//AlphaToCoverage
	EDXPSJ_VIEWPORT,
	EDXPSJ_CLEARCOLOR,
	EDXPSJ_CLEARTEXTURE,
	EDXPSJ_CLEARDEPTHSTENCIL,
	EDXPSJ_POINTSPRITE,
	EDXPSJ_UPDATECMDBUF,
	EDXPSJ_STATE_VCONSTBUFFER0,
	EDXPSJ_STATE_VCONSTBUFFER1,										//20
	EDXPSJ_STATE_VCONSTBUFFER2,
	EDXPSJ_STATE_VCONSTBUFFER3,
	EDXPSJ_STATE_VCONSTBUFFER4,
	EDXPSJ_STATE_VCONSTBUFFER5,
	EDXPSJ_STATE_VCONSTBUFFER6,
	EDXPSJ_STATE_VCONSTBUFFER7,
	EDXPSJ_STATE_VCONSTBUFFER8,
	EDXPSJ_STATE_VCONSTBUFFER9,
	EDXPSJ_STATE_VCONSTBUFFER10,
	EDXPSJ_STATE_VCONSTBUFFER11,									//30
	EDXPSJ_STATE_VCONSTBUFFER12,
	EDXPSJ_STATE_VCONSTBUFFER13,
	EDXPSJ_STATE_VCONSTBUFFER14,
	EDXPSJ_STATE_VCONSTBUFFER15,
	EDXPSJ_STATE_PCONSTBUFFER0,
	EDXPSJ_STATE_PCONSTBUFFER1,
	EDXPSJ_STATE_PCONSTBUFFER2,
	EDXPSJ_STATE_PCONSTBUFFER3,
	EDXPSJ_STATE_PCONSTBUFFER4,
	EDXPSJ_STATE_PCONSTBUFFER5,										//40
	EDXPSJ_STATE_PCONSTBUFFER6,
	EDXPSJ_STATE_PCONSTBUFFER7,
	EDXPSJ_STATE_PCONSTBUFFER8,
	EDXPSJ_STATE_PCONSTBUFFER9,
	EDXPSJ_STATE_PCONSTBUFFER10,
	EDXPSJ_STATE_PCONSTBUFFER11,
	EDXPSJ_STATE_PCONSTBUFFER12,
	EDXPSJ_STATE_PCONSTBUFFER13,
	EDXPSJ_STATE_PCONSTBUFFER14,
	EDXPSJ_STATE_PCONSTBUFFER15,									//50
	EDXPSJ_STATE_TEXTURE0,
	EDXPSJ_STATE_TEXTURE1,
	EDXPSJ_STATE_TEXTURE2,
	EDXPSJ_STATE_TEXTURE3,
	EDXPSJ_STATE_TEXTURE4,
	EDXPSJ_STATE_TEXTURE5,
	EDXPSJ_STATE_TEXTURE6,
	EDXPSJ_STATE_TEXTURE7,
	EDXPSJ_STATE_TEXTURE8,
	EDXPSJ_STATE_TEXTURE9,												//60
	EDXPSJ_STATE_TEXTURE10,
	EDXPSJ_STATE_TEXTURE11,
	EDXPSJ_STATE_TEXTURE12,
	EDXPSJ_STATE_TEXTURE13,
	EDXPSJ_STATE_TEXTURE14,
	EDXPSJ_STATE_TEXTURE15,
	EDXPSJ_STATE_SAMPLER0,
	EDXPSJ_STATE_SAMPLER1,
	EDXPSJ_STATE_SAMPLER2,
	EDXPSJ_STATE_SAMPLER3,												//70
	EDXPSJ_STATE_SAMPLER4,
	EDXPSJ_STATE_SAMPLER5,
	EDXPSJ_STATE_SAMPLER6,
	EDXPSJ_STATE_SAMPLER7,
	EDXPSJ_STATE_SAMPLER8,
	EDXPSJ_STATE_SAMPLER9,
	EDXPSJ_STATE_SAMPLER10,
	EDXPSJ_STATE_SAMPLER11,
	EDXPSJ_STATE_SAMPLER12,
	EDXPSJ_STATE_SAMPLER13,												//80
	EDXPSJ_STATE_SAMPLER14,
	EDXPSJ_STATE_SAMPLER15,
	EDXPSJ_STATE_VERTEXSHADER,
	EDXPSJ_STATE_PIXELSHADER,
	EDXPSJ_STATE_VERTEXBUFFER0,
	EDXPSJ_STATE_VERTEXBUFFER1,
	EDXPSJ_STATE_VERTEXBUFFER2,
	EDXPSJ_STATE_VERTEXBUFFER3,
	EDXPSJ_STATE_VERTEXBUFFER4,
	EDXPSJ_STATE_VERTEXBUFFER5,										//90
	EDXPSJ_STATE_VERTEXBUFFER6,
	EDXPSJ_STATE_VERTEXBUFFER7,
	EDXPSJ_STATE_VERTEXBUFFER8,
	EDXPSJ_STATE_VERTEXBUFFER9,
	EDXPSJ_STATE_VERTEXBUFFER10,
	EDXPSJ_STATE_VERTEXBUFFER11,
	EDXPSJ_STATE_VERTEXBUFFER12,
	EDXPSJ_STATE_VERTEXBUFFER13,
	EDXPSJ_STATE_VERTEXBUFFER14,
	EDXPSJ_STATE_VERTEXBUFFER15,									//100
	EDXPSJ_STATE_INPUTLAYOUT,
	EDXPSJ_STATE_BLENDFACTOR,
	EDXPSJ_STATE_BLENDSTATE,
	EDXPSJ_STATE_DEPTHSTENCILSTATE,
	EDXPSJ_STATE_RASTERIZERSTATE,
	EDXPSJ_STATE_SCISSOR,
	EDXPSJ_STATE_DEPTHBOUNDS,
	EDXPSJ_STATE_TOPOLOGY,
	EDXPSJ_STATE_PASS,
	EDXPSJ_STATE_DEBUGPROFILIE,
	EDXPSJ_STATE_DEBUGDATA
};

#if defined(__CRYCG__)
	//work around warnings and enum casts
	#define EDXPSJob unsigned int 
#endif

#define JOBRETURN(X)	\
uint8* pThis	=	reinterpret_cast<uint8*>(this);\
pThis-=DXPS_DEVICECMDBALLIGN;\
new (pThis)CDXPSRDJobPPU(sizeof(*this),X);\
return sizeof(*this);


//to be able to switch between SPU and PPU, add bit to job type (job must not be executed by both)
#define EDXPSJT_SPU ((uint32)0x80)
#define EDXPSJOB_MASK (~EDXPSJT_SPU)

ILINE const bool IsFlushingMaskJobType(const uint32 cMaskedType)
{
	return cMaskedType <= (uint32)EDXPSJ_EXIT;
}

ILINE const bool IsFlushingJobType(const uint32 cType)
{
	return IsFlushingMaskJobType(cType & EDXPSJOB_MASK);
}

struct SSizeTypePacked
{
	uint32					m_Size:24;
	uint32					m_Type:8;
};

struct SSizeType
{
	union
	{
		SSizeTypePacked sizeType;
		uint32 val;								//to access and assign at once (volatile handling)
	} u;
};

class CDXPSRDJob
{
protected:
	volatile SSizeType						m_SizeType;	//volatile handled for query type and when setting it

																CDXPSRDJob(const uint32 S,const uint32 T)
																{
																	//assign with only one volatile storage
																	SSizeType sizeType;
																	sizeType.u.sizeType.m_Size = (S + DXPS_DEVICECMDBALLIGNMASK) & ~DXPS_DEVICECMDBALLIGNMASK;
																	sizeType.u.sizeType.m_Type = IsSPUEnabled()?((uint32)EDXPSJT_SPU | T) : T;
																	//no need to do volatile
																	((SSizeType*)&m_SizeType)->u.val = sizeType.u.val;
																}
public:

	const uint32									Size()const
																{
																	const SSizeType*const __restrict pSizeType = (SSizeType*)&m_SizeType;
																	return pSizeType->u.sizeType.m_Size;
																}
	const EDXPSJob								Type()const
																{
																	return static_cast<EDXPSJob>(m_SizeType.u.sizeType.m_Type & EDXPSJOB_MASK);
																}
	const uint32									FullType()const
																{
																	const SSizeType*const __restrict pSizeType = (SSizeType*)&m_SizeType;
																	return pSizeType->u.sizeType.m_Type;
																}
	const uint32									FullTypeVolatile()const
																{
																	return m_SizeType.u.sizeType.m_Type;
																}

	template<class T>
	const T&											Job()	const
																{
																	return *reinterpret_cast<const T*>(reinterpret_cast<const uint8*>(this)+DXPS_DEVICECMDBALLIGN);
																}
};


class CDXPSRDJobSPU	:	public CDXPSRDJob
{
public:
	const bool										IsSPUJob() const{return (uint32)FullTypeVolatile() >= EDXPSJT_SPU;}
};

#if !defined(__SPU__)
extern PS3SystemEnvironment gPS3Env;

class CDXPSRDJobPPU	:	public CDXPSRDJob
{
public:
																CDXPSRDJobPPU(const uint32 S,EDXPSJob Job):
																CDXPSRDJob(S,(uint32)Job)
																{
																}
};

#endif





#include "CCryDXPSRDJob.hpp"
#include "CCryDXPSRDJClearColor.hpp"
#include "CCryDXPSRDJClearDepthStencil.hpp"
#include "CCryDXPSRDJCopy.hpp"
#include "CCryDXPSRDJCopyResource.hpp"
#include "CCryDXPSRDJCopyResourceScaled.hpp"
#include "CCryDXPSRDJDrawIndexed.hpp"
#include "CCryDXPSRDJDummy.hpp"
#include "CCryDXPSRDJPointSprite.hpp"
#include "CCryDXPSRDJSwap.hpp"
#include "CCryDXPSRDJUpdateRenderTargets.hpp"
#include "CCryDXPSRDJUpdateState.hpp"
#include "CCryDXPSRDJViewPort.hpp"



#endif

