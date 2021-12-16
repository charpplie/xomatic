#ifndef _DEFLECTION_GAME_EFFECT_
#define _DEFLECTION_GAME_EFFECT_

#pragma once

// Includes
#include "GameEffect.h"
#include "IMaterialEffects.h"
#include "Effects/GameEffectsSystem.h"

//==================================================================================================
// Name: CDeflectionGameEffect
// Desc: Deflection game effect
// Author: James Chilvers
//==================================================================================================
class CDeflectionGameEffect : public CGameEffect
{
public:
	CDeflectionGameEffect();
	~CDeflectionGameEffect();

	void	Initialise(const SGameEffectParams* gameEffectParams = NULL);
	void	Release();
	void	Update(float frameTime);

	void	SetActive(bool isActive);

	void	Deflect(const Quat& playerRot, const Vec3& bulletDir);

#if DEBUG_GAME_FX_SYSTEM
	static void  DebugOnInputEvent(int keyId);
	static void	 DebugDisplay(const Vec2& textStartPos,float textSize,float textYStep);
#endif

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));	
	}
private:

	TMFXEffectId							m_screenEffectId;

};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: SDeflectionGameEffectParams
// Desc: Deflection game effect parameters
// Author: James Chilvers
//==================================================================================================
struct SDeflectionGameEffectParams : public SGameEffectParams
{
	SDeflectionGameEffectParams()
	{
		screenEffectId = InvalidEffectId;
	}

	TMFXEffectId		screenEffectId;

};//------------------------------------------------------------------------------------------------

#endif // _DEFLECTION_GAME_EFFECT_
