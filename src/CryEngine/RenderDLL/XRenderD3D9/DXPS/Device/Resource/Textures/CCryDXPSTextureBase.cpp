#include "StdAfx.h"
#include "../../../Layer0/CCryDXPS.hpp"
#include "../../../CCryDXPSMisc.hpp"
#include "CCryDXPSTextureBase.hpp"
#include "CCryDXPSTexture1D.hpp"
#include "CCryDXPSTexture2D.hpp"
#include "CCryDXPSTexture3D.hpp"

#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
void CCryDXPSTexture::FreeStats()
{
		uint32 SizeX	=	m_GCMTexture.width;
		uint32 SizeY	=	m_GCMTexture.height;	
		uint32 SizeZ	=	m_GCMTexture.depth;
		uint32 Mips		=	m_GCMTexture.mipmap;

		if(Type()==EDXPS_RT_TEX1D)
		{
			MMRES_SUBMM(sizeof(CCryDXPSTexture1D));
			MMRES_SUBVM(GCM_TextureFormatSize(SizeX,DXGI_FORMAT2GCM[m_Format]));
		}
		else
		if(Type()==EDXPS_RT_TEX2D)
		{
			uint32 S;
			if(Po2(SizeX) && Po2(SizeY) && m_GCMTexture.format!=CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT)
				S=(Mips>1?SizeX*SizeY*4/3:SizeX*SizeY);
			else
				S=(SizeX*SizeY*(Mips>1?2:1));
			if(m_GCMTexture.cubemap)
			{
				S	=	(S+127)&~127;//128byte allignment needed
				S*=	6;//6sides on a cube
			}
			MMRES_SUBVM(GCM_TextureFormatSize(S,DXGI_FORMAT2GCM[m_Format]));
			MMRES_SUBMM(sizeof(CCryDXPSTexture2D));
		}
		else
		if(Type()==EDXPS_RT_TEX3D)
		{
			MMRES_SUBMM(sizeof(CCryDXPSTexture3D));
			MMRES_SUBVM(GCM_TextureFormatSize(SizeX*SizeY*SizeZ,DXGI_FORMAT2GCM[m_Format]));
		}
}
#endif


void CCryDXPSTexture::LockVMem()
{
	tdLayer0::Memory().Lock(MemItemID());
}

