/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: D3DIrradianceVolume.cpp,v 1.0 2008/05/19 12:14:13 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  Routine for rendering and managing of irradiance volumetric data
-------------------------------------------------------------------------
History:
- 19:5:2008   12:14 : Created by Anton Kaplanyan
*************************************************************************/

#include "StdAfx.h"
#include "I3DEngine.h"
#include <IEntityRenderState.h>
#include "DriverD3D.h"
#include "D3DDeferredShading.h"
#include "D3DIrradianceVolume.h"
#include "D3DPostProcess.h"
#include "CREIrradianceVolume.h"
#include "D3DPostProcess.h"

//#ifdef _DEBUG
//#	define CHK(x) assert(x)
//#else
#	define CHK(x) x
//#endif





#define IV_MAX_GRID_SIZE 32

#define IV_SHADER_NAME "IrradianceVolume"

// injection flags
#define RT_USE_R2VB g_HWSR_MaskBit[HWSR_SAMPLE0]

// apply flags
#define RT_APPLY_SPECULAR				g_HWSR_MaskBit[HWSR_SAMPLE0]
#define RT_STICKED_TO_CAMERA		g_HWSR_MaskBit[HWSR_SAMPLE1]
#define RT_USE_VOLUME_TEXTURE		g_HWSR_MaskBit[HWSR_SAMPLE2]
#define RT_USE_OCCLUSION				g_HWSR_MaskBit[HWSR_SAMPLE3]

// postinjection flags
#define RT_NEGATIVE_LIGHT g_HWSR_MaskBit[HWSR_SAMPLE0]

// debug apply flags
#define RT_DEBUG_VISUALIZATION g_HWSR_MaskBit[HWSR_DEBUG1]
#define RT_DEBUG_DIRECTIONS g_HWSR_MaskBit[HWSR_DEBUG2]

namespace
{
	static const ColorF colorBlack(0, 0, 0, 0);
	static const ColorF colorMiddleGrey(0.5f, 0.5f, 0.5f, 0.5f);
}
#	define RSM_COLOR_FMT			eTF_A8R8G8B8
#	define RSM_NORMALS_FMT		eTF_A8R8G8B8

// define platform-dependent RT formats
























#	define RSM_VOLUME_FMT				eTF_A16B16G16R16F
#	define RSM_HDR_VOLUME_FMT		eTF_A16B16G16R16F
#	define RSM_POOL_FMT					eTF_A16B16G16R16F
#	define RSM_OCCL_VOLUME_FMT	eTF_A16B16G16R16F
#	define RSM_COLOR_FMT				eTF_A8R8G8B8
#	define RSM_DEPTH_FMT				eTF_R32F
#	define RSM_NORMALS_FMT			eTF_A8R8G8B8
#	define irrVolumeColorZero		colorBlack



#define m_RenderSettings m_Settings[gcpRendD3D->m_RP.m_nProcessThreadID]

using namespace NPostEffects;

CIrradianceVolumesManager CIrradianceVolumesManager::s_pInstance;

CIrradianceVolumesManager::CIrradianceVolumesManager() : m_enabled(false)
{
	m_lightInjectionPointsListVB = NULL;
	m_simulateVB = NULL;
	m_postinjectVB = NULL;
	m_applyVB = NULL;
	m_R2VBVertexDeclaration = NULL;





	m_nNextFreeId = -1;

	// shader names
	m_techSolveTechName = "IVSimulate";
	m_techCollectTechName = "IVCollect";
	m_semGridMatrixSemantic = "g_gridMatrix";
	m_semInvGridMatrixSemantic = "g_invGridMatrix";
	m_semGridSizeSemantic = "g_gridSize";
	m_semInvGridSizeSemantic = "g_InvGridSize";
	m_semLightPositionSemantic = "g_lightPosition";
	m_semLightColorSemantic = "g_lightColor";
	m_techApplyTechName = "IVApply";
	m_semCameraMatrix = "g_mCamera";
	m_techInjectLight = "IVInjectLight";
	m_techInjectColorMap = "IVInjectColorMap";
	m_techPostinjectLight = "IVPostinjectLight";

	m_semCameraFrustrumLB = "g_vFrustrumLB";
	m_semCameraFrustrumLT = "g_vFrustrumLT";
	m_semCameraFrustrumRB = "g_vFrustrumRB";
	m_semCameraFrustrumRT = "g_vFrustrumRT";
}

CIrradianceVolumesManager::~CIrradianceVolumesManager()
{
	Cleanup();
}

bool CIrradianceVolumesManager::IsRenderable()
{
	if(!m_grids.empty())
		Toggle(CRenderer::CV_r_irradiancevolumes != 0);
	else
		Toggle(false);

	bool bIsRenderable = false;
	IrradianceVolumeSet::const_iterator itEnd = m_grids.end();
	for(IrradianceVolumeSet::iterator it = m_grids.begin();it != itEnd;++it)
	{
		if((*it)->IsRenderable())
		{
			bIsRenderable = true;
			break;
		}
	}

	return bIsRenderable;
}

void CIrradianceVolumesManager::RenderGI( CTexture* pDepthRT, CTexture* pNormalRT )
{
	if(!m_enabled)
		return;

	if(m_pGIIrradianceVolumes.empty())
		return;

	PROFILE_LABEL_PUSH( "GI_RENDER" );

	// render GI volume



	IrradianceVolumeSet::const_iterator itEnd = m_pGIIrradianceVolumes.end();
	for(IrradianceVolumeSet::iterator it = m_pGIIrradianceVolumes.begin();it != itEnd;++it)
		(*it)->DeferredApply(pDepthRT, pNormalRT);



	PROFILE_LABEL_POP( "GI_RENDER" );
}

void CIrradianceVolumesManager::Render( CTexture* pDepthRT, CTexture* pNormalRT )
{
	if(!m_enabled)
		return;

	if(m_grids.empty())
		return;

	PROFILE_LABEL_PUSH( "IRRADIANCE_VOLUMES" );

	// evaluate all grids
	IrradianceVolumeSet::const_iterator itEnd = m_grids.end();
	for(IrradianceVolumeSet::iterator it = m_grids.begin();it != itEnd;++it)
	{
		// skip rendering of GI volume
		if((*it)->GetFlags() & CREIrradianceVolume::efGIVolume)
			continue;

		(*it)->DeferredApply(pDepthRT, pNormalRT);
	}

	PROFILE_LABEL_POP( "IRRADIANCE_VOLUMES" );
}


void CIrradianceVolumesManager::Toggle( const bool enable )
{
	if(m_enabled == enable)
		return;

	m_enabled = enable;

	Cleanup();

	if(m_enabled)
	{
		static const SVF_P3F_T3F vbLightInjectionPoints[] = {	// octants
			{ Vec3(1, 1, 1), Vec3(0, 0, 0) }, { Vec3(1, 1, 0), Vec3(0, 0, 0) }, { Vec3(1, 0, 1), Vec3(0, 0, 0) }, { Vec3(1, 0, 0), Vec3(0, 0, 0) },
			{ Vec3(0, 1, 1), Vec3(0, 0, 0) }, { Vec3(0, 1, 0), Vec3(0, 0, 0) }, { Vec3(0, 0, 1), Vec3(0, 0, 0) }, { Vec3(0, 0, 0), Vec3(0, 0, 0) } };

		// create VB for lights injection/application
		m_lightInjectionPointsListVB = new CVertexBuffer((void*)vbLightInjectionPoints, eVF_P3F_T3F, 8);

		// create VB for simulation
		m_simulateVBPointer.resize(IV_MAX_GRID_SIZE*6);
		for(uint32 iQuad = 0;iQuad < IV_MAX_GRID_SIZE;++iQuad)
		{
			m_simulateVBPointer[iQuad*6+0].p = Vec3( 1, 1, (float)iQuad);
			m_simulateVBPointer[iQuad*6+1].p = Vec3( 1, 0, (float)iQuad);
			m_simulateVBPointer[iQuad*6+2].p = Vec3( 0, 1, (float)iQuad);

			m_simulateVBPointer[iQuad*6+3].p = Vec3( 0, 1, (float)iQuad);
			m_simulateVBPointer[iQuad*6+4].p = Vec3( 1, 0, (float)iQuad);
			m_simulateVBPointer[iQuad*6+5].p = Vec3( 0, 0, (float)iQuad);
		}
		m_simulateVB = new CVertexBuffer(&m_simulateVBPointer.front(), eVF_P3F_T3F, IV_MAX_GRID_SIZE*6);

		// create VB for post injected lights
		m_postinjectVBPointer.resize(IV_MAX_GRID_SIZE*6);
		for(int iQuad = 0;iQuad < IV_MAX_GRID_SIZE;++iQuad)
		{
			m_postinjectVBPointer[iQuad*6+0].p = Vec3(  1, 1, (float)iQuad);
			m_postinjectVBPointer[iQuad*6+1].p = Vec3(  1,-1, (float)iQuad);
			m_postinjectVBPointer[iQuad*6+2].p = Vec3( -1, 1, (float)iQuad);
			
			m_postinjectVBPointer[iQuad*6+3].p = Vec3(  1,-1, (float)iQuad);
			m_postinjectVBPointer[iQuad*6+4].p = Vec3( -1, 1, (float)iQuad);
			m_postinjectVBPointer[iQuad*6+5].p = Vec3( -1,-1, (float)iQuad);
		}
		m_postinjectVB = new CVertexBuffer();
		m_postinjectVB->m_eVF = eVF_P3F_T3F;
    m_postinjectVB = gcpRendD3D->m_DevBufMan.CreateVBuffer(m_postinjectVBPointer.size() * sizeof(m_postinjectVBPointer.front()), eVF_P3F_T3F, "IrradiancePostinject");
    gcpRendD3D->m_DevBufMan.UpdateVBuffer(m_postinjectVB, &m_postinjectVBPointer.front(), m_postinjectVBPointer.size());

		static const SVF_P3F_T3F vbApplyCube[] = {
			// front & back
			{ Vec3(1,0,0), Vec3(0, 0, 0) }, { Vec3(0,0,0), Vec3(0, 0, 0) }, { Vec3(0,0,1), Vec3(0, 0, 0) },
			{ Vec3(0,0,1), Vec3(0, 0, 0) }, { Vec3(1,0,1), Vec3(0, 0, 0) }, { Vec3(1,0,0), Vec3(0, 0, 0) },
			{ Vec3(1,1,0), Vec3(0, 0, 0) }, { Vec3(0,1,1), Vec3(0, 0, 0) }, { Vec3(0,1,0), Vec3(0, 0, 0) },
			{ Vec3(1,1,0), Vec3(0, 0, 0) }, { Vec3(1,1,1), Vec3(0, 0, 0) }, { Vec3(0,1,1), Vec3(0, 0, 0) },
			// left & right
			{ Vec3(0,0,0), Vec3(0, 0, 0) }, { Vec3(0,1,1), Vec3(0, 0, 0) }, { Vec3(0,0,1), Vec3(0, 0, 0) },
			{ Vec3(0,0,0), Vec3(0, 0, 0) }, { Vec3(0,1,0), Vec3(0, 0, 0) }, { Vec3(0,1,1), Vec3(0, 0, 0) },
			{ Vec3(1,0,0), Vec3(0, 0, 0) }, { Vec3(1,0,1), Vec3(0, 0, 0) }, { Vec3(1,1,1), Vec3(0, 0, 0) },
			{ Vec3(1,0,0), Vec3(0, 0, 0) }, { Vec3(1,1,1), Vec3(0, 0, 0) }, { Vec3(1,1,0), Vec3(0, 0, 0) },
			// top & bottom
			{ Vec3(1,0,1), Vec3(0, 0, 0) }, { Vec3(0,0,1), Vec3(0, 0, 0) }, { Vec3(0,1,1), Vec3(0, 0, 0) },
			{ Vec3(0,1,1), Vec3(0, 0, 0) }, { Vec3(1,1,1), Vec3(0, 0, 0) }, { Vec3(1,0,1), Vec3(0, 0, 0) },
			{ Vec3(1,0,0), Vec3(0, 0, 0) }, { Vec3(0,1,0), Vec3(0, 0, 0) }, { Vec3(0,0,0), Vec3(0, 0, 0) },
			{ Vec3(1,0,0), Vec3(0, 0, 0) }, { Vec3(1,1,0), Vec3(0, 0, 0) }, { Vec3(0,1,0), Vec3(0, 0, 0) }	};

		// create VB for deferred apply(unit cube)
		m_applyVB = new CVertexBuffer((void*)vbApplyCube, eVF_P3F_T3F, sizeof(vbApplyCube)/sizeof(SVF_P3F_T3F));

		// create VB for color shadow maps rendering(array of points)
		std::vector<SVF_P3F> vecRSMData;
		vecRSMData.resize(CREIrradianceVolume::espMaxInjectRSMSize * CREIrradianceVolume::espMaxInjectRSMSize);
		for(uint32 iVertex=0;iVertex<vecRSMData.size();++iVertex)
			vecRSMData[iVertex].xyz = Vec3((float)iVertex, cry_frand(), 0);
    m_RSMInjPointsVB = gcpRendD3D->m_DevBufMan.CreateVBuffer(vecRSMData.size(), eVF_P3F_MT, "IrradiancePoints");
    gcpRendD3D->m_DevBufMan.UpdateVBuffer(m_RSMInjPointsVB, &vecRSMData.front(), vecRSMData.size());
	}
}

