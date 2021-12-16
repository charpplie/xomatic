#include "StdAfx.h"
#include "../../../Layer0/CCryDXPS.hpp"
#include "CCryDXPSInputLayout.hpp"
#include "CCryDXPSShader.hpp"
#include "CCryDXPSShaderDesc.hpp"
//#include <assert.h>
#include <string.h>





	#define INIT_IL_PERF_STATS()
	#define ADD_IL_PERF_STATS()


namespace
{
	ILINE uint32 GetStrLen(const char* __restrict pStr)
	{
		const char* __restrict pStrOrg	=	pStr;
		while(*pStr++ != '\0');
		return pStr-pStrOrg-1;
	}

	//behaves like strncmp
	ILINE int CompAttrSemantic(const char* pAttributeSemName, const char* pSemantic, uint32 SemLen)
	{
		//this basically implements:
		//  return strncmp(pAttributeSemName,pSemantic,SemLen);
		char localBuf[256] _ALIGN(128);
		char localBuf2[256] _ALIGN(128);
		memcpy(localBuf, pAttributeSemName, SemLen);
		memcpy(localBuf2, pSemantic, SemLen);
		//simplified version of strncmp operating on local store
		const uint8* pS0 = (const uint8*)localBuf;
		const uint8* pS1 = (const uint8*)localBuf2;
		while(SemLen-- > 0)
		{
			const uint8 u1 = *pS0++;
			const uint8 u2 = *pS1++;
			if(u1 != u2)
				return u1 - u2;
			if(u1 == '\0')
				return 0;
		}
		return 0;
	}
}

const SRefVertexAttribute* GetParameterBySemantic(const CDXPSShaderDesc*	pShaderDesc,const char* pSemantic,uint32 SemanticIndex)
{
	const SRefVertexAttribute* pRet=0;

	const uint32 SemLen	=	GetStrLen(pSemantic);

	for(uint32 a=0,Size=pShaderDesc->VertexAttributeCount();a<Size;a++)
	{
		const char* pAttributeSemName	=	&pShaderDesc->NameTable()[pShaderDesc->VertexAttribute()[a].m_SemanticNameIndex];

		if(!CompAttrSemantic(pAttributeSemName,pSemantic,SemLen))
		{
			const uint32 ParamLen=GetStrLen(pAttributeSemName);
			if(ParamLen==SemLen)
				pRet=&pShaderDesc->VertexAttribute()[a];//memorize but go on checking if there is a better fit for that request
			if(ParamLen==SemLen+1)
			{
				const uint32 SIndex=pAttributeSemName[ParamLen-1]-'0';
				if(SIndex==SemanticIndex)
					return &pShaderDesc->VertexAttribute()[a];
			}
		}
	}
	return pRet;
}




CCryDXPSInputLayout::CCryDXPSInputLayout(const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,uint32 NumElements,const void *pShaderBytecodeWithInputSignature MMRES_PARAM):
CCryDXPSResource(EDXPS_RT_INPUTLAYOUT MMRES_PASS)
{
		MMRES_ADDCOUNT();
		MMRES_ADDMM(sizeof(CCryDXPSInputLayout));

		m_pInputElementDescs=0;
#if defined(DXPS_LZSS_COMPRESS)
		uint8 pProgram[LOCAL_VS_BUFFER_SIZE] _ALIGN(128);
		CDXPSShaderDesc::Decode(pProgram,reinterpret_cast<const uint8*>(pShaderBytecodeWithInputSignature)+4);
		Init(pInputElementDescs,NumElements,reinterpret_cast<CDXPSShaderDesc*>(pProgram));
#else
		Init(pInputElementDescs,NumElements,(const CDXPSShaderDesc*)&((const uint8*)pShaderBytecodeWithInputSignature)[4]);
#endif

		if(NumElements!=m_Count)
		{
			m_pInputElementDescs	=	new	D3D11_INPUT_ELEMENT_DESC[NumElements];
			m_NumElements					=	NumElements;
			for(uint32 a=0;a<NumElements;a++)
				m_pInputElementDescs[a]	=	pInputElementDescs[a];
		}
}

