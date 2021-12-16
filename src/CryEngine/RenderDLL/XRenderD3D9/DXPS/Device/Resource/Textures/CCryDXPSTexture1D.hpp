#ifndef __CRYDXPSTEXTURE1D__
#define __CRYDXPSTEXTURE1D__

#include "CCryDXPSTextureBase.hpp"

class CCryDXPSTexture1D;
class CCryDXPSTexture1D		:		public CCryDXPSTexture
{
	friend class CCryDXPSGCMSyncMan;
	//should never be called, but of the base-texture
	inline					~CCryDXPSTexture1D(){}
public:
									CCryDXPSTexture1D(uint32 SizeX,uint32 Mips,DXGI_FORMAT Format MMRES_PARAM);

	//unused yet
//	long						Map(uint32 Subresource,D3D11_MAP MapType,uint32 MapFlags,void** ppData);
//	long						Unmap(uint32 Subresource);
	long						GetDesc(D3D11_TEXTURE1D_DESC *pDesc);
};

typedef CCryDXPSTexture1D ID3D11Texture1D;
#ifdef CRY_DXPS_VALIDATEWEAKPTR
	typedef CCryAPtrRefCnt<CCryDXPSTexture1D>		APRefTexture1D;
	typedef CCryAPtrWeakCnt<CCryDXPSTexture1D>	APWeakTexture1D;
#else
	typedef CCryDXPSTexture1D*	__restrict 	APRefTexture1D;
	typedef CCryDXPSTexture1D*	__restrict	APWeakTexture1D;
#endif
#endif

