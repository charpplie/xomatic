#ifndef __CRYDXPSSHADER__
#define __CRYDXPSSHADER__

#include "../CCryDXPSResource.hpp"
#include "../../../CCryDXPSAPtr.hpp"
#include "../../../Layer0/CCryDXPS.hpp"
#include "../Buffer/CCryDXPSBuffer.hpp"
#include "CCryDXPSShaderDesc.hpp"
#include <CrySizer.h>

enum	ECRYDXPSShaderType
{
	EDXPS_ST_GEOMETRY,
	EDXPS_ST_PIXEL,
	EDXPS_ST_VERTEX,
};

#if defined(DXPS_LZSS_COMPRESS)
//	#define COMP_DECODE_SPU
#endif

class CDXPSShaderDesc;
class CCryDXPSBuffer;
class CCryDXPSGCMPixelshaderCacheMan;

struct CCryDXPSShaderCBCache
{
	uint64								m_CBLastUsed;
	uint64								pad;
//	const CCryDXPSBuffer*	m_pCBuffer;
//	uint32								pad;
} _ALIGN(16);

class CCryDXPShaderCache
{
	qword											m_VSConsts[512];
	
	uint32										m_CBDirty;

	uint32										m_LastVShader;
	uint32										m_LastPShader;
public:
														CCryDXPShaderCache()
														{Reset();}

	ILINE qword*							VSConsts(){return m_VSConsts;}
	ILINE void								InvalidatePCB(){m_CBDirty	|=	0xffff0000;}
	ILINE void								InvalidateVCB(){m_CBDirty	|=	0xffff;}

	ILINE uint32							LastVShader()const{return m_LastVShader;}
	ILINE uint32							LastPShader()const{return m_LastPShader;}
	ILINE void								LastVShader(uint32	LShader){m_LastVShader=LShader;}
	ILINE void								LastPShader(uint32	LShader){m_LastPShader=LShader;}


	ILINE uint32							VSCBDirty(){uint32 Ret=m_CBDirty;m_CBDirty&=0xffff0000;	return Ret&0xffff;}
	ILINE uint32							PSCBDirty(){uint32 Ret=m_CBDirty;m_CBDirty&=0xffff;			return Ret>>16;}
	ILINE void								CBDirty(uint32 Slot){m_CBDirty|=1<<Slot;}

	ILINE void								Reset()
														{
															memset(this,~0,sizeof(CCryDXPShaderCache));
														}

} _ALIGN(128);


class CCryDXPSCBData
{
	const uint8*	m_pData;
	uint32				m_Size;
public:
	void					Init(const uint8* pData,uint32 Size)
								{
									m_pData	=	pData;
									m_Size	=	Size;
								}

	const uint8*	Data()const{return m_pData;}
	uint32				Size()const{return m_Size;}

};

class CCryDXPSShader;
class CCryDXPSShader	:	public CCryDXPSResource	,	public	CCryRefAndWeak<CCryDXPSShader>
{
	mutable CCryDXPSShaderCBCache		m_CBCache[16];
#if defined(DXPS_LZSS_COMPRESS)
	uint8*													m_pProgramLZSS;
	uint32													m_ProgramLZSSSize;//128 byte aligned size of compressed shader
#else
	CDXPSShaderDesc*								m_pDesc;
	uint32													m_ProgramSize;
#endif
	uint32													m_uCodeOff;
	uint32													m_uCodeSize;
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	uint32													m_BufferSize;
#endif
#if defined(CRY_DXPS_PRECOMPILE_CMDBUFFER)
	uint32													m_CmdBufferOffset;
#endif

public:





	uint32													m_Crc32;

																	CCryDXPSShader(MMRES_PARAM_ALONE) : CCryDXPSResource(EDXPS_RT_NONE MMRES_PASS){}//dummy
																	CCryDXPSShader(const void* pProgram,ECRYDXPSShaderType SType MMRES_PARAM);

																	~CCryDXPSShader()
																	{
																		MMRES_SUBCOUNT();
																		MMRES_SUBMM(sizeof(CCryDXPSShader));
#if defined(CRY_DXPS_DESTRUCTORVALIDATION)
																		if(EDXPS_RT_SHADER==CCryDXPSResource::Type())
																			CCryDXPSResource::Type(EDXPS_RT_NONE);
#endif
																	}
	ILINE const CCryDXPSShaderCBCache* CBCache() const{return m_CBCache;}
	ILINE uint32										CBCacheSize(){return sizeof(m_CBCache);}
#if defined(DXPS_LZSS_COMPRESS)
	ILINE uint8*										ProgramLZSS()			const{return m_pProgramLZSS;}
	ILINE uint32										ProgramLZSSSize() const{return m_ProgramLZSSSize;}
#else
	ILINE uint8*										ProgramLZSS()			const{return reinterpret_cast<uint8*>(m_pDesc);}
	ILINE uint32										ProgramLZSSSize() const{return m_ProgramSize;}
#endif

	void 														SetPixelshader(SPU_DOMAIN_LOCAL const CDXPSShaderDesc* pDesc,const CCryDXPSCBData*  const __restrict pConstBuffer,CCryDXPShaderCache&	__restrict rCache,tdResHandle DrawCall,CCryDXPSGCMPixelshaderCacheMan& __restrict pPixelCacheMan)const;
	void 														SetVertexshader(SPU_DOMAIN_LOCAL const CDXPSShaderDesc* pDesc,const CCryDXPSCBData* const __restrict pConstBuffer,CCryDXPShaderCache&	__restrict rCache)const;


	void														ReleaseResources();
	inline	int											Release(){return DecRef();}
	uint32													CRC32()const{return m_Crc32;}


//	ECRYDXPSShaderType							ShaderType()const;
	ILINE const CDXPSShaderDesc*		Desc(SPU_DOMAIN_LOCAL uint8* __restrict pProgram, SPU_DOMAIN_LOCAL const uint8* const __restrict cpSource)const
																	{
#if defined(DXPS_LZSS_COMPRESS)






























																		CDXPSShaderDesc::Decode(pProgram,cpSource);

																		return reinterpret_cast<CDXPSShaderDesc*>(pProgram);
#else
																		return reinterpret_cast<const CDXPSShaderDesc*>(cpSource);
//																		return m_pDesc;
#endif
																	}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));		
		pSizer->AddObject(m_pDesc,m_ProgramSize);
	}
} _ALIGN(16);

/*
class CCryDXPSGeometryShader
{
public:
	unsigned long		AddRef(){};
	unsigned long		Release(){};
};

class CCryDXPSPixelShader
{
public:
	unsigned long		AddRef(){};
	unsigned long		Release(){};
};

class CCryDXPSVertexShader
{
public:
	unsigned long		AddRef(){};
	unsigned long		Release(){};
};*/

typedef CCryDXPSShader ID3D11GeometryShader;
typedef CCryDXPSShader ID3D11PixelShader;
typedef CCryDXPSShader ID3D11VertexShader;
typedef CCryDXPSShader ID3D11HullShader;
typedef CCryDXPSShader ID3D11DomainShader;
typedef CCryDXPSShader ID3D11ComputeShader;

#ifdef CRY_DXPS_VALIDATEWEAKPTR
	typedef CCryAPtrWeakCnt<CCryDXPSShader>	APWeakShader;
#else
	typedef CCryDXPSShader*	__restrict APWeakShader;
#endif

#endif

