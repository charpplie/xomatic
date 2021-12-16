#ifndef _EXPLOSION_SCREEN_GAME_EFFECT_
#define _EXPLOSION_SCREEN_GAME_EFFECT_

#pragma once

// Includes
#include "GameEffect.h"
#include "Effects/GameEffectsSystem.h"

// Forward declares
struct ExplosionInfo;

//==================================================================================================
// Name: CExplosionGameEffect
// Desc: Explosion game effect
// Author: James Chilvers
//==================================================================================================
class CExplosionGameEffect : public CGameEffect
{
public:
	CExplosionGameEffect();
	~CExplosionGameEffect();

	void	Initialise(const SGameEffectParams* gameEffectParams = NULL);
	void	Release();
	void	Update(float frameTime);

	void  Explode(ExplosionInfo &explosionInfo);
	void	SetCutSceneActive(bool isCutSceneActive) { m_cutSceneActive = isCutSceneActive; }

#if DEBUG_GAME_FX_SYSTEM
	static void  DebugOnInputEvent(int keyId);
	static void	 DebugDisplay(const Vec2& textStartPos,float textSize,float textYStep);
#endif
	
	void		SpawnMaterialEffect(const ExplosionInfo &explosionInfo);

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));	
	}
private:

	void		SpawnParticleEffect(const ExplosionInfo &explosionInfo);
	void		SpawnScreenExplosionEffect(const ExplosionInfo &explosionInfo);
	void		QueueMaterialEffect(ExplosionInfo &explosionInfo);

	bool		IsPlayer();

	bool		m_cutSceneActive;

};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: SExplosionGameEffectParams
// Desc: Explosion game effect parameters
// Author: James Chilvers
//==================================================================================================
struct SExplosionGameEffectParams : public SGameEffectParams
{
	SExplosionGameEffectParams()
	{
		
	}

};//------------------------------------------------------------------------------------------------

#endif // _EXPLOSION_SCREEN_GAME_EFFECT_