void CIrradianceVolumesManager::Cleanup()
{
	// destroy VBs
	SAFE_DELETE(m_lightInjectionPointsListVB);
	SAFE_DELETE(m_simulateVB);
	SAFE_DELETE(m_postinjectVB);
	SAFE_DELETE(m_applyVB);
	SAFE_DELETE(m_RSMInjPointsVB);
	m_postinjectVBPointer.clear();
	m_simulateVBPointer.clear();

	// destroying r2vb vd
	SAFE_RELEASE(m_R2VBVertexDeclaration);





	CTexture::s_ptexIrrVolumeRTDebug = NULL;

	m_RSM.Release();
}

void CIrradianceVolumesManager::RegisterIrradianceVolume(CREIrradianceVolume* p)
{
	p->m_nId = CryInterlockedIncrement(&m_nNextFreeId);
	IrradianceVolumeSet::const_iterator it(m_grids.find(p));
	assert(it == m_grids.end() && "IIrradianceVolumeRenderNode::RegisterIrradianceVolume() -- Object already registered!");
	m_grids.insert(p);
}

void CIrradianceVolumesManager::UnregisterIrradianceVolume(CREIrradianceVolume* p)
{
	IrradianceVolumeSet::iterator it(m_grids.find(p));
	assert(it != m_grids.end() && "IIrradianceVolumeRenderNode::UnregisterIrradianceVolume() -- Object not registered or previously removed!");
	if (it != m_grids.end())
	{
		m_grids.erase(it);
	}
	if(p->GetFlags() & CREIrradianceVolume::efGIVolume)
	{
		it = m_pGIIrradianceVolumes.find(p);
		assert(it != m_pGIIrradianceVolumes.end());
		if(it != m_pGIIrradianceVolumes.end())
			m_pGIIrradianceVolumes.erase(it);
	}
}

void CIrradianceVolumesManager::UpdateReflectiveShadowmapSize(SReflectiveShadowMap& rRSM, int nWidth, int nHeight)
{
	if(!rRSM.pNormalRT || rRSM.pNormalRT->GetWidth() != nWidth || rRSM.pNormalRT->GetHeight() != nHeight)
	{
		rRSM.Release();
		uint32 nFlags = 0;

		// if we have r2vb available, use it by default





		SD3DPostEffectsUtils::CreateRenderTarget( "$RSMDepthMap", (CTexture*&)rRSM.pDepthRT, nWidth, nHeight, false, false, RSM_DEPTH_FMT, TO_IRRADVOLUME_DEPTH, nFlags);
		SD3DPostEffectsUtils::CreateRenderTarget( "$RSMNormalMap", (CTexture*&)rRSM.pNormalRT, nWidth, nHeight, false, false, RSM_NORMALS_FMT, TO_IRRADVOLUME_NORMAL, nFlags);
		SD3DPostEffectsUtils::CreateRenderTarget( "$RSMColorMap", (CTexture*&)rRSM.pColorRT, nWidth, nHeight, false, false, RSM_COLOR_FMT, TO_IRRADVOLUME_COLOR, nFlags);





	}
}

void CIrradianceVolumesManager::SetSignedAdditiveBlendingMode()
{

















}
/////////////////////////////////////////////////////////////////////////////

void CREIrradianceVolume::RenderReflectiveShadowMap( SReflectiveShadowMap& rRSM )
{
	if(!IrrVolumes.IsEnabled())
		return;

	assert(rRSM.pDepthRT && rRSM.pNormalRT && rRSM.pColorRT);
	if(!rRSM.pDepthRT || !rRSM.pNormalRT || !rRSM.pColorRT)
		return;

	if(rRSM.pDepthRT->GetWidth() <= CREIrradianceVolume::espMaxInjectRSMSize || rRSM.pDepthRT->GetHeight() <= CREIrradianceVolume::espMaxInjectRSMSize)
	{
		InjectColorMap(rRSM);
		//CTexture::s_ptexIrrVolumeRTDebug = (CTexture*)rRSM.pColorRT;
#ifndef PS3
		CTexture::s_ptexIrrVolumeRTDebug = (CTexture*)m_pRT[0];


#endif
		if(GetFlags() & efHasOcclusion)
		{
			InjectOcclusion(rRSM, false);
			//CTexture::s_ptexIrrVolumeRTDebug = (CTexture*)m_pOcclusionTexture;
		}
	}
	else	// do down-sampling here
	{
		DownsampleRSM(rRSM, m_downsampledRSM);
		InjectColorMap(m_downsampledRSM);
		CTexture::s_ptexIrrVolumeRTDebug = (CTexture*)m_downsampledRSM.pColorRT;
		if(GetFlags() & efHasOcclusion)
		{
			InjectOcclusion(rRSM, false);
			CTexture::s_ptexIrrVolumeRTDebug = (CTexture*)m_pOcclusionTexture;
		}
	}

	//static int z = 0;
	//if(z<30)
	//	CTexture::s_ptexIrrVolumeRTDebug = (CTexture*)pDepthRT;
	//else 
	//if(z<60)
	//	CTexture::s_ptexIrrVolumeRTDebug = (CTexture*)pNormalRT;
	//else if(z<90)
	//	CTexture::s_ptexIrrVolumeRTDebug = (CTexture*)pColorRT;
	//else
	//	CTexture::s_ptexIrrVolumeRTDebug = (CTexture*)m_pRT[0];
	//z = (z+1)%120;
}

