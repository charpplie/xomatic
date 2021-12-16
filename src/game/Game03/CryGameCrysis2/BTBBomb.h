/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Bomb item for the BombTheBase gamemode

-------------------------------------------------------------------------
History:
- 01:12:2009   : Created by Colin Gulliver

*************************************************************************/
#ifndef __BTBBOMB_H__
#define __BTBBOMB_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Item.h"
#include "CryFixedString.h"

class CBTBBomb : public CItem
{
public:
	typedef CryFixedStringT<64> TFixedString;

	struct SSetStateParams
	{
		EntityId m_interactorId;
		int m_state;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("interactorId", m_interactorId, 'eid');
			ser.Value("state", m_state, 'ui3');
		};
	};

	CBTBBomb();
	virtual ~CBTBBomb();

	virtual void OnReset();

	virtual bool IsPickable() const;

	virtual bool CanPickUp(EntityId userId) const;
	void PickUp(EntityId pickerId, bool sound, bool select, bool keepHistory, const char* setup);

	virtual bool CanUse(EntityId userId) const;
	virtual void Use(EntityId userId);
	virtual void StartUse(EntityId userId);

	virtual bool CanDrop() const;
	virtual void Drop(float impulseScale=1.0f, bool selectNext=true, bool byDeath=false);

	virtual void Update( SEntityUpdateContext& ctx, int slot);

	virtual void PostInit( IGameObject * pGameObject );
	virtual void InitClient(int channelId);
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	DECLARE_CLIENT_RMI_NOATTACH(ClSetState, SSetStateParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestState, SSetStateParams, eNRT_ReliableOrdered);

	void GetMemoryUsage(ICrySizer *pSizer )const
	{
		pSizer->AddObject(this, sizeof(*this));
		CItem::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}
protected:
	enum EBTBBombState
	{
		eBS_Invalid = -1,		// Initially spawned or dropped somewhere
		eBS_Dropped = 0,		// Initially spawned or dropped somewhere
		eBS_Carried,		// Picked up and not deployed
		eBS_Deployed,		// Deployed at site
		eBS_Defused,		// Been defused, can remove
		eBS_Finished,		// Has exploded / is exploding
	};

	const static int k_renderSlot = 1;

	void ResetVariables();
	void ClEnableInputFilter(bool enable);

	void SvReceivedStateRequest(const SSetStateParams &params);
	void ClReceivedStateChange(const SSetStateParams &params);

	bool ClCanDeploy();

	Vec3 m_droppedLocation;

	float m_interactionTime;
	float m_requiredInteractionTime;
	float m_removeTime;
	float m_deployedTime;
	float m_dropTime;

	EntityId m_interactorId;
	EntityId m_deployerId;

	int m_state;
	int m_deployerTeam;

	EBTBBombState m_targetState;

	bool m_inputsLocked;
	bool m_sentRequest;
	bool m_isUsing;
};


#endif //__BTBBOMB_H__
