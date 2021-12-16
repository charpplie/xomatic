#include "StdAfx.h"
#include "PlayerPlugin_Perk_ModifyDamageBasedOnAngle.h"

#include "Player.h"
#include "PerkIconData.h"
#include "GameRules.h"

CPlayerPlugin_Perk_ModifyDamageBasedOnAngle::CPlayerPlugin_Perk_ModifyDamageBasedOnAngle(EPlayerPlugInEvent type, const char* hitType, const float * triggerWhenDotIsOver, const float * reduceDamageWhenBehind, const char * feedbackSignal, const char * particleName) :
	ResistantPerk(type, hitType, reduceDamageWhenBehind, feedbackSignal, particleName)
{
	m_triggerWhenDotIsOver = triggerWhenDotIsOver;
}

float CPlayerPlugin_Perk_ModifyDamageBasedOnAngle::CalculateFractionApplicable(const HitInfo * hitInfo)
{
	assert (hitInfo);

	const Vec3 & forwardDir = m_ownerPlayer->GetEntity()->GetForwardDir();
	const Vec3 & damageDir = hitInfo->dir;

	// These direction vectors should already be of unit length...
	ASSERT_FLOAT_ARE_EQUAL (forwardDir.len2(), 1.f, 0.0001f);
	ASSERT_FLOAT_ARE_EQUAL (damageDir.len2(), 1.f, 0.0001f);

	const float dot = forwardDir.Dot(damageDir);
	
	if (dot > *m_triggerWhenDotIsOver)
	{
		return (*m_triggerWhenDotIsOver - dot) / (*m_triggerWhenDotIsOver - 1.f);
	}

	return 0.f;
}
