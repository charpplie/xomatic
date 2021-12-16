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

#pragma once

#ifndef _PLAYER_CAMERA_H_
#define _PLAYER_CAMERA_H_

#include "ICameraMode.h"

class CPlayer;

class CPlayerCamera
{
public:

	CPlayerCamera(const CPlayer & ownerPlayer);
	~CPlayerCamera();

	void Update(SViewParams& viewParams, float frameTime);
	void SetCameraMode(ECameraMode newMode, const char * why);
	void SetBestCameraMode(const char * why);

private:
	ECameraMode GetCurrentCameraMode();
	void UpdateCommon(SViewParams& viewParams, float frameTime);
	ECameraMode PickBestCameraMode();

	// No need to use a dynamic container (eg. vector) as we know exactly how many modes there are going to be... [TF]
	ICameraMode * m_cameraModes[eCameraMode_Last];
	ECameraMode m_currentCameraMode;
	ECameraMode m_previousCameraMode;
	const CPlayer & m_ownerPlayer;
};

#endif