void CCryDXPSInputLayout::Init(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,uint32 NumElements,const CDXPSShaderDesc*	pShaderDesc)
{
	INIT_IL_PERF_STATS()
	m_Count=0;
//	m_FrequencyDivider	=	0;

	const SRefVertexAttribute* pUsedAttr[16];

	for(uint32 a=0;a<NumElements;a++)
	{
		uint32 Size=0,Type=0,AttribPitch;
		const DXGI_FORMAT cInputFormat = (DXGI_FORMAT)pInputElementDescs[a].Format;
		if(cInputFormat==DXGI_FORMAT_R32_FLOAT)
		{
			Type				=	CELL_GCM_VERTEX_F;
			Size				=	1;
			AttribPitch	=	4;
		}
		else
		if(cInputFormat==DXGI_FORMAT_R16G16_SINT)
		{
			Type				=	CELL_GCM_VERTEX_S32K;
			Size				=	2;
			AttribPitch	=	4;
		}
		else
		if(cInputFormat==DXGI_FORMAT_R32G32_FLOAT)
		{
			Type				=	CELL_GCM_VERTEX_F;
			Size				=	2;
			AttribPitch	=	8;
		}
		else
		if(cInputFormat==DXGI_FORMAT_R32G32B32_FLOAT)
		{
			Type				=	CELL_GCM_VERTEX_F;
			Size				=	3;
			AttribPitch	=	12;
		}
		else
		if(cInputFormat==DXGI_FORMAT_R32G32B32A32_FLOAT)
		{
			Type				=	CELL_GCM_VERTEX_F;
			Size				=	4;
			AttribPitch	=	16;
		}
		else
		if(cInputFormat==DXGI_FORMAT_R8G8B8A8_UNORM)
		{
			Type				=	CELL_GCM_VERTEX_UB;
			Size				=	4;
			AttribPitch	=	4;
		}
		else
		if(cInputFormat==DXGI_FORMAT_R16G16B16A16_SNORM)
		{
			Type				=	CELL_GCM_VERTEX_S1;
			Size				=	4;
			AttribPitch	=	8;
		}
#ifdef FP16_MESH
		else
		if(cInputFormat==DXGI_FORMAT_R16G16B16A16_FLOAT)
		{
			Type = CELL_GCM_VERTEX_SF;
			Size = 4;
			AttribPitch = 8;
		}
		else
		if(cInputFormat==DXGI_FORMAT_R16G16_FLOAT)
		{
			Type = CELL_GCM_VERTEX_SF;
			Size = 2;
			AttribPitch = 4;
		}
#endif
		else
		{
			CRY_DEBUGOUT("Unknown Format in InputLayout %d\n",cInputFormat);
			snPause();
		}




#if defined(_DEBUG) && !defined(__SPU__)
/*		m_Layout[m_Count].m_SemanticName[0]	=	0;
		assert(strlen(pInputElementDescs[a].SemanticName)+1<=sizeof(m_Layout[m_Count].m_SemanticName));
		if(strlen(pInputElementDescs[a].SemanticName)+1>sizeof(m_Layout[m_Count].m_SemanticName))
		{
			CRY_DEBUGOUT("Size of Semanticname %s exceeds the limit of %d bytes\n",pInputElementDescs[a].SemanticName,sizeof(m_Layout[m_Count].m_SemanticName));
			break;
		}
		strcpy(m_Layout[m_Count].m_SemanticName,pInputElementDescs[a].SemanticName);
*/
#endif




		pUsedAttr[a]	=	0;
		const SRefVertexAttribute* pAttr = GetParameterBySemantic(pShaderDesc,pInputElementDescs[a].SemanticName,pInputElementDescs[a].SemanticIndex);
		if(!pAttr)
			continue;

		bool AlreadyUsed=false;
		for(uint32 b=0;b<a;b++)
			AlreadyUsed|=pUsedAttr[b]==pAttr;

		if(AlreadyUsed)
			continue;

		pUsedAttr[a]	=	pAttr;

		//already tested for valid InputOffset in GetParameterBySemantic
		CRY_ASSERT_MESSAGE(pInputElementDescs[a].AlignedByteOffset<256 && (pInputElementDescs[a].AlignedByteOffset&3)==0,"InputLayout fails due to AlignedByteOffset>=128 or not 4byte alligned"); 
		CRY_ASSERT_MESSAGE(Type<8,"InputLayout fails due to Type>=8"); 
		CRY_ASSERT_MESSAGE(Size>0 && Size<5,"InputLayout fails due to Size==0 or Size>4"); 

		SLayout& rLayout = m_Layout[m_Count];
//		rLayout.m_InputOffset = InputOffset;
		rLayout.m_InputOffset = pAttr->m_InputOffset;
		rLayout.m_AttribCount	= pAttr->m_Size;
		rLayout.m_AttribPitch	= AttribPitch;
//		m_FrequencyDivider	|=	(pInputElementDescs[a].InstanceDataStepRate==0?1:0)<<pAttr->m_InputOffset;
		rLayout.m_ByteOffset=	pInputElementDescs[a].AlignedByteOffset;
		rLayout.m_Frequency	=	pInputElementDescs[a].InstanceDataStepRate;
		rLayout.m_Type			=	Type;
		rLayout.m_Size			=	Size;
		rLayout.m_InputSlot	=	pInputElementDescs[a].InputSlot;
		m_Count++;

	}
	if(NumElements==m_Count && m_pInputElementDescs)
	{
		//do not double delete if both, PPU and SPU execute this code
		delete[]	m_pInputElementDescs;
		m_pInputElementDescs	=	0;
	}
	ADD_IL_PERF_STATS()	
}

