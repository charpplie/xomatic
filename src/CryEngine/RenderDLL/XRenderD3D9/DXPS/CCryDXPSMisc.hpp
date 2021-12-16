#ifndef __CRYDXPSMISC__
#define __CRYDXPSMISC__

class CCryDXPSBlob;
typedef CCryDXPSBlob ID3D10Blob;

#ifndef __SPU__
	#include <StdAfx.h>
#endif
#include "CCryDXPSGuid.hpp"
#include "CCryTypes.hpp"
//#include "Include/d3d9.h"

#define __in
#define __out
#define __out_opt
#define __inout_opt
#define __out_ecount_opt(X)
#define D3D11_RECT CryRect
#ifdef NO_ILINE
	#define FORCEINLINE inline
#else
	#define FORCEINLINE __attribute__((always_inline)) inline
#endif

#define GetClientRect CryGetClientRect

typedef  uint32 HMONITOR;
//typedef const struct CryRect* LPCRECT;

//cannot be uin16!!!!!!!! cause of some vertexbuffer copies >65535 in copy sub resource
typedef struct D3D11_BOX
{
	uint32 left;
	uint32 top;
	uint32 front;
	uint32 right;
	uint32 bottom;
	uint32 back;
};

//typedef uint32 FOURCC;
typedef uint32 D3DCOLOR;

#undef SUCCEEDED
#define SUCCEEDED(x) ((x)>=0)
#define FAILED(x) (!(SUCCEEDED(x)))
#define D3DCOLOR_ARGB(a,r,g,b) ((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)
#define D3DCOLOR_XRGB(r,g,b)   D3DCOLOR_ARGB(0xff,r,g,b)
#define D3DCOLOR_COLORVALUE(r,g,b,a)   D3DCOLOR_RGBA((unsigned int)((r)*255.f),(unsigned int)((g)*255.f),(unsigned int)((b)*255.f),(unsigned int)((a)*255.f))

#define D3DSTREAMSOURCE_INDEXEDDATA  (1<<30)
#define D3DSTREAMSOURCE_INSTANCEDATA (2<<30)
#define S_OK                         0
#define S_FALSE                      1


#define	D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT					16
#define	D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT	128
#define D3D11_COMMONSHADER_CONSTANT_BUFFER_SLOT_COUNT 15
#define	D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT			16
#define	D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT						32



#define D3D11_BYTES_FROM_BITS(x) (((x)+7)/8)


#define __stdcall


//////////////////////////////////////////////////////////////////////////
//enums
//////////////////////////////////////////////////////////////////////////
enum {D3D11_SDK_VERSION = 0x0001};

enum D3D11_PRIMITIVE_TOPOLOGY
{
	D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED					=	0,
	D3D11_PRIMITIVE_TOPOLOGY_POINTLIST					=	1,
	D3D11_PRIMITIVE_TOPOLOGY_LINELIST						=	2,
	D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP					=	3,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST				=	4,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP			=	5,
	D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ				=	10,
	D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ			=	11,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ		=	12,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ	=	13,
};

enum D3D11_RESOURCE_DIMENSION
{
	D3D11_RESOURCE_DIMENSION_UNKNOWN						= 0,
	D3D11_RESOURCE_DIMENSION_BUFFER							= 1,
	D3D11_RESOURCE_DIMENSION_TEXTURE1D					= 2,
	D3D11_RESOURCE_DIMENSION_TEXTURE2D					= 3,
	D3D11_RESOURCE_DIMENSION_TEXTURE3D					= 4
};


enum D3D11_CBUFFER_TYPE
{
	D3D11_CT_CBUFFER,
	D3D11_CT_TBUFFER,
};

enum D3D11_CREATE_DEVICE_FLAG
{	
	D3D11_CREATE_DEVICE_SINGLETHREADED	= 0x1,
	D3D11_CREATE_DEVICE_SHADER_MIRROR		= 0x2,
	D3D11_CREATE_DEVICE_DEBUG						=	0x4,
	D3D11_CREATE_DEVICE_SWITCH_TO_REF		=	0x8,
};

enum D3D11_RESOURCE_MISC_FLAG
{
  D3D11_RESOURCE_MISC_GENERATE_MIPS = 0x1L,
  D3D11_RESOURCE_MISC_SHARED = 0x2L,
  D3D11_RESOURCE_MISC_TEXTURECUBE = 0x4L,
  D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS = 0x10L,
  D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS = 0x20L,
  D3D11_RESOURCE_MISC_BUFFER_STRUCTURED = 0x40L,
  D3D11_RESOURCE_MISC_RESOURCE_CLAMP = 0x80L,
  D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX = 0x100L,
  D3D11_RESOURCE_MISC_GDI_COMPATIBLE = 0x200L,
};



enum D3D_DRIVER_TYPE
{
		D3D_DRIVER_TYPE_HARDWARE  = 0,
		D3D_DRIVER_TYPE_REFERENCE = 1,
		D3D_DRIVER_TYPE_NULL      = 2,
	D3D_DRIVER_TYPE_SOFTWARE  = 3,
		D3D_DRIVER_TYPE_NETWORK		= 4,
};

enum DXGI_SWAP_EFFECT
{	
	DXGI_SWAP_EFFECT_DISCARD			=	0,
	DXGI_SWAP_EFFECT_SEQUENTIAL		=	1
};

enum DXGI_SWAP_CHAIN_FLAG
{
  	DXGI_SWAP_CHAIN_FLAG_NONPREROTATED	= 1,
	DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH	= 2
};

enum DXGI_FORMAT
{
	DXGI_FORMAT_UNKNOWN										= 0,
	DXGI_FORMAT_R32G32B32A32_TYPELESS			= 1,
	DXGI_FORMAT_R32G32B32A32_FLOAT				= 2,
	DXGI_FORMAT_R32G32B32A32_UINT					= 3,
	DXGI_FORMAT_R32G32B32A32_SINT					= 4,
	DXGI_FORMAT_R32G32B32_TYPELESS				= 5,
	DXGI_FORMAT_R32G32B32_FLOAT						= 6,
	DXGI_FORMAT_R32G32B32_UINT						= 7,
	DXGI_FORMAT_R32G32B32_SINT						= 8,
	DXGI_FORMAT_R16G16B16A16_TYPELESS			= 9,
	DXGI_FORMAT_R16G16B16A16_FLOAT				= 10,
	DXGI_FORMAT_R16G16B16A16_UNORM				= 11,
	DXGI_FORMAT_R16G16B16A16_UINT					= 12,
	DXGI_FORMAT_R16G16B16A16_SNORM				= 13,
	DXGI_FORMAT_R16G16B16A16_SINT					= 14,
	DXGI_FORMAT_R32G32_TYPELESS						= 15,
	DXGI_FORMAT_R32G32_FLOAT							= 16,
	DXGI_FORMAT_R32G32_UINT								= 17,
	DXGI_FORMAT_R32G32_SINT								= 18,
	DXGI_FORMAT_R32G8X24_TYPELESS					= 19,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT			= 20,
	DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS	= 21,
	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT		= 22,
	DXGI_FORMAT_R10G10B10A2_TYPELESS			= 23,
	DXGI_FORMAT_R10G10B10A2_UNORM					= 24,
	DXGI_FORMAT_R10G10B10A2_UINT					= 25,
	DXGI_FORMAT_R11G11B10_FLOAT						= 26,
	DXGI_FORMAT_R8G8B8A8_TYPELESS					= 27,
	DXGI_FORMAT_R8G8B8A8_UNORM						= 28,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB				= 29,
	DXGI_FORMAT_R8G8B8A8_UINT							= 30,
	DXGI_FORMAT_R8G8B8A8_SNORM						= 31,
	DXGI_FORMAT_R8G8B8A8_SINT							= 32,
	DXGI_FORMAT_R16G16_TYPELESS						= 33,
	DXGI_FORMAT_R16G16_FLOAT							= 34,
	DXGI_FORMAT_R16G16_UNORM							= 35,
	DXGI_FORMAT_R16G16_UINT								= 36,
	DXGI_FORMAT_R16G16_SNORM							= 37,
	DXGI_FORMAT_R16G16_SINT								= 38,
	DXGI_FORMAT_R32_TYPELESS							= 39,
	DXGI_FORMAT_D32_FLOAT									= 40,
	DXGI_FORMAT_R32_FLOAT									= 41,
	DXGI_FORMAT_R32_UINT									= 42,
	DXGI_FORMAT_R32_SINT									= 43,
	DXGI_FORMAT_R24G8_TYPELESS						= 44,
	DXGI_FORMAT_D24_UNORM_S8_UINT					= 45,
	DXGI_FORMAT_R24_UNORM_X8_TYPELESS			= 46,
	DXGI_FORMAT_X24_TYPELESS_G8_UINT			= 47,
	DXGI_FORMAT_R8G8_TYPELESS							= 48,
	DXGI_FORMAT_R8G8_UNORM								= 49,
	DXGI_FORMAT_R8G8_UINT									= 50,
	DXGI_FORMAT_R8G8_SNORM								= 51,
	DXGI_FORMAT_R8G8_SINT									= 52,
	DXGI_FORMAT_R16_TYPELESS							= 53,
	DXGI_FORMAT_R16_FLOAT									= 54,
	DXGI_FORMAT_D16_UNORM									= 55,
	DXGI_FORMAT_R16_UNORM									= 56,
	DXGI_FORMAT_R16_UINT									= 57,
	DXGI_FORMAT_R16_SNORM									= 58,
	DXGI_FORMAT_R16_SINT									= 59,
	DXGI_FORMAT_R8_TYPELESS								= 60,
	DXGI_FORMAT_R8_UNORM									= 61,
	DXGI_FORMAT_R8_UINT										= 62,
	DXGI_FORMAT_R8_SNORM									= 63,
	DXGI_FORMAT_R8_SINT										= 64,
	DXGI_FORMAT_A8_UNORM									= 65,
	DXGI_FORMAT_R1_UNORM									= 66,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP				= 67,
	DXGI_FORMAT_R8G8_B8G8_UNORM						= 68,
	DXGI_FORMAT_G8R8_G8B8_UNORM						= 69,
	DXGI_FORMAT_BC1_TYPELESS							= 70,
	DXGI_FORMAT_BC1_UNORM									= 71,
	DXGI_FORMAT_BC1_UNORM_SRGB						= 72,
	DXGI_FORMAT_BC2_TYPELESS							= 73,
	DXGI_FORMAT_BC2_UNORM									= 74,
	DXGI_FORMAT_BC2_UNORM_SRGB						= 75,
	DXGI_FORMAT_BC3_TYPELESS							= 76,
	DXGI_FORMAT_BC3_UNORM									= 77,
	DXGI_FORMAT_BC3_UNORM_SRGB						= 78,
	DXGI_FORMAT_BC4_TYPELESS							= 79,
	DXGI_FORMAT_BC4_UNORM									= 80,
	DXGI_FORMAT_BC4_SNORM									= 81,
	DXGI_FORMAT_BC5_TYPELESS							= 82,
	DXGI_FORMAT_BC5_UNORM									= 83,
	DXGI_FORMAT_BC5_SNORM									= 84,
	DXGI_FORMAT_B5G6R5_UNORM							= 85,
	DXGI_FORMAT_B5G5R5A1_UNORM						= 86,
	DXGI_FORMAT_B8G8R8A8_UNORM						= 87,
	DXGI_FORMAT_B8G8R8X8_UNORM						= 88,
	DXGI_FORMAT_FORCE_UINT								= 0xffffffffUL
};

