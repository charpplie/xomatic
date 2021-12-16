#ifndef __CRYDXPSSHADERDESC__
#define __CRYDXPSSHADERDESC__


#define LOCAL_PS_BUFFER_SIZE (6*1024)
#define LOCAL_VS_BUFFER_SIZE (10*1024)
#define LOCAL_SHADER_GUARD_BUFFER_SIZE (4*1024)
#if !defined(__SPU__) && !defined(_RELEASE)
	#define cellGcmCpySyncVertexCode()
	#define cellGcmSyncUCodeLS()
	#define __spu_dma_to_ls_no_cache_no_sync memcpy
	#define __spu_sync_dma_no_cache_no_sync()
	extern uint8 g_LocalPSBuf[];
	extern uint8 g_LocalVSBuf[];
	ILINE void* cellGcmCpyUCodeLS(void* const __restrict pUCode, const uint32 cUCodeSize, const uint32 cOff)
	{
		if(cUCodeSize >= LOCAL_PS_BUFFER_SIZE)snPause();
		memcpy((void*)&g_LocalPSBuf[cOff], (const void*)pUCode, cUCodeSize);
		return (void*)&g_LocalPSBuf[cOff];
	}
	ILINE void* cellGcmCpyVertexCodeLS(void* const __restrict pMainUCode, const uint32 cUCodeSize, const uint32 cOff)
	{
		if(cUCodeSize >= LOCAL_VS_BUFFER_SIZE)snPause();
		memcpy((void*)&g_LocalVSBuf[cOff], (const void*)pMainUCode, cUCodeSize);
		return (void*)&g_LocalVSBuf[cOff];
	}
	#define cellGcmGetVSBuf() g_LocalVSBuf
	#define cellGcmGetPSBuf() g_LocalPSBuf
#else
	#include <console/Cry_PS3_Math.h>
#endif

#include "../../../Tools/DXPSShaderCompiler/DXPSShaderCompiler/Core/Reflect/DXPSReflectionShared.hpp"
class CCryDXPSShader;

class CDXPSShaderDesc	:	protected SShaderDesc
{
	inline uint8*												Data()const{return (uint8*)this;}
public:

	inline	uint32											ValidateVersion()const{return m_Version==DXPS_REF_SHADER_VERSION;}
	inline	uint32											PixelShader()const{return m_PixelShader;}

	inline	uint32											AttributeMask()const{return m_VertexAttributeMask;}

	inline	const SRefVertexAttribute*	VertexAttribute()const{return reinterpret_cast<const SRefVertexAttribute*>(&Data()[m_VertexAttributeOffset]);}
	inline	uint32											VertexAttributeCount()const{return m_VertexAttributeCount;}

	inline	const SRefSampler*					Sampler()const{return reinterpret_cast<const SRefSampler*>(&Data()[m_SamplerOffset]);}
	inline	uint32											SamplerCount()const{return m_SamplerCount;}

	inline	const SRefConstant*					Constant()const{return reinterpret_cast<const SRefConstant*>(&Data()[m_ConstOffset]);}
	inline	uint32											ConstantCount()const{return m_ConstCount;}

	inline	const SVSConst*							VSConstant()const{return reinterpret_cast<const SVSConst*>(&Data()[m_VSConstOffset]);}
	inline	uint32											VSConstantCount()const{return m_VSConstCount;}

	inline	const	uint16*								PatchVertexByOff(uint32 Off)const{return reinterpret_cast<const uint16*>(&Data()[Off]);}
	inline	const	uint16*								PatchVertex(uint32 Idx)const{return reinterpret_cast<const uint16*>(&Data()[m_PatchOffset[Idx]]);}

	inline	const	SRefPatch*						PatchPixelByOff(uint32 Off)const{return reinterpret_cast<const SRefPatch*>(&Data()[Off]);}
	inline	const	SRefPatch*						PatchPixel(uint32 Idx)const{return reinterpret_cast<const SRefPatch*>(&Data()[m_PatchOffset[Idx]]);}

	inline	uint32											PatchCount(uint32 Idx)const{return m_PatchCount[Idx];}

