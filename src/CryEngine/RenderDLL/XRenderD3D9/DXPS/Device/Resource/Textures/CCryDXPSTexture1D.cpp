#include "StdAfx.h"
#include "../../../Layer0/CCryDXPS.hpp"
#include "../../../CCryDXPSMisc.hpp"
#include "CCryDXPSTexture1D.hpp"
//#include <assert.h>

CCryDXPSTexture1D::CCryDXPSTexture1D(uint32 SizeX,uint32 Mips,DXGI_FORMAT Format MMRES_PARAM):
CCryDXPSTexture(EDXPS_RT_TEX1D,SizeX,1,1 MMRES_PASS)
{
	MMRES_ADDCOUNT();
	MMRES_ADDMM(sizeof(CCryDXPSTexture1D));

	m_Format	=	Format;

	if(DXGI_FORMAT2GCM[Format]>=255)
	{
#if defined(_DEBUG)
		CRY_DEBUGOUT("unsupported pixelformat %d\n",Format);
#endif
		return;
	}
	m_GCMTexture.width	=	SizeX;
	m_GCMTexture.mipmap	=	Mips;

#ifdef CRY_MM_DEBUG
		CRY_DEBUGOUT("Creating Texture(%d) with Pixelsize of %d Bits\n",SizeX,GCM_TextureFormatSize(8,DXGI_FORMAT2GCM[Format]));
#endif
	MemItemID(tdLayer0::Memory().Allocate(GCM_TextureFormatSize(SizeX,DXGI_FORMAT2GCM[Format])));
	MMRES_ADDVM(GCM_TextureFormatSize(SizeX,DXGI_FORMAT2GCM[m_Format]));
}
/*
long CCryDXPSTexture1D::Map(uint32 Subresource,D3D11_MAP MapType,uint32 MapFlags,void** ppData)
{
	ppData->pData			=	RawPointer();
	ppData->RowPitch	=	Pitch();
	IncModCount();
	Sync();//SLOW
	return 0;
}

long CCryDXPSTexture1D::Unmap(uint32 Subresource)
{
	return 0;
}
*/
long CCryDXPSTexture1D::GetDesc(D3D11_TEXTURE1D_DESC *pDesc)
{
	return 0;
}

