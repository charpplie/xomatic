////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   COcclusionCuller.h
//  Version:     v1.00
//  Created:     04/01/2008 by Michael Kopietz
//  Compilers:   Visual Studio.NET
//  Description: Occlusion Culler using hardware generated ZBuffer
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef _ZBUFFERCULLER_H
#define _ZBUFFERCULLER_H







#include <Cry_Math.h>
struct IRenderMesh;

typedef	uint16			TZBZexel;
const uint64				TZB_MAXDEPTH	=	(1<<(sizeof(TZBZexel)*8))-1;

// equivalent to MIN(a, b); [branchless]
ILINE int min_branchless(int a, int b)
{
	int diff = a - b;
	int mask = diff >> 31; // sign extend
	return (b & (~mask)) | (a & mask); 
}


class CZBufferCuller : public Cry3DEngineBase
{
protected:
	bool											m_DebugFreez;
	uint32										m_SizeX;
	uint32										m_SizeY;
	f32												m_fSizeX;
	f32												m_fSizeY;
	f32												m_fSizeZ;
	SPU_DOMAIN_LOCAL TZBZexel* m_ZBuffer;
	Matrix44									m_MatProj;
	Matrix44									m_MatView;
	Matrix44									m_MatViewProj;
	Matrix44A									m_MatViewProjT;
	Vec3											m_Position;










	int32											m_Bias;
	uint32										m_RotationSafe;
	uint32										m_AccurateTest;
	uint32										m_Treshold;

	f32												m_FrameTime;
	f32												m_FixedZFar;
	uint32										m_ObjectsTested;
	uint32										m_ObjectsTestedAndRejected;
	CCamera										m_Camera;
	int												m_OutdoorVisible;

	template<uint32 ROTATE,class T>
	bool											Rasterize(const T rVertices,const uint32	VCount)
														{
															int64	MinX=m_SizeX;
															int64	MaxX=0;
															int64	MinY=m_SizeY;
															int64	MaxY=0;
															int64 MinZ=TZB_MAXDEPTH;
															for(uint32 a=0;a<VCount;a++)
															{
																Vec4	V	=	rVertices[a];
																const f32 InvW	=	1.f/V.w;
																int64	X	=	static_cast<int64>((V.x*InvW*0.5f+0.5f)*m_fSizeX+0.5f);
																int64	Y	=	static_cast<int64>((V.y*InvW*0.5f+0.5f)*m_fSizeY+0.5f);
																int64 Z	=	static_cast<int64>(V.z*InvW*m_fSizeZ);
																if(X<MinX)
																	MinX=X;
																else
																if(X>MaxX)
																	MaxX=X;
																if(Y<MinY)
																	MinY=Y;
																else
																if(Y>MaxY)
																	MaxY=Y;
																if(Z<MinZ)
																	MinZ=Z;
															}
															if(MinX<0)
															{
																if(ROTATE==1)
																	return true;
																MinX=0;
															}
															if(MaxX>m_SizeX)
															{
																if(ROTATE==1)
																	return true;
																MaxX=m_SizeX;
															}
															if(MinY<0)
															{
																if(ROTATE==1)
																	return true;
																MinY=0;
															}
															if(MaxY>m_SizeY)
															{
																if(ROTATE==1)
																	return true;
																MaxY=m_SizeY;
															}
															if(ROTATE==2)
															{
																if(MinX>=m_SizeX ||	MinY>=m_SizeY || MaxX<0 ||	MaxX<0)
																	return true;
															}
															for(int64 y=MinY;y<MaxY;y++)
															for(int64 x=MinX;x<MaxX;x++)
																if(static_cast<int64>(m_ZBuffer[static_cast<int32>(x)+static_cast<int32>(y)*m_SizeX])>MinZ)
																	return true;
/*
															if(MinX<0)
																MinX=0;
															if(MaxX>m_SizeX-1)
																MaxX=m_SizeX-1;
															if(MinY<0)
																MinY=0;
															if(MaxY>m_SizeY-1)
																MaxY=m_SizeY-1;

															for(int64 x=MinX;x<MaxX;x++)
																if(m_ZBuffer[x+MinY*m_SizeX]<MinZ)
																	m_ZBuffer[x+MinY*m_SizeX]=MinZ;
															for(int64 x=MinX;x<MaxX;x++)
																if(m_ZBuffer[x+MaxY*m_SizeX]<MinZ)
																	m_ZBuffer[x+MaxY*m_SizeX]=MinZ;
															for(int64 y=MinY;y<MaxY;y++)
																if(m_ZBuffer[MinX+y*m_SizeX]<MinZ)
																	m_ZBuffer[MinX+y*m_SizeX]=MinZ;
															for(int64 y=MinY;y<MaxY;y++)
																if(m_ZBuffer[MaxX+y*m_SizeX]<MinZ)
																	m_ZBuffer[MaxX+y*m_SizeX]=MinZ;
*/
															return false;
														}