enum DXGI_MODE_SCANLINE_ORDER
{
	DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED				= 0,
	DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE				= 1,
	DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST	= 2,
	DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST	= 3
};

enum DXGI_MODE_SCALING
{
	DXGI_MODE_SCALING_UNSPECIFIED								= 0,
	DXGI_MODE_SCALING_CENTERED									= 1,
	DXGI_MODE_SCALING_STRETCHED									= 2
};

enum DXGI_MODE_ROTATION
{	DXGI_MODE_ROTATION_UNSPECIFIED							= 0,
	DXGI_MODE_ROTATION_IDENTITY									= 1,
	DXGI_MODE_ROTATION_ROTATE90									= 2,
	DXGI_MODE_ROTATION_ROTATE180								= 3,
	DXGI_MODE_ROTATION_ROTATE270								= 4
};

enum D3D11_MAP
{
	D3D11_MAP_READ										= 1,
	D3D11_MAP_WRITE										= 2,
	D3D11_MAP_READ_WRITE							= 3,
	D3D11_MAP_WRITE_DISCARD						= 4,
	D3D11_MAP_WRITE_NO_OVERWRITE			= 5,
	D3D11_MAP_VOID										= ~0
};

enum D3D11_USAGE
{
	D3D11_USAGE_DEFAULT								= 0,
	D3D11_USAGE_IMMUTABLE							= 1,
	D3D11_USAGE_DYNAMIC								= 2,
	D3D11_USAGE_STAGING								= 3
};

enum DXGI_USAGE
{
		DXGI_CPU_ACCESS_NONE						=	0,
		DXGI_CPU_ACCESS_DYNAMIC					=	1,
		DXGI_CPU_ACCESS_READ_WRITE			= 2,
		DXGI_CPU_ACCESS_SCRATCH					=	3,
		DXGI_CPU_ACCESS_FIELD						=	15,
		DXGI_USAGE_SHADER_INPUT         =	1L<<4,
		DXGI_USAGE_RENDER_TARGET_OUTPUT =	1L<<5,
		DXGI_USAGE_BACK_BUFFER          =	1L<<6,
		DXGI_USAGE_SHARED               =	1L<<7,
		DXGI_USAGE_READ_ONLY            =	1L<<8,
};


enum D3D11_INPUT_CLASSIFICATION
{
	D3D11_INPUT_PER_VERTEX_DATA			=	0,
	D3D11_INPUT_PER_INSTANCE_DATA		= 1
};

enum D3D11_BIND_FLAG
{
	D3D11_BIND_VERTEX_BUFFER		= 0x01,
	D3D11_BIND_INDEX_BUFFER			= 0x02,
	D3D11_BIND_CONSTANT_BUFFER	= 0x04,
	D3D11_BIND_SHADER_RESOURCE	= 0x08,
	D3D11_BIND_STREAM_OUTPUT		= 0x10,
	D3D11_BIND_RENDER_TARGET		= 0x20,
	D3D11_BIND_DEPTH_STENCIL		= 0x40,
};

enum D3D11_RTV_DIMENSION
{
	D3D11_RTV_DIMENSION_UNKNOWN						= 0,
	D3D11_RTV_DIMENSION_BUFFER						= 1,
	D3D11_RTV_DIMENSION_TEXTURE1D					= 2,
	D3D11_RTV_DIMENSION_TEXTURE1DARRAY		= 3,
	D3D11_RTV_DIMENSION_TEXTURE2D					= 4,
	D3D11_RTV_DIMENSION_TEXTURE2DARRAY		= 5,
	D3D11_RTV_DIMENSION_TEXTURE2DMS				= 6,
	D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY	= 7,
	D3D11_RTV_DIMENSION_TEXTURE3D					= 8,
};

enum D3D11_CPU_ACCESS_FLAG
{
	D3D11_CPU_ACCESS_WRITE								= 0x10000L,
	D3D11_CPU_ACCESS_READ									= 0x20000L
};

enum D3D11_BLEND
{
  D3D11_BLEND_ZERO											= 1,
  D3D11_BLEND_ONE 											= 2,
  D3D11_BLEND_SRC_COLOR 								= 3,
  D3D11_BLEND_INV_SRC_COLOR 						= 4,
  D3D11_BLEND_SRC_ALPHA 								= 5,
  D3D11_BLEND_INV_SRC_ALPHA 						= 6,
  D3D11_BLEND_DEST_ALPHA  							= 7,
  D3D11_BLEND_INV_DEST_ALPHA  					= 8,
  D3D11_BLEND_DEST_COLOR  							= 9,
  D3D11_BLEND_INV_DEST_COLOR  					= 10,
  D3D11_BLEND_SRC_ALPHA_SAT  						= 11,
  D3D11_BLEND_BLEND_FACTOR  						= 14,
  D3D11_BLEND_INV_BLEND_FACTOR  				= 15,
  D3D11_BLEND_SRC1_COLOR  							= 16,
  D3D11_BLEND_INV_SRC1_COLOR  					= 17,
  D3D11_BLEND_SRC1_ALPHA								= 18,
  D3D11_BLEND_INV_SRC1_ALPHA  					= 19,
};

enum D3D11_BLEND_OP
{
  D3D11_BLEND_OP_ADD										= 1,
  D3D11_BLEND_OP_SUBTRACT								= 2,
  D3D11_BLEND_OP_REV_SUBTRACT						= 3,
  D3D11_BLEND_OP_MIN										= 4,
	D3D11_BLEND_OP_MAX										= 5,
	D3D11_BLEND_OP_ADD_SIGNED							= 6,
};

enum D3D11_FILL_MODE
{
  D3D11_FILL_POINTPS3										= 1,
  D3D11_FILL_WIREFRAME									= 2,
  D3D11_FILL_SOLID											= 3,
};

enum D3D11_CULL_MODE
{
  D3D11_CULL_NONE												= 1,
  D3D11_CULL_FRONT											= 2,
  D3D11_CULL_BACK 											= 3,
};

enum D3D11_COMPARISON_FUNC
{
	D3D11_COMPARISON_NEVER								= 1,
	D3D11_COMPARISON_LESS									= 2,
	D3D11_COMPARISON_EQUAL								= 3,
	D3D11_COMPARISON_LESS_EQUAL						= 4,
	D3D11_COMPARISON_GREATER							= 5,
	D3D11_COMPARISON_NOT_EQUAL						= 6,
	D3D11_COMPARISON_GREATER_EQUAL				= 7,
	D3D11_COMPARISON_ALWAYS								= 8,
};

