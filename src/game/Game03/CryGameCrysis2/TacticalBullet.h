#ifndef _TACTICAL_BULLET_H_
#define _TACTICAL_BULLET_H_

#include "Projectile.h"



class TacticalBulletState;


class CTacticalBullet : public CProjectile
{
public:
	CTacticalBullet();
	virtual ~CTacticalBullet();

	//CProjectile
	virtual void HandleEvent( const SGameObjectEvent &);
	virtual void Update(SEntityUpdateContext &ctx, int updateSlot);
	virtual void Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale);


	IPhysicalEntity* GetTargetPhysicalEntity() { return m_pParentPhysicalEntity; }
	int GetTargetPhysicalEntityPartID() { return m_ParentPhysicalEntityPartID; }

	EntityId GetTargetId() const {return m_parentId;}
	bool IsStuck() const {return m_attached;}

protected:


	float GetRadius() const {return m_params->m_effectRadius;}
	void SpawnActivationEffect(const char* particlesName);
	void SpawnExplosionEffect();
	virtual void IntensifyActionEffect();


private:
	friend class TacticalBulletState;
	friend class FlyingState;
	friend class ActivatingState;
	friend class ActiveState;
	friend class GoToPointState;

	virtual void StartBulletAction();
	virtual void UpdateBulletAction(float deltaTime);

	TacticalBulletState* m_actualState;
	void ChangeState(TacticalBulletState* nextState);


	struct Params
	{
		Params()
			:	m_activationEffectName(0)
			,	m_stickTime(2.5f)
			,	m_activationTime(1.0f)
			,	m_effectRadius(10.0f)
			,	m_effectRadiusSum(1.0f)
			,	m_hommingSpeed(10.0f)
		{
		}

		const char* m_activationEffectName;
		float m_stickTime;
		float m_activationTime;
		float m_effectRadius;
		float m_effectRadiusSum;
		float m_hommingSpeed;
	}* m_params;


	void Stick(EventPhysCollision *pCollision);

	bool		m_attached;
	EntityId	m_parentId;

	IPhysicalEntity* m_pParentPhysicalEntity;
	int m_ParentPhysicalEntityPartID;
	int m_activationEffectId;
	float m_activationEffectSize;


	static CTacticalBullet* m_mainBulllet;
};



class CTacticalEMP : public CTacticalBullet
{
private:
	virtual void StartBulletAction();
};



class CTacticalExplosion : public CTacticalBullet
{
private:
	virtual void StartBulletAction();
};



#endif
