#include "StdAfx.h"
#include "../../../Layer0/CCryDXPS.hpp"
#include "../../../CCryDXPSMisc.hpp"
#include "CCryDXPSBuffer.hpp"
//#include <assert.h>

CCryDXPSBuffer::CCryDXPSBuffer(const void* pData,uint32 Size,ECRYDXPSResType T,ECRYDXPSResFreq	Freq MMRES_PARAM):
CCryDXPSResource(T MMRES_PASS),
m_Size(Size),
m_BufferCount(0),
m_CurrentBuffer(0),
m_TmpLock(0),
m_MemItemID(0)
{
	MMRES_ADDCOUNT();
	MMRES_ADDMM(sizeof(CCryDXPSBuffer));
#if defined(CRY_DXPS_SOFTWARE_TRANSFORMATIONS)
//	if((Type()==EDXPS_RT_INDEXBUFFER || Type()==EDXPS_RT_VERTEXBUFFER) && Freq!=EDXPS_RF_DYNAMIC)
	if((Type()==EDXPS_RT_INDEXBUFFER))// && Freq!=EDXPS_RF_DYNAMIC)
	{
		uint32 VSize=(Size+3)&~3;
		m_Size	=	VSize;
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
		if(Freq==EDXPS_RF_DYNAMIC)
		{
			m_BufferCount=2;
			VSize*=2;
		}
#endif		
		MMRES_ADDMM(VSize);
		m_pBuffer		=	(uint8*)CRY_DXPS_NEWARRAY(vec_uint4,(VSize+sizeof(vec_uint4)-1)/sizeof(vec_uint4));//allocate 16 byte aligned (SPU access)
	}
	else
#endif
	if(Type()==EDXPS_RT_INDEXBUFFER || Type()==EDXPS_RT_VERTEXBUFFER || Type()==EDXPS_RT_PIXELSHADERCACHE)
	{
		uint32 VSize=(Size+3)&~3;
		m_Size	=	VSize;
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
		if(Freq==EDXPS_RF_DYNAMIC)
		{
			m_BufferCount=2;
			VSize*=2;
		}
#endif
		MMRES_ADDVM(VSize);
		m_MemItemID	=	tdLayer0::Memory().Allocate(VSize);
		m_pBuffer		=	reinterpret_cast<uint8*>(tdLayer0::Memory().ResolveHandle(m_MemItemID));
		if(Type()==EDXPS_RT_PIXELSHADERCACHE)
			tdLayer0::Memory().Lock(m_MemItemID);
	}
	else
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	if(!CRenderer::CV_r_PS3ConstBufferAdjustment && Type()==EDXPS_RT_CONSTBUFFER && Freq==EDXPS_RF_DYNAMIC)
	{
/*				uint32 Page=4*1024*1024/(Size+sizeof(tdResHandle));
		if(Page>1024)
			Page	=	1024;
		m_BufferCount=Page;*/
		m_BufferCount=32;
		const uint32 VSize=m_BufferCount*(Size+sizeof(tdResHandle));

		MMRES_ADDMM(VSize);
		m_pBuffer		=	(uint8*)CRY_DXPS_NEWARRAY(vec_uint4,VSize/sizeof(vec_uint4));//allocate 16 byte aligned (SPU access)
		memset(m_pBuffer,0,VSize);
	}
	else
#endif
	{
		MMRES_ADDMM(Size);
		m_pBuffer		=	(uint8*)CRY_DXPS_NEWARRAY(vec_uint4,Size/sizeof(vec_uint4));//allocate 16 byte aligned (SPU access)
	}
	if(pData)
		memcpy(RawData(),pData,Size);
}

long CCryDXPSBuffer::Map(D3D11_MAP MapType,uint32 MapFlags,void** ppData)
{
	using namespace CRY_DXPS_GCMNAMESPACE;

//	if(MapType!=D3D11_MAP_WRITE_NO_OVERWRITE && MapType!=D3D11_MAP_WRITE_DISCARD && MapType!=D3D11_MAP_VOID)
//		snPause();

	IncModCount();
	if(MapType!=D3D11_MAP_WRITE_NO_OVERWRITE && Type()!=EDXPS_RT_CONSTBUFFERTEMP)
	{
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
		tdResHandle				TmpResDCLock=m_ResDCLock;
		NextBuffer();
		if(Type()==EDXPS_RT_CONSTBUFFER)
		{
				if(SyncCPU() && m_BufferCount<CRenderer::CV_r_PS3ConstBufferAdjustment)
				{
					MMRES_SUBMM(m_Size*m_CurrentBuffer);
					tdLayer0::Device()->ThreadFinish();
					const uint32 VSizeOld=m_BufferCount==0?m_Size:(m_BufferCount*(m_Size+sizeof(tdResHandle)));
					if(m_BufferCount<2)
						m_BufferCount=2;
					else
						m_BufferCount<<=1;

					const uint32 VSize=m_BufferCount*(m_Size+sizeof(tdResHandle));
					MMRES_ADDMM(VSize);
					uint8* pBuffer		=	(uint8*)CRY_DXPS_NEWARRAY(vec_uint4,VSize/sizeof(vec_uint4));//allocate 16 byte aligned (SPU access)
					mymemcpy16(pBuffer,m_pBuffer,VSizeOld);
					if(VSizeOld < VSize)
						memset(pBuffer+VSizeOld,0,VSize - VSizeOld);

					uint8* pBuffer2=m_pBuffer;
					m_pBuffer	=	pBuffer;

					READ_WRITE_BARRIER

					//before deleting old array, give rsx some time to finish
					Sleep(1);
					CRY_DXPS_DELETEARRAY(pBuffer2);
				}
		}
		else
#endif
		{
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
			if(MultiBuffer())
				tdLayer0::Device()->ResetBuffer(this);
#endif
			Sync();//SLOW
		}
	}
	(*ppData)	=	RawData();
	return 0;
}

bool CCryDXPSBuffer::SyncNoWaitAll()
{
	uint32 zWriteCount;
	tdResHandle ResDCLock=m_ResDCLock;

	if(m_BufferCount==2)
	{
		ResDCLock	=	m_ResDCLock>m_TmpLock?m_ResDCLock:m_TmpLock;
	}
	else
	{
		tdResHandle*	pData	=	reinterpret_cast<tdResHandle*>(RawData(m_BufferCount));//get ptr to beyond real data
		for(uint32 a=0,S=m_BufferCount;a<S;a++)
			if(pData[a]>ResDCLock)
				ResDCLock	=	pData[a];
	}

	return tdLayer0::Sync().SyncToRSX<false,true>(ResDCLock, zWriteCount);
}

CCryDXPSBuffer::CCryDXPSBuffer(uint32 Size MMRES_PARAM):
CCryDXPSResource(EDXPS_RT_CONSTBUFFERTEMP MMRES_PASS),
m_Size(Size),
m_BufferCount(0),
m_CurrentBuffer(0),
m_TmpLock(0)
{
	m_pBuffer	=	DataPtr(this);
}


