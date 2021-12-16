#include "StdAfx.h"
#include "CullBuffer.h"
#include "cvars.h"

#if defined(OCCLUSIONCULLER_RUNTIME)
void CCullBuffer::BeginFrame(const CCamera& rCam)
{
	m_CBuffer	=	GetCVars()->e_CoverageBufferVersion;
#if !defined(PS3)
	if(m_CBuffer==3)
	{
		assert("Not supported on this platform");
		m_CBuffer=2;
	}
#endif
	if(m_CBuffer==1)
	{
		m_BufferV.BeginFrame(rCam);
	}
	else
	if(m_CBuffer==2)
	{
		m_BufferM.BeginFrame(rCam);
	}
	else
	if(m_CBuffer==3)
	{
		m_BufferZ.BeginFrame(rCam);
	}
}

// render into buffer
void CCullBuffer::AddRenderMesh(IRenderMesh * pRM, Matrix34A* pTranRotMatrix, IMaterial * pMaterial, bool bOutdoorOnly, bool bCompletelyInFrustum,bool bNoCull)
{
	if(m_CBuffer==1)
	{
		m_BufferV.AddRenderMesh(pRM,pTranRotMatrix,pMaterial,bOutdoorOnly,bCompletelyInFrustum,bNoCull);
	}
	else
	if(m_CBuffer==2)
	{
		m_BufferM.AddRenderMesh(pRM,pTranRotMatrix,pMaterial,bOutdoorOnly,bCompletelyInFrustum,bNoCull);
	}
	else
	if(m_CBuffer==3)
	{
		m_BufferZ.AddRenderMesh(pRM,pTranRotMatrix,pMaterial,bOutdoorOnly,bCompletelyInFrustum,bNoCull);
	}
}

void CCullBuffer::AddHeightMap(const SRangeInfo & m_rangeInfo, float X1, float Y1, float X2, float Y2)
{
	if(m_CBuffer==1)
	{
//		m_BufferV.AddHeightMap(pRM,pTranRotMatrix,pMaterial,bOutdoorOnly,bCompletelyInFrustum,bNoCull);
	}
	else
	if(m_CBuffer==2)
	{
		m_BufferM.AddHeightMap(m_rangeInfo, X1, Y1, X2, Y2);
	}
	else
	if(m_CBuffer==3)
	{
		m_BufferZ.AddHeightMap(m_rangeInfo, X1, Y1, X2, Y2);
	}
}


// test visibility
bool CCullBuffer::IsObjectVisible(const AABB& objBox, EOcclusionObjectType eOcclusionObjectType, float fDistance, uint32* pRetVal)
{
	if(m_CBuffer==1)
	{
		return m_BufferV.IsObjectVisible(objBox,eOcclusionObjectType,fDistance, pRetVal);
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.IsObjectVisible(objBox,eOcclusionObjectType,fDistance, pRetVal);
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.IsObjectVisible(objBox,eOcclusionObjectType,fDistance, pRetVal);
	}
	return true;
}

// Extrude and test shadowcasterBBox visibility
bool CCullBuffer::IsShadowcasterVisible(const AABB& objBox,Vec3 rExtrusionDir)
{
	if(m_CBuffer==1)
	{
		return m_BufferV.IsShadowcasterVisible(objBox,rExtrusionDir);
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.IsShadowcasterVisible(objBox,rExtrusionDir);
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.IsShadowcasterVisible(objBox,rExtrusionDir);
	}
	return true;
}

// draw content to the screen for debug
void CCullBuffer::DrawDebug(int nStep)
{
	if(m_CBuffer==1)
	{
		return m_BufferV.DrawDebug(nStep);
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.DrawDebug(nStep);
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.DrawDebug(nStep);
	}
}

// update tree
void CCullBuffer::UpdateDepthTree()
{
	if(m_CBuffer==1)
	{
		return m_BufferV.UpdateDepthTree();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.UpdateDepthTree();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.UpdateDepthTree();
	}
}

// return current camera
const CCamera& CCullBuffer::GetCamera()
{
	if(m_CBuffer==1)
	{
		return m_BufferV.GetCamera();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.GetCamera();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.GetCamera();
	}
	return m_BufferM.GetCamera();
}