void CREIrradianceVolume::DownsampleRSM( SReflectiveShadowMap& rSourceRSM, SReflectiveShadowMap& rDestRSM )
{
	// update dest size
	Vec2i size(min((const int)CREIrradianceVolume::espMaxInjectRSMSize, rSourceRSM.pDepthRT->GetWidth()), 
						min((const int)CREIrradianceVolume::espMaxInjectRSMSize, rSourceRSM.pDepthRT->GetHeight()));
	IrrVolumes.UpdateReflectiveShadowmapSize(rDestRSM, size.x, size.y);
	
	PROFILE_LABEL_PUSH( "IV_DOWNSAMPLERSM" );

	CHK(gcpRendD3D->FX_PushRenderTarget(0, (CTexture*)rDestRSM.pDepthRT, NULL));
	CHK(gcpRendD3D->FX_PushRenderTarget(1, (CTexture*)rDestRSM.pNormalRT, NULL));
	CHK(gcpRendD3D->FX_PushRenderTarget(2, (CTexture*)rDestRSM.pColorRT, NULL));
	gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &colorBlack); 

	// save original RTs
	CTexture* pOriginalColorMap = CTexture::s_ptexIrrVolumeColorMap;
	CTexture* pOriginalNormalMap = CTexture::s_ptexIrrVolumeNormalMap;
	CTexture* pOriginalDepthMap = CTexture::s_ptexIrrVolumeDepthMap;

	// set up textures
	CTexture::s_ptexIrrVolumeColorMap = (CTexture*)rSourceRSM.pColorRT;
	CTexture::s_ptexIrrVolumeNormalMap = (CTexture*)rSourceRSM.pNormalRT;
	CTexture::s_ptexIrrVolumeDepthMap = (CTexture*)rSourceRSM.pDepthRT;

	static CCryName techDownsampleRSM("IVDownsampleRSM");
	SD3DPostEffectsUtils::ShBeginPass(m_pShader, techDownsampleRSM, FEF_DONTSETSTATES);

	gcpRendD3D->EF_SetState(GS_NODEPTHTEST);
	gcpRendD3D->SetCullMode(R_CULL_NONE);

	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridMatrixSemantic, (Vec4*)m_RenderSettings.m_mat.GetData(), 4));
	CHK(m_pShader->FXSetPSFloat(IrrVolumes.m_semGridMatrixSemantic, (Vec4*)m_RenderSettings.m_mat.GetData(), 4));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridSizeSemantic, &m_RenderSettings.m_gridDimensions, 1));
	CHK(m_pShader->FXSetPSFloat(IrrVolumes.m_semGridSizeSemantic, &m_RenderSettings.m_gridDimensions, 1));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridSizeSemantic, &m_RenderSettings.m_invGridDimensions, 1));
	CHK(m_pShader->FXSetPSFloat(IrrVolumes.m_semInvGridSizeSemantic, &m_RenderSettings.m_invGridDimensions, 1));

	// set RSMs dimensions
	static CCryName semSrcRSMSize("g_vSrcRSMSize");
	Vec4 vSrcRSMSize((float)rSourceRSM.pDepthRT->GetWidth(), (float)rSourceRSM.pDepthRT->GetHeight(), 0, 0);
	vSrcRSMSize.z = 1.f / vSrcRSMSize.x;
	vSrcRSMSize.w = 1.f / vSrcRSMSize.y;
	CHK(m_pShader->FXSetVSFloat(semSrcRSMSize, &vSrcRSMSize, 1));
	CHK(m_pShader->FXSetPSFloat(semSrcRSMSize, &vSrcRSMSize, 1));
	static CCryName semDestRSMSize("g_vDestRSMSize");
	Vec4 vDestRSMSize((float)rDestRSM.pDepthRT->GetWidth(), (float)rDestRSM.pDepthRT->GetHeight(), 0, 0);
	vDestRSMSize.z = 1.f / vDestRSMSize.x;
	vDestRSMSize.w = 1.f / vDestRSMSize.y;
	CHK(m_pShader->FXSetVSFloat(semDestRSMSize, &vDestRSMSize, 1));
	CHK(m_pShader->FXSetPSFloat(semDestRSMSize, &vDestRSMSize, 1));

	// set dest/src and src/dest ratios of dimensions
	static CCryName semRSMRatio("g_vRSMRatio");
	Vec4 vRSMRatio = vSrcRSMSize / vDestRSMSize;
	CHK(m_pShader->FXSetVSFloat(semRSMRatio, &vRSMRatio, 1));
	CHK(m_pShader->FXSetPSFloat(semRSMRatio, &vRSMRatio, 1));

	Matrix44A mxInvLight = rSourceRSM.mxLightViewProj;
	mxInvLight.Invert();
	mxInvLight.Transpose();
	static CCryName semInvRSMMatrix("g_invRSMMatrix");
	CHK(m_pShader->FXSetVSFloat(semInvRSMMatrix, (Vec4*)mxInvLight.GetData(), 4));
	CHK(m_pShader->FXSetPSFloat(semInvRSMMatrix, (Vec4*)mxInvLight.GetData(), 4));

	// direction to light
	static CCryName semLightDir("g_lightDir");
	const Vec4 v0 = mxInvLight * Vec4(0, 0, 0, 1);
	const Vec4 v1 = mxInvLight * Vec4(0, 0, 1, 1);
	const Vec4 vDirToLight = Vec4((Vec3(v0.x, v0.y, v0.z)/v0.w - Vec3(v1.x, v1.y, v1.z)/v1.w).GetNormalized(), 0);
	CHK(m_pShader->FXSetPSFloat(semLightDir, &vDirToLight, 1));

	gcpRendD3D->DrawPrimitives(IrrVolumes.m_simulateVB, 6, R_PRIMV_TRIANGLES);

	SD3DPostEffectsUtils::ShEndPass();
	CHK(gcpRendD3D->FX_PopRenderTarget(2));
	CHK(gcpRendD3D->FX_PopRenderTarget(1));
	CHK(gcpRendD3D->FX_PopRenderTarget(0));

	PROFILE_LABEL_POP( "IV_DOWNSAMPLERSM" );

	// restore RTs
	CTexture::s_ptexIrrVolumeColorMap = pOriginalColorMap;
	CTexture::s_ptexIrrVolumeNormalMap = pOriginalNormalMap;
	CTexture::s_ptexIrrVolumeDepthMap = pOriginalDepthMap;

	// copy matrix
	rDestRSM.mxLightViewProj = rSourceRSM.mxLightViewProj;
}

void CREIrradianceVolume::InsertLight( const CDLight &light )
{
	if(!IrrVolumes.IsEnabled())
		return;

	const int& nThreadID = gRenDev->m_RP.m_nFillThreadID;
	const int32 nRecurseLevel = max(0, SRendItem::m_RecurseLevel[nThreadID] - 1);

	Lights* lights;
	//if()
	//{
	//	lights = &m_lightsToInject[nThreadID][nRecurseLevel];
	//	// TODO: recalculate m_nNumIterations
	//}
	//else
		lights = &m_lightsToPostinject[nThreadID][nRecurseLevel];
	lights->push_back(light);	// add light to render
}

void CREIrradianceVolume::Evaluate()
{
	m_bIsRenderable = false;

	static ICVar *pGICache = iConsole->GetCVar("e_GICache");
	assert(pGICache);

	const int nCurrentFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

	const bool bCahchedValue = ((m_nGridFlags & efGIVolume) != 0) && (pGICache->GetIVal() > 0) && 
															((nCurrentFrameID - m_nUpdateFrameID) < pGICache->GetIVal());

	if(IrrVolumes.IsEnabled())
	{
		if(!m_bIsUpToDate)
			UpdateRenderParameters();

		PROFILE_LABEL_PUSH( "IRRADIANCE_VOLUME_EVALUATE" );

		if(InjectAllLightSources())
			m_bNeedPropagate = true;

		if(m_bNeedPropagate)
		{
			Simulate();	// propagate irradiance
			m_bIsRenderable = true;
		}

		if(!bCahchedValue)
			m_bIsRenderable |= Postinject();

		PROFILE_LABEL_POP( "IRRADIANCE_VOLUME_EVALUATE" );

		// update frame ID
		if(m_bIsRenderable)
			m_nUpdateFrameID = nCurrentFrameID;

		// for RSM caching
		if(!m_bIsRenderable)
			m_bIsRenderable = bCahchedValue;

		Lights& lights = m_lightsToPostinject[gcpRendD3D->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID] - 1];
		lights.Clear();
	}
}

void CREIrradianceVolume::InjectColorMap(SReflectiveShadowMap& rCSM)
{
	if(!IrrVolumes.IsEnabled())
		return;

	if(!m_bIsUpToDate)
		UpdateRenderParameters();

	assert(m_pRT[0]);
	assert(rCSM.pDepthRT && rCSM.pNormalRT && rCSM.pColorRT);

	PROFILE_LABEL_PUSH( "IV_INJECTION" );





	// reset all flags
	const uint64 nOldFlags = gcpRendD3D->m_RP.m_FlagsShader_RT;
	gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(RT_USE_R2VB);

	CHK(gcpRendD3D->FX_PushRenderTarget(0, (CTexture*)m_pRT[0], NULL));
	CHK(gcpRendD3D->FX_PushRenderTarget(1, (CTexture*)m_pRT[1], NULL));
	CHK(gcpRendD3D->FX_PushRenderTarget(2, (CTexture*)m_pRT[2], NULL));
	gcpRendD3D->EF_SetState(GS_NODEPTHTEST| GS_BLSRC_ONE | GS_BLDST_ONE);

	if(m_bNeedClear)
	{
		gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &irrVolumeColorZero);




		m_bNeedClear = false;
	}

	// save original RTs
	CTexture* pOriginalColorMap = CTexture::s_ptexIrrVolumeColorMap;

	// set up RTs
	CTexture::s_ptexIrrVolumeColorMap = (CTexture*)rCSM.pColorRT;

	// set the R2VB flag
	if(rCSM.pDepthRT->GetFlags() & FT_USAGE_VERTEX_BUFFER)
		gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_USE_R2VB;

	SD3DPostEffectsUtils::ShBeginPass(m_pShader, IrrVolumes.m_techInjectColorMap, FEF_DONTSETSTATES);

	IrrVolumes.SetSignedAdditiveBlendingMode();

	gcpRendD3D->SetCullMode(R_CULL_NONE);

	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridMatrixSemantic, (Vec4*)m_RenderSettings.m_mat.GetData(), 4));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridSizeSemantic, &m_RenderSettings.m_gridDimensions, 1));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridSizeSemantic, &m_RenderSettings.m_invGridDimensions, 1));
	
	// calc light dir
	Matrix44A mxInvLight = rCSM.mxLightViewProj;
	mxInvLight.Invert();
	mxInvLight.Transpose();
	const Vec4 v0 = mxInvLight * Vec4(0, 0, 0, 1);
	const Vec4 v1 = mxInvLight * Vec4(0, 0, 1, 1);
	const Vec4 vDirToLight = Vec4((Vec3(v0.x, v0.y, v0.z)/v0.w - Vec3(v1.x, v1.y, v1.z)/v1.w).GetNormalized(), 1.f);
	Vec4 vDirToLightGridSpace = Vec4(m_RenderSettings.m_mat.TransformVector(Vec3(vDirToLight.x, vDirToLight.y, vDirToLight.z)).GetNormalized(), 0);
	vDirToLightGridSpace.x *= m_RenderSettings.m_invGridDimensions.x;
	vDirToLightGridSpace.y *= m_RenderSettings.m_invGridDimensions.y;
	vDirToLightGridSpace.z *= m_RenderSettings.m_invGridDimensions.z;

	// set up injection matrix
	Matrix44A mxInjectionMatrix = m_RenderSettings.m_mat * mxInvLight;
	static CCryName sInjectionMatrix("g_injectionMatrix");
	CHK(m_pShader->FXSetVSFloat(sInjectionMatrix, (Vec4*)mxInjectionMatrix.GetData(), 4));
	// set up constants
	static CCryName sLightDirGridSpace("g_dirToLightGridSpace");
	CHK(m_pShader->FXSetVSFloat(sLightDirGridSpace, &vDirToLightGridSpace, 1));
	static CCryName sLightDir("g_dirToLight");
	CHK(m_pShader->FXSetPSFloat(sLightDir, &vDirToLight, 1));
	static CCryName sSMSize("g_smSize");
	const Vec4 nSize((float)rCSM.pDepthRT->GetWidth(), (float)rCSM.pDepthRT->GetHeight(), 1.f/(float)rCSM.pDepthRT->GetWidth(), 1.f/(float)rCSM.pDepthRT->GetHeight());
	CHK(m_pShader->FXSetVSFloat(sSMSize, &nSize, 1));
	static CCryName sSMInvPixelSize("g_smInvPixelSize");

	// TODO: take into account light matrix rotation and perspective
	const float fMaxGridExtend = max(m_RenderSettings.m_gridDimensions.x, max(m_RenderSettings.m_gridDimensions.y, m_RenderSettings.m_gridDimensions.z));
	const float fTexelArea = fMaxGridExtend * fMaxGridExtend 
														/ (rCSM.pDepthRT->GetWidth() * rCSM.pDepthRT->GetHeight());
	const Vec4 nInvPixelSize(fTexelArea, 0, 0, 0);
	CHK(m_pShader->FXSetPSFloat(sSMInvPixelSize, &nInvPixelSize, 1));

	if(rCSM.pDepthRT->GetFlags() & FT_USAGE_VERTEX_BUFFER)
		_injectWithR2VB(rCSM);
	else
		_injectWithVTF(rCSM);

	SD3DPostEffectsUtils::ShEndPass();
	CHK(gcpRendD3D->FX_PopRenderTarget(2));
	CHK(gcpRendD3D->FX_PopRenderTarget(1));
	CHK(gcpRendD3D->FX_PopRenderTarget(0));

	// set up the flag to propagate
	m_bNeedPropagate = true;

	gcpRendD3D->m_RP.m_FlagsShader_RT = nOldFlags;





	// restore original RTs
	CTexture::s_ptexIrrVolumeColorMap = pOriginalColorMap;

	PROFILE_LABEL_POP( "IV_INJECTION" );
}


