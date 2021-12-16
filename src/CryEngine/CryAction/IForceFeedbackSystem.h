/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-20010.
-------------------------------------------------------------------------

Interface for force feedback system

-------------------------------------------------------------------------
History:
- 18-02-2010:	Created by Benito Gangoso Rodriguez

*************************************************************************/

#include DEVIRTUALIZE_HEADER_FIX(IForceFeedbackSystem.h)

#pragma once

#ifndef _IFORCEFEEDBACKSYSTEM_H_
#define _IFORCEFEEDBACKSYSTEM_H_

UNIQUE_IFACE struct IFFSPopulateCallBack
{
	virtual void AddFFSEffectName( const char* const pName ) = 0;
};


UNIQUE_IFACE struct IForceFeedbackSystem
{
	virtual void PlayForceFeedbackEffect(const char* effectName, const float intensity) = 0;
	virtual void StopForceFeedbackEffect(const char* effectName) = 0;
	virtual void StopAllEffects() = 0;
 
	virtual void AddFrameCustomForceFeedback(const float amplifierA, const float amplifierB) = 0;
	
	virtual void EnumerateEffects( IFFSPopulateCallBack* pCallBack ) = 0;  // intended to be used only from the editor
};

#endif