void CCullBuffer::GetMemoryUsage(ICrySizer * pSizer)
{
	if(m_CBuffer==1)
	{
		return m_BufferV.GetMemoryUsage(pSizer);
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.GetMemoryUsage(pSizer);
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.GetMemoryUsage(pSizer);
	}
}

void CCullBuffer::SetFrameTime(f32 fTime) 
{
	if(m_CBuffer==1)
	{
		return m_BufferV.SetFrameTime(fTime);
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.SetFrameTime(fTime);
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.SetFrameTime(fTime);
	}
}

f32 CCullBuffer::GetFrameTime() 
{
	if(m_CBuffer==1)
	{
		return m_BufferV.GetFrameTime();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.GetFrameTime();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.GetFrameTime();
	}
	return 1.f;
}

bool CCullBuffer::IsOutdooVisible()
{
	if(m_CBuffer==1)
	{
		return m_BufferV.IsOutdooVisible();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.IsOutdooVisible();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.IsOutdooVisible();
	}
	return true;
}

void CCullBuffer::TrisWritten(int n)
{
	if(m_CBuffer==1)
	{
		m_BufferV.TrisWritten(n);
	}
	else
	if(m_CBuffer==2)
	{
		m_BufferM.TrisWritten(n);
	}
	else
	if(m_CBuffer==3)
	{
		m_BufferZ.TrisWritten(n);
	}
}

int CCullBuffer::TrisWritten()const
{
	if(m_CBuffer==1)
	{
		return m_BufferV.TrisWritten();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.TrisWritten();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.TrisWritten();
	}
	return 0;
}

void CCullBuffer::ObjectsWritten(int n)
{
	if(m_CBuffer==1)
	{
		m_BufferV.ObjectsWritten(n);
	}
	else
	if(m_CBuffer==2)
	{
		m_BufferM.ObjectsWritten(n);
	}
	else
	if(m_CBuffer==3)
	{
		m_BufferZ.ObjectsWritten(n);
	}
}

int CCullBuffer::ObjectsWritten()const
{
	if(m_CBuffer==1)
	{
		return m_BufferV.ObjectsWritten();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.ObjectsWritten();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.ObjectsWritten();
	}
	return 0;
}

int CCullBuffer::TrisTested()const
{
	if(m_CBuffer==1)
	{
		return m_BufferV.TrisTested();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.TrisTested();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.TrisTested();
	}
	return 0;
}

int CCullBuffer::ObjectsTested()const
{
	if(m_CBuffer==1)
	{
		return m_BufferV.ObjectsTested();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.ObjectsTested();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.ObjectsTested();
	}
	return 0;
}

int CCullBuffer::ObjectsTestedAndRejected()const
{
	if(m_CBuffer==1)
	{
		return m_BufferV.ObjectsTestedAndRejected();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.ObjectsTestedAndRejected();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.ObjectsTestedAndRejected();
	}
	return 0;
}

int CCullBuffer::SelRes()const
{
	if(m_CBuffer==1)
	{
		return m_BufferV.SelRes();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.SelRes();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.SelRes();
	}
	return 0;
}

float CCullBuffer::FixedZFar()const
{
	if(m_CBuffer==1)
	{
		return m_BufferV.FixedZFar();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.FixedZFar();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.FixedZFar();
	}
	return m_BufferM.FixedZFar();
}

float CCullBuffer::GetZNearInMeters()const
{
	if(m_CBuffer==1)
	{
		return m_BufferV.GetZNearInMeters();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.GetZNearInMeters();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.GetZNearInMeters();
	}
	return m_BufferM.GetZNearInMeters();
}

float CCullBuffer::GetZFarInMeters()const
{
	if(m_CBuffer==1)
	{
		return m_BufferV.GetZFarInMeters();
	}
	else
	if(m_CBuffer==2)
	{
		return m_BufferM.GetZFarInMeters();
	}
	else
	if(m_CBuffer==3)
	{
		return m_BufferZ.GetZFarInMeters();
	}
	return m_BufferM.GetZFarInMeters();
}

#endif
