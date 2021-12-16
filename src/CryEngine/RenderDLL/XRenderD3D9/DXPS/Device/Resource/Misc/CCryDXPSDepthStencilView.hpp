#ifndef __CRYDXPSDEPTHSTENCILVIEW__
#define __CRYDXPSDEPTHSTENCILVIEW__

#include "../CCryDXPSResource.hpp"
#include "../../../CCryDXPSMisc.hpp"
#include "../../../Layer0/CCryDXPS.hpp"
#include "../Textures/CCryDXPSTexture2D.hpp"

class CCryDXPSDepthStencilView;
class CCryDXPSDepthStencilView	:	public CCryDXPSResource	,	public CCryRefAndWeak<CCryDXPSDepthStencilView>
{
private:
	APWeakTexture2D						m_pTex;
#if defined(CRY_DXPS_TILEDREGIONS)
	uint16										m_ZCull;
	uint16										m_TiledRegion;
#endif
public:
														CCryDXPSDepthStencilView(CCryDXPSTexture2D* pTex MMRES_PARAM):
														CCryDXPSResource(EDXPS_RT_DEPTHSTENCILVIEW MMRES_PASS),
														m_pTex(pTex)
														{
															MMRES_ADDCOUNT();
															MMRES_ADDMM(sizeof(CCryDXPSDepthStencilView));
#if defined(CRY_DXPS_TILEDREGIONS)
															uint32 compression = m_pTex->IsFloat()? CELL_GCM_COMPMODE_Z32_SEPSTENCIL : CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REGULAR;
															
															m_TiledRegion	=	m_pTex->MakeTiled(compression,64,DXPSRTVDTI_FORCETILING);
															m_ZCull	=	m_pTex->MakeZCull();
#endif
														}
														~CCryDXPSDepthStencilView()
														{
															MMRES_SUBCOUNT();
															MMRES_SUBMM(sizeof(CCryDXPSDepthStencilView));
#if defined(CRY_DXPS_TILEDREGIONS)
															tdLayer0::TiledRegion().Free(m_TiledRegion);
															tdLayer0::ZCull().Free(m_ZCull);
#endif
#if defined(CRY_DXPS_DESTRUCTORVALIDATION)
															if(EDXPS_RT_DEPTHSTENCILVIEW==CCryDXPSResource::Type())
																CCryDXPSResource::Type(EDXPS_RT_NONE);
#endif
														}
	CCryDXPSTexture2D*				Texture(){return m_pTex;}

	inline	void							ReleaseResources()
														{
														}

	void											GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc);
	void											GetResource(ID3D11Resource** pResource);
	unsigned long							AddRef(){return IncRef();}
	inline int								Release(){return DecRef();}
};

typedef CCryDXPSDepthStencilView ID3D11DepthStencilView;

#endif

