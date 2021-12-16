#include "StdAfx.h"
#include "../../../Layer0/CCryDXPS.hpp"
#include "../../../CCryDXPSMisc.hpp"
#include "CCryDXPSTexture2D.hpp"
//#include <assert.h>

extern SHWOccZBuffer HWZBuffer;

CCryDXPSTexture2D::CCryDXPSTexture2D(uint32 SizeX,uint32 SizeY,uint32 Mips,DXGI_FORMAT Format,ECryDXPSTex2DCreateFlags CreateFlags,bool CubeMap MMRES_PARAM):
CCryDXPSTexture(EDXPS_RT_TEX2D,SizeX,SizeY,1 MMRES_PASS),
m_Mode(CreateFlags)
{
	MMRES_ADDCOUNT();
	MMRES_ADDMM(sizeof(CCryDXPSTexture2D));

	//PS3workaround no runtime conversion but streaming loads BC5 textures
	//MartinM will make a dxt 
	if(Format==DXGI_FORMAT_BC5_UNORM)
	{
		CRY_DEBUGOUT("unsupported texture format: DXGI_FORMAT_BC5_UNORM for this device");
		Format	=	DXGI_FORMAT_BC3_UNORM;
	}
	m_Format	=	Format;

	if(!SizeX || !SizeY)
	{
		CRY_ASSERT_MESSAGE(!SizeX,"Trying to create texture with Zero width");
		CRY_ASSERT_MESSAGE(!SizeY,"Trying to create texture with Zero height");
		SizeX	=	1;
		SizeY	=	1;
	}


	if(DXGI_FORMAT2GCM[Format]>=255)
	{
#if defined(_DEBUG)
		CRY_DEBUGOUT("unsupported pixelformat %d\n",Format);
		while(true)
		{
			int a=0;
		}
#endif
		return;
	}
#ifdef CRY_MM_DEBUG
		CRY_DEBUGOUT("Creating Texture(%d,%d) with Pixelsize of %d Bits\n",SizeX,SizeY,GCM_TextureFormatSize(8,DXGI_FORMAT2GCM[Format]));
#endif
	m_GCMTexture.format	=	DXGI_FORMAT2GCM[Format];

	//needed for  Pitch()
	m_GCMTexture.width	=	SizeX;
	m_GCMTexture.height	=	SizeY;
	m_GCMTexture.mipmap	=	Mips;

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

	uint32 S=0;
	if(Po2(SizeX) && Po2(SizeY) && !m_IsFloat)
	{
		m_GCMTexture.format|=CELL_GCM_TEXTURE_SZ;
		m_GCMTexture.pitch	=	0;
		uint32 X=SizeX;
		uint32 Y=SizeY;
		if(Mips>1)
		{
			do
			{
				S+=X*Y;
				X>>=1;
				Y>>=1;
				if(X<1)	X=1;
				if(Y<1)	Y=1;
			}while(X>4 || Y>4);
		}
		else
			S=X*Y;
	}
	else
	{
		m_GCMTexture.format|=CELL_GCM_TEXTURE_LN;
		m_GCMTexture.pitch	=	Pitch(0);
		//OPTIMIZE TODO PS3workaround memory calculation could be optimized slightly by taking the real number of mips into account
		S=(SizeX*SizeY*(Mips>1?2:1));
	}
	if(CubeMap)
	{
		S	=	(S+127)&~127;//128byte allignment needed
		S*=	6;//6sides on a cube
	}
	if(CreateFlags==ECDXPSTCF_DUMMY)
		MemItemID(0);
	else
		MemItemID(tdLayer0::Memory().Allocate(GCM_TextureFormatSize(S,DXGI_FORMAT2GCM[Format]),CreateFlags==ECDXPSTCF_FRAMEBUFFER?0x10000:0));
	MMRES_ADDVM(GCM_TextureFormatSize(S,DXGI_FORMAT2GCM[Format]));

	m_GCMTexture.dimension	=	CELL_GCM_TEXTURE_DIMENSION_2;
	m_GCMTexture.cubemap	=	CubeMap?CELL_GCM_TRUE:CELL_GCM_FALSE;
	m_GCMTexture.depth	=	1;
	m_GCMTexture.location	=	CELL_GCM_LOCATION_LOCAL;
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


long CCryDXPSTexture2D::Map(uint32 SubResource,D3D11_MAP MapType,uint32 MapFlags,D3D11_MAPPED_SUBRESOURCE* ppData)
{
	using namespace CRY_DXPS_GCMNAMESPACE;
	ppData->pData			=	reinterpret_cast<uint8*>(RawPointer());
	ppData->RowPitch	=	RowPitch(SubResource);

	uint32 ArraySize=0;
	if(m_GCMTexture.cubemap==CELL_GCM_TRUE)
	{
		ArraySize	=	SubResource/m_GCMTexture.mipmap;
		SubResource	%=	m_GCMTexture.mipmap;
	}

	if((SizeY()>>(SubResource>0?SubResource-1:0))<1 || SubResource >= m_GCMTexture.mipmap)
	{
		CRY_ASSERT_MESSAGE(0,"Trying to map wrong mip");
		CRY_DEBUGOUT("Trying to map wrong mip %d, number of mips %d\n",SubResource,m_GCMTexture.mipmap);
		return -1;
	}

	const uint32 Offset=CalcOffset(ArraySize,SubResource);
	ppData->pData+=Offset;
	ppData->RowPitch=RowPitch(SubResource);
	IncModCount();
	Sync();
	return 0;
}

long CCryDXPSTexture2D::Unmap(uint32 Subresource)
{
	return 0;
}

long CCryDXPSTexture2D::GetDesc(D3D11_TEXTURE2D_DESC *pDesc)
{

	pDesc->Width							=	SizeX();
	pDesc->Height							=	SizeY();
	pDesc->MipLevels					=	m_GCMTexture.mipmap;
	pDesc->ArraySize					=	m_GCMTexture.cubemap!=CELL_GCM_TRUE?6:1;
	pDesc->Format							=	m_Format;
	pDesc->SampleDesc.Count		=	1;
	pDesc->SampleDesc.Quality	=	0;
	pDesc->Usage							=	D3D11_USAGE_DEFAULT;
	pDesc->CPUAccessFlags			=	D3D11_CPU_ACCESS_WRITE |D3D11_CPU_ACCESS_READ;
	pDesc->MiscFlags					=	m_GCMTexture.cubemap==CELL_GCM_TRUE?D3D11_RESOURCE_MISC_TEXTURECUBE:0;
	return 0;
}

uint32 CCryDXPSTexture2D::MakeTiled(uint32 Compression,uint32 HeightAllign,DXPSRTVDTilingInfo TilingInfo)
{
	if(TilingInfo==DXPSRTVDTI_NOTILING)
	{
		if(m_IsFloat)
			TilingInfo=DXPSRTVDTI_FORCETILING;
		else
		if(m_SizeX&(m_SizeX-1))
			TilingInfo=DXPSRTVDTI_FORCETILING;
		else
		if(m_SizeY&(m_SizeY-1))
			TilingInfo=DXPSRTVDTI_FORCETILING;
		else
		if(m_SizeX<16)
			TilingInfo=DXPSRTVDTI_FORCETILING;
	}
	if(m_Mode==ECDXPSTCF_FRAMEBUFFER && !(TilingInfo==DXPSRTVDTI_FORCETILING && (m_GCMTexture.format&CELL_GCM_TEXTURE_LN)))
		return ~0;

	const bool ZBuffer	=	((Compression==CELL_GCM_COMPMODE_Z32_SEPSTENCIL) || (Compression==CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REGULAR)) && tdLayer0::ZCull().Count()==0;

	m_Mode	=	ECDXPSTCF_FRAMEBUFFER;

	if(TilingInfo!=DXPSRTVDTI_NOTILING)
		DisableSwizzling();
	ReleaseResources();

#if defined(CRY_DXPS_TILEDREGIONS)
	if(Format() != DXGI_FORMAT_B5G6R5_UNORM)
		m_SizeX	=	cellGcmGetTiledPitchSize(m_SizeX*4)/4;
	else
		m_SizeX	=	cellGcmGetTiledPitchSize(m_SizeX*8)/8;

	m_SizeY	=	ZBuffer&&m_SizeY==704?720:m_SizeY;
	m_SizeY	=	(m_SizeY+(HeightAllign-1))&~(HeightAllign-1);
#endif

	m_GCMTexture.pitch	=	Pitch(0);
	const uint32 S=m_SizeX*m_SizeY;

	MemItemID(tdLayer0::Memory().Allocate(GCM_TextureFormatSize(S,DXGI_FORMAT2GCM[m_Format]),0x10000));

	uint32 ID	=	CRY_TRM_COUNT;
#if defined(CRY_DXPS_TILEDREGIONS)
	cellGcmAddressToOffset(RawPointer(),&m_GCMTexture.offset);
	if(TilingInfo!=DXPSRTVDTI_NOTILING)
	{
		const uint32 tilePitch = cellGcmGetTiledPitchSize(m_GCMTexture.pitch);
		ID	=	tdLayer0::TiledRegion().Create(m_GCMTexture.offset,tilePitch,tilePitch*m_SizeY,Compression);
	}
#endif
	LockVMem();
	return	ID;
}

uint32 CCryDXPSTexture2D::MakeZCull()
{
	if(m_Mode!=ECDXPSTCF_FRAMEBUFFER)
		return ~0;

	uint32 ID	=	0;
	if(!HWZBuffer.pZBufferVMem)
	{
		HWZBuffer.pZBufferVMem =	(uint32*)RawPointer();
		cellGcmAddressToOffset(HWZBuffer.pZBufferVMem, &HWZBuffer.ZBufferVMemRSXOff);
	}
#if defined(CRY_DXPS_TILEDREGIONS)
	cellGcmAddressToOffset(RawPointer(),&m_GCMTexture.offset);
	ID	=	tdLayer0::ZCull().Create(m_GCMTexture.offset,m_SizeX,m_SizeY);
#endif
	LockVMem();
	return	ID;
}