enum D3D11_DEPTH_WRITE_MASK
{
	D3D11_DEPTH_WRITE_MASK_ZERO						= 0,
	D3D11_DEPTH_WRITE_MASK_ALL						= 1,
};

enum D3D11_STENCIL_OP
{
	D3D11_STENCIL_OP_KEEP									= 1,
	D3D11_STENCIL_OP_ZERO 								= 2,
	D3D11_STENCIL_OP_REPLACE 							= 3,
	D3D11_STENCIL_OP_INCR_SAT 						= 4,
	D3D11_STENCIL_OP_DECR_SAT 						= 5,
	D3D11_STENCIL_OP_INVERT 							= 6,
	D3D11_STENCIL_OP_INCR 								= 7,
	D3D11_STENCIL_OP_DECR 								= 8,
};

enum D3D11_DSV_DIMENSION
{
	D3D11_DSV_DIMENSION_UNKNOWN						= 0,
	D3D11_DSV_DIMENSION_TEXTURE1D					= 1,
	D3D11_DSV_DIMENSION_TEXTURE1DARRAY		= 2,
	D3D11_DSV_DIMENSION_TEXTURE2D					= 3,
	D3D11_DSV_DIMENSION_TEXTURE2DARRAY		= 4,
	D3D11_DSV_DIMENSION_TEXTURE2DMS				= 5,
	D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY	= 6,
};

enum D3D10_SHADER_INPUT_TYPE
{
	D3D10_SIT_CBUFFER											=	0,
	D3D10_SIT_TBUFFER											=	1,
	D3D10_SIT_TEXTURE											=	2,
	D3D10_SIT_SAMPLER											=	3,
};

enum D3D11_RESOURCE_RETURN_TYPE
{
	D3D11_RETURN_TYPE_UNORM								= 1,
	D3D11_RETURN_TYPE_SNORM								= 2,
	D3D11_RETURN_TYPE_SINT								= 3,
	D3D11_RETURN_TYPE_UINT								= 4,
	D3D11_RETURN_TYPE_FLOAT								= 5,
	D3D11_RETURN_TYPE_MIXED								= 6,
};

enum D3D10_SRV_DIMENSION
{
    D3D10_SRV_DIMENSION_UNKNOWN					= 0,
    D3D10_SRV_DIMENSION_BUFFER					= 1,
    D3D10_SRV_DIMENSION_TEXTURE1D				= 2,
    D3D10_SRV_DIMENSION_TEXTURE1DARRAY	= 3,
    D3D10_SRV_DIMENSION_TEXTURE2D				= 4,
    D3D10_SRV_DIMENSION_TEXTURE2DARRAY	= 5,
    D3D10_SRV_DIMENSION_TEXTURE2DMS			= 6,
    D3D10_SRV_DIMENSION_TEXTURE2DMSARRAY= 7,
    D3D10_SRV_DIMENSION_TEXTURE3D				= 8,
    D3D10_SRV_DIMENSION_TEXTURECUBE			= 9,
};

enum D3D10_SHADER_VARIABLE_CLASS
{
  D3D10_SVC_SCALAR,
  D3D10_SVC_VECTOR,
  D3D10_SVC_MATRIX_ROWS,
  D3D10_SVC_MATRIX_COLUMNS,
  D3D10_SVC_OBJECT,
  D3D10_SVC_STRUCT,
  D3D10_SVC_FORCE_DWORD = 0x7fffffff,
};

enum D3D10_SHADER_VARIABLE_TYPE
{
	D3D11_SVT_VOID													= 0,
	D3D11_SVT_BOOL													= 1,
	D3D11_SVT_INT														= 2,
	D3D11_SVT_FLOAT													= 3,
	D3D11_SVT_STRING												= 4,
	D3D11_SVT_TEXTURE												= 5,
	D3D11_SVT_TEXTURE1D											= 6,
	D3D11_SVT_TEXTURE2D 										= 7,
	D3D11_SVT_TEXTURE3D 										= 8,
	D3D11_SVT_TEXTURECUBE 									= 9,
	D3D11_SVT_SAMPLER												= 10,
	D3D11_SVT_PIXELSHADER 									= 15,
	D3D11_SVT_VERTEXSHADER									= 16,
	D3D11_SVT_UINT													= 19,
	D3D11_SVT_UINT8													= 20,
	D3D11_SVT_GEOMETRYSHADER								= 21,
	D3D11_SVT_RASTERIZER										= 22,
	D3D11_SVT_DEPTHSTENCIL									= 23,
	D3D11_SVT_BLEND													= 24,
	D3D11_SVT_BUFFER												= 25,
	D3D11_SVT_CBUFFER 											= 26,
	D3D11_SVT_TBUFFER 											= 27,
	D3D11_SVT_TEXTURE1DARRAY								= 28,
	D3D11_SVT_TEXTURE2DARRAY								= 29,
	D3D11_SVT_RENDERTARGETVIEW							= 30,
	D3D11_SVT_DEPTHSTENCILVIEW							= 31,
	D3D11_SVT_FORCE_DWORD										= 0x7fffffff,
};

enum D3D10_SHADER_CBUFFER_FLAGS
{
	D3D10_CBF_USERPACKED										= 1,
	D3D11_CBF_FORCE_DWORD										= 0x7fffffff,
};

enum D3D10_SHADER_VARIABLE_FLAGS
{
	D3D10_SVF_USERPACKED										= 1,
	D3D10_SVF_USED													= 2,
	D3D10_SVF_FORCE_DWORD 									= 0x7fffffff,
};

enum D3D10_SHADER_INPUT_FLAGS
{
	D3D10_SIF_USERPACKED										= 1,
	D3D10_SIF_COMPARISON_SAMPLER						= 2,
	D3D10_SIF_FORCE_DWORD										= 0x7fffffff,
};

enum D3D11_CLEAR_FLAG
{
	D3D11_CLEAR_DEPTH												= 0x1,
	D3D11_CLEAR_STENCIL											= 0x2,
	D3D11_CLEAR_ZCULL												= 0x4,
};

enum D3D11_PASS
{
	D3D11_PASS_ZONLY												= 0x1,
	D3D11_PASS_ZCULLUPDATE									= 0x2,
	D3D11_PASS_DIFFUSE											= 0x3,
	D3D11_PASS_STENCIL_REFRESH              = 0x4,
	D3D11_PASS_DONESHADOWLAYER0							= 0x5,
	D3D11_PASS_DONESHADOWLAYER1							= 0x6,
	D3D11_PASS_DONESHADOWLAYER2							= 0x7,
};

enum D3D11_COLOR_WRITE_ENABLE
{
	D3D11_COLOR_WRITE_ENABLE_RED						= 0x1,
	D3D11_COLOR_WRITE_ENABLE_GREEN					= 0x2,
	D3D11_COLOR_WRITE_ENABLE_BLUE						= 0x4,
	D3D11_COLOR_WRITE_ENABLE_ALPHA					= 0x8,
	D3D11_COLOR_WRITE_ENABLE_ALL						=	0xf,
};

enum D3D10_NAME 
{
	D3D11_NAME_UNDEFINED										= 0,
	D3D11_NAME_POSITION											= 1,
	D3D11_NAME_CLIP_DISTANCE								= 2,
	D3D11_NAME_CULL_DISTANCE								= 3,
	D3D11_NAME_RENDER_TARGET_ARRAY_INDEX		= 4,
	D3D11_NAME_VIEWPORT_ARRAY_INDEX					= 5,
	D3D11_NAME_VERTEX_ID										= 6,
	D3D11_NAME_PRIMITIVE_ID									= 7,
	D3D11_NAME_INSTANCE_ID									= 8,
	D3D11_NAME_IS_FRONT_FACE								= 9,
	D3D11_NAME_TARGET												= 64,
	D3D11_NAME_DEPTH												= 65,
};

enum D3D11_REGISTER_COMPONENT_TYPE
{
	D3D11_REGISTER_COMPONENT_UNKNOWN				= 0,
	D3D11_REGISTER_COMPONENT_UINT32					= 1,
	D3D11_REGISTER_COMPONENT_SINT32					= 2,
	D3D11_REGISTER_COMPONENT_FLOAT32				= 3,
};

enum D3D11_TEXTURE_ADDRESS_MODE
{
	D3D11_TEXTURE_ADDRESS_WRAP							= 1,
	D3D11_TEXTURE_ADDRESS_MIRROR						= 2,
	D3D11_TEXTURE_ADDRESS_CLAMP							= 3,
	D3D11_TEXTURE_ADDRESS_BORDER						= 4,
	D3D11_TEXTURE_ADDRESS_MIRROR_ONCE				= 5,
};

#define D3DTADDRESS_WRAP D3D11_TEXTURE_ADDRESS_WRAP