void CREIrradianceVolume::InjectOcclusion( SReflectiveShadowMap& rCSM, bool bCamera )
{
	if(!(GetFlags() & efHasOcclusion))
		return;
	assert(m_pOcclusionTexture);
	assert(rCSM.pDepthRT);
	PROFILE_LABEL_PUSH( "IV_OCCL_GEN" );

	// reset all flags
	const uint64 nOldFlags = gcpRendD3D->m_RP.m_FlagsShader_RT;
	gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(RT_USE_R2VB|RT_STICKED_TO_CAMERA);

	CTexture* pOcclTex = (CTexture*)m_pOcclusionTexture;

	const int nFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
	const bool bNeedClear = pOcclTex->m_nUpdateFrameID != nFrameID;

	CHK(gcpRendD3D->FX_PushRenderTarget(0, pOcclTex, NULL));

	// clear that
	if(bNeedClear)
	{
		gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &colorBlack);
	}

	// set the R2VB flag
	if(rCSM.pDepthRT->GetFlags() & FT_USAGE_VERTEX_BUFFER)
		gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_USE_R2VB;
	if(bCamera)
		gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_STICKED_TO_CAMERA;

	static CCryName techInjectOcclusionMap("IVInjectOcclusion");
	SD3DPostEffectsUtils::ShBeginPass(m_pShader, techInjectOcclusionMap, FEF_DONTSETSTATES);

	gcpRendD3D->EF_SetState( GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE );

	gcpRendD3D->SetCullMode(R_CULL_NONE);

	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridMatrixSemantic, (Vec4*)m_RenderSettings.m_mat.GetData(), 4));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridSizeSemantic, &m_RenderSettings.m_gridDimensions, 1));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridSizeSemantic, &m_RenderSettings.m_invGridDimensions, 1));

	if(bCamera)
	{
		SD3DPostEffectsUtils::UpdateFrustrumCorners();
		const Vec4 vLB(SD3DPostEffectsUtils::m_vLB, 0);
		const Vec4 vLT(SD3DPostEffectsUtils::m_vLT, 0);
		const Vec4 vRB(SD3DPostEffectsUtils::m_vRB, 0);
		const Vec4 vRT(SD3DPostEffectsUtils::m_vRT, 0);
		CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semCameraFrustrumLB, &vLB, 1));
		CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semCameraFrustrumLT, &vLT, 1));
		CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semCameraFrustrumRB, &vRB, 1));
		CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semCameraFrustrumRT, &vRT, 1));
	}

	// set up injection matrix
	Matrix44A mxInjectionMatrix;
	if(bCamera)
	{
		mxInjectionMatrix = m_RenderSettings.m_mat;
	}
	else
	{
		// calc light unprojection mx
		Matrix44A mxInvLight = rCSM.mxLightViewProj;
		mxInvLight.Invert();
		mxInvLight.Transpose();
		mxInjectionMatrix = m_RenderSettings.m_mat * mxInvLight;
	}

	static CCryName sInjectionMatrix("g_injectionMatrix");
	CHK(m_pShader->FXSetVSFloat(sInjectionMatrix, (Vec4*)mxInjectionMatrix.GetData(), 4));
	static CCryName sSMSize("g_smSize");
	const Vec4 nSize((float)rCSM.pDepthRT->GetWidth(), (float)rCSM.pDepthRT->GetHeight(), 1.f/(float)rCSM.pDepthRT->GetWidth(), 1.f/(float)rCSM.pDepthRT->GetHeight());
	CHK(m_pShader->FXSetVSFloat(sSMSize, &nSize, 1));
	static CCryName sSMInvPixelSize("g_smInvPixelSize");

	const float fMaxGridExtend = max(m_RenderSettings.m_gridDimensions.x, max(m_RenderSettings.m_gridDimensions.y, m_RenderSettings.m_gridDimensions.z));
	const float fTexelArea = fMaxGridExtend * fMaxGridExtend / (rCSM.pDepthRT->GetWidth() * rCSM.pDepthRT->GetHeight());
	const Vec4 nInvPixelSize(fTexelArea, 0, 0, 0);
	CHK(m_pShader->FXSetPSFloat(sSMInvPixelSize, &nInvPixelSize, 1));

	if(rCSM.pDepthRT->GetFlags() & FT_USAGE_VERTEX_BUFFER)
		_injectWithR2VB(rCSM);
	else
		_injectWithVTF(rCSM);

	SD3DPostEffectsUtils::ShEndPass();
	CHK(gcpRendD3D->FX_PopRenderTarget(0));

	gcpRendD3D->m_RP.m_FlagsShader_RT = nOldFlags;

	PROFILE_LABEL_POP( "IV_OCCL_GEN" );
}

void CREIrradianceVolume::_injectWithVTF( SReflectiveShadowMap& rCSM )
{
	HRESULT h;

	// set up vertex RT
	((CTexture*)rCSM.pDepthRT)->SetVertexTexture(true);
	SD3DPostEffectsUtils::SetTexture((CTexture*)rCSM.pDepthRT, 0, FILTER_POINT, TADDR_BORDER);
	((CTexture*)rCSM.pNormalRT)->SetVertexTexture(true);
	SD3DPostEffectsUtils::SetTexture((CTexture*)rCSM.pNormalRT, 1, FILTER_POINT, TADDR_BORDER);

	// set vb
	CVertexBuffer *vb = IrrVolumes.m_RSMInjPointsVB;
	h = gcpRendD3D->FX_SetVertexDeclaration(0, vb->m_eVF);
	if(SUCCEEDED(h))
	{
		int nPrimitives = rCSM.pDepthRT->GetWidth() * rCSM.pDepthRT->GetHeight();
		int nOffset = 0;
		D3DVertexBuffer* pRawVB = gcpRendD3D->m_DevBufMan.GetD3DVB(vb->m_VS.m_nDevID, &nOffset);
		assert(pRawVB);
		h = gcpRendD3D->FX_SetVStream(0, pRawVB, nOffset, CRenderMesh2::m_cSizeVF[vb->m_eVF]);
		assert(SUCCEEDED(h));

		// draw
		gcpRendD3D->FX_Commit();
	#if defined (DIRECT3D9) || defined (OPENGL)
		h = gcpRendD3D->m_pd3dDevice->DrawPrimitive(D3DPT_POINTLIST, 0, nPrimitives); 
		assert(SUCCEEDED(h));
	#elif defined (DIRECT3D10)
		gcpRendD3D->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		if (gcpRendD3D->m_pd3dDebug) assert(SUCCEEDED(gcpRendD3D->m_pd3dDebug->ValidateContext(gcpRendD3D->m_pd3dDeviceContext)));
		gcpRendD3D->m_pd3dDeviceContext->Draw(nPrimitives, 0);
	#endif
		gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nPolygons[gcpRendD3D->m_RP.m_nPassGroupDIP] += nPrimitives;
		gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nDIPs[gcpRendD3D->m_RP.m_nPassGroupDIP]++;
	}
	else
		assert(0);

	((CTexture*)rCSM.pDepthRT)->SetVertexTexture(false);
	((CTexture*)rCSM.pNormalRT)->SetVertexTexture(false);
}

