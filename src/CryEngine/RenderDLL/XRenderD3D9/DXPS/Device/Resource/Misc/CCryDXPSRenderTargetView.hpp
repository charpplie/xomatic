#ifndef __CRYDXPSRENDERTARGETVIEW__
#define __CRYDXPSRENDERTARGETVIEW__

#include "../CCryDXPSResource.hpp"
#include "../../../CCryDXPSMisc.hpp"
#include "../../../Layer0/CCryDXPS.hpp"
#include "../Textures/CCryDXPSTexture2D.hpp"

class CCryDXPSRenderTargetView;
class CCryDXPSRenderTargetView	:	public CCryDXPSResource	,	public CCryRefAndWeak<CCryDXPSRenderTargetView>
{
private:
	APWeakTexture2D			m_pTex;
#if defined(CRY_DXPS_TILEDREGIONS)
	uint8								m_TiledRegion;
#endif
public:
											CCryDXPSRenderTargetView(CCryDXPSTexture2D* pTex,DXPSRTVDTilingInfo TilingInfo MMRES_PARAM):
											CCryDXPSResource(EDXPS_RT_RENDERTARGETVIEW MMRES_PASS),
											m_pTex(pTex)
											{
												MMRES_ADDCOUNT();
												MMRES_ADDMM(sizeof(CCryDXPSRenderTargetView));
#if defined(CRY_DXPS_TILEDREGIONS)
												m_TiledRegion	=	m_pTex->MakeTiled((m_pTex->IsFloat()||m_pTex->Format() == DXGI_FORMAT_B5G6R5_UNORM)?CELL_GCM_COMPMODE_DISABLED:CELL_GCM_COMPMODE_C32_2X1,32,TilingInfo);
#endif
											}
	CCryDXPSTexture2D*	Texture(){return m_pTex;}
	void								Texture(CCryDXPSTexture2D* pTex){m_pTex=pTex;}
											~CCryDXPSRenderTargetView()
											{
												MMRES_SUBCOUNT();
												MMRES_SUBMM(sizeof(CCryDXPSRenderTargetView));
#if defined(CRY_DXPS_TILEDREGIONS)
												tdLayer0::TiledRegion().Free(m_TiledRegion);
#endif
#if defined(CRY_DXPS_DESTRUCTORVALIDATION)
													if(EDXPS_RT_RENDERTARGETVIEW==CCryDXPSResource::Type())
														CCryDXPSResource::Type(EDXPS_RT_NONE);
#endif
											}
	inline	void				ReleaseResources()
											{
											}

	void								GetDesc(D3D11_RENDER_TARGET_VIEW_DESC *pDesc);
	void								GetResource(ID3D11Resource** pResource);
	inline int					Release(){return DecRef();}
};

typedef CCryDXPSRenderTargetView ID3D11RenderTargetView;

#endif

