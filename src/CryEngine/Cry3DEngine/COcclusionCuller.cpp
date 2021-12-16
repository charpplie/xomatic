////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   COcclusionCuller.h
//  Version:     v1.00
//  Created:     13/8/2006 by Michael Kopietz
//  Compilers:   Visual Studio.NET
//  Description: Occlusion buffer
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#ifndef PS3
#include "COcclusionCuller.h"
#include "COcclusionCullerPoly.h"
#include "COcclusionCullerClipper.h"
#include "StatObj.h"

COcclusionCuller::COcclusionCuller():
#ifdef  OC_ZEROCYCLECLEAR
m_ClearCounter(0),
#endif
m_OutdoorVisible(1)
{
#ifdef OC_HIERARCHICAL_ZBUFFER
	m_Dirty	=	true;
#endif
#if !defined(PS3)
	m_ProjectedVertexCacheX.PreAllocate(OC_MAX_VERTPERPOLY,OC_MAX_VERTPERPOLY);
	m_ProjectedVertexCacheY.PreAllocate(OC_MAX_VERTPERPOLY,OC_MAX_VERTPERPOLY);
	m_ProjectedVertexCacheZ.PreAllocate(OC_MAX_VERTPERPOLY,OC_MAX_VERTPERPOLY);
#endif
	m_TrisWritten		=
	m_ObjectsWritten=
	m_TrisTested		=
	m_ObjectsTested	=
	m_ObjectsTestedAndRejected	=	m_ObjectsTestedAndRejectedAsync = 0;
	m_TopLeftX			=
	m_TopLeftY			=	0.f;
	m_BottomRightX	=
	m_BottomRightY	=	1.f;

#ifdef OC_LUTDIVISION
	for(int32 a=-255;a<256;a++)
		if(a)
			m_IDivLut[a+255]	=	256/a;
	m_UDivLut[0]	=	0xffffff;
	for(int32 a=1;a<256;a++)
			m_UDivLut[a]	=	(1<<14)/a;
#endif
	m_LastSizeZ = 0;

	m_ZBuffer[ezbtBack]	 = NULL;
	m_ZBuffer[ezbtFront] = NULL;

#ifdef OC_HIERARCHICAL_ZBUFFER
	m_YCBuffer	= NULL;
	m_CBuffer		= NULL;
#endif
}

f32 COcclusionCuller::GetFrameTime()
{ 
	return m_FrameTime;
}

void COcclusionCuller::BeginFrame(const CCamera& rCam)
{
	FUNCTION_PROFILER_3DENGINE;

	m_FrameTime			=	0.f;
	m_ZNearInMeters	=	0.f;
	m_ZFarInMeters	=	1024.f;

	m_TrisWritten		=
		m_ObjectsWritten=
		m_TrisTested		=
		m_ObjectsTested	=
		m_ObjectsTestedAndRejected	=	m_ObjectsTestedAndRejectedAsync = 0;
	m_Camera				=	rCam;
	Vec3 rCamPosition(rCam.GetPosition());
	m_EyePos				=	*reinterpret_cast<const Vec4*>(&rCamPosition); 
	m_EyePos.w			=	1.f;
	m_TopLeftX			=
		m_TopLeftY			=	0.f;
	m_BottomRightX	=
		m_BottomRightY	=	1.f;
	m_SizeX					=	
		m_SizeY					= min(max(1, GetCVars()->e_CoverageBufferResolution),256);
	m_InvSize				=	10.f*GetCVars()->e_CoverageBufferBias/static_cast<f32>(m_SizeX);//scale by pixelsize
																																										//mul by magic 10 to keep same default value
																																										//for bias as Vlad's CBuffer uses

	m_SizeShift	=	-1;
	while(!((1<<(++m_SizeShift))&m_SizeX));
	m_SizeZ			=	m_SizeX*m_SizeY;
#ifdef OC_HIERARCHICAL_ZBUFFER
	m_SizeC			=	m_SizeZ>>OC_ZEXELSKIP_SHIFT;
#endif
	if(m_LastSizeZ != m_SizeZ)
	{
		m_LastSizeZ = m_SizeZ;

		CryModuleMemalignFree(m_ZBuffer[ezbtBack]);
		m_ZBuffer[ezbtBack]		= (TOCZexel*)CryModuleMemalign(m_SizeZ * sizeof(TOCZexel), 128);
//		if(ezbtBack != ezbtFront)	// in case we have double-buffering
		{
			CryModuleMemalignFree(m_ZBuffer[ezbtFront]);
			m_ZBuffer[ezbtFront]	= (TOCZexel*)CryModuleMemalign(m_SizeZ * sizeof(TOCZexel), 128);
		}

	#ifdef OC_HIERARCHICAL_ZBUFFER
			CryModuleMemalignFree(m_CBuffer);
			m_CBuffer = (TOCCexel*)CryModuleMemalign(m_SizeC * sizeof(TOCCexel), 128);
			CryModuleMemalignFree(m_YCBuffer);
			m_YCBuffer = (TOCCexel*)CryModuleMemalign(m_SizeY * sizeof(TOCCexel), 128);
	#endif
	}
	
//	RasterizeWriteMMX(32,39,32,137,141,142,2146643972,2146643972,2146643972);

	if(!GetCVars()->e_CoverageBufferDebugFreeze)
	{
		CCamera tmpCam	= m_pRenderer->GetCamera();
		m_pRenderer->SetCamera(m_Camera);
		m_pRenderer->GetModelViewMatrix(reinterpret_cast<f32*>(&m_MatView));
		m_pRenderer->GetProjectionMatrix(reinterpret_cast<f32*>(&m_MatProj));
		m_MatViewProj		=	m_MatView*m_MatProj;
		m_MatViewProj.Transpose();
		m_pRenderer->SetCamera(tmpCam);

		m_FixedZFar			=	1.f/(m_Camera.GetFarPlane()-m_Camera.GetNearPlane());

		Clear();
	}
}

