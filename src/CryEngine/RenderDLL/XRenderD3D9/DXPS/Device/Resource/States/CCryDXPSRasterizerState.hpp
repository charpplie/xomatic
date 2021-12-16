#ifndef __CRYDXPSRASTERIZERSTATE__
#define __CRYDXPSRASTERIZERSTATE__

#include "../CCryDXPSResource.hpp"
#include "../../../CCryDXPSAPtr.hpp"
#include "../../../Layer0/CCryDXPS.hpp"

class CCryDXPSRasterizerState;
class CCryDXPSRasterizerState	:	public CCryDXPSResource	,	public	CCryRefAndWeak<CCryDXPSRasterizerState>
{

	//D3D11_FILL_MODE FillMode
	//cellGcmSetFrontPolygonMode
	uint32	m_FillMode;

	//D3D11_CULL_MODE CullMode
	//BOOL FrontCounterClockwise //bs
	//cellGcmSetCullFace
	uint32	m_CullFace;
	uint32	m_CullFaceEnable;

  uint32 m_PolygonOffset;
  f32 m_fDepthBias;
  f32 m_fSlopeScaledDepthBias;

	//INT DepthBias;
	//FLOAT DepthBiasClamp
	//FLOAT SlopeScaledDepthBias
	//currently ignored


	//BOOL DepthClipEnable
	//cellGcmSetDepthTestEnable
	uint32	m_DepthTestEnable;

	//BOOL ScissorEnable
	//cellGcmSetScissor
	uint32 m_ScissorTestEnable;// if(false)cellGcmSetScissor(0,0,4095,4095);

	//BOOL MultisampleEnable;
	//BOOL AntialiasedLineEnable;
	//currently ignored



	//ps3 specific extension
	uint32 m_AlphaTestFunc;
	uint32 m_AlphaTestEnable;

public:
	CCryDXPSRasterizerState(const D3D11_RASTERIZER_DESC& rState MMRES_PARAM);
	~CCryDXPSRasterizerState()
	{
		MMRES_SUBCOUNT();
		MMRES_SUBMM(sizeof(CCryDXPSRasterizerState));
	}

	inline void Set(uint32 AlphaTestRef)	const
	{
		using namespace CRY_DXPS_GCMNAMESPACE;
		cellGcmSetCullFaceEnable(m_CullFaceEnable);
		if(m_CullFaceEnable)
		{
			cellGcmSetCullFace(m_CullFace);
			switch(m_CullFace)
			{
			case CELL_GCM_FRONT:
				cellGcmSetFrontPolygonMode(m_FillMode);
				break;
			case CELL_GCM_BACK:
				cellGcmSetBackPolygonMode(m_FillMode);
				break;
			case CELL_GCM_FRONT_AND_BACK:
				cellGcmSetBackPolygonMode(m_FillMode);
				cellGcmSetFrontPolygonMode(m_FillMode);
				break;
			}
		}
		else
		{
			cellGcmSetBackPolygonMode(m_FillMode);
			cellGcmSetFrontPolygonMode(m_FillMode);
		}
		if(!m_ScissorTestEnable)
			cellGcmSetScissor(0,0,4096,4096);
		if(m_AlphaTestEnable)
			cellGcmSetAlphaFunc(m_AlphaTestFunc,AlphaTestRef);
		cellGcmSetAlphaTestEnable(m_AlphaTestEnable);

    cellGcmSetPolygonOffsetFillEnable(m_PolygonOffset);
    if (m_PolygonOffset)
    {
      cellGcmSetPolygonOffset(m_fSlopeScaledDepthBias, m_fDepthBias);
    }
	}

	inline	void		ReleaseResources()
	{
	}

	inline void GetDesc(D3D11_RASTERIZER_DESC *pDesc)
	{
	}
};

typedef CCryDXPSRasterizerState ID3D11RasterizerState;
#ifdef CRY_DXPS_VALIDATEWEAKPTR
	typedef CCryAPtrWeakCnt<CCryDXPSRasterizerState>	APWeakRasterizerState;
#else
	typedef CCryDXPSRasterizerState*	__restrict APWeakRasterizerState;
#endif

#endif

