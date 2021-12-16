#ifndef __CRYDXPSBLENDSTATE__
#define __CRYDXPSBLENDSTATE__

#include "../CCryDXPSResource.hpp"
#include "../../../CCryDXPSAPtr.hpp"
#include "../../../Layer0/CCryDXPS.hpp"

class CCryDXPSBlendState;
class CCryDXPSBlendState	:	public CCryDXPSResource	,	public	CCryRefAndWeak<CCryDXPSBlendState>
{
	//cellGcmSetBlendEnable
	union
	{
		uint8		m_BlendEnable[4];
		uint32	m_Blends;
	};

	//cellGcmSetBlendEquation
	uint16		m_BlendFuncColor;
	uint16		m_BlendFuncAlpha;

	//cellGcmSetBlendFunc
	uint16		m_BlendDstColorFunc;
	uint16		m_BlendSrcColorFunc;
	uint16		m_BlendDstAlphaFunc;
	uint16		m_BlendSrcAlphaFunc;

	//cellGcmSetColorMask
	uint32		m_ColorMask;
	//cellGcmSetColorMaskMrt
	uint32		m_ColorMaskMrt;

public:
	CCryDXPSBlendState(const D3D11_BLEND_DESC& rState MMRES_PARAM);
	~CCryDXPSBlendState()
	{
		MMRES_SUBCOUNT();
		MMRES_SUBMM(sizeof(CCryDXPSBlendState));
	}

	inline uint32 Set() const
	{
		using namespace CRY_DXPS_GCMNAMESPACE;
		cellGcmSetBlendEnable(m_BlendEnable[0]);
		cellGcmSetBlendEnableMrt(m_BlendEnable[1],m_BlendEnable[2],m_BlendEnable[3]);
		if(m_Blends)
		{
			cellGcmSetBlendEquation(m_BlendFuncColor,m_BlendFuncAlpha);
			cellGcmSetBlendFunc(m_BlendSrcColorFunc,m_BlendDstColorFunc,m_BlendSrcAlphaFunc,m_BlendDstAlphaFunc);
		}
		cellGcmSetColorMask(m_ColorMask);
		cellGcmSetColorMaskMrt(m_ColorMaskMrt);
		return m_ColorMask;
	}

	inline	void		ReleaseResources()
	{
	}

	inline void GetDesc(D3D11_BLEND_DESC *pDesc)
	{
	}
};

typedef CCryDXPSBlendState ID3D11BlendState;
#ifdef CRY_DXPS_VALIDATEWEAKPTR
	typedef CCryAPtrWeakCnt<CCryDXPSBlendState>	APWeakBlendState;
#else
	typedef CCryDXPSBlendState*__restrict 	APWeakBlendState;
#endif

#endif

