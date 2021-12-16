#ifndef __DXPSREFLECTIONSHARED__
#define __DXPSREFLECTIONSHARED__

#define DXPS_REF_SHADER_VERSION	0xff
//#define DXPS_LZSS_COMPRESS

#ifndef _ALIGN
#define _ALIGN(X)
#endif

typedef int8_t		int8;
typedef int16_t		int16;
typedef int32_t		int32;
typedef uint8_t		uint8;
typedef uint16_t	uint16;
typedef uint32_t	uint32;

enum {MAX_SLOTCOUNT	=	16};
enum R_CG_TYPE
{
	R_CG_HALF					=	0,
	R_CG_FLOAT				=	1,
	R_CG_BOOL					=	2,
	R_CG_BOOL2				=	3,
	R_CG_BOOL3				=	4,
	R_CG_BOOL4				=	5,
	R_CG_HALF2				=	6,
	R_CG_HALF3				=	7,
	R_CG_HALF4				=	8,
	R_CG_FLOAT2				=	9,
	R_CG_FLOAT3				=	10,
	R_CG_FLOAT4				=	11,
/*
	R_CG_BOOL2x2			=	12,
	R_CG_BOOL3x2			=	13,
	R_CG_BOOL4x2			=	14,
	R_CG_BOOL2x3			=	15,
	R_CG_BOOL3x3			=	16,
	R_CG_BOOL4x3			=	17,
	R_CG_BOOL2x4			=	18,
	R_CG_BOOL3x4			=	19,
	R_CG_BOOL4x4			=	20,
	R_CG_HALF2x2			=	21,
	R_CG_HALF3x2			=	22,
	R_CG_HALF4x2			=	23,
	R_CG_HALF2x3			=	24,
	R_CG_HALF3x3			=	25,
	R_CG_HALF4x3			=	26,
	R_CG_HALF2x4			=	27,
	R_CG_HALF3x4			=	28,
	R_CG_HALF4x4			=	29,
	R_CG_FLOAT2x2			=	30,
	R_CG_FLOAT3x2			=	31,
	R_CG_FLOAT4x2			=	32,
	R_CG_FLOAT2x3			=	33,
	R_CG_FLOAT3x3			=	34,
	R_CG_FLOAT4x3			=	35,
	R_CG_FLOAT2x4			=	36,
	R_CG_FLOAT3x4			=	37,
	R_CG_FLOAT4x4			=	38,*/
	R_CG_SAMPLER1D		=	39,
	R_CG_SAMPLER2D		=	40,
	R_CG_SAMPLER3D		=	41,
	R_CG_SAMPLERRECT	=	42,
	R_CG_SAMPLERCUBE	=	43,
	R_CG_INVALIDE			=	0xffffffff
};

#define	R_CG_BOOL2x2			R_CG_BOOL2
#define	R_CG_BOOL3x2			R_CG_BOOL2
#define	R_CG_BOOL4x2			R_CG_BOOL2
#define	R_CG_BOOL2x3			R_CG_BOOL3
#define	R_CG_BOOL3x3			R_CG_BOOL3
#define	R_CG_BOOL4x3			R_CG_BOOL3
#define	R_CG_BOOL2x4			R_CG_BOOL4
#define	R_CG_BOOL3x4			R_CG_BOOL4
#define	R_CG_BOOL4x4			R_CG_BOOL4
#define	R_CG_HALF2x2			R_CG_HALF2
#define	R_CG_HALF3x2			R_CG_HALF2
#define	R_CG_HALF4x2			R_CG_HALF2
#define	R_CG_HALF2x3			R_CG_HALF3
#define	R_CG_HALF3x3			R_CG_HALF3
#define	R_CG_HALF4x3			R_CG_HALF3
#define	R_CG_HALF2x4			R_CG_HALF4
#define	R_CG_HALF3x4			R_CG_HALF4
#define	R_CG_HALF4x4			R_CG_HALF4
#define	R_CG_FLOAT2x2			R_CG_FLOAT2
#define	R_CG_FLOAT3x2			R_CG_FLOAT2
#define	R_CG_FLOAT4x2			R_CG_FLOAT2
#define	R_CG_FLOAT2x3			R_CG_FLOAT3
#define	R_CG_FLOAT3x3			R_CG_FLOAT3
#define	R_CG_FLOAT4x3			R_CG_FLOAT4
#define	R_CG_FLOAT2x4			R_CG_FLOAT4
#define	R_CG_FLOAT3x4			R_CG_FLOAT4
#define	R_CG_FLOAT4x4			R_CG_FLOAT4