void CREIrradianceVolume::_injectWithR2VB( SReflectiveShadowMap& rCSM )
{
	HRESULT h;

	if(!rCSM.pNormalRT)	// TODO: don't support occlusion injection
	{
		assert(0);
		return;
	}

	// create & set vd
#if defined (DIRECT3D9) || defined(OPENGL)
	if(!IrrVolumes.m_R2VBVertexDeclaration)
	{
		D3DVERTEXELEMENT9 VElemSH[] = {{0, 0, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, // R32F pos
																		{1, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0}, // ARGB8 normal
																		{2, 0, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0}, // R32F pos
																		D3DDECL_END()};
		h = gcpRendD3D->m_pd3dDevice->CreateVertexDeclaration(VElemSH, &IrrVolumes.m_R2VBVertexDeclaration);
		assert(SUCCEEDED(h));
	}
	gcpRendD3D->m_pd3dDevice->SetVertexDeclaration(IrrVolumes.m_R2VBVertexDeclaration);
#elif defined (DIRECT3D10) 
	if(!IrrVolumes.m_R2VBVertexDeclaration)
	{
		gcpRendD3D->FX_Commit();
		D3D11_INPUT_ELEMENT_DESC vdElems[] = {{"POSITION", 0, DXGI_FORMAT_R32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
																					{"NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
																					{"TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};
		// in case we still don't have shader we're just still compiling it
		if(!CHWShader_D3D::m_pCurInstVS || !CHWShader_D3D::m_pCurInstVS->m_pShaderData)
			return;
		h = gcpRendD3D->m_pd3dDevice->CreateInputLayout(vdElems, 3, CHWShader_D3D::m_pCurInstVS->m_pShaderData, 
																										CHWShader_D3D::m_pCurInstVS->m_nShaderByteCodeSize, &IrrVolumes.m_R2VBVertexDeclaration);
		assert(SUCCEEDED(h));
	}
	gcpRendD3D->m_pd3dDeviceContext->IASetInputLayout(IrrVolumes.m_R2VBVertexDeclaration);
#endif
	assert(IrrVolumes.m_R2VBVertexDeclaration);
	gcpRendD3D->m_pLastVDeclaration = IrrVolumes.m_R2VBVertexDeclaration;

	int nPrimitives = rCSM.pDepthRT->GetWidth() * rCSM.pDepthRT->GetHeight();

	// bind main vb
	CVertexBuffer *vb = IrrVolumes.m_RSMInjPointsVB;
	int nOffset = 0;
	void* pRawVB = gcpRendD3D->m_DevBufMan.GetD3DVB(vb->m_VS.m_nDevID, &nOffset);
	assert(pRawVB);
	h = gcpRendD3D->FX_SetVStream(0, pRawVB, nOffset, CRenderMesh2::m_cSizeVF[vb->m_eVF]);	// R32F
	assert(SUCCEEDED(h));

#if defined (DIRECT3D9) && !defined (XENON)
	// set up dummy vertex buffers
	h = gcpRendD3D->FX_SetVStream(1, pRawVB, 0, sizeof(uint32)); // ARGB8
	assert(SUCCEEDED(h));
	h = gcpRendD3D->FX_SetVStream(2, pRawVB, 0, sizeof(float)); // R32F
	assert(SUCCEEDED(h));
#endif

	// set up texture vertex buffers
	h = gcpRendD3D->FX_SetTextureAsVStream(1, (CTexture*)rCSM.pNormalRT, sizeof(uint32)); // ARGB8
	assert(SUCCEEDED(h));
	h = gcpRendD3D->FX_SetTextureAsVStream(2, (CTexture*)rCSM.pDepthRT, sizeof(float)); // R32F
	assert(SUCCEEDED(h));

	// draw
	gcpRendD3D->FX_Commit();
#if defined (DIRECT3D9) || defined (OPENGL)
	h = gcpRendD3D->m_pd3dDevice->DrawPrimitive(D3DPT_POINTLIST, 0, nPrimitives); 
	assert(SUCCEEDED(h));
#elif defined (DIRECT3D10)
	gcpRendD3D->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	if (gcpRendD3D->m_pd3dDebug) assert(SUCCEEDED(gcpRendD3D->m_pd3dDebug->ValidateContext(gcpRendD3D->m_pd3dDeviceContext)));
	gcpRendD3D->m_pd3dDeviceContext->Draw(nPrimitives, 0);
#endif
	gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nPolygons[gcpRendD3D->m_RP.m_nPassGroupDIP] += nPrimitives;
	gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nDIPs[gcpRendD3D->m_RP.m_nPassGroupDIP]++;

	// reset texture vertex buffers
	h = gcpRendD3D->FX_SetTextureAsVStream(1, NULL, 0);
	assert(SUCCEEDED(h));
	h = gcpRendD3D->FX_SetTextureAsVStream(2, NULL, 0);
	assert(SUCCEEDED(h));
}

bool CREIrradianceVolume::InjectAllLightSources()
{
	Lights& lights = m_lightsToInject[gcpRendD3D->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID] - 1];
	if(lights.empty())
		return false;

	// prepare renderer to injection
	assert(m_pRT[0]);
	CHK(gcpRendD3D->FX_PushRenderTarget(0, (CTexture*)m_pRT[0], NULL));
	CHK(gcpRendD3D->FX_PushRenderTarget(1, (CTexture*)m_pRT[1], NULL));
	CHK(gcpRendD3D->FX_PushRenderTarget(2, (CTexture*)m_pRT[2], NULL));

	if(m_bNeedClear)
	{
		gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &irrVolumeColorZero); 




		m_bNeedClear = false;
	}

	// inject all light sources
	for(uint32 iLight = 0;iLight < lights.Num();++iLight)
		InjectLight(lights[iLight]);

	// end of injection phase
	CHK(gcpRendD3D->FX_PopRenderTarget(2));
	CHK(gcpRendD3D->FX_PopRenderTarget(1));
	CHK(gcpRendD3D->FX_PopRenderTarget(0));

	lights.clear();

	return true;
}

void CREIrradianceVolume::InjectLight( const CDLight& rLight )
{
	assert(IrrVolumes.m_lightInjectionPointsListVB);

	SD3DPostEffectsUtils::ShBeginPass(m_pShader, IrrVolumes.m_techInjectLight, FEF_DONTSETSTATES);

	gcpRendD3D->EF_SetState( GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE );
	IrrVolumes.SetSignedAdditiveBlendingMode();

	gcpRendD3D->SetCullMode(R_CULL_NONE);

	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridMatrixSemantic, (Vec4*)m_RenderSettings.m_mat.GetData(), 4));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridMatrixSemantic, (Vec4*)m_RenderSettings.m_matInv.GetData(), 4));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridSizeSemantic, &m_RenderSettings.m_gridDimensions, 1));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridSizeSemantic, &m_RenderSettings.m_invGridDimensions, 1));

	Vec4 vcLightPos(rLight.m_Origin.x, rLight.m_Origin.y, rLight.m_Origin.z, 1.f);
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semLightPositionSemantic, &vcLightPos, 1));
	const Vec4 cLightCol = rLight.m_Color.toVec4();
	CHK(m_pShader->FXSetPSFloat(IrrVolumes.m_semLightColorSemantic, &cLightCol, 1));
	gRenDev->DrawPrimitives(IrrVolumes.m_lightInjectionPointsListVB, 8, R_PRIMV_POINTS);  

	SD3DPostEffectsUtils::ShEndPass();
}


void CREIrradianceVolume::_injectOcclusionFromCamera()
{
	PROFILE_LABEL_PUSH( "IV_CAMERA_OCCLUSION" );
	SDynTexture* pDownscNBuffer = new SDynTexture(CREIrradianceVolume::espMaxInjectRSMSize, CREIrradianceVolume::espMaxInjectRSMSize, CTexture::s_ptexSceneNormalsMap->GetTextureDstFormat(), eTT_2D,  FT_STATE_CLAMP, "TempDownscNBuff", 95);
	pDownscNBuffer->Update(CREIrradianceVolume::espMaxInjectRSMSize, CREIrradianceVolume::espMaxInjectRSMSize);
	SDynTexture* pDownscDBuffer = new SDynTexture(CREIrradianceVolume::espMaxInjectRSMSize, CREIrradianceVolume::espMaxInjectRSMSize, CTexture::s_ptexZTarget->GetTextureDstFormat(), eTT_2D,  FT_STATE_CLAMP, "TempDownscDBuff", 95);
	pDownscDBuffer->Update(CREIrradianceVolume::espMaxInjectRSMSize, CREIrradianceVolume::espMaxInjectRSMSize);

	if(pDownscNBuffer->m_pTexture && pDownscDBuffer->m_pTexture)
	{
		// downscale buffers with min filter
		GetUtils().StretchRect(CTexture::s_ptexSceneNormalsMap, pDownscNBuffer->m_pTexture);
		GetUtils().StretchRect(CTexture::s_ptexZTarget, pDownscDBuffer->m_pTexture);

		SReflectiveShadowMap cameraRSM;
		cameraRSM.pDepthRT = pDownscDBuffer->m_pTexture;
		cameraRSM.pNormalRT = pDownscNBuffer->m_pTexture;

		InjectOcclusion(cameraRSM, true);

		CTexture::s_ptexIrrVolumeRTDebug = (CTexture *)m_pOcclusionTexture;
	}
	else
		assert(0);	// ran out of memory?

	SAFE_DELETE(pDownscNBuffer);
	SAFE_DELETE(pDownscDBuffer);

	PROFILE_LABEL_POP( "IV_CAMERA_OCCLUSION" );
}