enum D3D11_FILTER
{
	D3D11_FILTER_MIN_MAG_MIP_POINT													= 0,
	D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR										= 0x1,
	D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT							= 0x4,
	D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR 									= 0x5,
	D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT 									= 0x10,
	D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR						= 0x11,
	D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT 									= 0x14,
	D3D11_FILTER_MIN_MAG_MIP_LINEAR 												= 0x15,
	D3D11_FILTER_ANISOTROPIC																= 0x55,
	D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT								= 0x80,
	D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR				= 0x81,
	D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x84,
	D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR 				= 0x85,
	D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT 				= 0x90,
	D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
	D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT				= 0x94,
	D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR							= 0x95,
	D3D11_FILTER_COMPARISON_ANISOTROPIC 										= 0xd5,
	D3D11_FILTER_TEXT_1BIT																	= 0x80000000,
};

enum D3DSWAPEFFECT
{
    D3DSWAPEFFECT_DISCARD																	= 1,
    D3DSWAPEFFECT_FLIP 																		= 2,
    D3DSWAPEFFECT_COPY 																		= 3,
    D3DSWAPEFFECT_FORCE_DWORD 														= 0xFFFFFFFF,
};

enum D3D11_SHADER_Constants
{
	D3D10_SHADER_DEBUG 																			=	1,
	D3D10_SHADER_SKIP_VALIDATION 														=	1<<1,
	D3D10_SHADER_SKIP_OPTIMIZATION 													=	1<<2,
	D3D10_SHADER_PACK_MATRIX_ROW_MAJOR 											=	1<<3,
	D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR 									=	1<<4,
	D3D10_SHADER_PARTIAL_PRECISION 													=	1<<5,
	D3D10_SHADER_FORCE_VS_SOFTWARE_NO_OPT 									=	1<<6,
	D3D10_SHADER_FORCE_PS_SOFTWARE_NO_OPT 									=	1<<7,
	D3D10_SHADER_NO_PRESHADER 															=	1<<8,
	D3D10_SHADER_AVOID_FLOW_CONTROL 												=	1<<9,
	D3D10_SHADER_PREFER_FLOW_CONTROL 												=	1<<10,
	D3D10_SHADER_ENABLE_STRICTNESS 													=	1<<11,
	D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY 						=	1<<12,
	D3D10_SHADER_IEEE_STRICTNESS 														=	1<<13,
	D3D10_SHADER_OPTIMIZATION_LEVEL0 												=	1<<14,
	D3D10_SHADER_OPTIMIZATION_LEVEL1 												=	0,
	D3D10_SHADER_OPTIMIZATION_LEVEL2 												=	(1<<14) | (1<<15),
	D3D10_SHADER_OPTIMIZATION_LEVEL3 												=	1<<15,
};

enum D3DX11_IMAGE_FILE_FORMAT
{
	D3DX11_IFF_BMP																					= 0,
	D3DX11_IFF_JPG 																					= 1,
	D3DX11_IFF_PNG 																					= 3,
	D3DX11_IFF_DDS 																					= 4,
	D3DX11_IFF_TIFF 																				= 10,
	D3DX11_IFF_GIF 																					= 11,
	D3DX11_IFF_WMP 																					= 12,
	D3DX11_IFF_FORCE_DWORD																	= 0x7fffffff,
};

enum D3DTEXTUREFILTERTYPE
{
	D3DTEXF_NONE																						= 0,
	D3DTEXF_POINT																						= 1,
	D3DTEXF_LINEAR																					= 2,
	D3DTEXF_ANISOTROPIC 																		= 3,
	D3DTEXF_PYRAMIDALQUAD 																	= 6,
	D3DTEXF_GAUSSIANQUAD																		= 7,
	D3DTEXF_FORCE_DWORD																			= 0x7fffffff,
};

enum D3DMULTISAMPLE_TYPE
{
	D3DMULTISAMPLE_NONE																			= 0,
	D3DMULTISAMPLE_NONMASKABLE  														= 1,
	D3DMULTISAMPLE_2_SAMPLES 																= 2,
	D3DMULTISAMPLE_3_SAMPLES 																= 3,
	D3DMULTISAMPLE_4_SAMPLES 																= 4,
	D3DMULTISAMPLE_5_SAMPLES 																= 5,
	D3DMULTISAMPLE_6_SAMPLES 																= 6,
	D3DMULTISAMPLE_7_SAMPLES 																= 7,
	D3DMULTISAMPLE_8_SAMPLES 																= 8,
	D3DMULTISAMPLE_9__SAMPLES 															= 9,
	D3DMULTISAMPLE_10_SAMPLES 															= 10,
	D3DMULTISAMPLE_11_SAMPLES 															= 11,
	D3DMULTISAMPLE_12_SAMPLES 															= 12,
	D3DMULTISAMPLE_13_SAMPLES 															= 13,
	D3DMULTISAMPLE_14_SAMPLES 															= 14,
	D3DMULTISAMPLE_15_SAMPLES 															= 15,
	D3DMULTISAMPLE_16_SAMPLES 															= 16,
	D3DMULTISAMPLE_FORCE_DWORD															= 0xffffffff,
};

typedef enum D3D11_QUERY
{
	D3D11_QUERY_EVENT																				= 0,
	D3D11_QUERY_OCCLUSION 																	= ( D3D11_QUERY_EVENT + 1 ),
	D3D11_QUERY_TIMESTAMP 																	= ( D3D11_QUERY_OCCLUSION + 1 ),
	D3D11_QUERY_TIMESTAMP_DISJOINT													= ( D3D11_QUERY_TIMESTAMP + 1 ),
	D3D11_QUERY_PIPELINE_STATISTICS													= ( D3D11_QUERY_TIMESTAMP_DISJOINT + 1 ),
	D3D11_QUERY_OCCLUSION_PREDICATE													= ( D3D11_QUERY_PIPELINE_STATISTICS + 1 ),
	D3D11_QUERY_SO_STATISTICS																= ( D3D11_QUERY_OCCLUSION_PREDICATE + 1 ),
	D3D11_QUERY_SO_OVERFLOW_PREDICATE 											= ( D3D11_QUERY_SO_STATISTICS + 1 ),
};

typedef enum D3DCUBEMAP_FACES
{
	D3DCUBEMAP_FACE_POSITIVE_X															= 0,
	D3DCUBEMAP_FACE_NEGATIVE_X															= 1,
	D3DCUBEMAP_FACE_POSITIVE_Y															= 2,
	D3DCUBEMAP_FACE_NEGATIVE_Y															= 3,
	D3DCUBEMAP_FACE_POSITIVE_Z															= 4,
	D3DCUBEMAP_FACE_NEGATIVE_Z															= 5,
	D3DCUBEMAP_FACE_FORCE_DWORD 														= 0xffffffff,
};

typedef enum D3D11_FORMAT_SUPPORT
{
	D3D11_FORMAT_SUPPORT_BUFFER															= 0x1,
	D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER 									= 0x2,
	D3D11_FORMAT_SUPPORT_IA_INDEX_BUFFER										= 0x4,
	D3D11_FORMAT_SUPPORT_SO_BUFFER	 												= 0x8,
	D3D11_FORMAT_SUPPORT_TEXTURE1D 													= 0x10,
	D3D11_FORMAT_SUPPORT_TEXTURE2D 													= 0x20,
	D3D11_FORMAT_SUPPORT_TEXTURE3D 													= 0x40,
	D3D11_FORMAT_SUPPORT_TEXTURECUBE												= 0x80,
	D3D11_FORMAT_SUPPORT_SHADER_LOAD												= 0x100,
	D3D11_FORMAT_SUPPORT_SHADER_SAMPLE											= 0x200,
	D3D11_FORMAT_SUPPORT_SHADER_SAMPLE_COMPARISON						= 0x400,
	D3D11_FORMAT_SUPPORT_SHADER_SAMPLE_MONO_TEXT						= 0x800,
	D3D11_FORMAT_SUPPORT_MIP																= 0x1000,
	D3D11_FORMAT_SUPPORT_MIP_AUTOGEN												= 0x2000,
	D3D11_FORMAT_SUPPORT_RENDER_TARGET											= 0x4000,
	D3D11_FORMAT_SUPPORT_BLENDABLE													= 0x8000,
	D3D11_FORMAT_SUPPORT_DEPTH_STENCIL											= 0x10000,
	D3D11_FORMAT_SUPPORT_CPU_LOCKABLE												= 0x20000,
	D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE								= 0x40000,
	D3D11_FORMAT_SUPPORT_DISPLAY														= 0x80000,
	D3D11_FORMAT_SUPPORT_CAST_WITHIN_BIT_LAYOUT 						= 0x100000,
	D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET 					= 0x200000,
	D3D11_FORMAT_SUPPORT_MULTISAMPLE_LOAD										= 0x400000,
};


typedef enum D3DX11_FILTER_FLAG
{
	D3DX11_FILTER_NONE            =   (1 << 0),
	D3DX11_FILTER_POINT           =   (2 << 0),
	D3DX11_FILTER_LINEAR          =   (3 << 0),
	D3DX11_FILTER_TRIANGLE        =   (4 << 0),
	D3DX11_FILTER_BOX             =   (5 << 0),

	D3DX11_FILTER_MIRROR_U        =   (1 << 16),
	D3DX11_FILTER_MIRROR_V        =   (2 << 16),
	D3DX11_FILTER_MIRROR_W        =   (4 << 16),
	D3DX11_FILTER_MIRROR          =   (7 << 16),

	D3DX11_FILTER_DITHER          =   (1 << 19),
	D3DX11_FILTER_DITHER_DIFFUSION=   (2 << 19),
} D3DX11_FILTER_FLAG;


