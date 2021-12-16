#ifndef __CRYDXPSINPUTLAYOUT__
#define __CRYDXPSINPUTLAYOUT__

#include "../CCryDXPSResource.hpp"
#include "../../../CCryDXPSAPtr.hpp"
#include "../../../Layer0/CCryDXPS.hpp"

class CCryDXPSInputLayout;
class CCryDXPSInputLayout	:	public CCryDXPSResource	,	public CCryRefAndWeak<CCryDXPSInputLayout>
{

public:
	struct SLayout
	{
	public:
#if defined(_DEBUG)
//		char													m_SemanticName[64];
#else
		SLayout& operator =(const SLayout& crFrom){*(uint64*)this = *(uint64*)&crFrom;return *this;}
#endif
		uint16												m_Frequency;
		uint8													m_ByteOffset;
		uint8													m_AttribCount;
		uint8													m_AttribPitch;
		uint8													m_InputOffset;
		uint8													m_InputSlot;
		uint8													m_Type:4;
		uint8													m_Size:4;
	} _ALIGN(8);
	SLayout													m_Layout[16] _ALIGN(16);
	uint32													m_Count;
//	uint32													m_FrequencyDivider;
	D3D11_INPUT_ELEMENT_DESC*				m_pInputElementDescs;
	uint32													m_NumElements;


	void														Init(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,uint32 NumElements,const CDXPSShaderDesc*	pShaderDesc);
public:
																	CCryDXPSInputLayout(const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,uint32 NumElements,const void *pShaderBytecodeWithInputSignature MMRES_PARAM);
																	~CCryDXPSInputLayout()
																	{
																		MMRES_SUBCOUNT();
																		MMRES_SUBMM(sizeof(CCryDXPSInputLayout));
#if defined(CRY_DXPS_DESTRUCTORVALIDATION)
																		if(EDXPS_RT_INPUTLAYOUT==CCryDXPSResource::Type())
																			CCryDXPSResource::Type(EDXPS_RT_NONE);
#endif
																	}


	uint32 													Set(const CDXPSShaderDesc*	pShaderDesc,const uint8** pVBufferArray,uint32 BufferCount,uint32* pStride,uint32 IndexCount,uint32 LastMask) const;

	inline	void										ReleaseResources(){}
	inline	int											Release(){return DecRef();}
	inline	bool										Initialized()const{return m_pInputElementDescs==0;}
} _ALIGN(128);

typedef CCryDXPSInputLayout ID3D11InputLayout;
#ifdef CRY_DXPS_VALIDATEWEAKPTR
	typedef CCryAPtrWeakCnt<CCryDXPSInputLayout>	APWeakInputLayout;
#else
	typedef CCryDXPSInputLayout* __restrict 	APWeakInputLayout;
#endif

#endif

