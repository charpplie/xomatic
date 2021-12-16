/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: ColorBleeding.cpp,v 1.0 2008/08/4 12:14:13 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  Routine for rendering and managing of global illumination 
							approach based on reflective shadow maps and irradiance volumes
-------------------------------------------------------------------------
History:
- 4:8:2008   12:14 : Created by Anton Kaplanyan
*************************************************************************/

#include "StdAfx.h"
#include "ColorBleeding.h"
#include "LightEntity.h"
#include "IrradianceVolumeRenderNode.h"

#define DEG2RAD_R( a ) float( (f64)(a) * (g_PI/180.0) )
#define RAD2DEG_R( a ) float( (f64)(a) * (180.0/g_PI) )

#define fSunDistance (7000.)

CColorBleeding::CColorBleeding() : m_bEnabled(false), m_fMaxDistance(0.f), m_fCascadesRatio(0.f), m_fOffset(0.f), m_GlossyReflections(false), m_SecondaryOcclusion(false), m_nUpdateFrameId(-1)
{
}

void CColorBleeding::Update()
{
	// update local vars from console ones
	Cry3DEngineBase::GetCVars()->e_GIOffset = clamp_tpl(Cry3DEngineBase::GetCVars()->e_GIOffset, 0.f, 1.f);
	Cry3DEngineBase::GetCVars()->e_GIBlendRatio = clamp_tpl(Cry3DEngineBase::GetCVars()->e_GIBlendRatio, 0.1f, 2.f);
	Cry3DEngineBase::GetCVars()->e_GIPropagationAmp = clamp_tpl(Cry3DEngineBase::GetCVars()->e_GIPropagationAmp, 1.f, 5.f);
	Cry3DEngineBase::GetCVars()->e_GICache = clamp_tpl(Cry3DEngineBase::GetCVars()->e_GICache, 0, 30);
	Cry3DEngineBase::GetCVars()->e_GINumCascades = clamp_tpl(Cry3DEngineBase::GetCVars()->e_GINumCascades, 1, 6);
	Cry3DEngineBase::GetCVars()->e_GIMaxDistance = max(Cry3DEngineBase::GetCVars()->e_GIMaxDistance, 10.f);
	Cry3DEngineBase::GetCVars()->e_GIIterations = min((int32)CIrradianceVolumeRenderNode::nMaxGridSize, max(Cry3DEngineBase::GetCVars()->e_GIIterations, 0));
	const bool bGIEnabled = Cry3DEngineBase::GetCVars()->e_GI != 0;
	const bool bGlossyReflections = Cry3DEngineBase::GetCVars()->e_GIGlossyReflections != 0;
	const bool bSecondaryOcclusion = Cry3DEngineBase::GetCVars()->e_GISecondaryOcclusion != 0;

	if(bGIEnabled == m_bEnabled && bGlossyReflections == m_GlossyReflections &&
			bSecondaryOcclusion == m_SecondaryOcclusion &&
			Cry3DEngineBase::GetCVars()->e_GINumCascades == m_Cascades.size() &&
			Cry3DEngineBase::GetCVars()->e_GIMaxDistance == m_fMaxDistance &&
			Cry3DEngineBase::GetCVars()->e_GICascadesRatio == m_fCascadesRatio && 
			Cry3DEngineBase::GetCVars()->e_GIOffset == m_fOffset)
		return;

	m_fMaxDistance = Cry3DEngineBase::GetCVars()->e_GIMaxDistance;
	m_fCascadesRatio = Cry3DEngineBase::GetCVars()->e_GICascadesRatio;
	m_fOffset = Cry3DEngineBase::GetCVars()->e_GIOffset;
	m_GlossyReflections = bGlossyReflections;
	m_SecondaryOcclusion = bSecondaryOcclusion;
	Cleanup();

	if(bGIEnabled)
	{
		float fGICascadeSize = Cry3DEngineBase::GetCVars()->e_GIMaxDistance;

		// create cascades
		for(int i=0;i<Cry3DEngineBase::GetCVars()->e_GINumCascades;++i)
		{
			CLightPropagationVolumePtr pCascade(new CLightPropagationVolume());
			m_Cascades.push_back(pCascade);
			pCascade->Init(fGICascadeSize, Cry3DEngineBase::GetCVars()->e_GIOffset);
			fGICascadeSize /= Cry3DEngineBase::GetCVars()->e_GICascadesRatio;
		}

		m_bEnabled = true;
	}
}

