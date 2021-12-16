#ifndef __CRYDXPSBUFFER__
#define __CRYDXPSBUFFER__

#include "../CCryDXPSResource.hpp"
#include "../../../Layer0/CCryDXPS.hpp"
#include <CrySizer.h>

#if defined(CRY_DXPS_LSCONSTANTS)
	#define SPU_CB_BUFFER_DOMAIN SPU_DOMAIN_LOCAL
	#define SPU_CB_BUFFER_PTR SPU_LOCAL_PTR
#else
	#define SPU_CB_BUFFER_DOMAIN SPU_DOMAIN_MAIN
	#define SPU_CB_BUFFER_PTR SPU_MAIN_PTR
#endif

class CCryDXPSBuffer;
class CCryDXPSBuffer		:		public CCryDXPSResource	,	public CCryRefAndWeak<CCryDXPSBuffer>
{
	friend class						CDXPSRDWorker;
													CCryDXPSBuffer(MMRES_PARAM_ALONE):
													CCryDXPSResource(EDXPS_RT_NONE MMRES_PASS),
													m_pBuffer(0){}

	tdResHandle							m_TmpLock;
	uint8*									m_pBuffer;
	uint32									m_Size;
	uint16									m_MemItemID;
	uint16									m_BufferCount;
	uint16									m_CurrentBuffer;

	ILINE bool							MultiBuffer()const
													{
														return m_BufferCount>1;
													}


	ILINE void										NextBuffer()
	{
		if(!MultiBuffer())
			return;

		if(m_ResDCLock==TDRES_MAX)
			m_ResDCLock=tdLayer0::Sync().HandleRenderer()-1;

		if(m_BufferCount==2)
		{
			tdResHandle S		=	m_TmpLock;
			m_TmpLock				=	m_ResDCLock;
			m_ResDCLock			=	S;
			m_CurrentBuffer	=	(m_CurrentBuffer+1)%m_BufferCount;
		}
		else
		{
			uint8*	pData	=	RawData(m_BufferCount);//get ptr to beyond real data

			reinterpret_cast<tdResHandle*>(pData)[m_CurrentBuffer]=m_ResDCLock;
			m_CurrentBuffer	=	(m_CurrentBuffer+1)%m_BufferCount;
			m_ResDCLock=reinterpret_cast<tdResHandle*>(pData)[m_CurrentBuffer];
		}
	}
	ILINE uint8*									Buffer(uint32 Idx)
	{
		return &m_pBuffer[Idx*m_Size];
	}
	ILINE const uint8*						Buffer(uint32 Idx)const
	{
		return &m_pBuffer[Idx*m_Size];
	}

public:
		CCryDXPSBuffer(const void* pData,uint32 Size,ECRYDXPSResType T,ECRYDXPSResFreq	Freq MMRES_PARAM);
		CCryDXPSBuffer(uint32 Size MMRES_PARAM);
		ILINE ~CCryDXPSBuffer()
		{
//			ReleaseResources();
			MMRES_SUBMM(sizeof(CCryDXPSBuffer));
			MMRES_SUBCOUNT();
			if(Type()==EDXPS_RT_CONSTBUFFERTEMP)
			{
				snPause();
			}
		}
	ILINE void	ReleaseResources()
	{
		if(Type()==EDXPS_RT_CONSTBUFFERTEMP)
		{
			snPause();
		}
		if(m_pBuffer)
		{
			if(m_MemItemID)//Type()==EDXPS_RT_INDEXBUFFER || Type()==EDXPS_RT_VERTEXBUFFER || Type()==EDXPS_RT_PIXELSHADERCACHE)
			{
				tdLayer0::Memory().Free(m_MemItemID);
				m_MemItemID	=	0;
				MMRES_SUBVM(m_Size*m_CurrentBuffer);
			}
			else
			{
				MMRES_SUBMM(m_Size);
				CRY_DXPS_DELETEARRAY(m_pBuffer);
			}
			m_pBuffer=0;
		}
		else
		{
			snPause();
		}
	}
	long	Map(D3D11_MAP MapType,uint32 MapFlags,void** ppData);
	bool	SyncNoWaitAll();
	ILINE static uint32 EstimateSize(uint32 Size)
	{
		return sizeof(CCryDXPSBuffer)+Size;	//no need for alignment padding
//		return (sizeof(CCryDXPSBuffer)+16)+Size;
	}
	ILINE static uint8* DataPtr(void* pThis)
	{
		if(reinterpret_cast<size_t>(pThis)&15)
		{
			snPause();
		}
		CCryDXPSBuffer* pNext	=	reinterpret_cast<CCryDXPSBuffer*>(pThis)+1;
		if(reinterpret_cast<size_t>(pNext)&15)
		{
			snPause();
		}
		return reinterpret_cast<uint8*>(pNext);
//		uint8* pAligned	=	reinterpret_cast<uint8*>((reinterpret_cast<size_t>(pNext)+15)&~15);
//		return pAligned;
	}
	ILINE static CCryDXPSBuffer* ClassPtr(void* pData)
	{
		CCryDXPSBuffer* pThis	=	reinterpret_cast<CCryDXPSBuffer*>(pData)-1;
		return pThis;
	}

	ILINE long	Unmap(){return 0;}
	ILINE long GetDesc(D3D11_BUFFER_DESC *pDesc)
	{
    memset(pDesc, 0, sizeof(*pDesc));
    pDesc->ByteWidth = m_Size;
		//CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
		return -1;
	}
	ILINE uint32	Size()const{return m_Size;}
//	void*	CBuffer(){return Type()==EDXPS_RT_CONSTBUFFER?m_pBuffer:0;}
	ILINE void	RawData(uint8* pBuffer){ m_pBuffer = pBuffer;}
	ILINE uint8*	RawData(uint32 Idx){return Buffer(Idx);}
	ILINE uint8*	RawData(){return RawData(m_CurrentBuffer);}
	ILINE const uint8*	RawData(uint32 Idx)const{return Buffer(Idx);}
	ILINE const uint8*	RawData()const{return RawData(m_CurrentBuffer);}
	ILINE unsigned long		AddRef(){return IncRef();}
	ILINE unsigned long		Release(){return DecRef();}
	ILINE uint32	Index()const{return m_CurrentBuffer;}

	ILINE void	Size(uint32 nSize){ m_Size = nSize;}
	ILINE void	MemItemID(uint16 nID){ m_MemItemID = nID;}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{		
		pSizer->AddObject(this, sizeof(*this));	
		if( !(Type()==EDXPS_RT_INDEXBUFFER || Type()==EDXPS_RT_VERTEXBUFFER || Type()==EDXPS_RT_PIXELSHADERCACHE) )
		{
			pSizer->AddObject(m_pBuffer, m_BufferCount ? m_Size *m_BufferCount : m_Size );
		}
	}
} _ALIGN(16);

typedef CCryDXPSBuffer ID3D11Buffer;
#ifdef CRY_DXPS_VALIDATEWEAKPTR
	typedef CCryAPtrWeakCnt<CCryDXPSBuffer>	APWeakBuffer;
#else
	typedef CCryDXPSBuffer* __restrict	APWeakBuffer;
#endif

#endif