enum R_CG_RESOURCE
{
	R_CG_C0								=0,
	R_CG_C468							=468,
	R_CG_ATTR0,
	R_CG_ATTR1,
	R_CG_ATTR2,
	R_CG_ATTR3,
	R_CG_ATTR4,
	R_CG_ATTR5,
	R_CG_ATTR6,
	R_CG_ATTR7,
	R_CG_ATTR8,
	R_CG_ATTR9,
	R_CG_ATTR10,
	R_CG_ATTR11,
	R_CG_ATTR12,
	R_CG_ATTR13,
	R_CG_ATTR14,
	R_CG_ATTR15,
	R_CG_SAMPLER0,
	R_CG_SAMPLER1,
	R_CG_SAMPLER2,
	R_CG_SAMPLER3,
	R_CG_SAMPLER4,
	R_CG_SAMPLER5,
	R_CG_SAMPLER6,
	R_CG_SAMPLER7,
	R_CG_SAMPLER8,
	R_CG_SAMPLER9,
	R_CG_SAMPLER10,
	R_CG_SAMPLER11,
	R_CG_SAMPLER12,
	R_CG_SAMPLER13,
	R_CG_SAMPLER14,
	R_CG_SAMPLER15,
	R_CG_CONST_UNDEFINED=0xffffff,
};

/////////////////////////////////////////////////////
//VSConstants
/////////////////////////////////////////////////////
struct	SVSConst
{
	uint32		m_Index;
	uint32		m_Value[4];
};

/////////////////////////////////////////////////////
//ShaderPatch
/////////////////////////////////////////////////////
struct SRefPatch
{
//	uint16	m_ConstSlot;
	uint16	m_ConstReg;
	uint16	m_Offset;
};

/////////////////////////////////////////////////////
//Constants
/////////////////////////////////////////////////////
struct SRefConstant
{
/*
	uint32		m_NameIndex;
	uint32		m_Type;
	uint32		m_Size;
	uint32		m_ConstRegister;
	uint32		m_ConstSlot;
/*/
	uint16		m_NameIndex;
	uint16		m_Size;
	uint16		m_ConstRegister;
	uint8			m_Type;
	uint8			m_ConstSlot;
/**/
	bool			operator<(const SRefConstant& rRConst)const{return m_ConstSlot<rRConst.m_ConstSlot ||
																													(m_ConstSlot==rRConst.m_ConstSlot	&& m_ConstRegister<rRConst.m_ConstRegister);}
};
/////////////////////////////////////////////////////
//Samplers
/////////////////////////////////////////////////////
struct SRefSampler
{
	uint16			m_NameIndex;
	uint16			m_SamplerIndex;
};

/////////////////////////////////////////////////////
//Vertex Attributes
/////////////////////////////////////////////////////
//InputOffset to SemanticIndex
const uint8 SEMANTICINDEX[16]=
{
	0,//pos
	0,//blendweight
	0,//normal
	0,//color0
	1,//color1
	0,//fog
	0,//psize
	0,//blendindices
	0,//texcoord0
	1,//texcoord1
	2,//texcoord2
	3,//texcoord3
	4,//texcoord4
	5,//texcoord5
	6,//texcoord6/tangent
	7//texcoord7/binormal
};

struct SRefVertexAttribute
{
	uint16		m_SemanticNameIndex;//
	uint8			m_InputOffset;//
	uint8			m_Size;//
};


/////////////////////////////////////////////////////
//Header
/////////////////////////////////////////////////////
struct SShaderDesc
{
	//<header>
	uint32												m_Version;
	uint16												m_PixelShader;//ShaderType -> vertex=0/pixel=1

	uint16												m_VertexAttributeMask;

	uint16												m_VertexAttributeOffset;
	uint16												m_VertexAttributeCount;

	uint16												m_SamplerOffset;
	uint16												m_SamplerCount;

	uint16												m_ConstOffset;
	uint16												m_ConstCount;

	uint16												m_VSConstOffset;
	uint16												m_VSConstCount;

	uint16												m_PatchOffset[MAX_SLOTCOUNT];
	uint16												m_PatchCount[MAX_SLOTCOUNT];

	uint16												m_ShaderBinaryOffset;
	uint16												m_ShaderBinarySize;

	uint16												m_NameTableOffset;
	uint16												m_NameTableSize;
//	uint8													m_Body[1];
} _ALIGN(128);

#endif
