/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Camera mode interface, enums, etc

-------------------------------------------------------------------------
History:
- 15:10:2009   Created by Benito Gangoso Rodriguez

*************************************************************************/

#pragma once

#ifndef _ICAMERA_MODE_H_
#define _ICAMERA_MODE_H_

#include "AutoEnum.h"

#if ENABLE_MINDCONTROL
#define OPTIONAL_MIND_CONTROLLED_CAMERA(func)   func(eCameraMode_MindControlled)
#else
#define OPTIONAL_MIND_CONTROLLED_CAMERA(func)
#endif

#define CameraModeList(f)            \
	f(eCameraMode_Default)             \
	f(eCameraMode_SpectatorFollow)     \
	f(eCameraMode_SpectatorFixed)      \
	f(eCameraMode_AnimationControlled) \
	f(eCameraMode_Death)               \
	f(eCameraMode_Vehicle)             \
	f(eCameraMode_PartialAnimationControlled)					 \
	OPTIONAL_MIND_CONTROLLED_CAMERA(f) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(ECameraMode, CameraModeList, eCameraMode_Invalid, eCameraMode_Last);

class CPlayer;
struct SViewParams;

struct ICameraMode
{
	virtual void Activate(const CPlayer & clientPlayer) {;}
	virtual void Deactivate(const CPlayer & clientPlayer) {;}
	virtual void UpdateView(const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime) = 0;
};

#endif
