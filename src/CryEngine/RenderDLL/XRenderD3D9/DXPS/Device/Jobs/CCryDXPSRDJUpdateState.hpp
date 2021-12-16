#ifndef __CRYDXPSRDJUPDATESTATE__
#define __CRYDXPSRDJUPDATESTATE__

#include "CCryDXPSRDJob.hpp"

///////////////////////////////////////////////////////////
// Texture
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateTexture
{
protected:
	CCryDXPSTexture*							m_pTexture;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateTextureSPU	:	public CDXPSRDJUpdateStateTexture
{
public:
	const CCryDXPSTexture*				Texture()const {return m_pTexture;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateTexturePPU	:	public CDXPSRDJUpdateStateTexture
{
public:
	uint32												Setup(uint32 Idx,CCryDXPSTexture* pTex)
																{
																	m_pTexture	=	pTex;
																	JOBRETURN(static_cast<EDXPSJob>(EDXPSJ_STATE_TEXTURE0+Idx));
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif


///////////////////////////////////////////////////////////
// ConstBuffer
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateConstBuffer
{
protected:
	uint32												m_ConstBufferSize;			//corresponds to m_pConstBuffer->Size()
	SPU_DOMAIN_MAIN void* 				m_pConstBufferRawData;	//corresponds to m_pConstBuffer->RawData(m_ConstBufferIdx)
public:
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateConstBufferSPU	:	public CDXPSRDJUpdateStateConstBuffer
{
public:
	void*													ConstBufferRawData()const {return m_pConstBufferRawData;}
	uint32												ConstBufferSize()const {return m_ConstBufferSize;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateConstBufferPPU	:	public CDXPSRDJUpdateStateConstBuffer
{
public:
	uint32												Setup(EDXPSJob Job,uint32 Size,uint8** ppConstantBuffer)
																{
																	uint8* pThis			=	reinterpret_cast<uint8*>(this);
																	uint32 AllocSize	= CCryDXPSBuffer::EstimateSize(Size);
																	*ppConstantBuffer	=	pThis	+	sizeof(*this);

																	m_pConstBufferRawData =		CCryDXPSBuffer::DataPtr(*ppConstantBuffer);
																	m_ConstBufferSize			=		CCryDXPSBuffer::EstimateSize(Size);


																	//job header
																	pThis-=DXPS_DEVICECMDBALLIGN;
																	uint32 jobSize = sizeof(*this)+AllocSize;
																	jobSize = (jobSize + DXPS_DEVICECMDBALLIGNMASK) & ~DXPS_DEVICECMDBALLIGNMASK;//align to multiple of 16 bytes for dma transfers
																	new (pThis)CDXPSRDJobPPU(jobSize,Job);

																	return jobSize;
																}
	uint32												Setup(EDXPSJob Job,ID3D11Buffer* pConstantBuffer)
																{
																	m_pConstBufferRawData =		(void*)pConstantBuffer->RawData();
																	m_ConstBufferSize			=		pConstantBuffer->Size();
																	JOBRETURN(Job);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif

///////////////////////////////////////////////////////////
// VertexBuffer
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateVertexBuffer
{
protected:
	uint8*												m_pBuffer;
	uint32												m_Stride:8;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateVertexBufferSPU	:	public CDXPSRDJUpdateStateVertexBuffer
{
public:
	uint8*												Buffer()const {return m_pBuffer;}
	uint32												Stride()const {return m_Stride;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateVertexBufferPPU	:	public CDXPSRDJUpdateStateVertexBuffer
{
public:
	uint32												Setup(uint32 Idx,CCryDXPSBuffer* pVertexBuffer,uint32 Stride,uint32 Offset)
																{
																	assert(pVertexBuffer);
																	m_pBuffer	=	pVertexBuffer->RawData()+Offset;
																	m_Stride	=	Stride;
																	JOBRETURN(static_cast<EDXPSJob>(EDXPSJ_STATE_VERTEXBUFFER0+Idx));
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif

///////////////////////////////////////////////////////////
// Sampler
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateSampler
{
protected:
	CCryDXPSSamplerState*					m_pSampler;
	int16													m_MinLOD;
	int16													m_MaxLOD;
	uint16												m_LODBias;
	uint16												m_sRGB;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateSamplerSPU	:	public CDXPSRDJUpdateStateSampler
{
public:
	const CCryDXPSSamplerState*		Sampler()const {return m_pSampler;}
	int16													MinLOD()const {return m_MinLOD;}
	int16													MaxLOD()const {return m_MaxLOD;}
	uint16												LODBias()const {return m_LODBias;}
	uint16												sRGB()const {return m_sRGB;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateSamplerPPU	:	public CDXPSRDJUpdateStateSampler
{
public:
	uint32												Setup(uint32 Idx,CCryDXPSSamplerState* pSamplers,int16 MinLOD,int16 MaxLOD, uint16 LODBias,uint16 sRGB)
																{
																	m_pSampler	=	pSamplers;
																	m_MinLOD		=	MinLOD;
																	m_MaxLOD		=	MaxLOD;
																	m_LODBias		= LODBias;
																	m_sRGB			=	sRGB;
																	JOBRETURN(static_cast<EDXPSJob>(EDXPSJ_STATE_SAMPLER0+Idx));
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif

///////////////////////////////////////////////////////////
// InputLayout
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateInputLayout
{
protected:
	mutable CCryDXPSInputLayout*	m_pInputLayout;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateInputLayoutSPU	:	public CDXPSRDJUpdateStateInputLayout
{
public:
	CCryDXPSInputLayout*					InputLayout()const {return m_pInputLayout;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateInputLayoutPPU	:	public CDXPSRDJUpdateStateInputLayout
{
public:
	uint32												Setup(CCryDXPSInputLayout* pInputLayout)
																{
																	m_pInputLayout	=	pInputLayout;
																	JOBRETURN(EDXPSJ_STATE_INPUTLAYOUT);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif

///////////////////////////////////////////////////////////
// Shader
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateShader
{
protected:
	CCryDXPSShader*								m_pShader;
	uint8*												m_pProgram;
	uint32												m_Size;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateShaderSPU	:	public CDXPSRDJUpdateStateShader
{
public:
	const CCryDXPSShader*					Shader()const {return m_pShader;}
	uint8*												Program()const{return m_pProgram;}
	uint32												Size()const{return m_Size;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateShaderPPU	:	public CDXPSRDJUpdateStateShader
{
public:
	uint32												Setup(EDXPSJob Job,CCryDXPSShader* pShader,uint8* pProgram,uint32 Size)
																{
																	m_pShader		=	pShader;
																	m_pProgram	=	pProgram;
																	m_Size			=	Size;
																	JOBRETURN(Job);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif

///////////////////////////////////////////////////////////
// BlendState
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateBlend
{
protected:
	CCryDXPSBlendState*						m_pBlendState;
	uint32												m_BlendFactor;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateBlendSPU	:	public CDXPSRDJUpdateStateBlend
{
public:
	const CCryDXPSBlendState*			BlendState()const {return m_pBlendState;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateBlendFactorSPU	:	public CDXPSRDJUpdateStateBlend
{
public:
	uint32												BlendFactor()const {return m_BlendFactor;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateBlendPPU	:	public CDXPSRDJUpdateStateBlend
{
public:
	uint32												Setup(CCryDXPSBlendState* pBlendState,const float BlendFactor[4])
																{
																	m_pBlendState			=	pBlendState;
																	if(BlendFactor)
																	{
																		int R=static_cast<int>(BlendFactor[0]*255.f);
																		int G=static_cast<int>(BlendFactor[1]*255.f);
																		int B=static_cast<int>(BlendFactor[2]*255.f);
																		int A=static_cast<int>(BlendFactor[3]*255.f);
																		R	=	R>255?255:(R<0)?0:R;
																		G	=	G>255?255:(G<0)?0:G;
																		B	=	B>255?255:(B<0)?0:B;
																		A	=	A>255?255:(A<0)?0:A;
																		m_BlendFactor	=	B|(G<<8)|(R<<16)|(A<<24);
																		JOBRETURN(EDXPSJ_STATE_BLENDFACTOR);
																	}
																	JOBRETURN(EDXPSJ_STATE_BLENDSTATE);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif


///////////////////////////////////////////////////////////
// StencilState
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateStencil
{
protected:
	CCryDXPSDepthStencilState*		m_pDepthStencilState;
	uint32												m_StencilRef;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateStencilSPU	:	public CDXPSRDJUpdateStateStencil
{
public:
	const CCryDXPSDepthStencilState*	DepthStencilState()const {return m_pDepthStencilState;}
	uint32														StencilRef()const {return m_StencilRef;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateStencilPPU	:	public CDXPSRDJUpdateStateStencil
{
public:
	uint32												Setup(CCryDXPSDepthStencilState* pDepthStencilState,uint32 StencilRef)
																{
																	m_pDepthStencilState	=	pDepthStencilState;
																	m_StencilRef					=	StencilRef;
																	JOBRETURN(EDXPSJ_STATE_DEPTHSTENCILSTATE);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif

///////////////////////////////////////////////////////////
// RasterizerState
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateRasterizer
{
protected:
	CCryDXPSRasterizerState*			m_pRasterizerState;
	uint32												m_sRGB;
	uint32												m_AlphaTestRef;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateRasterizerSPU	:	public CDXPSRDJUpdateStateRasterizer
{
public:
	const CCryDXPSRasterizerState*		RasterizerState()const {return m_pRasterizerState;}
	uint32														sRGB()const{return m_sRGB;}
	uint32														AlphaTestRef()const{return m_AlphaTestRef;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateRasterizerPPU	:	public CDXPSRDJUpdateStateRasterizer
{
public:
	uint32												Setup(CCryDXPSRasterizerState* pRasterizerState,uint32 sRGB,uint32 AlphaTestRef)
																{
																	m_pRasterizerState	=	pRasterizerState;
																	m_sRGB							=	sRGB;
																	m_AlphaTestRef			=	AlphaTestRef;
																	JOBRETURN(EDXPSJ_STATE_RASTERIZERSTATE);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif

///////////////////////////////////////////////////////////
// ScissorRect
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateScissor
{
protected:
	uint16												m_MinX;
	uint16												m_MinY;
	uint16												m_MaxX;
	uint16												m_MaxY;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateScissorSPU	:	public CDXPSRDJUpdateStateScissor
{
public:
	uint16												MinX()const {return m_MinX;}
	uint16												MinY()const {return m_MinY;}
	uint16												MaxX()const {return m_MaxX;}
	uint16												MaxY()const {return m_MaxY;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateScissorPPU	:	public CDXPSRDJUpdateStateScissor
{
public:
	uint32												Setup(const int MinX,const int MinY,const int MaxX,const int MaxY)
																{
																	m_MinX		=		static_cast<int16>(MinX);
																	m_MinY		=		static_cast<int16>(MinY);
																	m_MaxX		=		static_cast<int16>(MaxX);
																	m_MaxY		=		static_cast<int16>(MaxY);
																	JOBRETURN(EDXPSJ_STATE_SCISSOR);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif

///////////////////////////////////////////////////////////
// DepthBounds
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateDepthBounds
{
protected:
	uint32												m_Enable;
	float													m_ZMin;
	float													m_ZMax;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateDepthBoundsSPU	:	public CDXPSRDJUpdateStateDepthBounds
{
public:
	uint32												Enable()const {return m_Enable;}
	float													ZMin()const {return m_ZMin;}
	float													ZMax()const {return m_ZMax;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateDepthBoundsPPU	:	public CDXPSRDJUpdateStateDepthBounds
{
public:
	uint32												Setup(uint32 Enable,const float ZMin,const float ZMax)
																{	
																	m_Enable	=	Enable;
																	m_ZMin		=	ZMin;
																	m_ZMax		=	ZMax;
																	JOBRETURN(EDXPSJ_STATE_DEPTHBOUNDS);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif

///////////////////////////////////////////////////////////
// Topology
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStateTopology
{
protected:
	uint32												m_Topology;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateTopologySPU	:	public CDXPSRDJUpdateStateTopology
{
public:
	uint32												Topology()const{return m_Topology;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateTopologyPPU	:	public CDXPSRDJUpdateStateTopology
{
public:
	uint32												Setup(uint32 TPG)
																{
																	m_Topology=TPG;
																	JOBRETURN(EDXPSJ_STATE_TOPOLOGY);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif

///////////////////////////////////////////////////////////
// Pass
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateStatePass
{
protected:
	float													m_ViewMat[16];
	uint32												m_Pass;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStatePassSPU	:	public CDXPSRDJUpdateStatePass
{
public:
	uint32												Pass()const{return m_Pass;}
	const float*									ViewMat()const{return m_ViewMat;}

} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStatePassPPU	:	public CDXPSRDJUpdateStatePass
{
public:
	uint32												Setup(uint32 Pass,float* pViewMat=NULL)
																{
																	if(pViewMat)
																	{
																		for(uint32 a=0;a<32;a++)
																			m_ViewMat[a]	=	pViewMat[a];
																	}
																	m_Pass					=	Pass;
																	JOBRETURN(EDXPSJ_STATE_PASS);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif


///////////////////////////////////////////////////////////
// DEBUGPROFILIE
///////////////////////////////////////////////////////////
enum EDXPSJDPType
{
	EDXPSJDPT_SETMARKER,
	EDXPSJDPT_SETPUSHMARKER,
	EDXPSJDPT_SETPOPMARKER,
};
class CDXPSRDJUpdateStateDebugProfile
{
protected:
	EDXPSJDPType									m_Type;
	char													m_Name[256-sizeof(EDXPSJDPType)];//make sure it does not exceed 256 bytes
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateStateDebugProfileSPU	:	public CDXPSRDJUpdateStateDebugProfile
{
public:
	EDXPSJDPType									Type()const{return m_Type;};
	const char*										Name()const{return m_Name;};
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateStateDebugProfilePPU	:	public CDXPSRDJUpdateStateDebugProfile
{
public:
	uint32												SetMarker(const char* pName)
																{
																	m_Type	=	EDXPSJDPT_SETMARKER;
																	strcpy(m_Name,pName);
																	JOBRETURN(EDXPSJ_STATE_DEBUGPROFILIE);
																}
	uint32												PushMarker(const char* pName)
																{
																	m_Type	=	EDXPSJDPT_SETPUSHMARKER;
																	strcpy(m_Name,pName);
																	JOBRETURN(EDXPSJ_STATE_DEBUGPROFILIE);
																}
	uint32												PopMarker(const char* pName)
																{
																	m_Type	=	EDXPSJDPT_SETPOPMARKER;
																	strcpy(m_Name,pName);
																	JOBRETURN(EDXPSJ_STATE_DEBUGPROFILIE);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif


///////////////////////////////////////////////////////////
// DebugData
///////////////////////////////////////////////////////////
class CDXPSRDJUpdateDebugData
{
protected:
	uint32												m_Slot;
	uint32												m_Data;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJUpdateDebugDataSPU	:	public CDXPSRDJUpdateDebugData
{
public:
	uint32												Slot()const {return m_Slot;}
	uint32												Data()const {return m_Data;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJUpdateDebugDataPPU	:	public CDXPSRDJUpdateDebugData
{
public:
	uint32												Setup(const uint32 Slot,const uint32 Data)
																{
																	m_Slot	=	Slot;
																	m_Data	=	Data;
																	JOBRETURN(EDXPSJ_STATE_DEBUGDATA);
																}
} _ALIGN(DXPS_DEVICECMDBALLIGN);
#endif



#endif

