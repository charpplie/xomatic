#ifndef __CRYDXPSSHADERRESOURCEVIEW__
#define __CRYDXPSSHADERRESOURCEVIEW__

#include "../CCryDXPSResource.hpp"
#include "../../../CCryDXPSMisc.hpp"
#include "../../../Layer0/CCryDXPS.hpp"
#include "../Textures/CCryDXPSTextureBase.hpp"

class CCryDXPSShaderResourceView;
class CCryDXPSShaderResourceView	:	public CCryDXPSResource	,	public CCryRefAndWeak<CCryDXPSShaderResourceView>
{
private:
	APRefTexture										m_pTex;
	D3D11_SHADER_RESOURCE_VIEW_DESC	m_ShaderResourceViewDesc;
public:
										CCryDXPSShaderResourceView(CCryDXPSTexture* pTex,const D3D11_SHADER_RESOURCE_VIEW_DESC* pSRV MMRES_PARAM):
										CCryDXPSResource(EDXPS_RT_SHADERRESOURCEVIEW MMRES_PASS),
										m_pTex(pTex),
										m_ShaderResourceViewDesc(*pSRV)
										{
												MMRES_ADDCOUNT();
												MMRES_ADDMM(sizeof(CCryDXPSShaderResourceView));
										}
										~CCryDXPSShaderResourceView()
										{
												MMRES_SUBCOUNT();
												MMRES_SUBMM(sizeof(CCryDXPSShaderResourceView));
#if defined(CRY_DXPS_DESTRUCTORVALIDATION)
												if(EDXPS_RT_SHADERRESOURCEVIEW==CCryDXPSResource::Type())
													CCryDXPSResource::Type(EDXPS_RT_NONE);
#endif
										}
	CCryDXPSTexture*	TextureRaw(){return m_pTex;}

	inline	void			ReleaseResources()
										{
										}

	void							GetDesc(struct D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc);
	void							GetResource(ID3D11Resource** pResource);
//	unsigned long			AddRef(){return IncRef();}
	APRefTexture&		Texture(){return m_pTex;}
	void							Texture(CCryDXPSTexture2D* pTex){m_pTex=pTex;}
	const D3D11_SHADER_RESOURCE_VIEW_DESC& 
										ResourceView()const{return m_ShaderResourceViewDesc;}
	inline int				Release(){return DecRef();}
	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	};
};

typedef CCryDXPSShaderResourceView ID3D11ShaderResourceView;

#endif

