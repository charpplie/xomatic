#ifndef __RESISTANTPERK_H__
#define __RESISTANTPERK_H__

#include "IPerk.h"
#include "Effects/GameEffects/DeflectionGameEffect.h"

class CAudioSignalPlayer;

class ResistantPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(ResistantPerk);

	ResistantPerk(EPlayerPlugInEvent type, const char* hitType, const float *pDamageMultiplier, const char* feedbackSignal, const char * particleName);
	virtual ~ResistantPerk();

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);
	//virtual void Update(const float dt) {}

	virtual void Enter();

protected:
	
	inline bool IsDeflectionPerk() const { return (m_perkId == ePerk_Deflection) || (m_perkId == ePerk_RearDeflection); }

	int                   m_hitTypeId;
	const float *         m_pDamageMultiplier;
	const char *          m_particleEffectName;
	CDeflectionGameEffect	m_deflectionScreenEffect;
	CAudioSignalPlayer    m_feedback;
	EPlayerPlugInEvent		m_damageModifierType;

	virtual float CalculateFractionApplicable(const HitInfo * hitInfo)
	{
		return 1.f;
	}
};

#endif