void CColorBleeding::UpdatePosition( const CCamera& camera )
{
	Update();

	if(!m_bEnabled)
		return;

	const bool bActive = Cry3DEngineBase::GetCVars()->e_GIAmount * gEnv->p3DEngine->GetGIAmount() > .001f;

	const int nNextFrameID = Cry3DEngineBase::GetMainFrameID() + 1;

	const int nRSMCacheFrames = Cry3DEngineBase::GetCVars()->e_GICache;
	bool bUpdate = true;
	if(nRSMCacheFrames > 0 && m_nUpdateFrameId > 0)
		bUpdate = (nNextFrameID % nRSMCacheFrames) == 0;

	for(size_t i = 0;i < m_Cascades.size();++i)
	{
		CLightPropagationVolumePtr& pCascade = m_Cascades[i];

		// Preupdate event
		pCascade->GeneralUpdate(bActive);

		// cache for RSM
		if (bUpdate)
		{
			// update irradiance volume
			pCascade->UpdateIrradianceVolume(camera, bActive);
			// update shadow frustum position
			pCascade->UpdateRSMFrustrum(camera, bActive);
			m_nUpdateFrameId = nNextFrameID;
		}

		// Postupdate event
		CLightPropagationVolume* pNestedCascade = NULL;

		if(i < m_Cascades.size() - 1)
		{
			pNestedCascade = m_Cascades[i + 1];
		}

		pCascade->PostUpdate(bActive, bUpdate, pNestedCascade);
	}
}

CColorBleeding::~CColorBleeding()
{
	Cleanup();
}

void CColorBleeding::Cleanup()
{
	m_bEnabled = false;
	m_nUpdateFrameId = -1;
	m_Cascades.clear();
}
///////////////////////////////////////////////////////////////////////////
CLightPropagationVolume::CLightPropagationVolume() : CMultiThreadRefCount()
{
	m_pIrradianceVolume = NULL;
	m_pColoredShadowMap = NULL;
	m_vcGridPos = Vec3(0, 0, 0);
	m_fDistance = 20;
	m_fOffsetDistance = 10;
	m_bIsActive = false;
	m_bGenerateRSM = false;
}

CLightPropagationVolume::~CLightPropagationVolume()
{
	SAFE_DELETE(m_pIrradianceVolume);
	if(m_pColoredShadowMap)
	{
		m_pColoredShadowMap->Release(true);
		m_pColoredShadowMap = NULL;
	}
}

void CLightPropagationVolume::Init(float fRadius, float fOffset)
{
	// update local vars from console ones
	m_fDistance = fRadius;
	m_fOffsetDistance = m_fDistance * fOffset;

	const bool bGlossyReflections = Cry3DEngineBase::GetCVars()->e_GIGlossyReflections != 0;
	const bool bSecondaryOcclusion = Cry3DEngineBase::GetCVars()->e_GISecondaryOcclusion != 0;

	const Vec3 camPos = Cry3DEngineBase::Get3DEngine()->GetCamera().GetPosition() + m_fOffsetDistance * Cry3DEngineBase::Get3DEngine()->GetCamera().GetViewdir();

	// create RSM light
	{
		m_pColoredShadowMap = (CLightEntity*)Cry3DEngineBase::Get3DEngine()->CreateLightSource();
		// set up default light properties
		CDLight DynLight;
		DynLight.m_Origin.Set(0,0,0);
		DynLight.m_Flags = DLF_LIGHTSOURCE | DLF_PROJECT | DLF_CASTSHADOW_MAPS | DLF_REFLECTIVE_SHADOWMAP;
		DynLight.m_sName = "ColoredShadowMap";
		DynLight.m_Color = Col_White;
		DynLight.m_AreaLightType = DLAT_POINT;
		DynLight.m_Origin = camPos + Cry3DEngineBase::Get3DEngine()->GetSunDirNormalized() * fSunDistance;
		const float fMaxBoxDistance = m_fDistance * sqrtf(3.f);
		DynLight.m_fRadius = (float)fSunDistance + fMaxBoxDistance;
		//DynLight.m_fBaseRadius = fRadius;	// this variable will be used for lod selection during shaodow map gen
		DynLight.m_fLightFrustumAngle = RAD2DEG_R(atan2(f64(m_fDistance) * .25, fSunDistance));
		DynLight.m_fProjectorNearPlane = max(1.f, (float)fSunDistance - fMaxBoxDistance - 1000.f);		// -1000 means we have occludes even 1km before
		m_pColoredShadowMap->SetRndFlags(ERF_OUTDOORONLY, false);
		m_pColoredShadowMap->SetRndFlags(ERF_REGISTER_BY_POSITION, true);
		m_pColoredShadowMap->SetLightProperties(DynLight);
		m_pColoredShadowMap->m_fWSMaxViewDist = 100000.f;
	}

	// create irradiance volume
	{
		m_pIrradianceVolume = (CIrradianceVolumeRenderNode*)Cry3DEngineBase::Get3DEngine()->CreateRenderNode(eERType_IrradianceVolume);
		m_pIrradianceVolume->EnableSpecular(bGlossyReflections);
		m_pIrradianceVolume->SetRndFlags(ERF_RENDER_ALWAYS, true);
		if(m_pIrradianceVolume->m_pRE)
			m_pIrradianceVolume->m_pRE->SetNewFlags(CREIrradianceVolume::efGIVolume);
		else
			assert(0);
		if(bSecondaryOcclusion)
		{
			m_pIrradianceVolume->m_pRE->SetNewFlags(m_pIrradianceVolume->m_pRE->GetFlags() | CREIrradianceVolume::efHasOcclusion);
		}
		m_pIrradianceVolume->SetDensity(CIrradianceVolumeRenderNode::nMaxGridSize / m_fDistance);
		m_pIrradianceVolume->SetMatrix(Matrix34::CreateScale(Vec3(m_fDistance)));
		m_pIrradianceVolume->m_pRE->AttachLightSource(m_pColoredShadowMap);
	}
}

