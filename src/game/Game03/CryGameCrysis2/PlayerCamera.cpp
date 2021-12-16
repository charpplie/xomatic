/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Controls player camera update (Refactored from PlayerView)

-------------------------------------------------------------------------
History:
- 15:10:2009   Created by Benito Gangoso Rodriguez

*************************************************************************/

#include "StdAfx.h"
#include "PlayerCamera.h"
#include "Player.h"
#include <IViewSystem.h>
#include "GameCVars.h"
#include "Utility/CryWatch.h"
#include "CameraModes.h"

#if !defined(_RELEASE)

#define PLAYER_CAMERA_WATCH(...)  do { if (g_pGameCVars->pl_debug_watch_camera_mode)       CryWatch    ("[PlayerCamera] " __VA_ARGS__); } while(0)
#define PLAYER_CAMERA_LOG(...)    do { if (g_pGameCVars->pl_debug_log_camera_mode_changes) CryLogAlways("[PlayerCamera] " __VA_ARGS__); } while(0)

static AUTOENUM_BUILDNAMEARRAY(s_cameraModeNames, CameraModeList);

#else

#define PLAYER_CAMERA_WATCH(...)  (void)(0)
#define PLAYER_CAMERA_LOG(...)    (void)(0)

#endif

#define CAMERA_NEAR_PLANE_DISTANCE 0.25f  //Use always this value

CPlayerCamera::CPlayerCamera(const CPlayer & ownerPlayer) :
	m_ownerPlayer(ownerPlayer)
{
	m_previousCameraMode = eCameraMode_Invalid;
	m_currentCameraMode = eCameraMode_Default;

	memset (m_cameraModes, 0, sizeof(m_cameraModes));

	m_cameraModes[eCameraMode_Default]             = new CDefaultCameraMode();
	m_cameraModes[eCameraMode_SpectatorFollow]     = new CSpectatorFollowCameraMode();
	m_cameraModes[eCameraMode_SpectatorFixed]      = new CSpectatorFixedCameraMode();
	m_cameraModes[eCameraMode_AnimationControlled] = new CAnimationControlledCameraMode();
	m_cameraModes[eCameraMode_Death]               = new CDeathCameraMode();
	m_cameraModes[eCameraMode_Vehicle]             = new CVehicleCameraMode();
	m_cameraModes[eCameraMode_PartialAnimationControlled]					 = new CPartialAnimationControlledCameraMode();

#if ENABLE_MINDCONTROL
	m_cameraModes[eCameraMode_MindControlled]      = new CMindControlledCameraMode();
#endif

	PLAYER_CAMERA_LOG ("Creating PlayerCamera instance %p", this);
}

CPlayerCamera::~CPlayerCamera()
{
	for (unsigned int i = 0; i < eCameraMode_Last; ++i)
	{
		SAFE_DELETE(m_cameraModes[i]);
	}
}

void CPlayerCamera::SetCameraMode(ECameraMode newMode, const char * why)
{
	PLAYER_CAMERA_LOG ("%s '%s' changing camera mode from %u (%s) to %u (%s): %s", m_ownerPlayer.GetEntity()->GetClass()->GetName(), m_ownerPlayer.GetEntity()->GetName(), m_currentCameraMode, s_cameraModeNames[m_currentCameraMode], newMode, s_cameraModeNames[newMode], why);
	m_currentCameraMode = newMode;
}

