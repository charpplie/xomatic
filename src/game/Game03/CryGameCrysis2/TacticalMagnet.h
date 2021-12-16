#ifndef _TACTICAL_MAGNET_H_
#define _TACTICAL_MAGNET_H_

#include "TacticalBullet.h"


class CTacticalMagnet : public CTacticalBullet
{
public:
	virtual void Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale);

private:
	struct SImpulse
	{
		IPhysicalEntity* target;
		Vec3 impulse;
	};


	virtual void StartBulletAction();
	virtual void UpdateBulletAction(float frameTime);

	void QueryMagnetAffectedEntities(CTacticalMagnet::SImpulse** pImpulseArray, int* pNumEntitties) const;
	float ComputeMagnetImpulses(CTacticalMagnet::SImpulse* impulses, int numEntitties, float timeDissipation) const;
	Vec3 NormalizeMagnetImpulses(CTacticalMagnet::SImpulse* impulses, int numEntitties, float totalImpulse, float frameTime) const;
	void ApplyMagnetImpulses(CTacticalMagnet::SImpulse* impulses, int numEntitties) const;
	void ApplyReactionImpulse(const Vec3& totalImpulseVec) const;
	Vec3 CalculateInteractionImpulse(IPhysicalEntity* pPhisicalEntity) const;
	bool CanApplyImpulse(IPhysicalEntity* pPhisicalEntity) const;
	bool HasMetalPart(IPhysicalEntity* pPhisicalEntity) const;
	void ApplyImpulse(IPhysicalEntity* pPhisicalEntity, const Vec3& impulse) const;

	virtual void IntensifyActionEffect();


	float m_activeTime;
	float m_activeTimeSum;
	float m_countingTime;
	float m_maximumImpulsePerSecond;
	float m_maximumImpulsePerSecondSum;
};


#endif