void COcclusionCuller::Clear()
{
	FUNCTION_PROFILER_3DENGINE;
#ifdef  OC_ZEROCYCLECLEAR
	if(!m_ClearCounter)
#endif

#ifdef OC_MMX
	int32*	pZBuffer=	m_ZBuffer[ezbtFront];
	__m64 Clear64		=	_mm_cvtsi32_si64(OC_ZEXEL_CLEARVALUEHIGH);
	Clear64	=	_mm_or_si64(_mm_slli_si64(Clear64,32),Clear64);

//	__m64 OutdoorVisible	=	_mm_or_si64(_mm_slli_si64(_mm_cvtsi32_si64(0),32),_mm_cvtsi32_si64(0));

	TOCZexel* pBuffer	=	&m_ZBuffer[ezbtFront][0];
	for(uint32 a=0;a<m_SizeZ;a+=4)
	{
//		__m64 ZBuffer0	=	*(__m64*)&pZBuffer[a];
//		__m64 ZBuffer1	=	*(__m64*)&pZBuffer[a+2];
//		OutdoorVisible	=_mm_or_si64(OutdoorVisible,_mm_or_si64(_mm_cmpeq_pi32(ZBuffer1,Clear64),_mm_cmpeq_pi32(ZBuffer0,Clear64)));
		*(__m64*)&pZBuffer[a]	=	Clear64;
		*(__m64*)&pZBuffer[a+2]	=	Clear64;
	}

//	m_OutdoorVisible	=	(OutdoorVisible.m64_i32[0]|OutdoorVisible.m64_i32[1])?true:false;
	_mm_empty();











#else// if we have !(PS3 && PS3OPT) && !OC_MMX
	TOCZexel* pBuffer	=	m_ZBuffer[ezbtFront];
	for(uint32 a=0;a<m_SizeZ;a+=4)
	{
		pBuffer[a]		=	OC_ZEXEL_CLEARVALUEHIGH;
		pBuffer[a+1]	=	OC_ZEXEL_CLEARVALUEHIGH;
		pBuffer[a+2]	=	OC_ZEXEL_CLEARVALUEHIGH;
		pBuffer[a+3]	=	OC_ZEXEL_CLEARVALUEHIGH;
	}
#endif//OC_MMX

		//switch z double buffers
		TOCZexel*	const cOldBackBuf = m_ZBuffer[ezbtBack];
		m_ZBuffer[ezbtBack]			= m_ZBuffer[ezbtFront];
		m_ZBuffer[ezbtFront]		= cOldBackBuf;

#ifdef OC_HIERARCHICAL_ZBUFFER












		for(uint32 a=0;a<m_SizeC;a++)
			m_CBuffer[a]	=	OC_ZEXEL_CLEARVALUELOW;

#endif
#ifdef  OC_ZEROCYCLECLEAR
	m_ClearCounter	=	(m_ClearCounter-(1<<24))&OC_ZEXEL_CLEARVALUEHIGH;
	m_Nearest	=	OC_ZEXEL_MAXVALUEHIGH|m_ClearCounter;
#else
	m_Nearest	=	OC_ZEXEL_MAXVALUEHIGH;
#endif
}

#ifdef OC_MMX
void COcclusionCuller::RasterizeWriteMMX(int32 X1,int32 X2,int32 X3,int32 Y1,int32 Y2,int32 Y3,int32 Z1,int32 Z2,int32 Z3)
{
  if(Z1<m_Nearest)
    m_Nearest	=	Z1;
  if(Z2<m_Nearest)
    m_Nearest	=	Z2;
  if(Z3<m_Nearest)
    m_Nearest	=	Z3;

	if(!(Y1<=Y2 && Y2<=Y3))
	{
		int32 S;
		if(Y2<Y1)
		{
			if(Y1<Y3)
			{//just swap 1,2
				S=X1;X1=X2;X2=S;
				S=Y1;Y1=Y2;Y2=S;
				S=Z1;Z1=Z2;Z2=S;
			}
			else//Y1 is biggest
			{
				if(Y2<Y3)//Y2 is smallest
				{//rotate 2,3,1
					S=X1;X1=X2;X2=X3;X3=S;
					S=Y1;Y1=Y2;Y2=Y3;Y3=S;
					S=Z1;Z1=Z2;Z2=Z3;Z3=S;
				}
				else
				{//just swap 1,3
					S=X1;X1=X3;X3=S;
					S=Y1;Y1=Y3;Y3=S;
					S=Z1;Z1=Z3;Z3=S;
				}
			}
		}
		else//Y1<Y2
		{
			if(Y3<Y1)
			{//rotate 3,1,2
				S=X1;X1=X3;X3=X2;X2=S;
				S=Y1;Y1=Y3;Y3=Y2;Y2=S;
				S=Z1;Z1=Z3;Z3=Z2;Z2=S;
			}
			else
			{//just swap 2,3
				S=X2;X2=X3;X3=S;
				S=Y2;Y2=Y3;Y3=S;
				S=Z2;Z2=Z3;Z3=S;
			}
		}
	}
	if(Y1==Y3)
		return;

//	Z1/=16;
//	Z2/=16;
//	Z3/=16;
	int32 ZL	=	Z1;
	int32 ZR	=	Z1;
	const uint32 cSizeShift = m_SizeShift;
	const uint32 cSizeX = m_SizeX;
	int32 PtL	=	((Y1<<cSizeShift)|min(X1,(int32)cSizeX-1))<<OC_FIXPS;
	int32 PtR	=	PtL;
	int32 Pt2	=	((Y2<<cSizeShift)|min(X2,(int32)cSizeX-1))<<OC_FIXPS;
	int32 Pt3	=	((Y3<<cSizeShift)|min(X3,(int32)cSizeX-1))<<OC_FIXPS;
	const int32 Add13		=	OC_DIVU32S14((Pt3-PtL),(Y3-Y1));	//already tested for Y1==Y3
	const int32 Add13z	=	OC_DIVU32S14((Z3-Z1),(Y3-Y1));

/*	if(X1>=cSizeX)
	{
		PtL--;
		PtR--;
	}
	if(X2>=cSizeX)
		Pt2--;
	if(X3>=cSizeX)
		Pt3--;*/

	if(Y2==Y1)
	{
		//Y1=Y2;
		Y2=Y3;
		ZR=Z2;
		Z2=Z3;
		PtR=Pt2;
		Pt2=Pt3;
	}

	int32*	pZBuffer	=	m_ZBuffer[ezbtBack];
	while(Y2>Y1)
	{
		//prefetch of m_ZBuffer[PtL] here!
		const int32 Add12		=	OC_DIVU32S14((Pt2-PtR),(Y2-Y1));
		const int32 Add12z	=	OC_DIVU32S14((Z2-ZR),(Y2-Y1));
		do
		{
			int32 DeltaX=(PtR>>OC_FIXPS)-(PtL>>OC_FIXPS);//must be shifted down first to get correct filling
			if(DeltaX!=0)
			{
				const int32 AddZ=OC_DIVI32((ZR-ZL),DeltaX);
				int32 Pt,PtZ;
				if(DeltaX>0)
				{
					Pt=PtL>>OC_FIXPS;
					PtZ=ZL;
				}
				else
				{
					DeltaX=-DeltaX;
					Pt=PtR>>OC_FIXPS;
					PtZ=ZR;
				}
				if(DeltaX&1)
				{
					--DeltaX;
					//																			if(PtZ<pZBuffer[Pt++])
					//																				pZBuffer[Pt-1]	=	PtZ;
					const int32 Mask=(PtZ-pZBuffer[Pt])>>31;
					pZBuffer[Pt]	&=	~Mask;
					pZBuffer[Pt++]	|=	PtZ&Mask;
				}
				if(DeltaX)
				{
					__m64 AddZ64	=	_mm_cvtsi32_si64(AddZ<<1);//   *2 cause of double stepsize
					__m64 PtZ64		=	_mm_cvtsi32_si64(PtZ);
					PtZ64	=	_mm_or_si64(_mm_add_pi32(_mm_slli_si64(PtZ64,32),_mm_slli_si64(_mm_cvtsi32_si64(AddZ),32)),PtZ64);
					AddZ64	=	_mm_or_si64(_mm_slli_si64(AddZ64,32),AddZ64);
					for(int32 a=0;a<DeltaX;a+=2,Pt+=2)
					{
						__m64 ZBuffer	=	*(__m64*)&pZBuffer[Pt];
						const __m64 Mask	=	_mm_cmpgt_pi32(ZBuffer,PtZ64);
						ZBuffer	=_mm_or_si64(_mm_andnot_si64(Mask,ZBuffer),_mm_and_si64(Mask,PtZ64));
						*(__m64*)&pZBuffer[Pt]	=	ZBuffer;
						PtZ64=_mm_add_pi32(PtZ64,AddZ64);
					}
				}
			}
			PtL+=Add13;
			PtR+=Add12;
			ZL+=Add13z;
			ZR+=Add12z;
		}while(++Y1<Y2);
		Y1=Y2;
		Y2=Y3;
		ZR=Z2;
		Z2=Z3;
		PtR=Pt2;
		Pt2=Pt3;
	}

}
#endif

