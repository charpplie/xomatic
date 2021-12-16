/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Particle info for player perks ('Sonar Vision' and 'Cloak Awareness')
-------------------------------------------------------------------------
History:
- 22:5:2009: Created by Tim Furnish

*************************************************************************/

#ifndef __PLAYER_PERK_PARTICLE_INFO_H__
#define __PLAYER_PERK_PARTICLE_INFO_H__

class CPlayer;

enum EPlayerPerkParticleEmitterType
{
	ePPPET_Movement,
	ePPPET_CloakAwareness,
	ePPPET_Refraction,
	ePPPET_Vib,
	ePPPET_NumTypes,
};

enum EPlayerPerkParticleElementID
{
	ePPPEI_Body,
	ePPPEI_Gunfire,
	ePPPEI_Projectile,
	ePPPEI_CloakAwareness,
};

class CPlayerPerkParticleInfo
{
	public:
	CPlayerPerkParticleInfo ();

	void								Reset ();
	void								AddNoiseBurst (float amount);
	void								Tick (const SEntityUpdateContext& ctx);
	static bool					ShouldGenerateEffectsForEntity (EntityId thisEntity, CPlayer * pLocalPlayer, EPlayerPerkParticleElementID element);
	IParticleEmitter *	GetEmitter (IEntity * pThisEntity, EPlayerPerkParticleEmitterType mode, const char * particleType);
	static float				CalculateBaseEffectScale(const Vec3 & pos, CPlayer * pLocalPlayer, float range, float reduceAmountWhenBusy);
	float								CalculateMovementModifierScale(IEntity * pThisEntity);

  private:
	float								m_movementNoiseBurst;
	int									m_emitterSlot[ePPPET_NumTypes];
};

#endif // __PLAYER_PERK_PARTICLE_INFO_H__