void CREIrradianceVolume::Simulate()
{
	m_bNeedPropagate = false;

	if(m_bHasSpecular)
		return;

	static ICVar *pGIPropagationAmp = iConsole->GetCVar("e_GIPropagationAmp");
	assert(pGIPropagationAmp);

	// inject occlusion from camera
	if(GetFlags() & efHasOcclusion)
		_injectOcclusionFromCamera();

	PROFILE_LABEL_PUSH( "IV_PROPAGATION" );





	// prepare renderer to injection
	assert(m_pRT[0]);

	gcpRendD3D->SetCullMode(R_CULL_NONE);

	uint64 nOldFlags = gcpRendD3D->m_RP.m_FlagsShader_RT;

	const Vec3 vCellLength = Vec3(m_RenderSettings.m_matInv.TransformVector(Vec3(m_RenderSettings.m_invGridDimensions.x, 0, 0)).GetLength(), 
																m_RenderSettings.m_matInv.TransformVector(Vec3(0, m_RenderSettings.m_invGridDimensions.y, 0)).GetLength(), 
																m_RenderSettings.m_matInv.TransformVector(Vec3(0, 0, m_RenderSettings.m_invGridDimensions.z)).GetLength());

	CTexture* pOriginalRTs[3] = { CTexture::s_ptexIrrVolumeRT[0], CTexture::s_ptexIrrVolumeRT[1], CTexture::s_ptexIrrVolumeRT[2] };
	CTexture* pOriginalColorMap = CTexture::s_ptexIrrVolumeColorMap;
	CTexture* pOriginalNormalMap = CTexture::s_ptexIrrVolumeNormalMap;
	CTexture* pOriginalDepthMap = CTexture::s_ptexIrrVolumeDepthMap;

	//	Solve equation in the 3D texture	
	for(int iIter = 1;iIter<m_RenderSettings.m_nNumIterations;++iIter)
	{
		CTexture** rtPrev = iIter == 1 ? ((CTexture**)m_pRT) : m_poolRT.GetTarget();
		m_poolRT.Swap();
		CTexture** rt = m_poolRT.GetTarget();

		// iterations
		CHK(gcpRendD3D->FX_PushRenderTarget(0, rt[0], NULL));
		CHK(gcpRendD3D->FX_PushRenderTarget(1, rt[1], NULL));
		CHK(gcpRendD3D->FX_PushRenderTarget(2, rt[2], NULL));
		gcpRendD3D->EF_SetState(GS_NODEPTHTEST);

		// set textures
		CTexture::s_ptexIrrVolumeRT[0] = rtPrev[0];
		CTexture::s_ptexIrrVolumeRT[1] = rtPrev[1];
		CTexture::s_ptexIrrVolumeRT[2] = rtPrev[2];

		nOldFlags = gcpRendD3D->m_RP.m_FlagsShader_RT;

		// set RT flags
		if(GetFlags() & efHasOcclusion && iIter > 1)
		{
			assert(m_pOcclusionTexture);
			gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_USE_OCCLUSION;

			// set up RTs
			CTexture::s_ptexIrrVolumeColorMap = (CTexture*)m_pOcclusionTexture;
		}

		SD3DPostEffectsUtils::ShBeginPass(m_pShader, IrrVolumes.m_techSolveTechName, FEF_DONTSETSTATES);
		CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridMatrixSemantic, (Vec4*)m_RenderSettings.m_matInv.GetData(), 4));
		CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridSizeSemantic, &m_RenderSettings.m_gridDimensions, 1));
		CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridSizeSemantic, &m_RenderSettings.m_invGridDimensions, 1));
		CHK(m_pShader->FXSetPSFloat(IrrVolumes.m_semGridSizeSemantic, &m_RenderSettings.m_gridDimensions, 1));
		CHK(m_pShader->FXSetPSFloat(IrrVolumes.m_semInvGridSizeSemantic, &m_RenderSettings.m_invGridDimensions, 1));

		// calculate dx/dy/dz
		Vec4 vPropagationAmp_Iteration(pGIPropagationAmp->GetFVal(), float(iIter), 0, 0);
		static CCryName sPropagationAmp_Iteration("g_PropagationAmp_Iteration");
		CHK(m_pShader->FXSetPSFloat(sPropagationAmp_Iteration, &vPropagationAmp_Iteration, 1));
		
		gcpRendD3D->EF_SetState(GS_NODEPTHTEST);
		gcpRendD3D->DrawPrimitives(IrrVolumes.m_simulateVB, m_RenderSettings.m_nGridDepth * 6, R_PRIMV_TRIANGLES);
		SD3DPostEffectsUtils::ShEndPass();
		CHK(gcpRendD3D->FX_PopRenderTarget(2));
		CHK(gcpRendD3D->FX_PopRenderTarget(1));
		CHK(gcpRendD3D->FX_PopRenderTarget(0));

		// collect data










		{
			CHK(gcpRendD3D->FX_PushRenderTarget(0, (CTexture*)m_pRT[0], NULL));
			CHK(gcpRendD3D->FX_PushRenderTarget(1, (CTexture*)m_pRT[1], NULL));
			CHK(gcpRendD3D->FX_PushRenderTarget(2, (CTexture*)m_pRT[2], NULL));
		}

		// set textures
		CTexture::s_ptexIrrVolumeRT[0] = rt[0];
		CTexture::s_ptexIrrVolumeRT[1] = rt[1];
		CTexture::s_ptexIrrVolumeRT[2] = rt[2];

		// we use existing texture slots for different temporary purposes: manual additive blend on X360










		gcpRendD3D->m_RP.m_FlagsShader_RT = nOldFlags;

		int nStates = GS_NODEPTHTEST;




		nStates |= GS_BLSRC_ONE | GS_BLDST_ONE;

		SD3DPostEffectsUtils::ShBeginPass(m_pShader, IrrVolumes.m_techCollectTechName, FEF_DONTSETSTATES);
		gcpRendD3D->EF_SetState(nStates);

		CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridSizeSemantic, &m_RenderSettings.m_invGridDimensions, 1));

























		gcpRendD3D->DrawPrimitives(IrrVolumes.m_simulateVB, m_RenderSettings.m_nGridDepth * 6, R_PRIMV_TRIANGLES);

		SD3DPostEffectsUtils::ShEndPass();








		// if it's the last one, resolve
		if(iIter==m_RenderSettings.m_nNumIterations-1)
		{
			ResolveToVolumeTexture();
		}

		CHK(gcpRendD3D->FX_PopRenderTarget(2));
		CHK(gcpRendD3D->FX_PopRenderTarget(1));
		CHK(gcpRendD3D->FX_PopRenderTarget(0));

		gcpRendD3D->m_RP.m_FlagsShader_RT = nOldFlags;
	}

	gcpRendD3D->m_RP.m_FlagsShader_RT = nOldFlags;





	// restore RTs
	CTexture::s_ptexIrrVolumeRT[0] = pOriginalRTs[0];
	CTexture::s_ptexIrrVolumeRT[1] = pOriginalRTs[1];
	CTexture::s_ptexIrrVolumeRT[2] = pOriginalRTs[2];
	CTexture::s_ptexIrrVolumeColorMap = pOriginalColorMap;
	CTexture::s_ptexIrrVolumeNormalMap = pOriginalNormalMap;
	CTexture::s_ptexIrrVolumeDepthMap = pOriginalDepthMap;

	PROFILE_LABEL_POP( "IV_PROPAGATION" );
}

bool CREIrradianceVolume::Postinject()
{
	Lights& lights = m_lightsToPostinject[gcpRendD3D->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID] - 1];
	if(lights.empty())
		return false;

	assert(m_pRT[0]);

	CHK(gcpRendD3D->FX_PushRenderTarget(0, (CTexture*)m_pRT[0], NULL));
	CHK(gcpRendD3D->FX_PushRenderTarget(1, (CTexture*)m_pRT[1], NULL));
	CHK(gcpRendD3D->FX_PushRenderTarget(2, (CTexture*)m_pRT[2], NULL));

	if(m_bNeedClear)
	{
		gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &irrVolumeColorZero); 




		m_bNeedClear = false;
	}

	assert(IrrVolumes.m_postinjectVB);

	gcpRendD3D->SetCullMode(R_CULL_NONE);

	// bind main vb
	CVertexBuffer *vb = IrrVolumes.m_postinjectVB;
	int nOffset = 0;
  D3DVertexBuffer *pVB = gcpRendD3D->m_DevBufMan.GetD3DVB(vb->m_VS.m_nDevID, &nOffset);
	assert(pVB);
  HRESULT h = gcpRendD3D->FX_SetVStream(0, pVB, nOffset, CRenderMesh2::m_cSizeVF[vb->m_eVF]);	// R32F
	assert(SUCCEEDED(h));

	// inject all light sources
	for(uint32 iLight = 0;iLight < lights.Num();++iLight)
	{
		int nStates = GS_NODEPTHTEST|GS_BLSRC_ONE|GS_BLDST_ONE;
		gcpRendD3D->m_RP.m_FlagsShader_RT &= ~RT_NEGATIVE_LIGHT;
		if(lights[iLight].m_Flags & DLF_NEGATIVE)
		{
			//nStates &= ~GS_BLEND_MASK;
			//nStates |= GS_BLSRC_ZERO | GS_BLDST_ONEMINUSSRCALPHA;
			gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_NEGATIVE_LIGHT;
		}
		gcpRendD3D->EF_SetState(nStates);
		SD3DPostEffectsUtils::ShBeginPass(m_pShader, IrrVolumes.m_techPostinjectLight, FEF_DONTSETSTATES);
		PostnjectLight(lights[iLight]);
		SD3DPostEffectsUtils::ShEndPass();
	}

	// if it's the last one, resolve
	ResolveToVolumeTexture();

	CHK(gcpRendD3D->FX_PopRenderTarget(2));
	CHK(gcpRendD3D->FX_PopRenderTarget(1));
	CHK(gcpRendD3D->FX_PopRenderTarget(0));

	lights.clear();

	return true;
}

