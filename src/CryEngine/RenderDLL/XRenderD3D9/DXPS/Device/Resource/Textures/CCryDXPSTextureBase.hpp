#ifndef __CRYDXPSTEXTUREBASE__
#define __CRYDXPSTEXTUREBASE__

#include "../CCryDXPSResource.hpp"
#include "../../../Layer0/CCryDXPS.hpp"


template<class T>
bool Po2(T x){return ((x & (x-1)) == 0);}


class CCryDXPSTexture;
class CCryDXPSTexture	:	public CCryDXPSResource	,	public	CCryRefAndWeak<CCryDXPSTexture>
{
protected:
	CellGcmTexture								m_GCMTexture;
	DXGI_FORMAT										m_Format;
private:
	uint16												m_MemItemID;
protected:
	uint8*												m_pRawData;
	//we need the resolution mirrored as the real
	//texture resolution might change due to padding
	//for zcull and tiled region rendering
	uint16												m_SizeX;
	uint16												m_SizeY;
	uint16												m_SizeZ:14;//4096 -> 12 bit limit for tex res
	uint16												m_IsFloat:1;
	uint16												m_NoFiltering:1;


#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	void													FreeStats();
#endif
public:
																CCryDXPSTexture(ECRYDXPSResType T,uint16 SizeX,uint16 SizeY,uint16 SizeZ MMRES_PARAM):
																CCryDXPSResource(T MMRES_PASS),
																m_SizeX(SizeX),
																m_SizeY(SizeY),
																m_SizeZ(SizeZ)
																{
																}
																inline	~CCryDXPSTexture()
																{
//																	ReleaseResources();
																	MMRES_SUBCOUNT();
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
																	FreeStats();
#endif
#if defined(CRY_DXPS_DESTRUCTORVALIDATION)
																	if(EDXPS_RT_TEX1D==CCryDXPSResource::Type())
																		CCryDXPSResource::Type(EDXPS_RT_NONE);
																	if(EDXPS_RT_TEX2D==CCryDXPSResource::Type())
																		CCryDXPSResource::Type(EDXPS_RT_NONE);
																	if(EDXPS_RT_TEX3D==CCryDXPSResource::Type())
																		CCryDXPSResource::Type(EDXPS_RT_NONE);
#endif
																}

	ILINE	void									ReleaseResources()
																{
																	if(MemItemID())
																		tdLayer0::Memory().Free(MemItemID());
																	MemItemID(0);
																}

