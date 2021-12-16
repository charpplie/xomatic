#ifndef __CRYDXPSDEPTHSTENCILSTATE__
#define __CRYDXPSDEPTHSTENCILSTATE__

#include "../CCryDXPSResource.hpp"
#include "../../../CCryDXPSAPtr.hpp"
#include "../../../Layer0/CCryDXPS.hpp"

class CCryDXPSDepthStencilState;
class CCryDXPSDepthStencilState	:	public CCryDXPSResource	,	public	CCryRefAndWeak<CCryDXPSDepthStencilState>
{
	//BOOL DepthEnable;
	//cellGcmSetDepthTestEnable
	uint32 m_DepthTestEnable;

  //D3D11_DEPTH_WRITE_MASK DepthWriteMask;
	//cellGcmSetDepthMask
	uint32	m_DepthMask;

	//D3D11_COMPARISON_FUNC DepthFunc;
	//cellGcmSetDepthFunc
	uint32	m_DepthFunc;

	//BOOL StencilEnable;
	//cellGcmSetStencilEnable
	uint32 m_StencilTestEnable;

	//UINT8 StencilReadMask;
  //UINT8 StencilWriteMask;
	//cellGcmSetStencilMask
	uint32 m_StencilMaskW;

	//cellGcmSetStencilFunc
	uint32 m_StencilFunc;
	//uint32 m_StencilRef; //is will be passed on setting of the state
	uint32 m_StencilMaskR;

	//cellGcmSetStencilOp
	uint32 m_StencilOpFail;
	uint32 m_StencilOpDepthFail;
	uint32 m_StencilOpDepthPass;

  //D3D11_DEPTH_STENCILOP_DESC FrontFace;
  //D3D11_DEPTH_STENCILOP_DESC BackFace;

public:
	CCryDXPSDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& rState MMRES_PARAM);
	~CCryDXPSDepthStencilState()
	{
		MMRES_SUBCOUNT();
		MMRES_SUBMM(sizeof(CCryDXPSDepthStencilState));
	}

  void Set(uint32 StencilRef) const;

	inline	void		ReleaseResources()
	{
	}

	inline void GetDesc(D3D11_DEPTH_STENCIL_DESC *pDesc)
	{
	}
};

typedef CCryDXPSDepthStencilState ID3D11DepthStencilState;
#ifdef CRY_DXPS_VALIDATEWEAKPTR
	typedef CCryAPtrWeakCnt<CCryDXPSDepthStencilState>	APWeakDepthStencilState;
#else
	typedef CCryDXPSDepthStencilState* __restrict 	APWeakDepthStencilState;
#endif

ILINE void CCryDXPSDepthStencilState::Set(uint32 StencilRef) const
{
  using namespace CRY_DXPS_GCMNAMESPACE;
  cellGcmSetDepthTestEnable(m_DepthTestEnable);
  if(m_DepthTestEnable)
  {
    cellGcmSetDepthFunc(m_DepthFunc);
		cellGcmSetDepthMask(m_DepthMask);
	}

  cellGcmSetStencilTestEnable(m_StencilTestEnable);
  if(m_StencilTestEnable)
  {
    if( m_StencilMaskW==0x0 && 
        m_StencilOpFail==CELL_GCM_KEEP && 
        m_StencilOpDepthFail==CELL_GCM_KEEP && 
        m_StencilOpDepthPass==CELL_GCM_KEEP && 
        m_DepthTestEnable == CELL_GCM_FALSE
      ) 
    {
      //cellGcmSetColorMask(0);
      //cellGcmSetBlendEnable(CELL_GCM_FALSE);
      //cellGcmSetBlendEnableMrt(CELL_GCM_FALSE, CELL_GCM_FALSE, CELL_GCM_FALSE);
      //cellGcmSetAlphaTestEnable(CELL_GCM_FALSE);
      ////cellGcmSetZMinMaxControl(CELL_GCM_TRUE, CELL_GCM_TRUE, CELL_GCM_FALSE);
      //cellGcmSetDepthBoundsTestEnable(CELL_GCM_FALSE); 
      cellGcmSetDepthMask(CELL_GCM_FALSE);

      cellGcmSetStencilMask(0xFF);

      cellGcmSetScullControl((m_StencilFunc != CELL_GCM_NOTEQUAL) ? CELL_GCM_SCULL_SFUNC_EQUAL:CELL_GCM_SCULL_SFUNC_NOTEQUAL ,StencilRef,m_StencilMaskR);
      cellGcmSetStencilFunc(CELL_GCM_LESS, 0xFF, 0xFF);
      cellGcmSetStencilOp(CELL_GCM_KEEP, CELL_GCM_INVERT, CELL_GCM_KEEP);
      cellGcmSetTwoSidedStencilTestEnable(CELL_GCM_FALSE);

      cellGcmSetClearZcullSurface(CELL_GCM_FALSE, CELL_GCM_TRUE);
			cellGcmSetZcullEnable(CELL_GCM_FALSE,CELL_GCM_TRUE);
    }
    else
    {
			cellGcmSetZcullEnable(CELL_GCM_TRUE,CELL_GCM_TRUE);
      cellGcmSetStencilMask(m_StencilMaskW);
      //PS3workaround, CELL_GCM_NV4097_SET_STENCIL_FUNC_REF, should be removed for SDK190
#if defined(CELL_GCM_DEBUG)
      cellGcmDebugCheckEnable(CELL_GCM_FALSE);
#endif
      cellGcmSetStencilFunc(m_StencilFunc,StencilRef,m_StencilMaskR);
#if defined(CELL_GCM_DEBUG)
      cellGcmDebugCheckEnable(CELL_GCM_TRUE);
#endif
      cellGcmSetStencilOp(m_StencilOpFail,m_StencilOpDepthFail,m_StencilOpDepthPass);
    }
  }
}

#endif