void CREIrradianceVolume::PostnjectLight( const CDLight& rLight )
{
	// calc bounding in grid space to get number of slices to render
	const Vec3 vRadius = Vec3(rLight.m_fRadius, rLight.m_fRadius, rLight.m_fRadius);
	AABB lightBox(rLight.m_Origin - vRadius, rLight.m_Origin + vRadius);
	Matrix34 mx(m_RenderSettings.m_mat);
	lightBox.SetTransformedAABB(mx, lightBox);
	int nEndSlice = max(0, min(IV_MAX_GRID_SIZE, (int)ceilf(lightBox.max.z * m_RenderSettings.m_gridDimensions.z)));
	int nStartSlice = max(0, min(IV_MAX_GRID_SIZE, (int)floorf(lightBox.min.z * m_RenderSettings.m_gridDimensions.z)));
	nEndSlice = max(nEndSlice, nStartSlice);
	int nSlices = min(IV_MAX_GRID_SIZE, nEndSlice - nStartSlice + 1);

	const Vec4 vcLightPos(rLight.m_Origin.x, rLight.m_Origin.y, rLight.m_Origin.z, rLight.m_fRadius);
	Vec4 cLightCol = rLight.m_Color.toVec4();
	cLightCol.w = (float)nSlices;	// store num slices in the alpha channel of color

	// Apply LBuffers range rescale
	if(!(rLight.m_Flags & DLF_NEGATIVE))
	{
		cLightCol.x *= gcpRendD3D->m_fAdaptedSceneScaleLBuffer;
		cLightCol.y *= gcpRendD3D->m_fAdaptedSceneScaleLBuffer;
		cLightCol.z *= gcpRendD3D->m_fAdaptedSceneScaleLBuffer;
	}

	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridMatrixSemantic, (Vec4*)m_RenderSettings.m_mat.GetData(), 4));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridMatrixSemantic, (Vec4*)m_RenderSettings.m_matInv.GetData(), 4));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridSizeSemantic, &m_RenderSettings.m_gridDimensions, 1));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridSizeSemantic, &m_RenderSettings.m_invGridDimensions, 1));

	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semLightPositionSemantic, &vcLightPos, 1));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semLightColorSemantic, &cLightCol, 1));

	// draw
	gcpRendD3D->FX_Commit();
	UINT nPrimitives = nSlices*6;
#if defined (DIRECT3D9) || defined (OPENGL)
	HRESULT h = gcpRendD3D->m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, nPrimitives); 
	assert(SUCCEEDED(h));
#elif defined (DIRECT3D10)
	gcpRendD3D->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	if (gcpRendD3D->m_pd3dDebug) assert(SUCCEEDED(gcpRendD3D->m_pd3dDebug->ValidateContext(gcpRendD3D->m_pd3dDeviceContext)));
	gcpRendD3D->m_pd3dDeviceContext->Draw(nPrimitives, 0);
#endif
	gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nPolygons[gcpRendD3D->m_RP.m_nPassGroupDIP] += nPrimitives;
	gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nDIPs[gcpRendD3D->m_RP.m_nPassGroupDIP]++;
}

void CREIrradianceVolume::DeferredApply( ITexture* pDepthRT, ITexture* pNormalRT )
{
	if(!m_bIsRenderable)
		return;

	// DEBUG
	{
		//gcpRendD3D->GetIRenderAuxGeom()->DrawAABB(m_pParent->GetBBox(), false, Col_White, eBBD_Faceted);
		//float density = m_pParent->GetDensity();
		//Vec3 size = m_pParent->GetBBox().GetSize() * density;
		//Vec3 origSize = size;
		//Vec3 offset = size;
		//size.x = floor_tpl(size.x);
		//size.y = floor_tpl(size.y);
		//size.z = floor_tpl(size.z);
		//offset -= size;
		//offset *= .5f;
		//for(float i=offset.x;i<origSize.x;++i)
		//	for(float j=offset.y;j<origSize.y;++j)
		//		for(float k=offset.z;k<origSize.z;++k)
		//		{
		//			Vec3 pos = Vec3(i/origSize.x, j/origSize.y, k/origSize.z);
		//			pos = m_matInv.TransformPoint(pos);
		//			Vec3 siz = Vec3(.5f, .5f, .5f);
		//			AABB bb(pos - siz, pos + siz);
		//			gcpRendD3D->GetIRenderAuxGeom()->DrawAABB(bb, false, Col_White, eBBD_Faceted);
		//			//gcpRendD3D->GetIRenderAuxGeom()->DrawAABB(Vec3(i/origSize.x-.5f, j/origSize.y-.5f, k/origSize.z-.5f), Col_CornflowerBlue, 10);
		//		}
	}

	PROFILE_LABEL_PUSH( "IV_APPLY" );

	assert(pDepthRT && pNormalRT);

	// prepare renderer to injection
	assert(m_pRT[0]);

	const uint64 nOldFlags = gcpRendD3D->m_RP.m_FlagsShader_RT;
	gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(RT_USE_VOLUME_TEXTURE|RT_APPLY_SPECULAR|RT_STICKED_TO_CAMERA|RT_DEBUG_VISUALIZATION|RT_DEBUG_DIRECTIONS);

	const Settings& rRendSettings = m_RenderSettings;
	const Vec3 gridCenter = rRendSettings.m_matInv.TransformPoint(Vec3(.5f, .5f, .5f));

	CTexture* pOriginalRTs[3] = { CTexture::s_ptexIrrVolumeRT[0], CTexture::s_ptexIrrVolumeRT[1], CTexture::s_ptexIrrVolumeRT[2] };

	// setup textures
	CTexture::s_ptexIrrVolumeRT[0] = (CTexture*)m_pVolumeTextures[0];
	CTexture::s_ptexIrrVolumeRT[1] = (CTexture*)m_pVolumeTextures[1];
	CTexture::s_ptexIrrVolumeRT[2] = (CTexture*)m_pVolumeTextures[2];

	if(m_pVolumeTextures[0] && m_pVolumeTextures[0] != m_pRT[0])
		gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_USE_VOLUME_TEXTURE;

	if(m_nGridFlags & efGIVolume)
		gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_STICKED_TO_CAMERA;

	if(CRenderer::CV_r_irradiancevolumes > 1)
	{
		gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_DEBUG_VISUALIZATION;
		if(CRenderer::CV_r_irradiancevolumes == 3)	// view-dependent visualization
			gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_DEBUG_DIRECTIONS;
	}
	else if(m_bHasSpecular)
		gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_APPLY_SPECULAR;

	bool bUseStencilPrepass = false;
	// for now it's faster to draw it without stencil prepass
	if(0)//(m_nGridFlags & efGIVolume) != 0 && CRenderer::CV_r_deferredshadingstencilprepass && CRenderer::CV_r_irradiancevolumes == 1)
	{
		gcpRendD3D->EF_Scissor(false, 0, 0, 0, 0);
		gcpRendD3D->SetDepthBoundTest(0.0f, 1.0f, false);
		CDLight dummyDL;
		dummyDL.m_Origin = gridCenter;
		dummyDL.m_fRadius = m_fDistance * .5f;
		gcpRendD3D->FX_StencilFrustumCull(-1, &dummyDL, NULL, 0);
		gcpRendD3D->FX_StencilRefresh(STENC_FUNC(FSS_STENCFUNC_EQUAL), gcpRendD3D->m_nStencilMaskRef, 0xFFFFFFFF);
		bUseStencilPrepass = true;
	}

	SD3DPostEffectsUtils::ShBeginPass(m_pShader, IrrVolumes.m_techApplyTechName, FEF_DONTSETSTATES);

	gcpRendD3D->SetCullMode(R_CULL_BACK);

	int nStates = GS_NODEPTHTEST;





	nStates |= GS_BLSRC_ONE | GS_BLDST_ONE;	// usual additive blending for irradiance volumes


	gcpRendD3D->EF_SetState(nStates);

	if(bUseStencilPrepass)
		gcpRendD3D->FX_StencilTestCurRef(true);

	// set frustum corners
	SD3DPostEffectsUtils::UpdateFrustrumCorners();
	const Vec4 vLB(SD3DPostEffectsUtils::m_vLB, 0);
	const Vec4 vLT(SD3DPostEffectsUtils::m_vLT, 0);
	const Vec4 vRB(SD3DPostEffectsUtils::m_vRB, 0);
	const Vec4 vRT(SD3DPostEffectsUtils::m_vRT, 0);
	CHK(m_pShader->FXSetPSFloat(IrrVolumes.m_semGridMatrixSemantic, (Vec4*)rRendSettings.m_mat.GetData(), 4));
	CHK(m_pShader->FXSetPSFloat(IrrVolumes.m_semInvGridMatrixSemantic, (Vec4*)rRendSettings.m_matInv.GetData(), 4));
	CHK(m_pShader->FXSetPSFloat(IrrVolumes.m_semGridSizeSemantic, &rRendSettings.m_gridDimensions, 1));
	CHK(m_pShader->FXSetPSFloat(IrrVolumes.m_semInvGridSizeSemantic, &rRendSettings.m_invGridDimensions, 1));
	static CCryName semGridSpaceCamPos("g_gridSpaceCamPos");
	const Vec4 vGridSpaceCamPos(rRendSettings.m_mat.TransformPoint(gcpRendD3D->GetRCamera().Orig), 1.f );
	CHK(m_pShader->FXSetPSFloat(semGridSpaceCamPos, &vGridSpaceCamPos, 1));
	if(m_nGridFlags & efGIVolume)
	{
		static CCryName semAttenuation("g_vAttenuation");
		const float d = m_fDistance * .5f;
		const float offset = min(d * .5f, 20.f);
		// att(d) = kd+b;
		const float k = -1.f / offset;
		const float b = d / offset;

		float fGIAmount = m_fAmount * gEnv->p3DEngine->GetGIAmount();
		// Apply LBuffers range rescale
		fGIAmount *= gcpRendD3D->m_fAdaptedSceneScaleLBuffer;

		const Vec4 vAttenuation(k, b, fGIAmount, 0);
		CHK(m_pShader->FXSetPSFloat(semAttenuation, &vAttenuation, 1));

		static CCryName semGridCenter("g_gridCenter");
		const Vec4 vGridCenter(gridCenter, 0);
		CHK(m_pShader->FXSetPSFloat(semGridCenter, &vGridCenter, 1));
	}

	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridMatrixSemantic, (Vec4*)rRendSettings.m_matInv.GetData(), 4));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridMatrixSemantic, (Vec4*)rRendSettings.m_mat.GetData(), 4));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semCameraMatrix, (Vec4*) gcpRendD3D->m_CameraProjMatrix.GetData(), 4)); 
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semCameraFrustrumLB, &vLB, 1));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semCameraFrustrumLT, &vLT, 1));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semCameraFrustrumRB, &vRB, 1));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semCameraFrustrumRT, &vRT, 1));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semGridSizeSemantic, &rRendSettings.m_gridDimensions, 1));
	CHK(m_pShader->FXSetVSFloat(IrrVolumes.m_semInvGridSizeSemantic, &rRendSettings.m_invGridDimensions, 1));

	gcpRendD3D->DrawPrimitives(IrrVolumes.m_applyVB, 36, R_PRIMV_TRIANGLES);

	SD3DPostEffectsUtils::ShEndPass();

	//// reset stencil func
	if(bUseStencilPrepass)
		gcpRendD3D->FX_StencilTestCurRef(false);

	// set up the clear-once flag
	m_bNeedClear = true;
	// reset the renderable flag
	m_bIsRenderable = false;

	gcpRendD3D->m_RP.m_FlagsShader_RT = nOldFlags;

	// restore RTs
	CTexture::s_ptexIrrVolumeRT[0] = pOriginalRTs[0];
	CTexture::s_ptexIrrVolumeRT[1] = pOriginalRTs[1];
	CTexture::s_ptexIrrVolumeRT[2] = pOriginalRTs[2];

	PROFILE_LABEL_POP( "IV_APPLY" );
}

