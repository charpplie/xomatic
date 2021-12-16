#include "StdAfx.h"
#include "../../../Layer0/CCryDXPS.hpp"
#include "CCryDXPSShader.hpp"
#include "CCryDXPSShaderReflection.hpp"

extern "C" void *mwprivate2_memalign(size_t, size_t, ECryModule);

#if !defined(__SPU__) || !defined(SUPP_SPU_FRAME_STATS)
	#define cellGcmAddPerfTicks0
	#define cellGcmAddPerfTicks1
#endif

#if !defined(__SPU__)
#ifndef _RELEASE
ILINE void cellGcmCpyUCodeMain(void* const __restrict pData, void* const __restrict puCode, const uint32 cUCodeSize)
{
	mymemcpy16(pData,cellGcmGetPSBuf(),cUCodeSize);
	mymemcpy16(puCode,cellGcmGetPSBuf(),cUCodeSize);
}
ILINE void cellGcmCpyUCodeMainFromLS(void* const __restrict pData, void* const __restrict pUCode, const uint32 cUCodeSize)
{
	mymemcpy16(pData,pUCode,cUCodeSize);
}
#endif

#define thisContext gCellGcmCurrentContext

typedef struct S128ALigned
{
	uint8 dummy[128];
} _ALIGN(128) S128ALigned;

CCryDXPSShader::CCryDXPSShader(const void* pProgram,ECRYDXPSShaderType SType MMRES_PARAM):
CCryDXPSResource(EDXPS_RT_SHADER MMRES_PASS)
{
	using namespace CRY_DXPS_GCMNAMESPACE;
	Crc32Gen *pGen = GetISystem()->GetCrc32Gen();

	MMRES_ADDCOUNT();
	MMRES_ADDMM(sizeof(CCryDXPSShader));

	void*		puCode;

#if defined(DXPS_LZSS_COMPRESS)
	uint32 SizeLZSS=0;
	uint8 pProgram3[LOCAL_VS_BUFFER_SIZE] _ALIGN(128);
	uint8* pProgram2 = &pProgram3[16];
	const uint32 cDecodedSize = CDXPSShaderDesc::Decode(pProgram2,reinterpret_cast<const uint8*>(pProgram)+4,&SizeLZSS);
	const CDXPSShaderDesc* const __restrict pDesc=reinterpret_cast<const CDXPSShaderDesc*>(reinterpret_cast<const uint8*>(pProgram2));
	m_ProgramLZSSSize = (SizeLZSS + 127) & ~127;//align size to 128
	//make sure it becomes 128 byte aligned for transfer speed
	m_pProgramLZSS	=	reinterpret_cast<uint8*>(CRY_DXPS_NEWARRAY(S128ALigned,m_ProgramLZSSSize/sizeof(S128ALigned)));
	memcpy(m_pProgramLZSS,reinterpret_cast<const uint8*>(pProgram)+4,SizeLZSS);





#else	
	uint32 ByteSize	=	reinterpret_cast<const uint32*>(pProgram)[0];
	m_ProgramSize	=	(ByteSize + 127) & ~127;//align size to 128
	uint8* pData	=	(uint8*)CryModuleMemalign((ByteSize+127)&~127, 128, eCryM_Render);
	memcpy(pData,&reinterpret_cast<const uint8*>(pProgram)[4],ByteSize);
	m_pDesc	=	reinterpret_cast<CDXPSShaderDesc*>(pData);
	CDXPSShaderDesc* pDesc=m_pDesc;
#endif

	cellGcmCgGetUCode(pDesc->Program(), &puCode, &m_uCodeSize);
	m_uCodeOff = (uint8*)puCode - (uint8*)pDesc->Program();

	m_Crc32	=	pGen->GetCRC32((char*)puCode,m_uCodeSize,0xffffffff);
#if defined(CRY_DXPS_PRECOMPILE_CMDBUFFER)
	m_CmdBufferOffset	=	0;
	tdLayer0::Device()->ThreadFinish();
	if(!pDesc->PixelShader())
	{
		if(tdLayer0::PCCMB().EnableCMD())
		{
			const SVSConst* pConsts	=	pDesc->VSConstant();
			for(uint32 a=0,Size=pDesc->VSConstantCount();a<Size;a++)
			{
				const SVSConst& rConstant	=	pConsts[a];
				cellGcmSetVertexProgramConstants(	rConstant.m_Index,
																					4,
																					reinterpret_cast<const f32*>(&rConstant.m_Value[0]));
			}
			cellGcmSetVertexProgram(pDesc->Program(),puCode);
			cellGcmSetReturnCommand();
			m_CmdBufferOffset	=	tdLayer0::PCCMB().DisableCMD();
		}
	}
#endif


#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	m_BufferSize=(ByteSize+127)&~127;
	CRY_DEBUGOUT("Shader bytecode-size %d\n",ByteSize);
#endif
	MMRES_ADDMM(m_BufferSize);

	CRY_DEBUGOUT("Shader ucode-size %d\n",m_uCodeSize);

//	for(uint32 a=0;a<MAX_SLOTCOUNT;a++)
//		m_CBCache[a].m_pCBuffer	=	0;
}


