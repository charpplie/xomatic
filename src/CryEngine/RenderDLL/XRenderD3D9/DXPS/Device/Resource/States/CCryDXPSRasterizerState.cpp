#include "StdAfx.h"
#include "../../../Layer0/CCryDXPS.hpp"
#include "CCryDXPSRasterizerState.hpp"

extern uint32 g_Comp2Gcm[9];

using namespace CRY_DXPS_GCMNAMESPACE;

CCryDXPSRasterizerState::CCryDXPSRasterizerState(const D3D11_RASTERIZER_DESC& rState MMRES_PARAM):
CCryDXPSResource(EDXPS_RT_RASTERIZERSTATE MMRES_PASS)
{
	MMRES_ADDCOUNT();
	MMRES_ADDMM(sizeof(CCryDXPSRasterizerState));

		//D3D11_FILL_MODE FillMode
	//cellGcmSetFrontPolygonMode
	m_FillMode	=	rState.FillMode==D3D11_FILL_WIREFRAME?CELL_GCM_POLYGON_MODE_LINE:
								rState.FillMode==D3D11_FILL_POINTPS3?CELL_GCM_POLYGON_MODE_POINT:CELL_GCM_POLYGON_MODE_FILL;


	//D3D11_CULL_MODE CullMode
	//BOOL FrontCounterClockwise //bs
	//cellGcmSetCullFace
	m_CullFace	=	rState.CullMode==D3D11_CULL_FRONT?CELL_GCM_FRONT:CELL_GCM_BACK;
	m_CullFaceEnable	=	rState.CullMode!=D3D11_CULL_NONE;
	
		
  m_PolygonOffset = ((rState.DepthBias>0) || rState.SlopeScaledDepthBias>0.0f)?CELL_GCM_TRUE:CELL_GCM_FALSE;
  m_fDepthBias = f32(rState.DepthBias);
  m_fSlopeScaledDepthBias = rState.SlopeScaledDepthBias;

	//BOOL DepthClipEnable
	//currently ignored

	//BOOL ScissorEnable
	//cellGcmSetScissor
	m_ScissorTestEnable	=	rState.ScissorEnable;// if(false)cellGcmSetScissor(0,0,4095,4095);

	//BOOL MultisampleEnable;
	//BOOL AntialiasedLineEnable;
	//currently ignored

	m_AlphaTestFunc			=	g_Comp2Gcm[rState.AlphaTestFunc];
	m_AlphaTestEnable		=	rState.AlphaTestEnable?CELL_GCM_TRUE:CELL_GCM_FALSE;
}