	inline	CGprogram										Program()const{return reinterpret_cast<CGprogram>(&Data()[m_ShaderBinaryOffset]);}
	inline	uint32											ProgramSize()const{return m_ShaderBinarySize;}

	inline	const char*									NameTable()const{return reinterpret_cast<const char*>(&Data()[m_NameTableOffset]);}
	inline	uint32											NameTableSize()const{return m_NameTableSize;}

	inline	void												RemoveProgramInfo()
																			{
																				m_NameTableOffset		-=	ProgramSize();
																				m_ShaderBinarySize	=		0;
																			}


	union SDataTemp
	{
		uint8 byteVals[2];
		uint16 shortVal;
	};	

	static uint32												



																			Decode(uint8*	__restrict pDst,const uint8* __restrict pSrc,uint32* __restrict pSize=0)

																			{
																				uint32	IdxIn	=	0;
																				uint32	IdxOut	=	0;
																				uint32	FinalSize		=	*reinterpret_cast<const uint32*>(pSrc)&0xffffff;
																				if(!(FinalSize&0x800000))
																				{
																					memmove(pDst,pSrc+4,FinalSize);
/*																				uint8 Hash=0;
																				for(uint32 a=0;a<FinalSize;a++)
																					Hash^=pDst[a+4];
*/
#ifndef __SPU__
																					if(pSize)
																						*pSize	=	FinalSize+4+1;
#endif
																					return FinalSize;
																				}
																				else
																					FinalSize&=0x7fffff;
																				const uint32	OffsetBits	=	pSrc[0];
																				const uint32	MaskOffset	=	(1<<OffsetBits)-1;
																				const uint32	TresHold		=	2;
																				pSrc+=12;
//																				memset(pDst,0,FinalSize);
																				uint16	Bits;
																				uint16 a=0;
																				for(;IdxOut<FinalSize;)
																				{
																					IF(a==0,0)
																					{
																						Bits	=	(uint16)pSrc[IdxIn++];
																						IF(Bits == 0, 0)
																						{
																							CopyVecUnaligned(&pSrc[IdxIn], &pDst[IdxOut]);
																							IdxIn 	+= 8;
																							IdxOut  += 8;
																							continue;
																						}
																						a=8;
																					}
																					IF(Bits&1,1)
																					{
																						SDataTemp DatatTemp;
																						DatatTemp.byteVals[0] = pSrc[IdxIn];	
																						DatatTemp.byteVals[1] = pSrc[IdxIn+1];
																						uint16	Data	=	DatatTemp.shortVal;
																						IdxIn	+=	2;
																						uint32 Len		=	(Data>>OffsetBits)+TresHold;
																						uint32 Offset	=	IdxOut-(Data&MaskOffset);
																						uint32 b=0;
																						IF(Offset + Len > IdxOut, 0)
																						{
																							do
																							{
																								const uint8 c0 = pDst[Offset+b];
																								const uint8 c1 = pDst[Offset+b+1];
																								pDst[IdxOut+b]		=	c0;
																								pDst[IdxOut+b+1]	=	c1;
																							}WHILE((b+=2) < Len, 1);
																						}
																						else
																						do
																						{
																							CopyVecUnaligned(&pDst[Offset+b],&pDst[IdxOut+b]);
																						}WHILE((b+=16) < Len, 0);
																						IdxOut	+=	Len;
																					}
																					else
																						pDst[IdxOut++] =	pSrc[IdxIn++];
																					Bits>>=1;
																					--a;
																				}
/*
																				while(reinterpret_cast<const uint32*>(pSrc)[-2]-12-((a&7)==0)!=IdxIn)
																				{
																					snPause();
																				}
																				uint8 Hash=0;
																				for(uint32 a=0;a<FinalSize;a++)
																					Hash^=pDst[a];
																				while(pSrc[IdxIn+((a&7)==0)]!=Hash)
																				{
																					snPause();
																				}
*/
#ifndef __SPU__
																				IdxIn+=1+((a&7)==0);
																				if(pSize)
																					*pSize	=	IdxIn+12+((a&7)==0);
#endif
																				return FinalSize;
																			}
	friend class CCryDXPSShader;
} _ALIGN(128);

#endif