void CREIrradianceVolume::ResolveToVolumeTexture()
{
#ifdef USE_VOLUME_TEXTURE
	if(!m_pVolumeTextures[0] || m_pVolumeTextures[0] == m_pRT[0])
		return;




	PROFILE_LABEL_PUSH( "IV_RESOLVE_3D" );
	assert(m_RenderSettings.m_nGridWidth%32==0 && m_RenderSettings.m_nGridHeight%32==0 && m_RenderSettings.m_nGridDepth%32==0);




























	PROFILE_LABEL_POP( "IV_RESOLVE_3D" );
#endif
}

void CREIrradianceVolume::UpdateRenderParameters()
{
	// load shader
	if(m_pShader == NULL)
		m_pShader = gRenDev->m_cEF.mfForName(IV_SHADER_NAME, 0);

	// update RT if necessary
	if(!m_pRT[0] || (m_pRT[0]->GetWidth() != m_RenderSettings.m_nGridWidth*m_RenderSettings.m_nGridDepth || m_pRT[0]->GetHeight() != m_RenderSettings.m_nGridHeight))
	{
		Cleanup();
		char str[256];

		// create RT pool
		CTexture* pRT0[3] = {NULL, NULL, NULL}, *pRT1[3] = { NULL, NULL, NULL };
		SD3DPostEffectsUtils::CreateRenderTarget( "$IrradVolumeRedPool0", pRT0[0], m_RenderSettings.m_nGridWidth*m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, true, false, RSM_POOL_FMT, TO_IRRADVOLUME_R);
		SD3DPostEffectsUtils::CreateRenderTarget( "$IrradVolumeGreenPool0", pRT0[1], m_RenderSettings.m_nGridWidth*m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, true, false, RSM_POOL_FMT, TO_IRRADVOLUME_G);
		SD3DPostEffectsUtils::CreateRenderTarget( "$IrradVolumeBluePool0", pRT0[2], m_RenderSettings.m_nGridWidth*m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, true, false, RSM_POOL_FMT, TO_IRRADVOLUME_B);
		SD3DPostEffectsUtils::CreateRenderTarget( "$IrradVolumeRedPool1", pRT1[0], m_RenderSettings.m_nGridWidth*m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, true, false, RSM_POOL_FMT, TO_IRRADVOLUME_R);
		SD3DPostEffectsUtils::CreateRenderTarget( "$IrradVolumeGreenPool1", pRT1[1], m_RenderSettings.m_nGridWidth*m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, true, false, RSM_POOL_FMT, TO_IRRADVOLUME_G);
		SD3DPostEffectsUtils::CreateRenderTarget( "$IrradVolumeBluePool1", pRT1[2], m_RenderSettings.m_nGridWidth*m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, true, false, RSM_POOL_FMT, TO_IRRADVOLUME_B);

		if(pRT0[0] == NULL || pRT0[1] == NULL || pRT0[2] == NULL || 
			pRT1[0] == NULL || pRT1[1] == NULL || pRT1[2] == NULL)
		{
			assert(0);
			iLog->Log("Failed to initialize Irradiance Volume: Insufficient video memory");
			return;
		}

		pRT0[0]->SetHighQualityRT();
		pRT0[1]->SetHighQualityRT();
		pRT0[2]->SetHighQualityRT();
		pRT1[0]->SetHighQualityRT();
		pRT1[1]->SetHighQualityRT();
		pRT1[2]->SetHighQualityRT();








		pRT0[0]->Fill(irrVolumeColorZero);
		pRT0[1]->Fill(irrVolumeColorZero);
		pRT0[2]->Fill(irrVolumeColorZero);
		pRT1[0]->Fill(irrVolumeColorZero);
		pRT1[1]->Fill(irrVolumeColorZero);
		pRT1[2]->Fill(irrVolumeColorZero);

		m_poolRT.SetRTs(pRT0, pRT1);

		ETEX_Format eVolumeFmt = (m_nGridFlags & efGIVolume) ? RSM_VOLUME_FMT : RSM_HDR_VOLUME_FMT;

		sprintf_s(str, "$IrradVolumeRed%d", m_nId);
		SD3DPostEffectsUtils::CreateRenderTarget( str, (CTexture*&)m_pRT[0], m_RenderSettings.m_nGridWidth*m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, true, false, eVolumeFmt, TO_IRRADVOLUME_R);
		sprintf_s(str, "$IrradVolumeGreen%d", m_nId);
		SD3DPostEffectsUtils::CreateRenderTarget( str, (CTexture*&)m_pRT[1], m_RenderSettings.m_nGridWidth*m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, true, false, eVolumeFmt, TO_IRRADVOLUME_G);
		sprintf_s(str, "$IrradVolumeBlue%d", m_nId);
		SD3DPostEffectsUtils::CreateRenderTarget( str, (CTexture*&)m_pRT[2], m_RenderSettings.m_nGridWidth*m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, true, false, eVolumeFmt, TO_IRRADVOLUME_B);
		assert(m_pRT[0] && m_pRT[1] && m_pRT[2]);
		((CTexture*&)m_pRT[0])->SetHighQualityRT();
		((CTexture*&)m_pRT[1])->SetHighQualityRT();
		((CTexture*&)m_pRT[2])->SetHighQualityRT();
		((CTexture*&)m_pRT[0])->Fill(irrVolumeColorZero);
		((CTexture*&)m_pRT[1])->Fill(irrVolumeColorZero);
		((CTexture*&)m_pRT[2])->Fill(irrVolumeColorZero);





		m_pVolumeTextures[0] = m_pRT[0];
		m_pVolumeTextures[1] = m_pRT[1];
		m_pVolumeTextures[2] = m_pRT[2];

		// create RT for secondary occlusion
		if(GetFlags() & efHasOcclusion)
		{
			SD3DPostEffectsUtils::CreateRenderTarget( "$IrradVolumeOcclusion", (CTexture*&)m_pOcclusionTexture, m_RenderSettings.m_nGridWidth*m_RenderSettings.m_nGridDepth, m_RenderSettings.m_nGridHeight, true, false, RSM_OCCL_VOLUME_FMT, TO_IRRADVOLUME_COLOR);
			assert(m_pOcclusionTexture);
			((CTexture*&)m_pOcclusionTexture)->SetHighQualityRT();
			((CTexture*&)m_pOcclusionTexture)->Fill(colorBlack);



		}

		if(CTexture::s_ptexIrrVolumeRTDebug == NULL)
			CTexture::s_ptexIrrVolumeRTDebug = (CTexture*)m_pRT[0];

#ifdef USE_VOLUME_TEXTURE
		if(m_RenderSettings.m_nGridWidth%32==0 && m_RenderSettings.m_nGridHeight%32==0 && m_RenderSettings.m_nGridDepth%32==0)
		{
			static const uint32 flags =  FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_DONT_RESIZE | FT_NOMIPS;
			sprintf_s(str, "$IrradVolumeRed%dVol", m_nId);
			m_pVolumeTextures[0] = CTexture::Create3DTexture(str, m_RenderSettings.m_nGridWidth, m_RenderSettings.m_nGridHeight, m_RenderSettings.m_nGridDepth, 1, flags, NULL, 
				eVolumeFmt, eVolumeFmt);
			sprintf_s(str, "$IrradVolumeGreen%dVol", m_nId);
			m_pVolumeTextures[1] = CTexture::Create3DTexture(str, m_RenderSettings.m_nGridWidth, m_RenderSettings.m_nGridHeight, m_RenderSettings.m_nGridDepth, 1, flags , NULL, 
				eVolumeFmt, eVolumeFmt);
			sprintf_s(str, "$IrradVolumeBlue%dVol", m_nId);
			m_pVolumeTextures[2] = CTexture::Create3DTexture(str, m_RenderSettings.m_nGridWidth, m_RenderSettings.m_nGridHeight, m_RenderSettings.m_nGridDepth, 1, flags , NULL, 
				eVolumeFmt, eVolumeFmt);
			assert(m_pVolumeTextures[0] && m_pVolumeTextures[1] && m_pVolumeTextures[2]);
			((CTexture*&)m_pVolumeTextures[0])->SetCustomID(TO_IRRADVOLUME_R);
			((CTexture*&)m_pVolumeTextures[1])->SetCustomID(TO_IRRADVOLUME_G);
			((CTexture*&)m_pVolumeTextures[2])->SetCustomID(TO_IRRADVOLUME_B);
			((CTexture*&)m_pVolumeTextures[0])->Fill(irrVolumeColorZero);
			((CTexture*&)m_pVolumeTextures[1])->Fill(irrVolumeColorZero);
			((CTexture*&)m_pVolumeTextures[2])->Fill(irrVolumeColorZero);














































		}
#endif
	}

	if(GetFlags() & efGIVolume)
	{
		IrrVolumes.m_pGIIrradianceVolumes.insert(this);
	}

	m_bIsUpToDate = true;
}

CREIrradianceVolume::Settings* CREIrradianceVolume::GetFillSettings()
{
	ASSERT_IS_MAIN_THREAD(gcpRendD3D->m_pRT)
	return &m_Settings[gcpRendD3D->m_RP.m_nFillThreadID];
}

bool CREIrradianceVolume::mfPreDraw(SShaderPass *sl)
{
	return true;
}