enum D3D10_DEVICE_STATE_TYPES
{
	D3D11_DST_SO_BUFFERS					= 1,
	D3D11_DST_OM_RENDER_TARGETS,
	D3D11_DST_OM_DEPTH_STENCIL_STATE,
	D3D11_DST_OM_BLEND_STATE,
	D3D11_DST_VS,
	D3D11_DST_VS_SAMPLERS,
	D3D11_DST_VS_SHADER_RESOURCES,
	D3D11_DST_VS_CONSTANT_BUFFERS,
	D3D11_DST_GS,
	D3D11_DST_GS_SAMPLERS,
	D3D11_DST_GS_SHADER_RESOURCES,
	D3D11_DST_GS_CONSTANT_BUFFERS,
	D3D11_DST_PS,
	D3D11_DST_PS_SAMPLERS,
	D3D11_DST_PS_SHADER_RESOURCES,
	D3D11_DST_PS_CONSTANT_BUFFERS,
	D3D11_DST_IA_VERTEX_BUFFERS,
	D3D11_DST_IA_INDEX_BUFFER,
	D3D11_DST_IA_INPUT_LAYOUT,
	D3D11_DST_IA_PRIMITIVE_TOPOLOGY,
	D3D11_DST_RS_VIEWPORTS,
	D3D11_DST_RS_SCISSOR_RECTS,
	D3D11_DST_RS_RASTERIZER_STATE,
	D3D11_DST_PREDICATION,
};

// DirectX 11 additions

typedef enum D3D11_FEATURE
{	
	D3D11_FEATURE_THREADING	= 0,
	D3D11_FEATURE_DOUBLES	= ( D3D11_FEATURE_THREADING + 1 ) ,
	D3D11_FEATURE_FORMAT_SUPPORT	= ( D3D11_FEATURE_DOUBLES + 1 ) ,
	D3D11_FEATURE_FORMAT_SUPPORT2	= ( D3D11_FEATURE_FORMAT_SUPPORT + 1 ) ,
	D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS	= ( D3D11_FEATURE_FORMAT_SUPPORT2 + 1 ) 
} 	D3D11_FEATURE;

typedef enum D3D_FEATURE_LEVEL
{
	D3D_FEATURE_LEVEL_9_1	= 0x9100,
	D3D_FEATURE_LEVEL_9_2	= 0x9200,
	D3D_FEATURE_LEVEL_9_3	= 0x9300,
	D3D_FEATURE_LEVEL_10_0	= 0xa000,
	D3D_FEATURE_LEVEL_10_1	= 0xa100,
	D3D_FEATURE_LEVEL_11_0	= 0xb000
} 	D3D_FEATURE_LEVEL;

typedef enum D3D11_SRV_DIMENSION
{	
	D3D11_SRV_DIMENSION_UNKNOWN	= 0,
	D3D11_SRV_DIMENSION_BUFFER	= 1,
	D3D11_SRV_DIMENSION_TEXTURE1D	= 2,
	D3D11_SRV_DIMENSION_TEXTURE1DARRAY	= 3,
	D3D11_SRV_DIMENSION_TEXTURE2D	= 4,
	D3D11_SRV_DIMENSION_TEXTURE2DARRAY	= 5,
	D3D11_SRV_DIMENSION_TEXTURE2DMS	= 6,
	D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY	= 7,
	D3D11_SRV_DIMENSION_TEXTURE3D	= 8,
	D3D11_SRV_DIMENSION_TEXTURECUBE	= 9,
	D3D11_SRV_DIMENSION_TEXTURECUBEARRAY	= 10,
	D3D11_SRV_DIMENSION_BUFFEREX	= 11
} 	D3D11_SRV_DIMENSION;

#define IID_ID3D11ShaderReflection 0

#include "CCryDXPSMisc9.hpp"

//////////////////////////////////////////////////////////////////////////
//struct
//////////////////////////////////////////////////////////////////////////
struct D3D11_SUBRESOURCE_DATA;
struct D3D11_SO_DECLARATION_ENTRY;
struct D3D11_COUNTER_DESC;
struct D3D11_COUNTER_INFO;
struct D3D11_COUNTER_TYPE;
struct DXGI_FRAME_STATISTICS;


struct D3DX11_IMAGE_INFO
{
	uint32                       Width;
	uint32                      Height;
	uint32                      Depth;
	uint32                      ArraySize;
	uint32                      MipLevels;
	uint32                      MiscFlags;
	DXGI_FORMAT                 Format;
	D3D11_RESOURCE_DIMENSION    ResourceDimension;
	D3DX11_IMAGE_FILE_FORMAT    ImageFileFormat;
};



typedef struct D3D11_QUERY_DESC
{
    D3D11_QUERY Query;
    UINT MiscFlags;
};

class ID3D11Debug
{
public:
	HRESULT ValidateContext(void *pContext){return S_OK;}
	void Release(){}
};

typedef struct D3D11_RENDER_TARGET_BLEND_DESC {
	D3D11_BLEND SrcBlend;
	D3D11_BLEND DestBlend;
	D3D11_BLEND_OP BlendOp;
	D3D11_BLEND SrcBlendAlpha;
	D3D11_BLEND DestBlendAlpha;
	D3D11_BLEND_OP BlendOpAlpha;
	UINT8 RenderTargetWriteMask;
	UINT8 BlendEnable;
	UINT8 PAD[2]; //USE ME!
} D3D11_RENDER_TARGET_BLEND_DESC;

typedef struct D3D11_BLEND_DESC
{
    UINT8 AlphaToCoverageEnable;
		UINT8 IndependentBlendEnable;
		UINT8 PAD[2]; //USE ME!
		D3D11_RENDER_TARGET_BLEND_DESC RenderTarget[8];
} D3D11_BLEND_DESC, *LPD3D11_BLEND_DESC;


struct DXGI_SAMPLE_DESC
{
	uint32				Count;
	uint32				Quality;
};

struct DXGI_RATIONAL
{
	uint32				Numerator;
	uint32				Denominator;
};

struct DXGI_MODE_DESC
{
	uint32							Width;
	uint32							Height;
	DXGI_RATIONAL							RefreshRate;
	DXGI_FORMAT								Format;
	DXGI_MODE_SCANLINE_ORDER	ScanlineOrdering;
	DXGI_MODE_SCALING					Scaling;
};

struct DXGI_SWAP_CHAIN_DESC
{
	DXGI_MODE_DESC			BufferDesc;
	DXGI_SAMPLE_DESC		SampleDesc;
	uint32							BufferUsage;
	uint32							BufferCount;
	uint32							MaxFrameLatency;
	int									OutputWindow;
	int									Windowed;
	DXGI_SWAP_EFFECT		SwapEffect;
	DXGI_MODE_ROTATION	BufferRotation;
	uint32				Flags;
};


struct D3D11_TEXTURE1D_DESC
{
	uint32			Width;
	uint32			MipLevels;
	uint32			ArraySize;
	DXGI_FORMAT				Format;
	D3D11_USAGE				Usage;
	uint32			BindFlags;
	uint32			CPUAccessFlags;
	uint32			MiscFlags;
};

struct D3D11_TEXTURE2D_DESC
{
	uint32			Width;
	uint32			Height;
	uint32			MipLevels;
	uint32			ArraySize;
	DXGI_FORMAT				Format;
	DXGI_SAMPLE_DESC	SampleDesc;
	D3D11_USAGE				Usage;
	uint32			BindFlags;
	uint32			CPUAccessFlags;
	uint32			MiscFlags;
};

struct D3D11_TEXTURE3D_DESC
{
	uint32			Width;
	uint32			Height;
	uint32			Depth;
	uint32			MipLevels;
	DXGI_FORMAT				Format;
	D3D11_USAGE				Usage;
	uint32			BindFlags;
	uint32			CPUAccessFlags;
	uint32			MiscFlags;
};

struct D3D11_VIEWPORT
{
	//cellgcm accept just uint16 anyway, so no strict need for compatibility
	uint16					TopLeftX;
	uint16					TopLeftY;
	uint16					Width;
	uint16					Height;
	float						MinDepth;
	float						MaxDepth;
};

struct D3D11_INPUT_ELEMENT_DESC
{
	const char*									SemanticName;
/*	uint32							SemanticIndex;
	DXGI_FORMAT						Format;
	uint32								InputSlot;
	uint32								AlignedByteOffset;
	D3D11_INPUT_CLASSIFICATION	InputSlotClass;
	uint32								InstanceDataStepRate;
*/
	uint8								SemanticIndex;
	uint8								Format;
	uint16							InputSlot;
	uint16							AlignedByteOffset;
	uint8								InputSlotClass;
	uint8								InstanceDataStepRate;
};

