#ifndef __CRYDXPSTEXTURE2D__
#define __CRYDXPSTEXTURE2D__

#include "CCryDXPSTextureBase.hpp"


//class CCryDXPSSwapChain;

class CCryDXPSTexture2D;
class CCryDXPSTexture2D		:		public CCryDXPSTexture
{
	friend class CCryDXPSGCMSyncMan;
	ECryDXPSTex2DCreateFlags	m_Mode;

	//should never be called, but of the base-texture
	inline										~CCryDXPSTexture2D(){}

public:
														CCryDXPSTexture2D(uint32 SizeX,uint32 SizeY,uint32 Mips,DXGI_FORMAT Format,ECryDXPSTex2DCreateFlags CreateFlags,bool CubeMap MMRES_PARAM);

	uint32										MakeTiled(uint32 Compression,uint32 HeightAllign,DXPSRTVDTilingInfo TilingInfo);
	uint32										MakeZCull();
	long											Map(uint32 Subresource,D3D11_MAP MapType,uint32 MapFlags,D3D11_MAPPED_SUBRESOURCE* ppData);
	long											Unmap(uint32 Subresource);
	long											GetDesc(D3D11_TEXTURE2D_DESC *pDesc);
	inline void								Swap(CCryDXPSTexture2D& rTex)
														{
															CellGcmTexture GCMTexture;
															for(uint32 i=0; i<sizeof(CellGcmTexture)/4;++i)
																((uint32*)&GCMTexture)[i] = ((uint32*)&m_GCMTexture)[i];//avoid lhs
															const DXGI_FORMAT	Format=	m_Format;
//															const uint16 MemItemID	=	MemItemID();
															uint8* pRawData	=	m_pRawData;
															for(uint32 i=0; i<sizeof(CellGcmTexture)/4;++i)
																((uint32*)&m_GCMTexture)[i] = ((uint32*)&rTex.m_GCMTexture)[i];//avoid lhs
															m_Format		=	rTex.m_Format;
//															MemItemID(rTex.MemItemID());
															m_pRawData	=	rTex.m_pRawData;
															rTex.m_Format			=	Format;
															rTex.m_GCMTexture	=	GCMTexture;
//															rTex.MemItemID(MemItemID());
															rTex.m_pRawData	=	pRawData;
														}
	void											Mode(ECryDXPSTex2DCreateFlags mode){m_Mode=mode;}
	ECryDXPSTex2DCreateFlags	Mode()const{return m_Mode;}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

typedef CCryDXPSTexture2D ID3D11Texture2D;
#ifdef CRY_DXPS_VALIDATEWEAKPTR
	typedef CCryAPtrRefCnt<CCryDXPSTexture2D>		APRefTexture2D;
	typedef CCryAPtrWeakCnt<CCryDXPSTexture2D>	APWeakTexture2D;
#else
	typedef CCryDXPSTexture2D*	__restrict 	APRefTexture2D;
	typedef CCryDXPSTexture2D*	__restrict	APWeakTexture2D;
#endif

#endif

