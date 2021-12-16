#ifndef __PERK_MODIFY_DAMAGE_BASED_ON_ANGLE_H__
#define __PERK_MODIFY_DAMAGE_BASED_ON_ANGLE_H__

#include "ResistantPerk.h"

struct HitInfo;

class CPlayerPlugin_Perk_ModifyDamageBasedOnAngle : public ResistantPerk
{
	public:
	SET_PLAYER_PLUGIN_NAME(CPlayerPlugin_Perk_ModifyDamageBasedOnAngle);

	CPlayerPlugin_Perk_ModifyDamageBasedOnAngle(EPlayerPlugInEvent type, const char * hitType, const float * triggerWhenDotIsOver, const float * reduceDamageWhenBehind, const char * feedbackSignal, const char * particleName);

	private:
	const float * m_triggerWhenDotIsOver;

	virtual float CalculateFractionApplicable(const HitInfo * hitInfo);
};

#endif	// __PERK_MODIFY_DAMAGE_BASED_ON_ANGLE_H__