void CLightPropagationVolume::UpdateIrradianceVolume( const CCamera& camera, const bool bActive )
{
	const Vec3 camPos = camera.GetPosition() + m_fOffsetDistance * camera.GetViewdir();

	assert(m_pIrradianceVolume);
	// snap 3D grid pos
	{
		Vec3 gridCellSize = m_pIrradianceVolume->GetBBox().GetSize();
		gridCellSize.x /= m_pIrradianceVolume->m_gridDimensions.x;
		gridCellSize.y /= m_pIrradianceVolume->m_gridDimensions.y;
		gridCellSize.z /= m_pIrradianceVolume->m_gridDimensions.z;
		const Vec3 deltaPos = m_vcGridPos - camPos;
		if(fabsf(deltaPos.x) > gridCellSize.x || fabsf(deltaPos.y) > gridCellSize.y || fabsf(deltaPos.z) > gridCellSize.z)
		{
			m_vcGridPos = camPos;
			m_vcGridPos.x -= fmodf(camPos.x, gridCellSize.x);
			m_vcGridPos.y -= fmodf(camPos.y, gridCellSize.y);
			m_vcGridPos.z -= fmodf(camPos.z, gridCellSize.z);
		}
	}

	const Vec3 gridCenter = m_pIrradianceVolume->m_matInv.TransformVector(Vec3(.5f, .5f, .5f));
	m_pIrradianceVolume->m_matInv.SetTranslation(m_vcGridPos - gridCenter);
	Matrix34 mxNew = m_pIrradianceVolume->m_matInv.GetInverted();
	mxNew.m03 -= .5f;
	mxNew.m13 -= .5f;
	mxNew.m23 -= .5f;
	mxNew.Invert();
	m_pIrradianceVolume->UpdateMetrics(mxNew, true);
}