struct D3D11_BUFFER_DESC
{
	uint32		ByteWidth;
	D3D11_USAGE			Usage;
	uint32		BindFlags;
	uint32		CPUAccessFlags;
	uint32		MiscFlags;
};

struct D3D11_SUBRESOURCE_DATA
{
	const void*			pSysMem;
	uint32		SysMemPitch;
	uint32		SysMemSlicePitch;
};

struct D3D11_BUFFER_RTV
{
	uint32		ElementOffset;
	uint32		ElementWidth;
};

struct D3D11_TEX1D_RTV
{
	uint32		MipSlice;
};

struct D3D11_TEX1D_ARRAY_RTV
{
	uint32		MipSlice;
	uint32		FirstArraySlice;
	uint32		ArraySize;
};

struct D3D11_TEX2D_RTV
{
	uint32		MipSlice;
};

struct D3D11_TEX2DMS_RTV
{
	uint32		UnusedField_NothingToDefine;
};

struct D3D11_TEX2D_ARRAY_RTV
{
	uint32		MipSlice;
	uint32		FirstArraySlice;
	uint32		ArraySize;
};

struct D3D11_TEX2DMS_ARRAY_RTV
{
	uint32		FirstArraySlice;
	uint32		ArraySize;
};

struct D3D11_TEX3D_RTV
{
	uint32		MipSlice;
	uint32		FirstWSlice;
	uint32		WSize;
};

enum DXPSRTVDTilingInfo
{
	DXPSRTVDTI_AUTO					=	0,
	DXPSRTVDTI_NOTILING			=	1,
	DXPSRTVDTI_FORCETILING	=	2,
};

struct D3D11_RENDER_TARGET_VIEW_DESC
{
	DXGI_FORMAT								Format;
	D3D11_RTV_DIMENSION 			ViewDimension;
	DXPSRTVDTilingInfo				TilingInfo;
	union 
	{
		D3D11_BUFFER_RTV				Buffer;
		D3D11_TEX1D_RTV					Texture1D;
		D3D11_TEX1D_ARRAY_RTV		Texture1DArray;
		D3D11_TEX2D_RTV					Texture2D;
		D3D11_TEX2D_ARRAY_RTV		Texture2DArray;
		D3D11_TEX2DMS_RTV				Texture2DMS;
		D3D11_TEX2DMS_ARRAY_RTV Texture2DMSArray;
		D3D11_TEX3D_RTV					Texture3D;
	};
														D3D11_RENDER_TARGET_VIEW_DESC():
														TilingInfo(DXPSRTVDTI_AUTO)
														{
														}
};


struct D3D11_RASTERIZER_DESC
{
    D3D11_FILL_MODE				FillMode;
    D3D11_CULL_MODE				CullMode;
    uint32								FrontCounterClockwise;
    int32									DepthBias;
    f32										DepthBiasClamp;
    f32										SlopeScaledDepthBias;
    UINT8									DepthClipEnable;
    UINT8									ScissorEnable;
    UINT8									MultisampleEnable;
    UINT8									AntialiasedLineEnable;
		D3D11_COMPARISON_FUNC	AlphaTestFunc;
		uint32								AlphaTestEnable;
};

typedef struct D3D11_DEPTH_STENCILOP_DESC 
{
    D3D11_STENCIL_OP			StencilFailOp;
    D3D11_STENCIL_OP			StencilDepthFailOp;
    D3D11_STENCIL_OP			StencilPassOp;
    D3D11_COMPARISON_FUNC StencilFunc;
} D3D11_DEPTH_STENCILOP_DESC, *LPD3D11_DEPTH_STENCILOP_DESC;


typedef struct D3D11_DEPTH_STENCIL_DESC
{
		D3D11_DEPTH_WRITE_MASK			DepthWriteMask;
		D3D11_COMPARISON_FUNC				DepthFunc;
		uint8												DepthEnable;
		uint8												StencilEnable;
		uint8 											StencilReadMask;
		uint8 											StencilWriteMask;
		D3D11_DEPTH_STENCILOP_DESC	FrontFace;
		D3D11_DEPTH_STENCILOP_DESC	BackFace;
} D3D11_DEPTH_STENCIL_DESC, *LPD3D11_DEPTH_STENCIL_DESC;

typedef struct DXGI_SURFACE_DESC
{
    uint32											Width;
    uint32											Height;
    DXGI_FORMAT									Format;
    DXGI_SAMPLE_DESC						SampleDesc;
};

typedef struct D3D11_TEX1D_ARRAY_DSV
{
    uint32											MipSlice;
    uint32											FirstArraySlice;
    uint32											ArraySize;
} D3D11_TEX1D_ARRAY_DSV, *LPD3D11_TEX1D_ARRAY_DSV;

typedef struct D3D11_TEX1D_DSV
{
    uint32											MipSlice;
} D3D11_TEX1D_DSV, *LPD3D11_TEX1D_DSV;

typedef struct D3D11_TEX2D_DSV
{
    uint32											MipSlice;
} D3D11_TEX2D_DSV, *LPD3D11_TEX2D_DSV;

typedef struct D3D11_TEX2D_ARRAY_DSV
{
    uint32 											MipSlice;
    uint32 											FirstArraySlice;
    uint32 											ArraySize;
} D3D11_TEX2D_ARRAY_DSV, *LPD3D11_TEX2D_ARRAY_DSV;

typedef struct D3D11_TEX2DDMS_DSV
{
    uint32											UnusedField_NothingToDefine;
} D3D11_TEX2DMS_DSV, *LPD3D11_TEX2DMS_DSV;

typedef struct D3D11_TEX2DDMS_ARRAY_RTV
{
    uint32 											FirstArraySlice;
    uint32 											ArraySize;
} D3D11_TEX2DMS_ARRAY_DSV, *LPD3D11_TEX2DMS_ARRAY_DSV;

typedef struct D3D11_DEPTH_STENCIL_VIEW_DESC 
{
    DXGI_FORMAT									Format;
    D3D11_DSV_DIMENSION					ViewDimension;
		uint32											Flags;
    union
		{
        D3D11_TEX1D_DSV					Texture1D;
        D3D11_TEX1D_ARRAY_DSV		Texture1DArray;
        D3D11_TEX2D_DSV					Texture2D;
        D3D11_TEX2D_ARRAY_DSV		Texture2DArray;
        D3D11_TEX2DMS_DSV				Texture2DMS;
        D3D11_TEX2DMS_ARRAY_DSV Texture2DMSArray;
    };
};

typedef struct D3D11_SHADER_TYPE_DESC
{
	D3D10_SHADER_VARIABLE_CLASS		Class;
	D3D10_SHADER_VARIABLE_TYPE		Type;
	uint32												Rows;
	uint32												Columns;
	uint32												Elements;
	uint32												Members;
};

typedef struct D3D11_SHADER_VARIABLE_DESC
{
	const char*										Name;
//	char													TmpName[256];
	uint32												StartOffset;
	uint32												Size;
	uint32												uFlags;
	void*													DefaultValue;
//not D3d10 conform
	uint32												CBufferIndex;
};

typedef struct D3D11_VERTEX_SHADER_DESC
{
	const uint8*									pFunction;
	uint32												SizeInBytes;
};

typedef struct D3D11_SHADER_INPUT_BIND_DESC
{
	const char*										Name;
//	char													TmpName[256];
	D3D10_SHADER_INPUT_TYPE				Type;
	uint32												BindPoint;
	uint32												BindCount;
	uint32												uFlags;
	D3D11_RESOURCE_RETURN_TYPE		ReturnType;
	D3D10_SRV_DIMENSION						Dimension;
	uint32												NumSamples;
};

typedef struct D3D11_SHADER_DESC
{
	uint32												Version;
	char*													Creator;
	uint32												Flags;
	uint32												ConstantBuffers;
	uint32												BoundResources;
	uint32												InputParameters;
	uint32												OutputParameters;
};

typedef struct D3DGAMMARAMP
{
	uint16												red[256];
	uint16												green[256];
	uint16												blue[256];
};

typedef struct D3D11_SHADER_BUFFER_DESC
{
	const char*									Name;
	D3D11_CBUFFER_TYPE					Type;
	uint32											Variables;
	uint32											Size;
	uint32											uFlags;
};

typedef struct D3D11_SIGNATURE_PARAMETER_DESC
{
	const char*										SemanticName;
	uint32 												SemanticIndex;
	uint32 												Register;
	D3D10_NAME 										SystemValueType;
	D3D11_REGISTER_COMPONENT_TYPE ComponentType;
	uint8 												Mask;
	uint8 												ReadWriteMask;
};

typedef struct D3DPRESENT_PARAMETERS {
    uint32												BackBufferWidth, BackBufferHeight;
    D3DFORMAT											BackBufferFormat;
    uint32												BackBufferCount;
    D3DMULTISAMPLE_TYPE						MultiSampleType;
    uint32												MultiSampleQuality;
    D3DSWAPEFFECT									SwapEffect;
    uint32												hDeviceWindow;
    uint32												Windowed;
    uint32												EnableAutoDepthStencil;
    D3DFORMAT											AutoDepthStencilFormat;
    uint32												Flags;
    uint32												FullScreen_RefreshRateInHz;
    uint32												PresentationInterval;
};

