/*=============================================================================
  PS2_RERender.cpp : implementation of the Rendering RenderElements pipeline.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honitch Andrey

=============================================================================*/

#include "StdAfx.h"
#include "NULL_Renderer.h"
#include "I3DEngine.h"

//#include "../cry3dengine/StatObj.h"

#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

//=======================================================================

bool CRESky::mfDraw(CShader *ef, SShaderPass *sfm)
{
  return true;
}

bool CREHDRSky::mfDraw( CShader *ef, SShaderPass *sfm )
{
  return true;
}

bool CREFogVolume::mfDraw( CShader* ef, SShaderPass* sfm )
{
  return true;
}

bool CREWaterVolume::mfDraw( CShader* ef, SShaderPass* sfm )
{
  return true;
}

bool CREWaterWave::mfDraw( CShader* ef, SShaderPass* sfm )
{
  return true;
}

void CREWaterOcean::FrameUpdate()
{

}

void CREWaterOcean::Create(  uint32 nVerticesCount, SVF_P3F_C4B_T2F *pVertices, uint32 nIndicesCount, uint16 *pIndices )
{
  
}

void CREWaterOcean::ReleaseOcean()
{
}

bool CREWaterOcean::mfDraw( CShader* ef, SShaderPass* sfm )
{
  return true;
}

CREOcclusionQuery::~CREOcclusionQuery()
{
  mfReset();
}

void CREOcclusionQuery::mfReset()
{
  m_nOcclusionID = 0;
}

uint32 CREOcclusionQuery::m_nQueriesPerFrameCounter = 0;
uint32 CREOcclusionQuery::m_nReadResultNowCounter = 0;
uint32 CREOcclusionQuery::m_nReadResultTryCounter = 0;

bool CREOcclusionQuery::mfDraw(CShader *ef, SShaderPass *sfm)
{
  return true;
}
bool CREOcclusionQuery::mfReadResult_Now(void)
{
  return true;
}
bool CREOcclusionQuery::mfReadResult_Try(void)
{
  return true;
}
bool CREOcclusionQuery::RT_ReadResult_Try()
{
  return true;
}

void CRETempMesh::mfReset()
{
  gRenDev->m_DevBufMan.ReleaseVBuffer(m_pVBuffer);
  gRenDev->m_DevBufMan.ReleaseIBuffer(m_pIBuffer);
  m_pVBuffer = NULL;
  m_pIBuffer = NULL;
}

bool CRETempMesh::mfPreDraw(SShaderPass *sl)
{
  return true;
}

bool CRETempMesh::mfDraw(CShader *ef, SShaderPass *sl)
{
  return true;
}

bool CREMesh::mfPreDraw(SShaderPass *sl)
{
  return true;
}

bool CREMesh::mfDraw(CShader *ef, SShaderPass *sl)
{
  return true;
}

bool CREFlare::mfCheckVis(CRenderObject *obj)
{
  return false;
}

bool CREFlare::mfDraw(CShader *ef, SShaderPass *sfm)
{
  return true;
}

bool CREHDRProcess::mfDraw(CShader *ef, SShaderPass *sfm)
{
  return true;
}

bool CREDeferredShading::mfDraw(CShader *ef, SShaderPass *sfm)
{
  return true;
}

bool CREBeam::mfDraw(CShader *ef, SShaderPass *sl)
{
  return true;
}

bool CREImposter::mfDraw(CShader *ef, SShaderPass *pPass)
{
  return true;
}

bool CREPanoramaCluster::mfDraw(CShader *ef, SShaderPass *pPass)
{
  return true;
}

bool CRECloud::mfDraw(CShader *ef, SShaderPass *pPass)
{
  return true;
}

bool CRECloud::UpdateImposter(CRenderObject *pObj)
{
  return true;
}

bool CRECloud::GenerateCloudImposter(CShader *pShader, SRenderShaderResources *pRes, CRenderObject *pObject)
{
	return true;
}

bool CREImposter::UpdateImposter()
{
  return true;
}

bool CREParticle::mfPreDraw(SShaderPass *sl)
{
	return true;
}

bool CREParticle::mfDraw(CShader* ef, SShaderPass* sfm)
{
	return true;
}

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
bool CREParticleGPU::mfDraw(CShader *ef, SShaderPass *sfm)
{
	return true;
}
#endif

bool CREVolumeObject::mfDraw(CShader* ef, SShaderPass* sfm)
{
	return true;
}


#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
bool CREPrismObject::mfDraw(CShader* ef, SShaderPass* sfm)
{
	return true;
}
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

bool CREIrradianceVolume::mfPreDraw(SShaderPass *sl)
{
	return true;
}

void CREIrradianceVolume::UpdateRenderParameters()
{

}

bool CREIrradianceVolume::mfDraw(CShader* ef, SShaderPass* sfm)
{
	return true;
}
void CREIrradianceVolume::InsertLight( const CDLight &light )
{

}

void CREIrradianceVolume::RenderReflectiveShadowMap( SReflectiveShadowMap& rRSM )
{

}

CREIrradianceVolume::Settings* CREIrradianceVolume::GetFillSettings()
{
	return NULL;
}

bool CREGameEffect::mfDraw(CShader* ef, SShaderPass* sfm)
{
	return true;
}
