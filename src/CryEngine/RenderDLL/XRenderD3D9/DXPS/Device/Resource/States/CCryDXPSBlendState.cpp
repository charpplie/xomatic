#include "StdAfx.h"
#include "../../../Layer0/CCryDXPS.hpp"
#include "CCryDXPSBlendState.hpp"

using namespace CRY_DXPS_GCMNAMESPACE;

static uint16 BlendOpD3D2Gcm[]=
{
	0,
	CELL_GCM_FUNC_ADD,								//D3D11_BLEND_OP_ADD = 1,
  CELL_GCM_FUNC_SUBTRACT,						//D3D11_BLEND_OP_SUBTRACT = 2,
  CELL_GCM_FUNC_REVERSE_SUBTRACT,	  //D3D11_BLEND_OP_REV_SUBTRACT = 3,
  CELL_GCM_MIN,											//D3D11_BLEND_OP_MIN = 4,
	CELL_GCM_MAX,											//D3D11_BLEND_OP_MAX = 5,
	CELL_GCM_FUNC_ADD_SIGNED,					//PS3 specific signed additive blending
};

static uint16 BlendFuncD3D2Gcm[]=
{
	0,
	CELL_GCM_ZERO,//D3D11_BLEND_ZERO = 1,
	CELL_GCM_ONE,//D3D11_BLEND_ONE = 2,
	CELL_GCM_SRC_COLOR,//D3D11_BLEND_SRC_COLOR = 3,
	CELL_GCM_ONE_MINUS_SRC_COLOR,//D3D11_BLEND_INV_SRC_COLOR = 4,
	CELL_GCM_SRC_ALPHA,//D3D11_BLEND_SRC_ALPHA = 5,
	CELL_GCM_ONE_MINUS_SRC_ALPHA,//D3D11_BLEND_INV_SRC_ALPHA = 6,
	CELL_GCM_DST_ALPHA,//D3D11_BLEND_DEST_ALPHA = 7,
	CELL_GCM_ONE_MINUS_DST_ALPHA,//D3D11_BLEND_INV_DEST_ALPHA = 8,
	CELL_GCM_DST_COLOR,//D3D11_BLEND_DEST_COLOR = 9,
	CELL_GCM_ONE_MINUS_DST_COLOR,//D3D11_BLEND_INV_DEST_COLOR = 10,
	CELL_GCM_SRC_ALPHA_SATURATE,//D3D11_BLEND_SRC_ALPHA_SAT = 11,
	CELL_GCM_CONSTANT_COLOR,//D3D11_BLEND_BLEND_FACTOR = 14,
	CELL_GCM_ONE_MINUS_CONSTANT_COLOR,//D3D11_BLEND_INV_BLEND_FACTOR = 15,
	CELL_GCM_SRC_COLOR,//D3D11_BLEND_SRC1_COLOR = 16,
	CELL_GCM_ONE_MINUS_SRC_COLOR,//D3D11_BLEND_INV_SRC1_COLOR = 17,
	CELL_GCM_SRC_ALPHA,//D3D11_BLEND_SRC1_ALPHA = 18,
	CELL_GCM_ONE_MINUS_SRC_ALPHA//D3D11_BLEND_INV_SRC1_ALPHA = 19,
};


CCryDXPSBlendState::CCryDXPSBlendState(const D3D11_BLEND_DESC& rState MMRES_PARAM):
CCryDXPSResource(EDXPS_RT_BLENDSTATE MMRES_PASS)
{
	MMRES_ADDCOUNT();
	MMRES_ADDMM(sizeof(CCryDXPSBlendState));

	m_BlendEnable[0]	=	rState.RenderTarget[0].BlendEnable?CELL_GCM_TRUE:CELL_GCM_FALSE;
	m_BlendEnable[1]	=	rState.RenderTarget[1].BlendEnable?CELL_GCM_TRUE:CELL_GCM_FALSE;
	m_BlendEnable[2]	=	rState.RenderTarget[2].BlendEnable?CELL_GCM_TRUE:CELL_GCM_FALSE;
	m_BlendEnable[3]	=	rState.RenderTarget[3].BlendEnable?CELL_GCM_TRUE:CELL_GCM_FALSE;
	m_BlendFuncColor	=	BlendOpD3D2Gcm[rState.RenderTarget[0].BlendOp];
	m_BlendFuncAlpha	=	BlendOpD3D2Gcm[rState.RenderTarget[0].BlendOpAlpha];

	m_BlendSrcColorFunc	=	BlendFuncD3D2Gcm[rState.RenderTarget[0].SrcBlend];
	m_BlendDstColorFunc	=	BlendFuncD3D2Gcm[rState.RenderTarget[0].DestBlend];
	m_BlendSrcAlphaFunc	=	BlendFuncD3D2Gcm[rState.RenderTarget[0].SrcBlendAlpha];
	m_BlendDstAlphaFunc	=	BlendFuncD3D2Gcm[rState.RenderTarget[0].DestBlendAlpha];

	m_ColorMask			=
	m_ColorMaskMrt	=	0;

	if(rState.RenderTarget[0].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_RED)
		m_ColorMask	|=	CELL_GCM_COLOR_MASK_R;
	if(rState.RenderTarget[0].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_GREEN)
		m_ColorMask	|=	CELL_GCM_COLOR_MASK_G;
	if(rState.RenderTarget[0].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_BLUE)
		m_ColorMask	|=	CELL_GCM_COLOR_MASK_B;
	if(rState.RenderTarget[0].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_ALPHA)
		m_ColorMask	|=	CELL_GCM_COLOR_MASK_A;

	if(rState.RenderTarget[1].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_RED)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT1_R;
	if(rState.RenderTarget[1].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_GREEN)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT1_G;
	if(rState.RenderTarget[1].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_BLUE)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT1_B;
	if(rState.RenderTarget[1].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_ALPHA)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT1_A;

	if(rState.RenderTarget[2].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_RED)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT2_R;
	if(rState.RenderTarget[2].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_GREEN)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT2_G;
	if(rState.RenderTarget[2].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_BLUE)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT2_B;
	if(rState.RenderTarget[2].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_ALPHA)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT2_A;

	if(rState.RenderTarget[3].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_RED)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT3_R;
	if(rState.RenderTarget[3].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_GREEN)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT3_G;
	if(rState.RenderTarget[3].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_BLUE)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT3_B;
	if(rState.RenderTarget[3].RenderTargetWriteMask&D3D11_COLOR_WRITE_ENABLE_ALPHA)
		m_ColorMaskMrt	|=	CELL_GCM_COLOR_MASK_MRT3_A;
}