	bool											IsBoxVisible_OCCLUDER(const AABB& objBox, uint32* const __restrict pResDest = NULL);
	bool											IsBoxVisible_OCEAN(const AABB& objBox, uint32* const __restrict pResDest = NULL);
	bool											IsBoxVisible_OCCELL(const AABB& objBox, uint32* const __restrict pResDest = NULL);
	bool											IsBoxVisible_OCCELL_OCCLUDER(const AABB& objBox, uint32* const __restrict pResDest = NULL);
	bool											IsBoxVisible_OBJECT(const AABB& objBox, uint32* const __restrict pResDest = NULL);
	bool											IsBoxVisible_OBJECT_TO_LIGHT(const AABB& objBox, uint32* const __restrict pResDest = NULL);
	bool											IsBoxVisible_TERRAIN_NODE(const AABB& objBox, uint32* const __restrict pResDest = NULL);
	bool											IsBoxVisible_PORTAL(const AABB& objBox, uint32* const __restrict pResDest = NULL);
	bool											IsBoxVisible(const AABB& objBox, uint32* const __restrict pResDest = NULL);


public:
#ifndef __SPU__
														CZBufferCuller();
														~CZBufferCuller(){CryModuleMemalignFree(m_ZBuffer);}




















#endif

	// start new frame
	void											BeginFrame(const CCamera& rCam);
	void											ReloadBuffer(const uint32 BufferID);

	// render into buffer
	ILINE void								AddRenderMesh(IRenderMesh * pRM, Matrix34A* pTranRotMatrix, IMaterial * pMaterial, bool bOutdoorOnly, bool bCompletelyInFrustum,bool bNoCull){}
	ILINE void								AddHeightMap(const struct SRangeInfo & m_rangeInfo, float X1, float Y1, float X2, float Y2){}

	// test visibility


















































































































































































































































































































































































































































































































































































































































































































































ILINE bool IsObjectVisible(const AABB& objBox, EOcclusionObjectType eOcclusionObjectType, float fDistance, uint32* pRetVal = NULL)
{
#if defined(PS3) || (defined(XENON) && XENON_CULLER_VECTORIZED)
  bool Ret;
	//TODOMK make accurate test after bounding rect returns 1













	{
		if(m_RotationSafe==0)
			Ret	=	IsObjectVisible<0>(objBox);
		else
		if(m_RotationSafe==1)
			Ret	=	IsObjectVisible<1>(objBox);
		else
			Ret	=	IsObjectVisible<2>(objBox);
	}
  return Ret;
#else
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
#endif
	return true;
}

	bool											IsShadowcasterVisible(const AABB& objBox,Vec3 rExtrusionDir){return true;};
	// draw content to the screen for debug
	void											DrawDebug(int32 nStep);

	// update tree
	void											UpdateDepthTree(){};

	// return current camera
	const CCamera&						GetCamera() const {return m_Camera;}

	//set the scissor for clipping (0.f|0.f to 1.f|1.f)
/*	void											Scissor(f32 TopLeftX,f32 TopLeftY,f32 BottomRightX,f32 BottomRightY)
														{
															m_TopLeftX			=	TopLeftX*2.f-1.f;
															m_TopLeftY			=	TopLeftY*2.f-1.f;
															m_BottomRightX	=	BottomRightX*2.f-1.f;
															m_BottomRightY	=	BottomRightY*2.f-1.f;
														}
*/

	void											GetMemoryUsage(ICrySizer * pSizer);

	void											SetFrameTime(f32 fTime) 
														{
															m_FrameTime = fTime; 
														}
	ILINE f32									GetFrameTime(){return m_FrameTime;}
	bool											IsOutdooVisible(){return m_OutdoorVisible==1;}
	void											TrisWritten(int32){}
	int32											TrisWritten()const{return 0;}
	void											ObjectsWritten(int32){}
	int32											ObjectsWritten()const{return 0;}
	int32											TrisTested()const{return 0;}
	int32											ObjectsTested()const{return m_ObjectsTested;}
	int32											ObjectsTestedAndRejected()const{return m_ObjectsTestedAndRejected;}
	int32											SelRes()const{return m_SizeX;}
	float											FixedZFar()const{return m_FixedZFar;}
	float											GetZNearInMeters()const{return 0.f;}
	float											GetZFarInMeters()const{return 1024;}

} _ALIGN(128);

ILINE bool CZBufferCuller::IsBoxVisible_TERRAIN_NODE(const AABB& objBox, uint32* const __restrict pResDest)
{
	return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OCCELL_OCCLUDER(const AABB& objBox, uint32* const __restrict pResDest)
{
	return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OCCLUDER(const AABB& objBox, uint32* const __restrict pResDest)
{
	return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OCEAN(const AABB& objBox, uint32* const __restrict pResDest)
{
	return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OCCELL(const AABB& objBox, uint32* const __restrict pResDest)
{
	if(GetCVars()->e_CoverageBufferDebugFreeze)
		return true;
	return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OBJECT(const AABB& objBox, uint32* const __restrict pResDest)
{
	return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OBJECT_TO_LIGHT(const AABB& objBox, uint32* const __restrict pResDest)
{
	return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_PORTAL(const AABB& objBox, uint32* const __restrict pResDest)
{
	return IsBoxVisible(objBox, pResDest);
}

#endif 