void CPlayerCamera::Update(SViewParams& viewParams, float frameTime )
{
	if (g_pGameCVars->g_detachCamera != 0)
		return;

	ECameraMode currentCameraMode = GetCurrentCameraMode();
	assert((currentCameraMode >= eCameraMode_Default) && (currentCameraMode < eCameraMode_Last));
	
	if (m_currentCameraMode != currentCameraMode)
	{
		SetCameraMode(currentCameraMode, "in update");
	}

	if (m_previousCameraMode != m_currentCameraMode)
	{
		if (m_previousCameraMode != eCameraMode_Invalid)
		{
			m_cameraModes[m_previousCameraMode]->Deactivate(m_ownerPlayer);
		}
		m_cameraModes[m_currentCameraMode]->Activate(m_ownerPlayer);
		m_previousCameraMode = m_currentCameraMode;
	}

	PLAYER_CAMERA_WATCH ("%s '%s' current camera mode is %d: %s", m_ownerPlayer.GetEntity()->GetClass()->GetName(), m_ownerPlayer.GetEntity()->GetName(), currentCameraMode, s_cameraModeNames[currentCameraMode]);

	UpdateCommon(viewParams, frameTime);
	assert (m_cameraModes[currentCameraMode]);
	m_cameraModes[currentCameraMode]->UpdateView(m_ownerPlayer, viewParams, frameTime);
}


void CPlayerCamera::UpdateCommon(SViewParams& viewParams, float frameTime )
{
	const SActorParams* pActorParams = m_ownerPlayer.GetActorParams();
	viewParams.fov = g_pGameCVars->cl_fov * pActorParams->viewFoVScale * pActorParams->viewFoVScale_Aux * (gf_PI/180.0f);
	viewParams.nearplane = CAMERA_NEAR_PLANE_DISTANCE;
}

void CPlayerCamera::SetBestCameraMode(const char * why)
{
	SetCameraMode(PickBestCameraMode(), why);
}

ECameraMode CPlayerCamera::PickBestCameraMode()
{
	if (m_ownerPlayer.GetLinkedVehicle())
	{
		return eCameraMode_Vehicle;
	}

	return eCameraMode_Default;
}

ECameraMode CPlayerCamera::GetCurrentCameraMode()
{
	//Benito
	//Note: Perhaps the mode should be set instead from the current player state, instead of checking stats every frame
	//NB: Have started working towards this! [TF]

	const SPlayerStats* pPlayerStats = static_cast<const SPlayerStats*>(m_ownerPlayer.GetActorStats());
	assert(pPlayerStats);

#if ENABLE_MINDCONTROL
	if (m_ownerPlayer.IsMindControlling())
	{
		return eCameraMode_MindControlled;
	}
#endif

	if ((pPlayerStats->spectatorInfo.mode == CActor::eASM_Fixed) || (pPlayerStats->spectatorInfo.mode == CActor::eASM_CCTV))
	{
		return eCameraMode_SpectatorFixed;
	}

	if(pPlayerStats->spectatorInfo.mode == CActor::eASM_Follow)
	{
		return eCameraMode_SpectatorFollow;
	}

	bool deathCamTarget = gEnv->bMultiplayer && (g_pGameCVars->g_deathCam != 0) && (m_ownerPlayer.GetHealth() <= 0) && (pPlayerStats->deathCamSubject != 0);
	if(deathCamTarget)
	{
		return eCameraMode_Death;
	}

	bool animationControlled = (pPlayerStats->isRagDoll || pPlayerStats->followCharacterHead) && (pPlayerStats->isThirdPerson == false);
	if (animationControlled)
	{
		return eCameraMode_AnimationControlled;
	}

	bool partiallyAnimationControlled = (m_ownerPlayer.IsSliding() || m_ownerPlayer.IsOnLedge());
	if (partiallyAnimationControlled)
	{
		return eCameraMode_PartialAnimationControlled;
	}

	// TEMP: Any camera mode selected by the above code also needs to be turned OFF by this function.
	//
	switch (m_currentCameraMode)
	{
		case eCameraMode_AnimationControlled:
		case eCameraMode_Death:
		case eCameraMode_SpectatorFollow:
		case eCameraMode_SpectatorFixed:
		case eCameraMode_PartialAnimationControlled:
#if ENABLE_MINDCONTROL
		case eCameraMode_MindControlled:
#endif
		return PickBestCameraMode();
	}

	return m_currentCameraMode;
}

