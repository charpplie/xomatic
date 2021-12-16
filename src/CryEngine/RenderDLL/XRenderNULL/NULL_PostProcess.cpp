/*=============================================================================
D3DPostProcess : Direct3D specific post processing special effects
Copyright (c) 2001 Crytek Studios. All Rights Reserved.

Revision history:
* 23/02/2005: Re-factored/Converted to CryEngine 2.0 by Tiago Sousa
* Created by Tiago Sousa

Todo:
* Erradicate StretchRect usage
* Cleanup code
* When we have a proper static branching support use it instead of shader switches inside code

=============================================================================*/

#include "StdAfx.h"
#include "NULL_Renderer.h"
#include "I3DEngine.h"
#include "../Common/PostProcess/PostEffects.h"

using namespace NPostEffects;

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CREPostProcessData::Create() 
{  
  // Initialize all post process data

  PostEffectMgr().Create();
}

void CREPostProcessData::Release() 
{
  // Free all used resources

  PostEffectMgr().Release();
}

void CREPostProcessData:: Reset()
{  
  // Reset all post process data

  PostEffectMgr().Reset();

}

void CFilterChromaShift::Render()
{
}

bool CAlphaTestAA::Preprocess()
{
  return ( IsActive() && CRenderer::CV_r_useedgeaa != 0 );
}

void CAlphaTestAA::Render()
{
}

void CDepthOfField::Render()
{
}

bool CMotionBlur::Preprocess()
{
  return true;
}
void CMotionBlur::Render()
{
}

bool CSunShafts::Preprocess()
{
  return true;
}

void CSunShafts::Render() 
{
}

void CPointLightShafts::Render() 
{
}


void CFilterSharpening::Render()
{
}
void CFilterBlurring::Render()
{
}

void CFilterRadialBlurring::Render()
{
}

void CFilterDepthEnhancement::Render()
{
}

void CUnderwaterGodRays::Render()
{
}

void CVolumetricScattering::Render()
{
}

void CWaterDroplets::Render()
{
}

void CWaterFlow::Render()
{


}

void CWaterPuddles::Render()
{
}

bool CBloodSplats::Preprocess()
{
  return true;
}

void CBloodSplats::Render()
{
}

void CScreenFrost::Render()
{
}

bool CRainDrops::Preprocess()
{
  return true;
}
void CRainDrops::SpawnParticle( SRainDrop *&pParticle )
{
}
void CRainDrops::UpdateParticles()
{
}
void CRainDrops::RainDropsMapGen()
{
}
void CRainDrops::Render()
{
}

bool CNightVision::Preprocess()
{
  return true;
}

void CNightVision::Render()
{
}

bool CFlashBang::Preprocess()
{
  return true;
}

void CFlashBang::Render()
{
}
void CGlow::Render() 
{
}
void CColorCorrection::Render() 
{
}

void CDistantRain::Render()
{
}

void CAlienInterference::Render()
{
}

void CFilterMaskedBlurring::Render()
{
}

void CFilterGrain::Render()
{
}

void CFilterStylized::Render()
{
}

void CCryVision::Render()
{
}

void CColorGrading::Render()
{

}

void CWaterVolume::Render()
{

}

void CGammaReference::Render()
{

}

void CWaterRipples::Render()
{

}


void CSceneRain::CreateBuffers( uint16 nVerts, void *&pINpVB, SVF_P3F_C4B_T2F *pVtxList, uint16 nInds, void *&INpIB, uint16 *pIndsList)
{

}

int CSceneRain::Create()
{

  return 1;
}

void CSceneRain::Release()
{

}

void CSceneRain::Render()
{

}

bool CPostMSAA::Preprocess()
{
	return true;
}

void CPostMSAA::Render()
{

}

void CPostStereo::Render()
{

}

bool C3DHud::Preprocess(void)
{

	return true;
}

void C3DHud::Render(void)
{

}

void C3DHud::Reset(void)
{

}

void C3DHud::OnBeginFrame()
{

}

void C3DHud::AddRE(const CRendElementBase* re, const SShaderItem* pShaderItem, const CRenderObject* pObj)
{

}

namespace WaterVolumeStaticData
{
	void GetMemoryUsage( ICrySizer *pSizer ){}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CSceneRain::Preprocess() { return true; }
//void CSceneRain::Render() {}
void CSceneRain::Reset() {}
void CSceneRain::OnLostDevice() {}

const char *CSceneRain::GetName() const {return 0;}
const char *CRainDrops::GetName() const {return 0;}


/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CREPostProcess::mfDraw(CShader *ef, SShaderPass *sfm)
{      
  return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////

