#include "StdAfx.h"
#include "../../../Layer0/CCryDXPS.hpp"
#include "../../../CCryDXPSMisc.hpp"
#include "CCryDXPSTexture3D.hpp"
//#include <assert.h>

CCryDXPSTexture3D::CCryDXPSTexture3D(uint32 SizeX,uint32 SizeY,uint32 SizeZ,uint32 Mips,DXGI_FORMAT Format MMRES_PARAM):
CCryDXPSTexture(EDXPS_RT_TEX3D,SizeX,SizeY,SizeZ MMRES_PASS)
{
	MMRES_ADDCOUNT();
	MMRES_ADDMM(sizeof(CCryDXPSTexture3D));

	m_Format	=	Format;


	if(!SizeX || !SizeY || !SizeZ)
	{
		CRY_ASSERT_MESSAGE(!SizeX,"Trying to create 3d-texture with Zero width");
		CRY_ASSERT_MESSAGE(!SizeY,"Trying to create 3d-texture with Zero height");
		CRY_ASSERT_MESSAGE(!SizeZ,"Trying to create 3d-texture with Zero Depth");
		SizeX	=	1;
		SizeY	=	1;
		SizeZ	=	1;
	}

	if(DXGI_FORMAT2GCM[Format]>=255)
	{
#if defined(_DEBUG)
		CRY_DEBUGOUT("unsupported pixelformat %d\n",Format);
#endif
		return;
	}

#ifdef CRY_MM_DEBUG
		CRY_DEBUGOUT("Creating Texture(%d,%d,%d) with Pixelsize of %d bits\n",SizeX,SizeY,SizeZ,GCM_TextureFormatSize(8,DXGI_FORMAT2GCM[Format]));
#endif
	m_GCMTexture.format	=	DXGI_FORMAT2GCM[Format];

	m_GCMTexture.width	=	SizeX;
	m_GCMTexture.height	=	SizeY;
	m_GCMTexture.depth	=	SizeZ;
	m_GCMTexture.mipmap	=	1;

	m_NoFiltering	=	m_GCMTexture.format==CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT |
									m_GCMTexture.format==CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT |
									m_GCMTexture.format==CELL_GCM_TEXTURE_X32_FLOAT |
									m_GCMTexture.format==CELL_GCM_TEXTURE_DEPTH16_FLOAT;

	m_IsFloat	=	m_GCMTexture.format==CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT |
							m_GCMTexture.format==CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT |
							m_GCMTexture.format==CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT |
							m_GCMTexture.format==CELL_GCM_TEXTURE_Y16_X16_FLOAT |
							m_GCMTexture.format==CELL_GCM_TEXTURE_X32_FLOAT |
							m_GCMTexture.format==CELL_GCM_TEXTURE_DEPTH16_FLOAT;

	m_GCMTexture.format|=	CELL_GCM_TEXTURE_SZ;
	m_GCMTexture.pitch	=	Pitch(0);
	//OPTIMIZE TODO PS3workaround memory calculation could be optimized slightly by taking the real number of mips into account
	const uint32 S=SizeX*SizeY*SizeZ;


	m_GCMTexture.dimension	=	CELL_GCM_TEXTURE_DIMENSION_3;
	m_GCMTexture.cubemap		=	CELL_GCM_FALSE;
	m_GCMTexture.location		=	CELL_GCM_LOCATION_LOCAL;

	MemItemID(tdLayer0::Memory().Allocate(GCM_TextureFormatSize(S,DXGI_FORMAT2GCM[Format])));

	if(DXGI_FORMAT2GCM[Format]==CELL_GCM_TEXTURE_B8)
	{
		m_GCMTexture.remap =	CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
													CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
													CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
													CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
													CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
													CELL_GCM_TEXTURE_REMAP_FROM_B << 4 |
													CELL_GCM_TEXTURE_REMAP_FROM_B << 2 |
													CELL_GCM_TEXTURE_REMAP_FROM_B;
	}
	else
	{
		m_GCMTexture.remap =	CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
													CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
													CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
													CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
													CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
													CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
													CELL_GCM_TEXTURE_REMAP_FROM_R << 2 |
													CELL_GCM_TEXTURE_REMAP_FROM_A;
	}
	cellGcmAddressToOffset(RawPointer(),&m_GCMTexture.offset);
}
/*
long CCryDXPSTexture3D::Map(uint32 Subresource,D3D11_MAP MapType,uint32 MapFlags,void** ppData)
{
	ppData->pData			=	RawPointer();
	ppData->RowPitch	=	Pitch();
	IncModCount();
	Sync();//SLOW
	return 0;
}

long CCryDXPSTexture3D::Unmap(uint32 Subresource)
{
	using namespace CRY_DXPS_GCMNAMESPACE;

	return 0;
}
*/
long CCryDXPSTexture3D::GetDesc(D3D11_TEXTURE3D_DESC *pDesc)
{
	return 0;
}