void CCryDXPSShader::ReleaseResources()
{
#if defined(DXPS_LZSS_COMPRESS)



		free((uint8*)m_pProgramLZSS);
#else
	delete[]	(uint8*)m_pDesc;
#endif

#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		MMRES_SUBMM(m_BufferSize);
#endif

}
#endif//__SPU__

struct SConstBufSwizzleHelper
{
	uint32 val0;
	uint32 val1;
	uint32 val2;
	uint32 val3;
} _ALIGN(16);

#define SPU_CACHE_DOMAIN SPU_DOMAIN_LOCAL

void CCryDXPSShader::SetPixelshader(	SPU_DOMAIN_LOCAL const CDXPSShaderDesc* pDesc, 
																			SPU_DOMAIN_LOCAL const CCryDXPSCBData* const __restrict pConstBuffer,
																			SPU_CACHE_DOMAIN CCryDXPShaderCache&	__restrict rCache,
																			tdResHandle DrawCall,
																			CCryDXPSGCMPixelshaderCacheMan& __restrict pPixelCacheMan)const
{
#if !defined(_RELEASE) || defined(__SPU__)
	SPU_FRAME_PROFILE_SECTION("CCryDXPSShader::SetPixelshader")
	const uint32 cStart = rdtsc();
	const qword SwizzleMask = (qword){0x2, 0x3, 0x0, 0x1, 0x6, 0x7, 0x4, 0x5, 0xA, 0xB, 0x8, 0x9, 0xE, 0xF, 0xC, 0xD};
	uint32 allocWait = 0;

	const uint32 cUCodeSize = m_uCodeSize;
	SPU_DOMAIN_LOCAL CGprogram Prog	=	pDesc->Program();
	SPU_DOMAIN_LOCAL void* const __restrict pUCode = (void*)((uint8*)Prog + m_uCodeOff);

	using namespace CRY_DXPS_GCMNAMESPACE;

//		SConstBufSwizzleHelper* const __restrict pCBRegDest	=	(SConstBufSwizzleHelper*)pUCode;
	qword* const __restrict pCBRegDest	=	reinterpret_cast<qword*>(pUCode);

	uint32 CBDirty	=	rCache.PSCBDirty();
	for(uint32 a=0;CBDirty;a++,CBDirty>>=1)
	{
		if(!(CBDirty&1))
			continue;
		const uint32 Size = (uint32)pDesc->m_PatchCount[a];
		if(!Size)
			continue;

		const CCryDXPSCBData& __restrict rCBuffer	=	pConstBuffer[a];
		CCryDXPSShaderCBCache& rCBCache	=	m_CBCache[a];
		const SRefPatch* const pPatches	=	pDesc->PatchPixelByOff(pDesc->m_PatchOffset[a]);
		SPU_CB_BUFFER_DOMAIN const qword* const __restrict pCBRegs	=	SPU_CB_BUFFER_PTR(reinterpret_cast<const qword*>(rCBuffer.Data()));
		for(uint32 b=0;b<Size;b++)
		{
			const SRefPatch&	rPatch	=	pPatches[b];
			SPU_CB_BUFFER_DOMAIN const qword* const __restrict pSrc	=	SPU_CB_BUFFER_PTR(&pCBRegs[rPatch.m_ConstReg]);
			qword* const __restrict pDst	=	&pCBRegDest[rPatch.m_Offset>>4];
			*pDst	=	__si_shufb(*pSrc,*pSrc,SwizzleMask);
		}
	}
	const uint32 cAllocStart = rdtsc();

	void*	pData	=	pPixelCacheMan.Alloc(cUCodeSize,DrawCall);
	allocWait = cAllocStart - rdtsc();

	//test uploaded results
	cellGcmCpyUCodeMainFromLS(SPU_MAIN_PTR(pData),SPU_LOCAL_PTR(pUCode),cUCodeSize);
	uint32 Offset;
	if(CELL_OK!=cellGcmAddressToOffset(pData,&Offset))
	{
		CRY_DEBUGOUT("Could not translate fragment-ucode-address to offset\n");
		return;
	}
	cellGcmSetFragmentProgramOffset(Prog, Offset, 0);
	cellGcmSetFragmentProgramControl(Prog,CELL_GCM_FALSE,1,0);

	cellGcmAddPerfTicks0(cStart - rdtsc() - allocWait);
	#undef patchCountOff
#endif
}

