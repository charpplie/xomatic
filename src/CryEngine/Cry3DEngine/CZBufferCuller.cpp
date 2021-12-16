////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   CZBufferCuller.h
//  Version:     v1.00
//  Created:     13/8/2006 by Michael Kopietz
//  Compilers:   Visual Studio.NET
//  Description: Occlusion buffer
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CZBufferCuller.h"

#ifndef __SPU__
//#define ENABLE_DEBUG_CLEAR
#endif




#define GLOBALEXTERN 


GLOBALEXTERN	SHWOccZBuffer HWZBuffer;


























#ifdef ENABLE_DEBUG_CLEAR
	static volatile int shi=16;
#else
	#define shi 16
#endif
void CZBufferCuller::BeginFrame(SPU_DOMAIN_LOCAL const CCamera& rCam)
{
	m_AccurateTest	=	GetCVars()->e_CoverageBufferAccurateOBBTest;






	m_Treshold			=	GetCVars()->e_CoverageBufferTolerance;

#if !defined(__SPU__)
	FUNCTION_PROFILER_3DENGINE;
	if(GetCVars()->e_CoverageBufferDebugFreeze || GetCVars()->e_CameraFreeze)
		return;





#endif//__SPU__

	m_ObjectsTested	=
	m_ObjectsTestedAndRejected =	0;
  //to enable statistics
	m_Camera		=	rCam;
	m_Position	=	rCam.GetPosition();
	uint32 oldSizeX = m_SizeX;		
	uint32 oldSizeY = m_SizeY;
	const uint32 sizeX = min(max(1, GetCVars()->e_CoverageBufferResolution),1024);
	const uint32 sizeY = sizeX;
	m_SizeX					=	
	m_SizeY					= sizeX;
	m_fSizeX				=	static_cast<f32>(sizeX);
	m_fSizeY				=	static_cast<f32>(sizeX);
	m_fSizeZ				=	static_cast<f32>(TZB_MAXDEPTH);

#ifndef __SPU__
	if(oldSizeX != sizeX)
	{
		CryModuleMemalignFree(m_ZBuffer);
		m_ZBuffer = (TZBZexel*)CryModuleMemalign(sizeof(TZBZexel)*sizeX*sizeY,128);
	}
#endif













//	CCamera tmpCam	= m_pRenderer->GetCamera();
//	m_pRenderer->SetCamera(m_Camera);
	//m_pRenderer->GetModelViewMatrix(reinterpret_cast<f32*>(&m_MatView));
	//m_pRenderer->GetProjectionMatrix(reinterpret_cast<f32*>(&m_MatProj));
	//m_MatViewProj				=		m_MatView*m_MatProj;

#if defined(XENON) && XENON_CULLER_VECTORIZED
	m_VMin	=	XMVectorSet(FLT_EPSILON,FLT_EPSILON,FLT_EPSILON,FLT_EPSILON);
	m_VMax	=	XMVectorSet(static_cast<float>(sizeX)-FLT_EPSILON,
		static_cast<float>(sizeY)-FLT_EPSILON,0.f,0.f);

  	const float SCALE		=	(const float)(sizeX / 2);
  	Matrix44A ScreenMat(	SCALE,0.f,0.f,SCALE,
  		0.f,SCALE,0.f,SCALE,
  		0.f,0.f,1.f,0.f,
  		0.f,0.f,0.f,1.f);

 	ScreenMat.Transpose();
	m_MatViewProjT	=	m_MatViewProj*ScreenMat;
	m_MatViewProj		=	m_MatViewProjT;
#endif
	m_MatViewProj.Transpose();
//	m_pRenderer->SetCamera(tmpCam);

	m_Bias					=	static_cast<int32>(GetCVars()->e_CoverageBufferBias);
	m_RotationSafe	=	GetCVars()->e_CoverageBufferRotationSafeCheck;
	m_DebugFreez		=	GetCVars()->e_CoverageBufferDebugFreeze!=0;
}

void CZBufferCuller::ReloadBuffer(const uint32 BufferID)
{
	if(m_DebugFreez)
		return;

































































































#undef HWZBuf
}

