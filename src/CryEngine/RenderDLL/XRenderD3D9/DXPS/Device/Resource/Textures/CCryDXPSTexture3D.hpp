#ifndef __CRYDXPSTEXTURE3D__
#define __CRYDXPSTEXTURE3D__

#include "CCryDXPSTextureBase.hpp"

class CCryDXPSTexture3D;
class CCryDXPSTexture3D		:		public CCryDXPSTexture
{
	//should never be called, but of the base-texture
	friend class CCryDXPSGCMSyncMan;
	inline	~CCryDXPSTexture3D(){}
public:
	CCryDXPSTexture3D(uint32 SizeX,uint32 SizeY,uint32 SizeZ,uint32 Mips,DXGI_FORMAT Format MMRES_PARAM);

	//unused yet
//	long						Map(uint32 Subresource,D3D11_MAP MapType,uint32 MapFlags,void** ppData);
//	long						Unmap(uint32 Subresource);
	long						GetDesc(D3D11_TEXTURE3D_DESC *pDesc);
};

typedef CCryDXPSTexture3D ID3D11Texture3D;
#ifdef CRY_DXPS_VALIDATEWEAKPTR
	typedef CCryAPtrRefCnt<CCryDXPSTexture3D>		APRefTexture3D;
	typedef CCryAPtrWeakCnt<CCryDXPSTexture3D>	APWeakTexture3D;
#else
	typedef CCryDXPSTexture3D*	__restrict APRefTexture3D;
	typedef CCryDXPSTexture3D*	__restrict APWeakTexture3D;
#endif

#endif

