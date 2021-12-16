#include "StdAfx.h"
#include <CREIrradianceVolume.h>
#include "../../../CryCommon/IEntityRenderState.h"
#ifndef NULL_RENDERER
#include "../../XRenderD3D9/D3DIrradianceVolume.h"
#endif

#if defined(DIRECT3D10) || defined(XENON)
#include "../../XRenderD3D9/DriverD3D.h"
#endif

//////////////////////////////////////////////////////////////////////////
CREIrradianceVolume::CREIrradianceVolume()
: CRendElementBase()
, m_bIsRenderable(false)
, m_bIsUpToDate(false)
, m_bNeedPropagate(false)
, m_bNeedClear(true)
, m_bHasSpecular(false)
, m_pParent(NULL)
, m_nId(-1)
, m_nGridFlags(0)
, m_fDistance(100.f)
, m_fAmount(1.f)
, m_pAttachedLightSource(NULL)
, m_pShader(NULL)
, m_nUpdateFrameID(-1)
{
	mfSetType(eDATA_IrradianceVolume);
	mfUpdateFlags(FCEF_TRANSFORM);
	m_pRT[0] = NULL;
	m_pRT[1] = NULL;
	m_pRT[2] = NULL;

	m_pVolumeTextures[0] = NULL;
	m_pVolumeTextures[1] = NULL;
	m_pVolumeTextures[2] = NULL;

	m_pOcclusionTexture = NULL;





#if PS3
	for ( UINT iChannel = 0; iChannel < 3; ++iChannel )
		m_p2DVolumeUnwraps[iChannel] = NULL;
#endif

#ifndef NULL_RENDERER
	IrrVolumes.RegisterIrradianceVolume(this);
#endif
}

CREIrradianceVolume::~CREIrradianceVolume()
{
	// release shader
	SAFE_RELEASE(m_pShader);

	Cleanup();
#ifndef NULL_RENDERER
	IrrVolumes.UnregisterIrradianceVolume(this);
#endif
}

void CREIrradianceVolume::mfPrepare()
{
	gRenDev->FX_CheckOverflow(0, 0, this);
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}

float CREIrradianceVolume::mfDistanceToCameraSquared(Matrix34& matInst)
{
	if(m_nGridFlags & efGIVolume)
		return 0.f;
	else
		return (gRenDev->GetRCamera().Orig - GetFillSettings()->m_pos).GetLengthSquared();
}

void CREIrradianceVolume::Cleanup()
{
	if(CTexture::s_ptexIrrVolumeRTDebug == m_pRT[0])
		CTexture::s_ptexIrrVolumeRTDebug = NULL;









#if PS3
	for ( UINT iChannel = 0; iChannel < 3; ++iChannel )
	{
		if(m_p2DVolumeUnwraps[iChannel])
		{
			CDeviceTexture* pTex = ((CTexture*)(m_p2DVolumeUnwraps[iChannel]))->GetDevTexture();
			// unbind resource to avoid double release
			if(pTex)
			{
				pTex->Get2DTexture()->Offset(0);
				pTex->Get2DTexture()->MemItemID(0);
			}
		}
		SAFE_RELEASE(m_p2DVolumeUnwraps[iChannel]);
	}
#endif

	if(m_pVolumeTextures[0] == m_pRT[0]) m_pVolumeTextures[0] = NULL;
	if(m_pVolumeTextures[1] == m_pRT[1]) m_pVolumeTextures[1] = NULL;
	if(m_pVolumeTextures[2] == m_pRT[2]) m_pVolumeTextures[2] = NULL;
	SAFE_RELEASE(m_pRT[0]);
	SAFE_RELEASE(m_pRT[1]);
	SAFE_RELEASE(m_pRT[2]);

	SAFE_RELEASE(m_pVolumeTextures[0]);
	SAFE_RELEASE(m_pVolumeTextures[1]);
	SAFE_RELEASE(m_pVolumeTextures[2]);

	SAFE_RELEASE(m_pOcclusionTexture);

	// destroy RT pool
	m_poolRT.SetRTs(NULL, NULL);

	// destroy downsampled RSM
	m_downsampledRSM.Release();
}

void CREIrradianceVolume::EnableSpecular( const bool bEnabled )
{
	m_bHasSpecular = bEnabled;
}

void SReflectiveShadowMap::Release()
{
	SAFE_RELEASE(pDepthRT);
	SAFE_RELEASE(pNormalRT);
	SAFE_RELEASE(pColorRT);
}

void CREIrradianceVolume::PingPongRTs::SetRTs( CTexture* pRT0[3], CTexture* pRT1[3] )
{
	SAFE_RELEASE(m_pRT[0][0]);
	SAFE_RELEASE(m_pRT[0][1]);
	SAFE_RELEASE(m_pRT[0][2]);
	SAFE_RELEASE(m_pRT[1][0]);
	SAFE_RELEASE(m_pRT[1][1]);
	SAFE_RELEASE(m_pRT[1][2]);
	if(pRT0)
	{
		m_pRT[0][0] = pRT0[0];
		m_pRT[0][1] = pRT0[1];
		m_pRT[0][2] = pRT0[2];
	}
	if(pRT1)
	{
		m_pRT[1][0] = pRT1[0];
		m_pRT[1][1] = pRT1[1];
		m_pRT[1][2] = pRT1[2];
	}
}

CREIrradianceVolume::Settings::Settings()
{
	m_pos = Vec3(0, 0, 0);
	m_bbox = AABB(AABB::RESET);
	m_mat.SetIdentity();
	m_matInv.SetIdentity();
	m_nGridWidth = 0;			
	m_nGridHeight = 0;		
	m_nGridDepth = 0;			
	m_nNumIterations = 0;	
	m_gridDimensions = Vec4(0, 0, 0, 0);
	m_invGridDimensions = Vec4(0, 0, 0, 0);
}