void CLightPropagationVolume::UpdateRSMFrustrum( const CCamera& camera, const bool bActive )
{
	assert(m_pColoredShadowMap);

	const Vec3 camPos = camera.GetPosition() + m_fOffsetDistance * camera.GetViewdir();
	m_pColoredShadowMap->m_light.m_Origin = camPos + Cry3DEngineBase::Get3DEngine()->GetSunDirNormalized() * fSunDistance;

	const Matrix33 mxSunRot = Matrix33::CreateRotationVDir(-Cry3DEngineBase::Get3DEngine()->GetSunDirNormalized());

	// align view matrix within shadow map texel size
	Vec3 vNewLightOrigin = m_pColoredShadowMap->m_light.m_Origin;
	ShadowMapFrustum* pFrust = m_pColoredShadowMap->GetShadowFrustum(0);
	if(pFrust) // otherwise we're still not initialized
	{
		assert(pFrust->nTexSize>0);

		// calculate shadow map pixel size (minimum snapping step)
		const float fKatetSize = float(fSunDistance * tan(DEG2RAD_R(m_pColoredShadowMap->m_light.m_fLightFrustumAngle)));
		const int nFinalRSMSize = min((uint32)CREIrradianceVolume::espMaxInjectRSMSize, (uint32)pFrust->nTexSize);
		float fSnapXY = max(0.00001f, fKatetSize * 2.f / nFinalRSMSize); //texture size should be valid already

		// get rot matrix and transform to projector space (rotate towards z and flip)
		Matrix33 mxNewRot = mxSunRot;
		const Vec3 xaxis = mxNewRot.GetColumn0();
		const Vec3 yaxis = -mxNewRot.GetColumn1();
		const Vec3 zaxis = mxNewRot.GetColumn2();
		mxNewRot.SetRow(0, -xaxis);
		mxNewRot.SetRow(1, -zaxis);
		mxNewRot.SetRow(2, -yaxis);

		// transform (rotate) frustum origin into its own space
		assert(mxNewRot.IsOrthonormal());
		Vec3 vViewSpaceOrigin = mxNewRot.TransformVector(m_pColoredShadowMap->m_light.m_Origin);
		// snap it to shadow map pixel
		vViewSpaceOrigin.x -= fmodf(vViewSpaceOrigin.x, fSnapXY);
		vViewSpaceOrigin.y -= fmodf(vViewSpaceOrigin.y, fSnapXY);
		// transform it back to world space
		mxNewRot.Transpose();
		vNewLightOrigin = mxNewRot.TransformVector(vViewSpaceOrigin);
	}

	// set up new snapped matrix into light 
	Matrix34 mxNewLight = Matrix34( mxSunRot, vNewLightOrigin ); 
	const Vec3 diry = mxNewLight.GetColumn(0);
	const Vec3 dirx = mxNewLight.GetColumn(1);
	mxNewLight.SetColumn(0, dirx);
	mxNewLight.SetColumn(1, diry);
	m_pColoredShadowMap->m_light.SetMatrix(mxNewLight);
	m_pColoredShadowMap->SetMatrix(mxNewLight);
	m_pColoredShadowMap->m_light.m_Origin = vNewLightOrigin;

	// update bbox and objects tree
	const float fBoxRadius = m_fDistance * sqrtf(3.f);
	m_pColoredShadowMap->SetBBox(AABB(camPos - Vec3(fBoxRadius,fBoxRadius,fBoxRadius),
		camPos + Vec3(fBoxRadius,fBoxRadius,fBoxRadius)));
}

void CLightPropagationVolume::GeneralUpdate(const bool bActive)
{

}

void CLightPropagationVolume::PostUpdate( const bool bActive, const bool bGenerateRSM, const CLightPropagationVolume* pNestedVolume )
{
	// post-update render parameters
	Matrix34 mxOld;
	m_pIrradianceVolume->GetMatrix(mxOld);
	m_pIrradianceVolume->UpdateMetrics(mxOld, false);
	if(m_pIrradianceVolume->m_pRE)
	{
		m_pIrradianceVolume->m_pRE->m_fDistance = m_fDistance;
		m_pIrradianceVolume->m_pRE->m_fAmount = Cry3DEngineBase::GetCVars()->e_GIAmount;
		m_pIrradianceVolume->m_pRE->Invalidate();
	}

	// Blending between cascades
	if(pNestedVolume && m_pIrradianceVolume->m_pRE)
	{
		CDLight dummyLight;
		dummyLight.m_Flags |= (DLF_DEFERRED_LIGHT | DLF_IRRAD_VOLUMES | DLF_NEGATIVE);
		dummyLight.m_Origin = pNestedVolume->m_vcGridPos;
		dummyLight.m_fRadius = pNestedVolume->m_fDistance * Cry3DEngineBase::GetCVars()->e_GIBlendRatio;
		dummyLight.m_Color = ColorF(100.f, 100.f, 100.f, 0.f) * Cry3DEngineBase::GetCVars()->e_GIAmount;
		m_pIrradianceVolume->m_pRE->InsertLight(dummyLight);
	}

	// add/remove RSM frustum
	if(bGenerateRSM != m_bGenerateRSM)
	{
		if(bGenerateRSM)
		{
			m_pColoredShadowMap->m_light.m_Flags |= DLF_CASTSHADOW_MAPS;
			Cry3DEngineBase::Get3DEngine()->RegisterEntity(m_pColoredShadowMap);
		}
		else
		{
			m_pColoredShadowMap->m_light.m_Flags &= ~DLF_CASTSHADOW_MAPS;
			Cry3DEngineBase::Get3DEngine()->UnRegisterEntity(m_pColoredShadowMap);
		}
		m_bGenerateRSM = bGenerateRSM;
	}

	// add/remove entities
	if(bActive != m_bIsActive)
	{
		if(bActive)
			Cry3DEngineBase::Get3DEngine()->RegisterEntity( m_pIrradianceVolume );
		else
			Cry3DEngineBase::Get3DEngine()->UnRegisterEntity( m_pIrradianceVolume );
		m_bIsActive = bActive;
	}
}