	ILINE const CellGcmTexture*	GcmTexture()const{return &m_GCMTexture;}
	ILINE uint32									Pitch(uint32 SubResource)const
																{
																	return (GCM_TextureFormatSize(VirtualSizeX(),DXGI_FORMAT2GCM[m_Format]))>>((m_GCMTexture.format&CELL_GCM_TEXTURE_LN)?0:SubResource);
																}
	ILINE uint32									RowPitch(uint32 SubResource)const
																{
																	return Pitch(SubResource)*((m_GCMTexture.format==CELL_GCM_TEXTURE_COMPRESSED_DXT1	||
																															m_GCMTexture.format==CELL_GCM_TEXTURE_COMPRESSED_DXT23 ||
																															m_GCMTexture.format==CELL_GCM_TEXTURE_COMPRESSED_DXT45)?4:1);
																}
	ILINE void										DisableSwizzling()
																{
																	m_GCMTexture.format|=CELL_GCM_TEXTURE_LN;
																	m_GCMTexture.pitch	=	Pitch(0);
																}
	ILINE void										ChangeFormat(DXGI_FORMAT Format)
																{
																	m_Format	=	Format;
																	m_GCMTexture.format&=(CELL_GCM_TEXTURE_LN|CELL_GCM_TEXTURE_UN);
																	m_GCMTexture.format|=DXGI_FORMAT2GCM[m_Format];

																}
	ILINE void										RemapDepth()
																{
																	ChangeFormat(DXGI_FORMAT_D24_UNORM_S8_UINT);
																}
	ILINE void										RemapToRGBA8()
																{
																	ChangeFormat(DXGI_FORMAT_R8G8B8A8_UINT);
																	m_GCMTexture.remap =	CELL_GCM_REMAP_MODE(CELL_GCM_TEXTURE_REMAP_ORDER_XYXY,
																																						CELL_GCM_TEXTURE_REMAP_FROM_B,
																																						CELL_GCM_TEXTURE_REMAP_FROM_A,
																																						CELL_GCM_TEXTURE_REMAP_FROM_R,
																																						CELL_GCM_TEXTURE_REMAP_FROM_G,
																																						CELL_GCM_TEXTURE_REMAP_REMAP,
																																						CELL_GCM_TEXTURE_REMAP_REMAP,
																																						CELL_GCM_TEXTURE_REMAP_REMAP,
																																						CELL_GCM_TEXTURE_REMAP_REMAP);
																}
	ILINE void										RemapDXT1AsNormalMap()
																{
																	// switch A and R channels for DXT1 texture to avoid another shader combination
																	CRY_ASSERT_MESSAGE(DXGI_FORMAT_BC1_UNORM_SRGB == m_Format || DXGI_FORMAT_BC1_UNORM == m_Format,"Trying to remap non-DXT1 texture as normal map");
																	m_GCMTexture.remap =	CELL_GCM_REMAP_MODE(CELL_GCM_TEXTURE_REMAP_ORDER_XYXY,
																																						CELL_GCM_TEXTURE_REMAP_FROM_R,
																																						CELL_GCM_TEXTURE_REMAP_FROM_A,
																																						CELL_GCM_TEXTURE_REMAP_FROM_G,
																																						CELL_GCM_TEXTURE_REMAP_FROM_B,
																																						CELL_GCM_TEXTURE_REMAP_REMAP,
																																						CELL_GCM_TEXTURE_REMAP_REMAP,
																																						CELL_GCM_TEXTURE_REMAP_REMAP,
																																						CELL_GCM_TEXTURE_REMAP_REMAP);
																}
	ILINE uint32									SizeX()const{return m_GCMTexture.width;}
	ILINE uint32									SizeY()const{return m_GCMTexture.height;}
	ILINE uint32									SizeZ()const{return m_GCMTexture.depth;}
	ILINE uint32									VirtualSizeX()const{return m_SizeX;}
	ILINE uint32									VirtualSizeY()const{return m_SizeY;}
	ILINE uint32									VirtualSizeZ()const{return m_SizeZ;}
	ILINE DXGI_FORMAT							Format()const{return m_Format;}
	ILINE SPU_DOMAIN_MAIN unsigned char*					RawPointer(){return m_pRawData;}			//unsigned char to get it compiled for spu
	ILINE SPU_DOMAIN_MAIN const unsigned char*		RawPointer()const{return m_pRawData;}
	ILINE int											Release(){return DecRef();}
	ILINE uint32									IsFloat()const{return m_IsFloat;}
	ILINE uint32									NoFiltering()const{return m_NoFiltering;}
	ILINE uint32									MipLevels()const{return m_GCMTexture.mipmap;}

	ILINE uint16									MemItemID()const{return m_MemItemID;}
	ILINE void										MemItemID(uint16 ID)
																{
																	m_MemItemID	=	ID;
																	m_pRawData	=	reinterpret_cast<uint8*>(ID?tdLayer0::Memory().ResolveHandle(ID):0);
																}

	ILINE uint32									CalcOffset(const uint32 ArraySize,const uint32 SubResource)const
																{
																	uint32 Offset=0;
																	for(uint32 b=0;b<=ArraySize;b++)
																	{
																		Offset	=	(Offset+127)&~127;
																		const bool Swizzled	=	(m_GCMTexture.format&CELL_GCM_TEXTURE_LN)!=CELL_GCM_TEXTURE_LN;
																		for(uint32 a=0;(b<ArraySize && a<m_GCMTexture.mipmap) || a<SubResource;a++)
																			Offset	+=	Pitch(Swizzled?a:0)*SizeY()>>a;
																	}
																	return Offset;
																}

	ILINE void										Offset(uint32_t Offset){ m_GCMTexture.offset = Offset;}

	void													LockVMem();
	ILINE void										ShareVMemFrameBuffer(CCryDXPSTexture* pTex,uint32 SizeY,uint32 OffsetY)
																{
																	m_SizeY	/=2;
																	m_GCMTexture.height	= SizeY;

#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
																	memcpy(pTex,this,sizeof(CCryDXPSTexture));
#else
																	*pTex	=	*this;
#endif
																	pTex->m_pRawData	=	m_pRawData	+	OffsetY*SizeX()*4;
																	cellGcmAddressToOffset(pTex->RawPointer(),&pTex->m_GCMTexture.offset);

																}

};


typedef CCryAPtrRefCnt<CCryDXPSTexture>		APRefTexture;
typedef CCryAPtrWeakCnt<CCryDXPSTexture>	APWeakTexture;


#endif