#ifndef __SPU__
CZBufferCuller::CZBufferCuller():
m_OutdoorVisible(1)
{
	//construct zbuffer as if might never get executed when running via SPUs
	m_SizeX = 
	m_SizeY = min(max(1, GetCVars()->e_CoverageBufferResolution),1024);
	m_ZBuffer = (TZBZexel*)CryModuleMemalign(sizeof(TZBZexel)*m_SizeX*m_SizeY,128);
	m_FrameTime			=	0.f;
	m_ObjectsTested	=
	m_ObjectsTestedAndRejected	=	0;
}

bool CZBufferCuller::IsBoxVisible(const AABB& objBox, uint32* const __restrict pResDest)
{
	FUNCTION_PROFILER_3DENGINE;
	m_ObjectsTested++;
	Vec4 Verts[8] = 
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
	bool CutNearPlane=Verts[0].w<=0.f;
	CutNearPlane	|=	Verts[1].w<=0.f;
	CutNearPlane	|=	Verts[2].w<=0.f;
	CutNearPlane	|=	Verts[3].w<=0.f;
	CutNearPlane	|=	Verts[4].w<=0.f;
	CutNearPlane	|=	Verts[5].w<=0.f;
	CutNearPlane	|=	Verts[6].w<=0.f;
	CutNearPlane	|=	Verts[7].w<=0.f;
	if(CutNearPlane)
		return true;

	if(m_RotationSafe==1)
		return Rasterize<1>(Verts,8);
	if(m_RotationSafe==2)
		return Rasterize<2>(Verts,8);
	return Rasterize<0>(Verts,8);

	++m_ObjectsTestedAndRejected;

	return false;
}


static int sh	=	8;
void CZBufferCuller::DrawDebug(int32 nStep)
{ // project buffer to the screen
	nStep	%=	32;
  if(!nStep)
    return;









	const CCamera& rCam = GetCamera();
	float farPlane = rCam.GetFarPlane();
	float nearPlane = rCam.GetNearPlane();
	
	float a = farPlane / (farPlane - nearPlane);
	float b = farPlane * nearPlane / (nearPlane - farPlane);
	
	const float scale = 5.0f;
	
	
	m_pRenderer->Set2DMode(true,m_SizeX,m_SizeY);
	SAuxGeomRenderFlags	Flags	=	e_Def3DPublicRenderflags;
	Flags.SetDepthWriteFlag(e_DepthWriteOff);
	Flags.SetAlphaBlendMode(e_AlphaBlended);
	m_pRenderer->GetIRenderAuxGeom()->SetRenderFlags(Flags);
	Vec3 vSize(.4f,.4f,.4f);
	if(nStep==1)
		vSize = Vec3(.5f,.5f,.5f);
	for(uint32 y=0; y<m_SizeY; y+=nStep)
	for(uint32 x=0; x<m_SizeX; x+=nStep)
	{
		Vec3 vPos((float)x,(float)(m_SizeY-y-1),0);
		vPos += Vec3(0.5f,-0.5f,0);
		const uint32 Value	=	m_ZBuffer[x+y*m_SizeX];
		//Value>>=sh;
		
		float w = Value / (65535.0f);
		
		float z = b / (w - a);

		
	  uint32 ValueC =  min(255u,(uint32)(z*scale));
		ColorB col(ValueC,ValueC,ValueC, 200);
		if(Value!=0xffff)
		{
			//ColorB col((Value&31)<<3,((Value>>5)&31)<<3,((Value>>10)&63)<<2,200);
			GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vPos-vSize, vPos+vSize), nStep<=2, col, eBBD_Faceted);
		}
	}
	//m_pRenderer->GetIRenderAuxGeom()->Flush();
  m_pRenderer->Set2DMode(false,m_SizeX,m_SizeY);
}

void CZBufferCuller::GetMemoryUsage(ICrySizer * pSizer)
{
	SIZER_COMPONENT_NAME(pSizer, "CoverageBuffer");
	pSizer->AddObject(this, sizeof(CZBufferCuller));
//	pSizer->AddObject(m_ZBuffer, 2 * m_SizeZ * sizeof(TOCZexel));
}
#endif//__SPU__
#undef shi