/*typedef struct DXUTDeviceSettings
{
	uint32												AdapterOrdinal;
	D3DDEVTYPE										DeviceType;
	D3DFORMAT											AdapterFormat;
	uint32												BehaviorFlags;
	D3DPRESENT_PARAMETERS					pp;
};*/


struct D3D11_SAMPLER_DESC
{
	D3D11_FILTER								Filter;
	D3D11_TEXTURE_ADDRESS_MODE	AddressU;
	D3D11_TEXTURE_ADDRESS_MODE	AddressV;
	D3D11_TEXTURE_ADDRESS_MODE	AddressW;
	f32													MipLODBias;
	UINT												MaxAnisotropy;
	D3D11_COMPARISON_FUNC				ComparisonFunc;
	f32													BorderColor[4];
	f32													MinLOD;
	f32													MaxLOD;
};

typedef struct D3D11_MAPPED_SUBRESOURCE
{
	uint8*												pData;
	uint32												RowPitch;
	uint32												DepthPitch;
};


typedef struct D3D11_BUFFER_SRV
{
    uint32 ElementOffset;
    uint32 ElementWidth;
} D3D11_BUFFER_SRV;

typedef struct D3D11_TEX1D_SRV 
{
    uint32 MostDetailedMip;
    uint32 MipLevels;
} D3D11_TEX1D_SRV;

typedef struct D3D11_TEX1D_ARRAY_SRV
{
    uint32 MostDetailedMip;
    uint32 MipLevels;
    uint32 FirstArraySlice;
    uint32 ArraySize;
} D3D11_TEX1D_ARRAY_SRV;

typedef struct D3D11_TEX2D_SRV
{
    uint32 MostDetailedMip;
    uint32 MipLevels;
} D3D11_TEX2D_SRV;

typedef struct D3D11_TEX2D_ARRAY_SRV
{
    uint32 MostDetailedMip;
    uint32 MipLevels;
    uint32 FirstArraySlice;
    uint32 ArraySize;
} D3D11_TEX2D_ARRAY_SRV;

typedef struct D3D11_TEX2DMS_SRV 
{
    uint32 UnusedField_NothingToDefine;
} D3D11_TEX2DMS_SRV;

typedef struct D3D11_TEX2DMS_ARRAY_SRV 
{
    uint32 FirstArraySlice;
    uint32 ArraySize;
} ;

typedef struct D3D11_TEX3D_SRV
{
    uint32 MostDetailedMip;
    uint32 MipLevels;
} D3D11_TEX3D_SRV;

struct D3D11_TEXCUBE_SRV
{
    uint32 MostDetailedMip;
    uint32 MipLevels;
};

struct D3D11_SHADER_RESOURCE_VIEW_DESC
{
    DXGI_FORMAT Format;
    D3D11_SRV_DIMENSION ViewDimension;
    union
		{
        D3D11_BUFFER_SRV Buffer;
        D3D11_TEX1D_SRV Texture1D;
        D3D11_TEX1D_ARRAY_SRV Texture1DArray;
        D3D11_TEX2D_SRV Texture2D;
        D3D11_TEX2D_ARRAY_SRV Texture2DArray;
        D3D11_TEX2DMS_SRV Texture2DMS;
        D3D11_TEX2DMS_ARRAY_SRV Texture2DMSArray;
        D3D11_TEX3D_SRV Texture3D;
        D3D11_TEXCUBE_SRV TextureCube;
    };
};

