/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: ColorBleeding.h,v 1.0 2008/08/4 12:14:13 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  Routine for rendering and managing of irradiance volumetric data
-------------------------------------------------------------------------
History:
- 4:8:2008   12:14 : Created by Anton Kaplanyan
*************************************************************************/

#ifndef _COLORBLEEDING_RENDERNODE_
#define _COLORBLEEDING_RENDERNODE_

#pragma once

#include "IInput.h"

class CIrradianceVolumeRenderNode;

class CLightPropagationVolume : public CMultiThreadRefCount
{
protected:
	struct CLightEntity*						m_pColoredShadowMap;
	Vec3														m_vcGridPos;
	float														m_fDistance;
	float														m_fOffsetDistance;
	bool														m_bIsActive;
	bool														m_bGenerateRSM;
public:
	CIrradianceVolumeRenderNode*		m_pIrradianceVolume;
public:
	CLightPropagationVolume();
	~CLightPropagationVolume();
	void UpdateIrradianceVolume(const CCamera& camera, const bool bActive);
	void UpdateRSMFrustrum(const CCamera& camera, const bool bActive);
	void GeneralUpdate(const bool bActive);
	void PostUpdate(const bool bActive, const bool bGenerateRSM, const CLightPropagationVolume* pNestedVolume);
	void Init(float fRadius, float fOffset);
};
typedef _smart_ptr<CLightPropagationVolume> CLightPropagationVolumePtr;

class CColorBleeding
{
public:
	CColorBleeding();
	~CColorBleeding();

	bool IsEnabled() const { return m_bEnabled; }
	void UpdatePosition(const CCamera& camera);
	void Cleanup();
private:
	void Update();
	std::vector<CLightPropagationVolumePtr>	m_Cascades;
	float																		m_fMaxDistance;
	float																		m_fCascadesRatio;
	float																		m_fOffset;
	bool																		m_GlossyReflections;
	bool																		m_SecondaryOcclusion;
	bool																		m_bEnabled;
	int																			m_nUpdateFrameId;
};



#endif // #ifndef _COLORBLEEDING_RENDERNODE_