inline bool IsZWriteMMX(const SShaderItem& rSI)
{
    IShader *pSH = rSI.m_pShader;
    if (pSH->GetFlags2() & EF2_NODRAW)
      return false;
    if (pSH->GetFlags2() & EF2_FORCE_ZPASS)
      return true;
    if (pSH->GetFlags() & EF_DECAL)
      return false;
    if (rSI.m_pShaderResources && *reinterpret_cast<int32*>(&rSI.m_pShaderResources->GetDiffuseColor()[3]) != 0x3f800000)
      return false;
    return true;
}





















































































































































































































































































































void COcclusionCuller::AddRenderMesh(IRenderMesh * pRM, Matrix34A* pTranRotMatrix, IMaterial * pMaterial, bool bOutdoorOnly, bool bCompletelyInFrustum,bool bNoCull)
{
	FUNCTION_PROFILER_3DENGINE;

	// check material
	if(!pMaterial)
		pMaterial = pRM->GetMaterial();
	assert(pMaterial);
	if(!pMaterial)
		return;

	m_ObjectsWritten++;
	if(GetCVars()->e_CoverageBufferDebugFreeze)
	{
		m_TrisWritten++;
		return;
	}

	const uint32 VertexCount = pRM->GetVerticesCount();
	int32 IndexCount = pRM->GetIndicesCount();
	int32 VertexSize	= 0;
	const uint16* pIndices = pRM->GetIndexPtr(FSL_READ);
	const byte* pPos = pRM->GetPosPtr(VertexSize, FSL_READ);
	assert(pIndices && pPos);
  if(!pPos)
    return;

	Matrix44 MatWorld(*pTranRotMatrix);
	Matrix44 MatWorldViewProj	=	m_MatViewProj * MatWorld;
	uint32 nVisibleChunksMask = 0;
	if(m_VertexCache.capacity()<VertexCount)
	{
		m_VertexCache.PreAllocate(VertexCount+1,VertexCount+1);
	#ifdef OC_COHENSUTHERLAND
		m_VertexClipOutcodes.PreAllocate(VertexCount+1,VertexCount+1);
	#endif
	}
#ifdef OC_SSE
	Vec4* pVertexCache	=	&m_VertexCache[0];
	uint32* pVertexClipOutcodes	=	&m_VertexClipOutcodes[0];
	_mm_prefetch((const char*)pPos,_MM_HINT_T0);
	_mm_prefetch((const char*)pVertexCache,_MM_HINT_NTA);
	_mm_prefetch((const char*)pVertexClipOutcodes,_MM_HINT_NTA);
	MatWorldViewProj.Transpose();
	float Mat[32];
	Matrix44& rMat=*reinterpret_cast<Matrix44*>(reinterpret_cast<size_t>(&Mat[15])&~0xf);
	rMat=MatWorldViewProj;//need to copy as the compiler kinda makes alligned load in _mm_loadu_ps on the first element ->crash
	register __m128	MX		=	_mm_load_ps(&((float*)&rMat)[0]);
	register __m128	MY		=	_mm_load_ps(&((float*)&rMat)[4]);
	register __m128	MZ		=	_mm_load_ps(&((float*)&rMat)[8]);
	register __m128	MW		=	_mm_load_ps(&((float*)&rMat)[12]);
	register __m128 WMask	=	_mm_set_ps(0.f,0.f,-1.f,-1.f);
	Vec4 OutPos;

#ifdef OC_COHENSUTHERLAND
	if(!bCompletelyInFrustum)
	{
		uint32 ObjClipMask=0;
		for(uint32 a=0;a<VertexCount;a++)
		{
			_mm_prefetch((const char*)(&pPos[VertexSize*a])+128,_MM_HINT_T0);
			_mm_prefetch((const char*)(&pVertexCache[a])+128,_MM_HINT_NTA);
			_mm_prefetch((const char*)(&pVertexClipOutcodes[a])+128,_MM_HINT_NTA);
			const Vec3&	Pos	=	*reinterpret_cast<const Vec3*>(&pPos[VertexSize*a]);
			register __m128 OutPos	=	_mm_loadu_ps((const float*)&Pos);
			OutPos	=	_mm_add_ps(_mm_add_ps(
																											_mm_mul_ps(MX,_mm_shuffle_ps(OutPos,OutPos,0x0)),
																											_mm_mul_ps(MY,_mm_shuffle_ps(OutPos,OutPos,_MM_SHUFFLE(1,1,1,1)))),
																						_mm_add_ps(
																											_mm_mul_ps(MZ,_mm_shuffle_ps(OutPos,OutPos,_MM_SHUFFLE(2,2,2,2))),
																											MW));
			_mm_storeu_ps((float*)&pVertexCache[a],OutPos);
			__m128 OutPosW	=	_mm_shuffle_ps(OutPos,OutPos,0xff);
			uint32 cc								=(_mm_movemask_ps(_mm_cmpgt_ps(OutPos,OutPosW))<<4)|
																_mm_movemask_ps(_mm_cmplt_ps(OutPos,_mm_mul_ps(WMask,OutPosW)));
			ObjClipMask	|=	cc;
			pVertexClipOutcodes[a]	=	cc&0x77;
		}
		bCompletelyInFrustum	=	(ObjClipMask&0x77)?false:true;
	}
	else
#endif
	{
		for(uint32 a=0;a<VertexCount;a++)
		{
			_mm_prefetch((const char*)(&pPos[VertexSize*a])+256,_MM_HINT_T0);
			_mm_prefetch((const char*)(&pVertexCache[a])+256,_MM_HINT_NTA);
			const Vec3&	Pos	=	*reinterpret_cast<const Vec3*>(&pPos[VertexSize*a]);
			register __m128 OutPos	=	_mm_loadu_ps((const float*)&Pos);
			OutPos	=	_mm_add_ps(_mm_add_ps(
																											_mm_mul_ps(MX,_mm_shuffle_ps(OutPos,OutPos,0x0)),
																											_mm_mul_ps(MY,_mm_shuffle_ps(OutPos,OutPos,_MM_SHUFFLE(1,1,1,1)))),
																						_mm_add_ps(
																											_mm_mul_ps(MZ,_mm_shuffle_ps(OutPos,OutPos,_MM_SHUFFLE(2,2,2,2))),
																											MW));
			_mm_storeu_ps((float*)&pVertexCache[a],OutPos);
		}
	}
#else
	for(uint32 a=0;a<VertexCount;a++)
	{
		Vec4	Pos	=	*reinterpret_cast<const Vec4*>(&pPos[VertexSize*a]);
		Pos.w	=	1.f;
		m_VertexCache[a]	=	MatWorldViewProj*Pos;
	}
#endif

#ifdef OC_HIERARCHICAL_ZBUFFER
	m_Dirty	=	true;
#endif
	if(bCompletelyInFrustum)
	{
		if(m_ProjectedVertexCacheX.capacity()<VertexCount)
		{
			m_ProjectedVertexCacheX.PreAllocate(VertexCount,VertexCount);
			m_ProjectedVertexCacheY.PreAllocate(VertexCount,VertexCount);
			m_ProjectedVertexCacheZ.PreAllocate(VertexCount,VertexCount);
		}
		ProjectVertices(m_VertexCache,VertexCount, &m_ProjectedVertexCacheX[0], &m_ProjectedVertexCacheY[0], &m_ProjectedVertexCacheZ[0]);
		PodArray<CRenderChunk> &Chunks = pRM->GetChunks();
    uint32 nIndexStep = 3;
		for(int32 nChunkId=0; nChunkId<Chunks.Count(); nChunkId++)
		{
			CRenderChunk * pChunk = &Chunks[nChunkId];
			if (pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
				continue;

			// skip transparent and alpha test
			const SShaderItem &shaderItem = pMaterial->GetShaderItem(pChunk->m_nMatID);
			if (!bNoCull && !IsZWriteMMX(shaderItem))
				continue;

			if(!shaderItem.m_pShader)
				continue;
 
			ECull nCullMode = shaderItem.m_pShader->GetCull();
			if (bNoCull)
				nCullMode = eCULL_None;

			nVisibleChunksMask |= (1 << (nChunkId));

			int32 nLastIndexId = pChunk->nFirstIndexId + pChunk->nNumIndices;
#ifndef OC_MMX
			int32 VerticesX[3];
			int32 VerticesY[3];
			int32 VerticesZ[3];
#endif
			m_TrisWritten	+=	(nLastIndexId-pChunk->nFirstIndexId)/3;
      for(int32 a=pChunk->nFirstIndexId; a<nLastIndexId-2; a+=nIndexStep)
      {
        int32 I0	=	pIndices[a];
        int32 I1	=	pIndices[a+1];
        int32 I2	=	pIndices[a+2];
        if(nIndexStep==1 && (a&1))
        {
          I1	=	pIndices[a+2];
          I2	=	pIndices[a+1];
        }
				assert(I0<(int)VertexCount);
				assert(I1<(int)VertexCount);
				assert(I2<(int)VertexCount);

				const int32 Facing = (m_ProjectedVertexCacheX[I1]-m_ProjectedVertexCacheX[I0])*(m_ProjectedVertexCacheY[I2]-m_ProjectedVertexCacheY[I0])-
											(m_ProjectedVertexCacheX[I2]-m_ProjectedVertexCacheX[I0])*(m_ProjectedVertexCacheY[I1]-m_ProjectedVertexCacheY[I0]);
				if(	nCullMode == eCULL_None || (nCullMode == eCULL_Back && Facing>0 ) || (nCullMode == eCULL_Front && Facing<0))
				{
#ifdef OC_MMX
					if(Facing>0)
						RasterizeWriteMMX(	m_ProjectedVertexCacheX[I0],m_ProjectedVertexCacheX[I1],m_ProjectedVertexCacheX[I2],
																m_ProjectedVertexCacheY[I0],m_ProjectedVertexCacheY[I1],m_ProjectedVertexCacheY[I2],
																m_ProjectedVertexCacheZ[I0],m_ProjectedVertexCacheZ[I1],m_ProjectedVertexCacheZ[I2]);
					else
						RasterizeWriteMMX(	m_ProjectedVertexCacheX[I0],m_ProjectedVertexCacheX[I2],m_ProjectedVertexCacheX[I1],
																m_ProjectedVertexCacheY[I0],m_ProjectedVertexCacheY[I2],m_ProjectedVertexCacheY[I1],
																m_ProjectedVertexCacheZ[I0],m_ProjectedVertexCacheZ[I2],m_ProjectedVertexCacheZ[I1]);
#else
					if(Facing>0)
					{
						VerticesX[0]	=	m_ProjectedVertexCacheX[I0];
						VerticesX[1]	=	m_ProjectedVertexCacheX[I1];
						VerticesX[2]	=	m_ProjectedVertexCacheX[I2];
						VerticesY[0]	=	m_ProjectedVertexCacheY[I0];
						VerticesY[1]	=	m_ProjectedVertexCacheY[I1];
						VerticesY[2]	=	m_ProjectedVertexCacheY[I2];
						VerticesZ[0]	=	m_ProjectedVertexCacheZ[I0];
						VerticesZ[1]	=	m_ProjectedVertexCacheZ[I1];
						VerticesZ[2]	=	m_ProjectedVertexCacheZ[I2];
					}
					else
					{
						VerticesX[0]	=	m_ProjectedVertexCacheX[I0];
						VerticesX[1]	=	m_ProjectedVertexCacheX[I2];
						VerticesX[2]	=	m_ProjectedVertexCacheX[I1];
						VerticesY[0]	=	m_ProjectedVertexCacheY[I0];
						VerticesY[1]	=	m_ProjectedVertexCacheY[I2];
						VerticesY[2]	=	m_ProjectedVertexCacheY[I1];
						VerticesZ[0]	=	m_ProjectedVertexCacheZ[I0];
						VerticesZ[1]	=	m_ProjectedVertexCacheZ[I2];
						VerticesZ[2]	=	m_ProjectedVertexCacheZ[I1];
					}

					Rasterize<true>(VerticesX,VerticesY,VerticesZ,3);
#endif
				}
			}
		}
#ifdef OC_MMX
	_mm_empty();
#endif
	}
	else
	{
		COCPoly	PolyClipped;
		Matrix44 IMatWorld(pTranRotMatrix->GetInverted());
		const Vec4	TmpIEyePos	=	IMatWorld*m_EyePos;
		const Vec3&	IEyePos	=	*reinterpret_cast<const Vec3*>(&TmpIEyePos);
		PodArray<CRenderChunk> &Chunks = pRM->GetChunks();
    uint32 nIndexStep = 3;
		for(int32 nChunkId=0; nChunkId<Chunks.Count(); nChunkId++)
		{
			CRenderChunk * pChunk = &Chunks[nChunkId];
			if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
				continue;

			// skip transparent and alpha test
			const SShaderItem &shaderItem = pMaterial->GetShaderItem(pChunk->m_nMatID);
			if (!bNoCull && !IsZWriteMMX(shaderItem))
				continue;

			if(!shaderItem.m_pShader)
				continue;

			ECull nCullMode = shaderItem.m_pShader->GetCull();
			if (bNoCull)
				nCullMode = eCULL_None;

			nVisibleChunksMask |= (1 << (nChunkId));

			int32 nLastIndexId = pChunk->nFirstIndexId + pChunk->nNumIndices;
			for(int32 a=pChunk->nFirstIndexId; a<nLastIndexId-2; a+=nIndexStep)
			{
				int32 I0	=	pIndices[a];
				int32 I1	=	pIndices[a+1];
				int32 I2	=	pIndices[a+2];
        if(nIndexStep==1 && (a&1))
        {
          I1	=	pIndices[a+2];
          I2	=	pIndices[a+1];
        }
				assert(I0<(int)VertexCount);
				assert(I1<(int)VertexCount);
				assert(I2<(int)VertexCount);

				const Vec3& rV0	=	*reinterpret_cast<const Vec3*>(&pPos[VertexSize*I0]);
				const Vec3& rV1	=	*reinterpret_cast<const Vec3*>(&pPos[VertexSize*I1]);
				const Vec3& rV2	=	*reinterpret_cast<const Vec3*>(&pPos[VertexSize*I2]);
				const Vec3 Nrm	=	(rV2-rV0)^(rV1-rV0);

				const f32 Facing =	Nrm*(rV0-IEyePos);
				if(	nCullMode == eCULL_None || (nCullMode == eCULL_Back && Facing>0.f ) || (nCullMode == eCULL_Front && Facing<0.f))
				{
#ifdef OC_COHENSUTHERLAND
					if(!(m_VertexClipOutcodes[I0]&m_VertexClipOutcodes[I1]&m_VertexClipOutcodes[I2]))	//if not completely outside
					{
						if(!(m_VertexClipOutcodes[I0]|m_VertexClipOutcodes[I1]|m_VertexClipOutcodes[I2]))//if completely inside
						{
							Vec4 Vertices[3];
//							PolyClipped	=	COCPoly(m_VertexCache[I0],m_VertexCache[I1],m_VertexCache[I2]);
							Vertices[0]	=	m_VertexCache[I0];
							Vertices[1]	=	m_VertexCache[I1];
							Vertices[2]	=	m_VertexCache[I2];
							ProjectVertices(Vertices,3, &m_ProjectedVertexCacheX[0], &m_ProjectedVertexCacheY[0], &m_ProjectedVertexCacheZ[0]);
							m_TrisWritten++;
#ifdef OC_MMX
							if(Facing>=0.f)
								RasterizeWriteMMX(	m_ProjectedVertexCacheX[0],m_ProjectedVertexCacheX[1],m_ProjectedVertexCacheX[2],
																		m_ProjectedVertexCacheY[0],m_ProjectedVertexCacheY[1],m_ProjectedVertexCacheY[2],
																		m_ProjectedVertexCacheZ[0],m_ProjectedVertexCacheZ[1],m_ProjectedVertexCacheZ[2]);
							else
								RasterizeWriteMMX(	m_ProjectedVertexCacheX[0],m_ProjectedVertexCacheX[2],m_ProjectedVertexCacheX[1],
																		m_ProjectedVertexCacheY[0],m_ProjectedVertexCacheY[2],m_ProjectedVertexCacheY[1],
																		m_ProjectedVertexCacheZ[0],m_ProjectedVertexCacheZ[2],m_ProjectedVertexCacheZ[1]);

							_mm_empty();
#else
							Rasterize<true>(m_ProjectedVertexCacheX,m_ProjectedVertexCacheY,m_ProjectedVertexCacheZ,3);
#endif
						}
						else //else clipp it
						{
#endif
							if(Facing>=0.f)
								COCClipper::Clip(PolyClipped,COCPoly(m_VertexCache[I0],m_VertexCache[I1],m_VertexCache[I2]));
							else
								COCClipper::Clip(PolyClipped,COCPoly(m_VertexCache[I0],m_VertexCache[I2],m_VertexCache[I1]));
							if(PolyClipped.Count()>2)
							{
								m_TrisWritten++;
								RasterizeClip<true>(PolyClipped);
							}
#ifdef OC_COHENSUTHERLAND
						}
					}
#endif
				}

			}
		}
	}
#ifdef OC_MMX
	_mm_empty();
#endif
	if (GetCVars()->e_CoverageBufferDrawOccluders)
	{
		SGeometryDebugDrawInfo dd;
		dd.tm = *pTranRotMatrix;
		dd.bNoCull = bNoCull;
		pRM->DebugDraw( dd,nVisibleChunksMask );
	}
}


void COcclusionCuller::AddHeightMap(const SRangeInfo & m_rangeInfo, float X1, float Y1, float X2, float Y2)
{

}

bool COcclusionCuller::IsObjectVisible(const AABB& _objBox, EOcclusionObjectType eOcclusionObjectType, float fDistance, uint32* pRetVal)
{
	AABB objBox = _objBox;
	float fExt = fDistance*m_InvSize;
	objBox.min -= Vec3(fExt,fExt,fExt);
	objBox.max += Vec3(fExt,fExt,fExt);

	switch(eOcclusionObjectType)
	{
	case eoot_OCCLUDER:
		return IsBoxVisible_OCCLUDER(objBox, pRetVal);
	case eoot_OCEAN:
		return IsBoxVisible_OCEAN(objBox, pRetVal);
	case eoot_OCCELL:
		return IsBoxVisible_OCCELL(objBox, pRetVal);
	case eoot_OCCELL_OCCLUDER:
		return IsBoxVisible_OCCELL_OCCLUDER(objBox, pRetVal);
	case eoot_OBJECT:
		return IsBoxVisible_OBJECT(objBox, pRetVal);
	case eoot_OBJECT_TO_LIGHT:
		return IsBoxVisible_OBJECT_TO_LIGHT(objBox, pRetVal);
	case eoot_TERRAIN_NODE:
		return IsBoxVisible_TERRAIN_NODE(objBox, pRetVal);
	case eoot_PORTAL:
		return IsBoxVisible_PORTAL(objBox, pRetVal);
	}

	assert(!"Undefined occluder type");
	
	return true;
}

bool COcclusionCuller::IsBoxVisible_TERRAIN_NODE(const AABB& objBox, uint32* const __restrict pResDest)
{
	FUNCTION_PROFILER_3DENGINE;

	return IsBoxVisible(objBox, pResDest);
}

bool COcclusionCuller::IsBoxVisible_OCCELL_OCCLUDER(const AABB& objBox, uint32* const __restrict pResDest)
{
	FUNCTION_PROFILER_3DENGINE;

	return IsBoxVisible(objBox, pResDest);
}

bool COcclusionCuller::IsBoxVisible_OCCLUDER(const AABB& objBox, uint32* const __restrict pResDest)
{
	FUNCTION_PROFILER_3DENGINE;

	return IsBoxVisible(objBox, pResDest);
}

bool COcclusionCuller::IsBoxVisible_OCEAN(const AABB& objBox, uint32* const __restrict pResDest)
{
	FUNCTION_PROFILER_3DENGINE;

	return IsBoxVisible(objBox, pResDest);
}

bool COcclusionCuller::IsBoxVisible_OCCELL(const AABB& objBox, uint32* const __restrict pResDest)
{
	FUNCTION_PROFILER_3DENGINE;
	if(GetCVars()->e_CoverageBufferDebugFreeze)
		return true;
	return IsBoxVisible(objBox, pResDest);
}

bool COcclusionCuller::IsBoxVisible_OBJECT(const AABB& objBox, uint32* const __restrict pResDest)
{
	FUNCTION_PROFILER_3DENGINE;

	return IsBoxVisible(objBox, pResDest);
}

bool COcclusionCuller::IsBoxVisible_OBJECT_TO_LIGHT(const AABB& objBox, uint32* const __restrict pResDest)
{
	FUNCTION_PROFILER_3DENGINE;

	return IsBoxVisible(objBox, pResDest);
}

bool COcclusionCuller::IsBoxVisible_PORTAL(const AABB& objBox, uint32* const __restrict pResDest)
{
	FUNCTION_PROFILER_3DENGINE;

	return IsBoxVisible(objBox, pResDest);
}

bool COcclusionCuller::RayBox(const AABB& objBox,const Vec3& rOrigin,const Vec3& rDir)const
{
	f32 txmax,txmin,tymax,tymin,tzmax,tzmin;

	if(rDir.x>=0.f)
	{
		const f32 IDirX	=	1.f/max(rDir.x,FLT_EPSILON);
		txmin	=	(objBox.min.x-rOrigin.x)*IDirX;
		txmax	=	(objBox.max.x-rOrigin.x)*IDirX;
	}
	else
	{
		const f32 IDirX	=	1.f/min(rDir.x,-FLT_EPSILON);
		txmin	=	(objBox.max.x-rOrigin.x)*IDirX;
		txmax	=	(objBox.min.x-rOrigin.x)*IDirX;
	}
	if(rDir.y>=0.f)
	{
		const f32 IDirY	=	1.f/max(rDir.y,FLT_EPSILON);
		tymin	=	(objBox.min.y-rOrigin.y)*IDirY;
		tymax	=	(objBox.max.y-rOrigin.y)*IDirY;
	}
	else
	{
		const f32 IDirY	=	1.f/min(rDir.y,-FLT_EPSILON);
		tymin	=	(objBox.max.y-rOrigin.y)*IDirY;
		tymax	=	(objBox.min.y-rOrigin.y)*IDirY;
	}
	if(txmin>tymax || tymin>txmax)
		return false;

	if(rDir.z>=0.f)
	{
		const f32 IDirZ	=	1.f/max(rDir.x,FLT_EPSILON);
		tzmin	=	(objBox.min.z-rOrigin.z)*IDirZ;
		tzmax	=	(objBox.max.z-rOrigin.z)*IDirZ;
	}
	else
	{
		const f32 IDirZ	=	1.f/min(rDir.z,-FLT_EPSILON);
		tzmin	=	(objBox.max.z-rOrigin.z)*IDirZ;
		tzmax	=	(objBox.min.z-rOrigin.z)*IDirZ;
	}

	if(tymin>txmin)
		txmin=tymin;
	if(tymax<txmax)
		txmax=tymax;

	return !(txmin>tzmax || tzmin>txmax);
}

bool COcclusionCuller::IsShadowcasterVisible(const AABB& objBox,Vec3 rExtrusionDir)
{
	FUNCTION_PROFILER_3DENGINE;
	if(!m_TrisWritten || !GetCVars()->e_CoverageBuffer || !(GetCVars()->e_CoverageBufferTestMode&2))
		return true;

	if(objBox.IsContainSphere(m_Camera.GetPosition(), -m_Camera.GetNearPlane()*2.f))
		return true;

	if(RayBox(objBox,m_Camera.GetPosition(),rExtrusionDir))
		return true;

	//if below the zero level, cut it to save fillrate-testing and get better results
	if(GetCVars()->e_ShadowsOcclusionCullingCaster>0 && objBox.max.z+rExtrusionDir.z<-1.f)
	{
		const f32 invHeight	=	(objBox.max.z-GetCVars()->e_ShadowsOcclusionCullingCaster)/(rExtrusionDir.z+0.000001f);
		rExtrusionDir*=-invHeight;
	}
	else
		rExtrusionDir*=-30.f;

	Vec4 arrVerts[16] = 
	{
		m_MatViewProj*Vec4(objBox.min.x,objBox.min.y,objBox.min.z,1.f),
		m_MatViewProj*Vec4(objBox.min.x,objBox.max.y,objBox.min.z,1.f),//1
		m_MatViewProj*Vec4(objBox.max.x,objBox.min.y,objBox.min.z,1.f),
		m_MatViewProj*Vec4(objBox.max.x,objBox.max.y,objBox.min.z,1.f),//3
		m_MatViewProj*Vec4(objBox.min.x,objBox.min.y,objBox.max.z,1.f),
		m_MatViewProj*Vec4(objBox.min.x,objBox.max.y,objBox.max.z,1.f),//5
		m_MatViewProj*Vec4(objBox.max.x,objBox.min.y,objBox.max.z,1.f),
		m_MatViewProj*Vec4(objBox.max.x,objBox.max.y,objBox.max.z,1.f),//7
		m_MatViewProj*Vec4(objBox.min.x+rExtrusionDir.x,objBox.min.y+rExtrusionDir.y,objBox.min.z+rExtrusionDir.z,1.f),
		m_MatViewProj*Vec4(objBox.min.x+rExtrusionDir.x,objBox.max.y+rExtrusionDir.y,objBox.min.z+rExtrusionDir.z,1.f),
		m_MatViewProj*Vec4(objBox.max.x+rExtrusionDir.x,objBox.min.y+rExtrusionDir.y,objBox.min.z+rExtrusionDir.z,1.f),
		m_MatViewProj*Vec4(objBox.max.x+rExtrusionDir.x,objBox.max.y+rExtrusionDir.y,objBox.min.z+rExtrusionDir.z,1.f),
		m_MatViewProj*Vec4(objBox.min.x+rExtrusionDir.x,objBox.min.y+rExtrusionDir.y,objBox.max.z+rExtrusionDir.z,1.f),
		m_MatViewProj*Vec4(objBox.min.x+rExtrusionDir.x,objBox.max.y+rExtrusionDir.y,objBox.max.z+rExtrusionDir.z,1.f),
		m_MatViewProj*Vec4(objBox.max.x+rExtrusionDir.x,objBox.min.y+rExtrusionDir.y,objBox.max.z+rExtrusionDir.z,1.f),
		m_MatViewProj*Vec4(objBox.max.x+rExtrusionDir.x,objBox.max.y+rExtrusionDir.y,objBox.max.z+rExtrusionDir.z,1.f)
	};

	const Vec3 vCamPos = m_Camera.GetPosition();

	// render only front faces of box
	COCPoly PolyClipped;


	//+x
	if(vCamPos.x>objBox.max.x)
	{
		if(rExtrusionDir.x<=-FLT_EPSILON)
		{//no extrusion
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[2],arrVerts[3],arrVerts[7],arrVerts[6]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
		}
//		else
		{//extrusion needed
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[2+8],arrVerts[3+8],arrVerts[7+8],arrVerts[6+8]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
			if(rExtrusionDir.y>0.f)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[2],arrVerts[2+8],arrVerts[6+8],arrVerts[6]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[3+8],arrVerts[3],arrVerts[7],arrVerts[7+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}

			if(rExtrusionDir.z>0.f)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[2],arrVerts[3],arrVerts[3+8],arrVerts[2+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[6+8],arrVerts[7+8],arrVerts[7],arrVerts[6]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
		}
	}

	//-x                                                                            
	if(vCamPos.x<objBox.min.x)
	{                                                                              
		if(rExtrusionDir.x>=FLT_EPSILON)
		{//no extrusion
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[1],arrVerts[0],arrVerts[4],arrVerts[5]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
		}
//		else
		{//extrusion needed
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[0+8],arrVerts[1+8],arrVerts[5+8],arrVerts[4+8]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
			if(rExtrusionDir.y>FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[0+8],arrVerts[0],arrVerts[4],arrVerts[4+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			if(rExtrusionDir.y<-FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[1],arrVerts[1+8],arrVerts[5+8],arrVerts[5]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			if(rExtrusionDir.z>FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[1],arrVerts[0],arrVerts[0+8],arrVerts[1+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			if(rExtrusionDir.z<-FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[4+8],arrVerts[4],arrVerts[5],arrVerts[5+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
		}
	}

	//+y
	if(vCamPos.y>objBox.max.y)                                                           
	{                                                                              
		if(rExtrusionDir.y<=-FLT_EPSILON)
		{//no extrusion
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[3],arrVerts[1],arrVerts[5],arrVerts[7]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
		}
//		else
		{//extrusion needed
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[3+8],arrVerts[1+8],arrVerts[5+8],arrVerts[7+8]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
			if(rExtrusionDir.x>0.f)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[1],arrVerts[3],arrVerts[3+8],arrVerts[1+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[5],arrVerts[7],arrVerts[7+8],arrVerts[5+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			if(rExtrusionDir.z>0.f)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[1],arrVerts[5],arrVerts[5+8],arrVerts[1+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[7+8],arrVerts[7],arrVerts[3],arrVerts[3+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
		}
	}//-y
	if(vCamPos.y<objBox.min.y)                                                           
	{                                                                              
		if(rExtrusionDir.y>=FLT_EPSILON)
		{//no extrusion
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[0],arrVerts[2],arrVerts[6],arrVerts[4]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
		}
//		else
		{//extrusion needed
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[0+8],arrVerts[2+8],arrVerts[6+8],arrVerts[4+8]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
			if(rExtrusionDir.x>0.f)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[4],arrVerts[0],arrVerts[0+8],arrVerts[4+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[2],arrVerts[6],arrVerts[6+8],arrVerts[2+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			if(rExtrusionDir.z>0.f)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[0+8],arrVerts[0],arrVerts[2],arrVerts[2+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[6],arrVerts[4],arrVerts[4+8],arrVerts[6+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
		}
	}
	//+z
	if(vCamPos.z>objBox.max.z)                                                           
	{                                                                              
		if(rExtrusionDir.z<=-FLT_EPSILON)
		{//no extrusion
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[5],arrVerts[4],arrVerts[6],arrVerts[7]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
		}
//		else
		{//extrusion needed
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[5+8],arrVerts[4+8],arrVerts[6+8],arrVerts[7+8]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
			if(rExtrusionDir.x>FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[5],arrVerts[4],arrVerts[4+8],arrVerts[5+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			if(rExtrusionDir.x<-FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[7+8],arrVerts[6+8],arrVerts[6],arrVerts[7]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			if(rExtrusionDir.y>FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[4],arrVerts[6],arrVerts[6+8],arrVerts[4+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			if(rExtrusionDir.y<-FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[5],arrVerts[5+8],arrVerts[7+8],arrVerts[7+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
		}
	}
	//-z
	if(vCamPos.z<objBox.min.z)                                                           
	{                                                                              
		if(rExtrusionDir.z>=FLT_EPSILON)
		{//no extrusion
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[0],arrVerts[1],arrVerts[3],arrVerts[2]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
		}
//		else
		{//extrusion needed
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[0+8],arrVerts[1+8],arrVerts[3+8],arrVerts[2+8]));
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
			if(rExtrusionDir.x>FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[0],arrVerts[1],arrVerts[1+8],arrVerts[0+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			if(rExtrusionDir.x<-FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[2+8],arrVerts[3+8],arrVerts[3],arrVerts[2]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			if(rExtrusionDir.y>FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[1],arrVerts[3],arrVerts[3+8],arrVerts[1+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
			else
			if(rExtrusionDir.y<-FLT_EPSILON)
			{
				COCClipper::Clip(PolyClipped,COCPoly(arrVerts[2],arrVerts[0],arrVerts[0+8],arrVerts[2+8]));
				if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
			}
		}
	}
	return false;
}

void COcclusionCuller::UpdateHierarchy()
{
#ifdef OC_HIERARCHICAL_ZBUFFER
	m_Dirty	=	false;
	m_CMin	=	m_CBuffer[0];
	for(uint32 y=0,pt=0;y<m_SizeY;y++)
	{
		TOCCexel MinC=m_CBuffer[pt++];
		for(uint32 x=1;x<(m_SizeX>>OC_ZEXELSKIP_SHIFT);x++)
		{
			TOCCexel	C=m_CBuffer[pt++];
			if(C>MinC)
				MinC=C;
		}
		m_YCBuffer[y]	=	MinC;
		if(MinC>m_CMin)
			m_CMin	=	MinC;
	}
#endif
}

#if !defined(CRCYG_CM)
SPU_ENTRY(CB_IsBoxVisible)
#endif
bool COcclusionCuller::IsBoxVisible(const AABB& objBox, uint32* const __restrict pResDest)
{
#if !defined(PS3)
	#ifdef OC_DEBUG_SHADOWVOLUMES
		if(GetCVars()->e_CoverageBufferDebug&32)
			return true;
	#endif
	if(!m_TrisWritten || !GetCVars()->e_CoverageBuffer)
		return true;
#endif
	// if camera is inside of box
  if(objBox.IsContainSphere(m_Camera.GetPosition(), -m_Camera.GetNearPlane()*2.f))
    return true;
/*
	if(m_bTreeIsReady && GetCVars()->e_CoverageBufferTestMode&1 && m_pTree)
	{ // world space test
//		FRAME_PROFILER( "CB::IsBoxVisible_WS", GetSystem(), PROFILE_3DENGINE );

		static PodArray<CWSNode*> lstNodes; 
		lstNodes.Clear();

		bool bVisible = m_pTree->IsBoxVisible( objBox, this, lstNodes );

		if(bVisible || !lstNodes.Count())
			return bVisible;

		if(GetCVars()->e_CoverageBufferTreeDebug)
			for(int32 i=0; i<lstNodes.Count(); i++)
				lstNodes[i]->DrawDebug(this, true);
	}
*/
#if !defined(PS3)
	if(!(GetCVars()->e_CoverageBufferTestMode&2))
		return true;
#ifdef OC_HIERARCHICAL_ZBUFFER
	if(m_Dirty)
		UpdateHierarchy();
#endif
#endif
	// image space test
	m_ObjectsTested++;

	Vec4 arrVerts[8] = 
	{
		m_MatViewProj*Vec4(objBox.min.x,objBox.min.y,objBox.min.z,1.f),//0
		m_MatViewProj*Vec4(objBox.min.x,objBox.max.y,objBox.min.z,1.f),//1
		m_MatViewProj*Vec4(objBox.max.x,objBox.min.y,objBox.min.z,1.f),//2
		m_MatViewProj*Vec4(objBox.max.x,objBox.max.y,objBox.min.z,1.f),//3
		m_MatViewProj*Vec4(objBox.min.x,objBox.min.y,objBox.max.z,1.f),//4
		m_MatViewProj*Vec4(objBox.min.x,objBox.max.y,objBox.max.z,1.f),//5
		m_MatViewProj*Vec4(objBox.max.x,objBox.min.y,objBox.max.z,1.f),//6
		m_MatViewProj*Vec4(objBox.max.x,objBox.max.y,objBox.max.z,1.f)//7
	};

//	Point2d arrVerts[8];
	
	// transform into screen space
//	for(int32 i=0; i<8; i++)
//		arrVerts[i] = m_MatViewProj*arrVerts[i];

	Vec3 vCamPos = m_Camera.GetPosition();

	{
//			FRAME_PROFILER( "CB::IsBoxVisible_IS_Scan", GetSystem(), PROFILE_3DENGINE );

		// render only front faces of box
		COCPoly PolyClipped;
		if(vCamPos.x>objBox.max.x)
		{
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[2],arrVerts[3],arrVerts[7],arrVerts[6]));
//			Rasterize<true,false>(PolyClipped);
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
					return true;
		}                                                                              
		else if(vCamPos.x<objBox.min.x)
		{                                                                              
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[1],arrVerts[0],arrVerts[4],arrVerts[5]));
//			Rasterize<true,false>(PolyClipped);
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
		}                                                                              

		if(vCamPos.y>objBox.max.y)                                                           
		{                                                                              
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[3],arrVerts[1],arrVerts[5],arrVerts[7]));
//			Rasterize<true,false>(PolyClipped);
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
		}                                                                              
		else if(vCamPos.y<objBox.min.y)                                                           
		{                                                                              
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[0],arrVerts[2],arrVerts[6],arrVerts[4]));
//			Rasterize<true,false>(PolyClipped);
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
		}

		if(vCamPos.z>objBox.max.z)                                                           
		{                                                                              
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[5],arrVerts[4],arrVerts[6],arrVerts[7]));
//			Rasterize<true,false>(PolyClipped);
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
		}                                                                              
		else if(vCamPos.z<objBox.min.z)                                                           
		{                                                                              
			COCClipper::Clip(PolyClipped,COCPoly(arrVerts[0],arrVerts[1],arrVerts[3],arrVerts[2]));
//			Rasterize<true,false>(PolyClipped);
			if(PolyClipped.Count()>2 && RasterizeClip<false>(PolyClipped))
				return true;
		}
	}










	++m_ObjectsTestedAndRejected;


	return false;
}

void COcclusionCuller::DrawDebug(int32 nStep)
{ // project buffer to the screen
	nStep	%=	32;
  if(!nStep)
    return;
	if(GetCVars()->e_CoverageBufferDebugFreeze)
	{
		Matrix44A	iVP;
		iVP.Invert(m_MatViewProj);

		Vec3 vSize(5.f,5.f,5.f);
		const f32 Addf	=	2.f/static_cast<f32>(nStep);
		f32 yf	=	-1.f;
		const	f32	fSizeZ		=	1.f/static_cast<f32>(OC_ZEXEL_MAXVALUEHIGH-64);//bias to prevent overflows or sign missmatches with int2float2int
		for(uint32 y=0; y<m_SizeY; y+=nStep,yf+=Addf)
		{
			f32 xf	=	-1.f;
			for(uint32 x=0; x<m_SizeX; x+=nStep,xf+=Addf)
			{
				const f32 Z=1.f-m_ZBuffer[ezbtFront][x+y*m_SizeX]*fSizeZ;
				Vec4 vPos4	=	iVP*Vec4(xf,yf,Z,1.f);
				vPos4	*=	1.f/vPos4.w;
				Vec3 vPos(vPos4.x,vPos4.y,vPos4.z);
				int32 Value	=	(int32)(Z*0xff);
				ColorB col(Value,Value,Value, (uint8)255);
				GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vPos-vSize*Z, vPos+vSize*Z), nStep==1, col, eBBD_Faceted);
			}
		}
		return;
	}

	m_pRenderer->Set2DMode(true,m_SizeX,m_SizeY);
#ifdef 		OC_ZEROCYCLECLEAR
	if(nStep==1)
	{
		Vec3 vSize(.4f,.4f,.4f);
		if(nStep==1)
			vSize = Vec3(.5f,.5f,.5f);
		for(int32 y=0; y<m_SizeY; y+=nStep)
		for(int32 x=0; x<m_SizeX; x+=nStep)
		{
			Vec3 vPos((float)x,(float)(m_SizeY-y-1),0);
			vPos += Vec3(0.5f,-0.5f,0);

			int32 Value	=	m_ZBuffer[ezbtFront][x+y*m_SizeX]>>(OC_ZEXELHIGH_2LOW+8);

			ColorB col(Value,Value,Value, (uint8)255);
			GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vPos-vSize, vPos+vSize), nStep==1, col, eBBD_Faceted);
		}
		m_pRenderer->Set2DMode(false,m_SizeX,m_SizeY);
		return;
	}
	else
		nStep--;
#endif
#ifdef OC_HIERARCHICAL_ZBUFFER
	if(nStep==1)
	{
		Vec3 vSize(.5f*(1<<OC_ZEXELSKIP_SHIFT),.5f,.5f);
		for(int32 y=0; y<m_SizeY; y+=nStep)
			for(int32 x=0; x<m_SizeX; x+=(nStep<<OC_ZEXELSKIP_SHIFT))
			{
				Vec3 vPos((float)x,(float)(m_SizeY-y-1),0);
				vPos += Vec3(0.5f*(1<<OC_ZEXELSKIP_SHIFT),-0.5f,0);
				int32 Value =	m_CBuffer[(x+y*m_SizeX)>>OC_ZEXELSKIP_SHIFT];
				ColorB col(Value,Value,Value, (uint8)255);
				GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vPos-vSize, vPos+vSize), 1/*TRUE*/, col, eBBD_Faceted);
			}
	}
	else
	{
		nStep--;
#else
	{
#endif

		Vec3 vSize(.4f,.4f,.4f);
		if(nStep==1)
			vSize = Vec3(.5f,.5f,.5f);
		for(uint32 y=0; y<m_SizeY; y+=nStep)
		for(uint32 x=0; x<m_SizeX; x+=nStep)
		{
			Vec3 vPos((float)x,(float)(m_SizeY-y-1),0);
			vPos += Vec3(0.5f,-0.5f,0);
			int32 Value	=	m_ZBuffer[ezbtFront][x+y*m_SizeX]==OC_ZEXEL_CLEARVALUEHIGH?0:(static_cast<int32>((m_ZBuffer[ezbtFront][x+y*m_SizeX])*GetCVars()->e_CoverageBufferDebugDrawScale)>>OC_ZEXELHIGH_2LOW)&0xff;
			ColorB col(Value,Value,Value, (uint8)255);
			GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vPos-vSize, vPos+vSize), nStep==1, col, eBBD_Faceted);
		}
	}
  m_pRenderer->Set2DMode(false,m_SizeX,m_SizeY);
}

void COcclusionCuller::GetMemoryUsage(ICrySizer * pSizer)
{
	SIZER_COMPONENT_NAME(pSizer, "CoverageBuffer");
	pSizer->AddObject(this, sizeof(COcclusionCuller));
	pSizer->AddObject(m_ZBuffer, 2 * m_SizeZ * sizeof(TOCZexel));
#ifndef PS3
	pSizer->AddContainer(m_VertexCache);
	pSizer->AddContainer(m_ProjectedVertexCacheX);
	pSizer->AddContainer(m_ProjectedVertexCacheY);
	pSizer->AddContainer(m_ProjectedVertexCacheZ);
#endif
#ifdef OC_HIERARCHICAL_ZBUFFER
	#if !defined(PS3)
		pSizer->AddContainer(m_CBuffer);
		pSizer->AddContainer(m_YCBuffer);
	#endif
#endif
//	pSizer->AddContainer(m_VertexClipCache);
}
#endif//PS3