typedef struct D3D10_STATE_BLOCK_MASK
{
	uint8 VS;
	uint8 VSSamplers[D3D11_BYTES_FROM_BITS(D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)];
	uint8 VSShaderResources[D3D11_BYTES_FROM_BITS(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
	uint8 VSConstantBuffers[D3D11_BYTES_FROM_BITS(D3D11_COMMONSHADER_CONSTANT_BUFFER_SLOT_COUNT)];
	uint8 GS;
	uint8 GSSamplers[D3D11_BYTES_FROM_BITS(D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)];
	uint8 GSShaderResources[D3D11_BYTES_FROM_BITS(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
	uint8 GSConstantBuffers[D3D11_BYTES_FROM_BITS(D3D11_COMMONSHADER_CONSTANT_BUFFER_SLOT_COUNT)];
	uint8 PS;
	uint8 PSSamplers[D3D11_BYTES_FROM_BITS(D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)];
	uint8 PSShaderResources[D3D11_BYTES_FROM_BITS(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
	uint8 PSConstantBuffers[D3D11_BYTES_FROM_BITS(D3D11_COMMONSHADER_CONSTANT_BUFFER_SLOT_COUNT)];
	uint8 IAVertexBuffers[D3D11_BYTES_FROM_BITS(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)];
	uint8 IAIndexBuffer;
	uint8 IAInputLayout;
	uint8 IAPrimitiveTopology;
	uint8 OMRenderTargets;
	uint8 OMDepthStencilState;
	uint8 OMBlendState;
	uint8 RSViewports;
	uint8 RSScissorRects;
	uint8 RSRasterizerState;
	uint8 SOBuffers;
	uint8 Predication;
};

typedef struct DXGI_OUTPUT_DESC
{
    char DeviceName[32];
    RECT DesktopCoordinates;
    int AttachedToDesktop;
    DXGI_MODE_ROTATION Rotation;
    HMONITOR Monitor;
} DXGI_OUTPUT_DESC, *LPDXGI_OUTPUT_DESC;


struct DXGI_GAMMA_CONTROL_CAPABILITIES
{
    BOOL ScaleAndOffsetSupported;
    float MaxConvertedValue;
    float MinConvertedValue;
    UINT NumGammaControlPoints;
    float ControlPointPositions[ 1025 ];
};

enum DXGI_RESIDENCY
{	DXGI_RESIDENCY_FULLY_RESIDENT	= 1,
	DXGI_RESIDENCY_RESIDENT_IN_SHARED_MEMORY	= 2,
	DXGI_RESIDENCY_EVICTED_TO_DISK	= 3
};


struct DXGI_RGB
{
    float Red;
    float Green;
    float Blue;
};

struct DXGI_GAMMA_CONTROL
{
    DXGI_RGB Scale;
    DXGI_RGB Offset;
    DXGI_RGB GammaCurve[ 1025 ];
};


struct DXGI_SHARED_RESOURCE
{
    uint32 Handle;
};

//////////////////////////////////////////////////////////////////////////
//interfaces
//////////////////////////////////////////////////////////////////////////

class ID3D10StateBlock;
#ifndef DO_RENDERLOG
	class CCryDXPSRenderDevice;
	typedef CCryDXPSRenderDevice ID3D11Device;
	typedef CCryDXPSRenderDevice ID3D11DeviceContext;
#endif
/*struct ID3D11RasterizerState
{
public:
    virtual void STDMETHODCALLTYPE GetDesc(D3D11_RASTERIZER_DESC *pDesc) = 0;
}; 

struct ID3D11DepthStencilState
{
public:
    virtual void STDMETHODCALLTYPE GetDesc( D3D11_DEPTH_STENCIL_DESC *pDesc) = 0;
};
*/


class ID3D11Query
{
public:
    virtual void GetDesc(D3D11_QUERY_DESC *pDesc)		= 0;
		virtual void Begin()														=	0;
		virtual void End()															=	0;
		virtual HRESULT STDMETHODCALLTYPE GetData(void *pData,uint32 DataSize,uint32 GetDataFlags) = 0;
		virtual uint32 STDMETHODCALLTYPE GetDataSize( void) = 0;
		virtual void Release(){};
};

typedef ID3D11Query ID3D11Asynchronous;


/*class ID3D11ShaderResourceView
{
public:
  virtual void GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc) = 0;
	virtual unsigned long Release()	=	0;
};*/

//////////////////////////////////////////////////////////////////////////
//DXUT
//////////////////////////////////////////////////////////////////////////






typedef struct D3DX11_IMAGE_LOAD_INFO
{
	uint32 Width;
	uint32 Height;
	uint32 Depth;
	uint32 FirstMipLevel;
	uint32 MipLevels;
	D3D11_USAGE Usage;
	uint32 BindFlags;
	uint32 CpuAccessFlags;
	uint32 MiscFlags;
	DXGI_FORMAT Format;
	uint32 Filter;
	uint32 MipFilter;
	D3DX11_IMAGE_INFO* pSrcInfo;
} D3DX11_IMAGE_LOAD_INFO, *LPD3DX11_IMAGE_LOAD_INFO;

//--------------------------------------------------------------------------------------
// Enumerates available Direct3D10 adapters, devices, modes, etc.
// Use DXUTGetD3D9Enumeration() to access global instance
//--------------------------------------------------------------------------------------
//#define LARGE_INTEGER uint32
//#define LONGLONG long
//#include "../dxut/dxutmisc.h"

/*FORCEINLINE 
HRESULT DXUTCreateDevice( bool bWindowed = true, int nSuggestedWidth=0, int nSuggestedHeight=0, int nSuggestStencil=0, int nSuggestColor=0 )
{
	return -1;
}

FORCEINLINE 
uint32 DXUTGetDXGIColorBits( DXGI_FORMAT fmt )
{
  switch( fmt )
  {
  case DXGI_FORMAT_R32G32B32A32_TYPELESS:
  case DXGI_FORMAT_R32G32B32A32_FLOAT:
  case DXGI_FORMAT_R32G32B32A32_UINT:
  case DXGI_FORMAT_R32G32B32A32_SINT:
    return 128;
  case DXGI_FORMAT_R32G32B32_TYPELESS:
  case DXGI_FORMAT_R32G32B32_FLOAT:
  case DXGI_FORMAT_R32G32B32_UINT:
  case DXGI_FORMAT_R32G32B32_SINT:
    return 96;

  case DXGI_FORMAT_R16G16B16A16_TYPELESS:
  case DXGI_FORMAT_R16G16B16A16_FLOAT:
  case DXGI_FORMAT_R16G16B16A16_UNORM:
  case DXGI_FORMAT_R16G16B16A16_UINT:
  case DXGI_FORMAT_R16G16B16A16_SNORM:
  case DXGI_FORMAT_R16G16B16A16_SINT:
    return 64;

  case DXGI_FORMAT_R10G10B10A2_TYPELESS:
  case DXGI_FORMAT_R10G10B10A2_UNORM:
  case DXGI_FORMAT_R10G10B10A2_UINT:
    return 32;

  case DXGI_FORMAT_R8G8B8A8_TYPELESS:
  case DXGI_FORMAT_R8G8B8A8_UNORM:
  case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
  case DXGI_FORMAT_R8G8B8A8_UINT:
  case DXGI_FORMAT_R8G8B8A8_SNORM:
  case DXGI_FORMAT_R8G8B8A8_SINT:
    return 32;

  case DXGI_FORMAT_B5G6R5_UNORM:
  case DXGI_FORMAT_B5G5R5A1_UNORM:
    return 16;

  case DXGI_FORMAT_A8_UNORM:
    return 8;

  case DXGI_FORMAT_BC1_UNORM:
    return 8;

  case DXGI_FORMAT_R16_FLOAT:
    return 16;
  case DXGI_FORMAT_R32_FLOAT:
    return 32;

  case DXGI_FORMAT_R8G8_SNORM:
    return 16;

  case DXGI_FORMAT_R16G16_FLOAT:
  case DXGI_FORMAT_R16G16_SNORM:
    return 32;

  case DXGI_FORMAT_BC2_UNORM:
  case DXGI_FORMAT_BC3_UNORM:
  case DXGI_FORMAT_BC5_UNORM:
    return 16;

  case DXGI_FORMAT_D24_UNORM_S8_UINT:
    return 32;
  case DXGI_FORMAT_D16_UNORM:
    return 16;
  case DXGI_FORMAT_D32_FLOAT:
    return 32;
  case DXGI_FORMAT_R24G8_TYPELESS:
    return 32;
  case DXGI_FORMAT_R16_TYPELESS:
    return 16;


  default:
    assert(0);
    return 0;
  }
}



template< typename TYPE >
class CGrowableArray
{
public:
    CGrowableArray()  { m_pData = NULL; m_nSize = 0; m_nMaxSize = 0; }
    CGrowableArray( const CGrowableArray<TYPE>& a ) { for( int i=0; i < a.m_nSize; i++ ) Add( a.m_pData[i] ); }
    ~CGrowableArray() { RemoveAll(); }

    const TYPE& operator[]( int nIndex ) const { return GetAt( nIndex ); }
    TYPE& operator[]( int nIndex ) { return GetAt( nIndex ); }
   
    CGrowableArray& operator=( const CGrowableArray<TYPE>& a ) { if( this == &a ) return *this; RemoveAll(); for( int i=0; i < a.m_nSize; i++ ) Add( a.m_pData[i] ); return *this; }

    HRESULT SetSize( int nNewMaxSize );
    HRESULT Add( const TYPE& value );
    HRESULT Insert( int nIndex, const TYPE& value );
    HRESULT SetAt( int nIndex, const TYPE& value );
    TYPE&   GetAt( int nIndex ) const { assert( nIndex >= 0 && nIndex < m_nSize ); return m_pData[nIndex]; }
    int     GetSize() const { return m_nSize; }
    TYPE*   GetData() { return m_pData; }
    bool    Contains( const TYPE& value ){ return ( -1 != IndexOf( value ) ); }

    int     IndexOf( const TYPE& value ) { return ( m_nSize > 0 ) ? IndexOf( value, 0, m_nSize ) : -1; }
    int     IndexOf( const TYPE& value, int iStart ) { return IndexOf( value, iStart, m_nSize - iStart ); }
    int     IndexOf( const TYPE& value, int nIndex, int nNumElements );

    int     LastIndexOf( const TYPE& value ) { return ( m_nSize > 0 ) ? LastIndexOf( value, m_nSize-1, m_nSize ) : -1; }
    int     LastIndexOf( const TYPE& value, int nIndex ) { return LastIndexOf( value, nIndex, nIndex+1 ); }
    int     LastIndexOf( const TYPE& value, int nIndex, int nNumElements );

    HRESULT Remove( int nIndex );
    void    RemoveAll() { SetSize(0); }

protected:
    TYPE* m_pData;      // the actual array of data
    int m_nSize;        // # of elements (upperBound - 1)
    int m_nMaxSize;     // max allocated

    HRESULT SetSizeInternal( int nNewMaxSize );  // This version doesn't call ctor or dtor.
};
*/
typedef class CCryDXPSSwapChain IDXGISwapChain;
/*
typedef bool    (*LPDXUTCALLBACKISD3D10DEVICEACCEPTABLE)( uint32 Adapter, uint32 Output, D3D_DRIVER_TYPE DeviceType, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
typedef HRESULT (*LPDXUTCALLBACKD3D10DEVICECREATED)( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
typedef HRESULT (*LPDXUTCALLBACKD3D10SWAPCHAINRESIZED)( ID3D11Device* pd3dDevice, IDXGISwapChain *pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
typedef void    (*LPDXUTCALLBACKD3D10FRAMERENDER)( ID3D11Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
typedef void    (*LPDXUTCALLBACKD3D10SWAPCHAINRELEASING)( void* pUserContext );
typedef void    (*LPDXUTCALLBACKD3D10DEVICEDESTROYED)( void* pUserContext );
*/
typedef struct _LUID {
  uint32 LowPart;
  long HighPart;
} LUID, *PLUID;

typedef struct DXGI_ADAPTER_DESC
{
    wchar_t Description[128];
    uint32 VendorId;
    uint32 DeviceId;
    uint32 SubSysId;
    uint32 Revision;
    size_t DedicatedVideoMemory;
    size_t DedicatedSystemMemory;
    size_t SharedSystemMemory;
    LUID AdapterLuid;
} DXGI_ADAPTER_DESC, *LPDXGI_ADAPTER_DESC;
/*
 #define DXGI_MAX_DEVICE_IDENTIFIER_STRING 128 
class IDXGIAdapter
{
public:
    virtual HRESULT EnumOutputs( uint32 Output,class IDXGIOutput **ppOutput) = 0;
    virtual HRESULT GetDesc( class DXGI_ADAPTER_DESC *pDesc) = 0;
    virtual HRESULT CheckInterfaceSupport( REFGUID InterfaceName,uint32 *pUMDVersion) = 0;
		virtual void Release();
    
};
*/

typedef struct DXGI_MAPPED_RECT
{
    int32 Pitch;
    uint8 *pBits;
} 	DXGI_MAPPED_RECT;

class IDXGISurface
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetDesc( DXGI_SURFACE_DESC *pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE Map( DXGI_MAPPED_RECT *pLockedRect,UINT MapFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unmap( void) = 0;
};

/*class IDXGIOutput
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetDesc(DXGI_OUTPUT_DESC *pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDisplayModeList(DXGI_FORMAT EnumFormat,UINT Flags,UINT *pNumModes,DXGI_MODE_DESC *pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE FindClosestMatchingMode( const DXGI_MODE_DESC *pModeToMatch,DXGI_MODE_DESC *pClosestMatch,void *pConcernedDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE WaitForVBlank( void) = 0;
    virtual HRESULT STDMETHODCALLTYPE TakeOwnership( void *pDevice,BOOL Exclusive) = 0;
    virtual void STDMETHODCALLTYPE ReleaseOwnership( void) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetGammaControlCapabilities( DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetGammaControl( const DXGI_GAMMA_CONTROL *pArray) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetGammaControl( DXGI_GAMMA_CONTROL *pArray) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDisplaySurface( IDXGISurface *pScanoutSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData( IDXGISurface *pDestination) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS *pStats) = 0;
		virtual void Release();
};
*/

inline UINT D3D11CalcSubresource( UINT MipSlice, UINT ArraySlice, UINT MipLevels )
{ 
	return MipSlice + ArraySlice * MipLevels; 
}

#endif