//stores information of the local copy of the vertex shader constants
struct SPatchCall
{
	uint32 patchPos;				//copy of PatchPos
	uint16 patchSize;				//copy of PatchSize
	uint16 patchArrayIndex;	//start index into localConstData
};

struct SPatchPos
{
	uint16 patchPos0;
	uint16 patchPos1;
} /*_ALIGN(4)*/;

typedef union SConstQWord
{
	qword val16;
	uint32 val4[4];
}SConstQWord;


SPU_NO_INLINE
void CCryDXPSShader::SetVertexshader(SPU_DOMAIN_LOCAL const CDXPSShaderDesc* pDesc,
																		 SPU_DOMAIN_LOCAL const CCryDXPSCBData* const __restrict pConstBuffer,
																		 SPU_CACHE_DOMAIN CCryDXPShaderCache&	__restrict rCache)const
{
#if !defined(_RELEASE) || defined(__SPU__)
	SPU_FRAME_PROFILE_SECTION("CCryDXPSShader::SetVertexshader")
	const uint32 cStart = rdtsc();

#if defined(CRY_DXPS_PRECOMPILE_CMDBUFFER)
	const uint32 cCmdBufferOffset = m_CmdBufferOffset;
#endif

	using namespace CRY_DXPS_GCMNAMESPACE;

	SPU_DOMAIN_LOCAL CGprogram Prog	=	pDesc->Program();
	SPU_DOMAIN_LOCAL void* const __restrict puCode = SPU_LOCAL_PTR((void*)((uint8*)Prog + m_uCodeOff));
	const uint32 cUCodeSize = m_uCodeSize;
	const bool cUpdateShader = rCache.LastVShader()!=m_Crc32;
	assert(((uint32)pDesc & 127) == 0);
	SPU_DOMAIN_LOCAL CgBinaryProgram * __restrict pBinaryProgSPU;
	if(cUpdateShader)
	{
		//TODO: just copy byte code and decrease buffer to 8 KB again once precompiled vertex buffers are available
	#if defined(CRY_DXPS_PRECOMPILE_CMDBUFFER)
		if(!cCmdBufferOffset)
	#endif
			pBinaryProgSPU = (CgBinaryProgram*)Prog;
	}
#if !defined(CRY_DXPS_LSCONSTANTS)
	SPatchCall patchCalls[512]; //1 for each constant at most
	uint32 curPatchCallCount = 0;
	float localConstData[2048] _ALIGN(16);//local copy of all constants to be updated, 512 * 16 byte at most
	uint32 curLocalConstIndex = 0;
#endif
	SPU_CACHE_DOMAIN qword *const pVSConsts = rCache.VSConsts();

	//do 2 passes, first gather all the information to be passed into cellGcmSetVertexProgramConstants, dma transfer all data and then loop again
	//1st pass: gather all data and start dma transfer
	uint32 CBDirty	=	rCache.VSCBDirty();
	for(uint32 a=0;CBDirty;a++,CBDirty>>=1)
	{
		if(!(CBDirty&1))
			continue;
		const CCryDXPSCBData& __restrict rCBuffer	=	pConstBuffer[a];
		uint32 Size = (uint32)pDesc->m_PatchCount[a];
		if(Size==0)
			continue;

		//it is always 4 byte aligned, so access 2x2 byte at once
		const SPatchPos* const __restrict pPatches	=	(SPatchPos*)pDesc->PatchVertexByOff(pDesc->m_PatchOffset[a]);
		SPU_CB_BUFFER_DOMAIN const float* const __restrict pData	=	SPU_CB_BUFFER_PTR(reinterpret_cast<const float*>(rCBuffer.Data()));
		IF(pData,1)
		{
			const uint32 CBSize =	rCBuffer.Size()>>2;
			Size = Size << 1;
			for(uint32 b=0;b<Size;b+=2)
			{
				const SPatchPos& crPatchPos	= pPatches[b>>1];
				const uint32 cPatchPos0			= crPatchPos.patchPos0;
				const uint32 cPatchPos1			= crPatchPos.patchPos1;
	#if !defined(CRY_DXPS_LSCONSTANTS)				
				SPatchCall& rPatchCall			= patchCalls[curPatchCallCount++];
				rPatchCall.patchPos					=	cPatchPos0;
				rPatchCall.patchSize				=	min(cPatchPos1,(uint32)(CBSize-(cPatchPos0<<2)));
				rPatchCall.patchArrayIndex	= curLocalConstIndex;
				assert((curLocalConstIndex & 3) == 0);
				//cope with non aligned transfers
				uint32 transferSize = rPatchCall.patchSize;
				transferSize = (transferSize + 3) & ~3;
				__spu_dma_to_ls_no_cache_no_sync(&localConstData[curLocalConstIndex], SPU_MAIN_PTR((const void*)&pData[cPatchPos0<<2]), transferSize * sizeof(float));
				curLocalConstIndex += transferSize;
	#else
				const uint32 cNum = (uint32)(min(cPatchPos1,(uint32)(CBSize-(cPatchPos0<<2)))) >> 2;
				const uint32 PatchPos = cPatchPos0;
				qword *const __restrict pData4 = (qword*)&pData[cPatchPos0<<2];
				for(uint32 c=0; c<cNum; ++c)
				{
					const uint32 cCurPatchPos		= PatchPos+c;
					const	qword TmpConst				=	pData4[c];
					qword& __restrict rVSConst	= pVSConsts[cCurPatchPos];
					const qword CmpMask					=	__si_ceq(rVSConst,TmpConst);
					const int Mask							=	__si_to_int(__si_gb(CmpMask));
					if(Mask!=15)
					{
						//inline implementation of cellGcmSetVertexProgramConstants(cCurPatchPos, 4, (float*)&pData4[c]);
						const uint32* const __restrict cpData = (uint32*)&pData4[c];
						uint32 *const __restrict pCurContext = thisContext->current;
						pCurContext[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD, 5);
						pCurContext[1] = cCurPatchPos;
						pCurContext[2] = cpData[0];
						pCurContext[3] = cpData[1];
						pCurContext[4] = cpData[2];
						pCurContext[5] = cpData[3];
						thisContext->current += 6;
						rVSConst = TmpConst;
					}
				}
	#endif
			}
		}
		else
			printf("SetVertexshader: const buffer not set\n");
	}

#if !defined(CRY_DXPS_LSCONSTANTS)
	const uint32 cPatchCount = curPatchCallCount;
	if(cPatchCount)
	{
		__spu_sync_dma_no_cache_no_sync();	//sync transfer
		//2nd pass
		for(uint32 i=0; i<cPatchCount; ++i)
		{
			const SPatchCall& crPatchCall = patchCalls[i];
			const uint32 cNum = (uint32)crPatchCall.patchSize >> 2;
			const uint32 PatchPos = crPatchCall.patchPos;
			qword *const __restrict pData4 = (qword*)&localConstData[crPatchCall.patchArrayIndex];
			for(uint32 c=0; c<cNum; ++c)
			{
				const uint32 cCurPatchPos		= PatchPos+c;
				const	qword TmpConst				=	pData4[c];
				qword& __restrict rVSConst	= pVSConsts[cCurPatchPos];
				const qword CmpMask					=	si_ceq(rVSConst,TmpConst);
				const int Mask							=	si_to_int(si_gb(CmpMask));
				if(Mask!=15)
				{
					//inline implementation of cellGcmSetVertexProgramConstants(cCurPatchPos, 4, (float*)&pData4[c]);
					const uint32* const __restrict cpData = (uint32*)&pData4[c];
					uint32 *const __restrict pCurContext = thisContext->current;
					pCurContext[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD, 5);
					pCurContext[1] = cCurPatchPos;
					pCurContext[2] = cpData[0];
					pCurContext[3] = cpData[1];
					pCurContext[4] = cpData[2];
					pCurContext[5] = cpData[3];
					thisContext->current += 6;
					rVSConst = TmpConst;
				}
			}
		}
	}
#endif//CRY_DXPS_LSCONSTANTS
	if(cUpdateShader)
	{
#if defined(CRY_DXPS_PRECOMPILE_CMDBUFFER)
		if(cCmdBufferOffset)
		{
			DXPS_PROFILE_FRAME(DXPS_SetVertexshader_CALL);
			cellGcmSetCallCommand(cCmdBufferOffset);
		}
		else
#endif
		{
			DXPS_PROFILE_FRAME(DXPS_SetVertexshaderConConstants);
			SPU_DOMAIN_LOCAL const SVSConst* const pConsts =	pDesc->VSConstant();
			SConstQWord TmpConst;
			for(uint32 a=0,Size=pDesc->VSConstantCount();a<Size;a++)
			{
				const SVSConst& __restrict rConstant	=	pConsts[a];
				const uint32 cIndexFirst		= rConstant.m_Index;
				qword& __restrict rVSConst	= pVSConsts[cIndexFirst];
				TmpConst.val4[0]		=	rConstant.m_Value[0];
				TmpConst.val4[1]		=	rConstant.m_Value[1];
				TmpConst.val4[2]		=	rConstant.m_Value[2];
				TmpConst.val4[3]		=	rConstant.m_Value[3];
				const qword CmpMask	=	__si_ceq(rVSConst,TmpConst.val16);
				const int Mask			=	__si_to_int(__si_gb(CmpMask));
				if(Mask!=15)
				{
					//inline implementation of cellGcmSetVertexProgramConstants(cCurPatchPos, 4, (float*)&pData4[c]);
					uint32 *const __restrict pCurContext = thisContext->current;
					pCurContext[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD, 5);
					pCurContext[1] = cIndexFirst;
					pCurContext[2] = TmpConst.val4[0];
					pCurContext[3] = TmpConst.val4[1];
					pCurContext[4] = TmpConst.val4[2];
					pCurContext[5] = TmpConst.val4[3];
					rVSConst	= TmpConst.val16;
					thisContext->current += 6;
				}
			}

			{
				DXPS_PROFILE_FRAME(DXPS_SetVertexshader);
				cellGcmSetVertexProgram((CGprogram)pBinaryProgSPU, (void*)((uint8*)pBinaryProgSPU + pBinaryProgSPU->ucode));
			}
		}
		rCache.LastVShader(m_Crc32);
	}
	cellGcmAddPerfTicks1(cStart - rdtsc());
	#undef patchCountOff
#endif
}