uint32 CCryDXPSInputLayout::Set(const CDXPSShaderDesc*	pShaderDesc,const uint8** pVBufferArray,uint32 BufferCount,uint32* pStride,uint32 IndexCount,uint32 LastMask)	const
{
	SPU_FRAME_PROFILE_SECTION("CCryDXPSInputLayout::Set")
	using namespace CRY_DXPS_GCMNAMESPACE;
	DXPS_PROFILE_FRAME(DXPS_SetVertexLayout);

	if(m_pInputElementDescs)
	{
		//initialization is not const
		if(m_Count<pShaderDesc->VertexAttributeCount())
			const_cast<CCryDXPSInputLayout*>(this)->Init(m_pInputElementDescs,m_NumElements,pShaderDesc);
	}

	INIT_IL_PERF_STATS()

//	cellGcmSetFrequencyDividerOperation(m_FrequencyDivider);	//dividers set to division or modulo

	const uint32 AttributeMask	=	pShaderDesc->AttributeMask();
	const uint32 cCount = m_Count;
	SLayout layout[16] _ALIGN(128);
	mymemcpy16(layout, m_Layout, sizeof(layout));
	for(uint32 a=0;a<cCount;++a)
	{
		uint32 ElementOffset;
		const SLayout& crLayout = layout[a];
		const uint8 cInputSlot	= crLayout.m_InputSlot;
		if(cInputSlot < BufferCount)
		{
			const uint8* const pVBuffer	=	pVBufferArray[cInputSlot];
			if(pVBuffer)
			{
				cellGcmAddressToOffset(SPU_MAIN_PTR(&pVBuffer[crLayout.m_ByteOffset]),&ElementOffset);

				const uint8 cSize					= crLayout.m_Size;
				const uint8 cType					= crLayout.m_Type;
				const uint8 cInputOffset	= crLayout.m_InputOffset;
				const uint16 cFrequency		= crLayout.m_Frequency;
				const uint8	cAttribCount	= crLayout.m_AttribCount;
				const uint16 AttribPitch	= crLayout.m_AttribPitch;
				const uint32 cStride			= pStride[cInputSlot];

				for(uint32 ac=0;ac<cAttribCount;++ac)
				{
					if((AttributeMask&(1<<(cInputOffset+ac)))==0)
							continue;
					cellGcmSetVertexDataArray(
														cInputOffset + ac,
														cFrequency * IndexCount,
														cStride,
														cSize,
														cType,
														CELL_GCM_LOCATION_LOCAL,
														ElementOffset+ac*AttribPitch);
				}
			}
			else
			{
				CRY_DEBUGOUT("ERROR: InputVertexBuffer indexing %d is a NULL-ptr\n",cInputSlot); 
			}
		}
		else
		{
			CRY_DEBUGOUT("ERROR: indexing InputVertexBuffer #%d, but set %d InputVertexBuffers\n",cInputSlot,BufferCount); 
		}
	}
	LastMask&=~AttributeMask;
	for(uint32 a=0,b=1;a<16;a++,b<<=1)
		if(LastMask&b)
			cellGcmSetVertexDataArray(a,0,0,0,CELL_GCM_VERTEX_F,CELL_GCM_LOCATION_LOCAL,0);
	ADD_IL_PERF_STATS()
	return AttributeMask;
}

