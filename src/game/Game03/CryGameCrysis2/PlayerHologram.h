/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: PlayerHologram.h$
$DateTime$
Description: Hologram player, a visual entity fired from the Hologram weapon mode

-------------------------------------------------------------------------
History:
- 01/04/2009 16:23:34: Created by Tom Berry

*************************************************************************/

#ifndef __PLAYERHOLOGRAM_H__
#define __PLAYERHOLOGRAM_H__

#include "Player.h"

class CPlayerHologram : public CPlayer, public IAIEventListener
{
public:
	CPlayerHologram();
	virtual ~CPlayerHologram();
	
	void SetParams(SmartScriptTable &rTable,bool resetFirst);

	virtual void Revive(EReasonForRevive reasonForRevive);
	virtual void PostPhysicalize();
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	virtual void SetHealth( int health );
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot);
	virtual void OnAIEvent(EAIStimulusType type, const Vec3& pos, float radius, float threat, EntityId sender);
	
	virtual void OnFootStepAnimEvent(ICharacterInstance* pCharacter, const char* boneName) {};

	void SetTarget(const Vec3 &targetPos);

	static  const char* GetActorClassType() { return "CPlayerHologram"; }
	virtual const char* GetActorClass() const { return CPlayerHologram::GetActorClassType(); }

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
		CPlayer::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}
protected:
	virtual bool ShouldRegisterAsAutoAimTarget() { return false; }
	virtual void OnChangeTeam();

private:

	void Initialise();
	void InitiateFlicker();

	Vec3 m_targetPosition;
	Vec3 m_targetFacing;
	Vec3 m_newLookFacing;
	Vec3 m_currentLookFacing;
	Vec3 m_lastPosition;
	float m_blockedTime;
	float m_lifetime;
	float m_updateLookTimer;
	float m_checkProxTimer;
	bool m_done;
	bool m_initialised;
	bool m_moving;
	bool m_crouch;
};

#endif //!__PLAYERHOLOGRAM_H__
