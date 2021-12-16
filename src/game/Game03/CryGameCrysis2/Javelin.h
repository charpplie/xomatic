/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Javelin Item for the end of Power struggle Lite

-------------------------------------------------------------------------
History:
- 10:11:2009   11:06 : Created by Ben Parbury

*************************************************************************/
#ifndef __JAVELIN_H__
#define __JAVELIN_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Item.h"

class CJavelin: public CItem
{
public:
	CJavelin();
	virtual ~CJavelin();

	static const NetworkAspectType ASPECT_JAVELIN	= eEA_GameServerStatic;

	virtual bool ReadItemParams(const IItemParamsNode *root);

	virtual void OnReset();

	virtual bool IsPickable() const;

	virtual bool CanPickUp(EntityId userId) const;
	virtual void PickUp(EntityId pickerId, bool sound=true, bool select=false, bool keepHistory=true, const char *setup=0);

	virtual bool CanUse(EntityId userId) const;
	virtual void Use(EntityId userId);

	virtual bool CanDrop() const;
	virtual void Drop(float impulseScale=1.0f, bool selectNext=true, bool byDeath=false);

	virtual void Update( SEntityUpdateContext& ctx, int );
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void PostInit( IGameObject * pGameObject );

	struct RequestJavelinState
	{
		RequestJavelinState() {};
		RequestJavelinState(int state): m_state(state) {};
		void SerializeWith(TSerialize ser)
		{
			ser.Value("state", m_state, 'ui2');
		};

		int m_state;
	};

	struct NoParams
	{
		void SerializeWith(const TSerialize& ser) {};
	};

	DECLARE_SERVER_RMI_NOATTACH(SvRequestJavelinState, RequestJavelinState, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClCanDefuse, NoParams, eNRT_ReliableUnordered);

	struct SMissileParams
	{
		SMissileParams()
		{
			name = "JavelinMissile";
			type = "frag";
			helper = "weapon_term";
			damage = 1000;
		}
		const char* name;
		const char* type;
		const char* helper;
		int damage;
	};

	struct STimingParams
	{
		float deployTime;
		float defuseTime;
		float launchTime;
		float tapToDropTime;
	};

	struct STargetParams
	{
		const char* targetClass;
	};

	struct SAudioParams
	{
		const char* pickup;
		const char* defused;
	};

	void GetMemoryUsage(ICrySizer *pSizer )const
	{
		pSizer->AddObject(this, sizeof(*this));
		CItem::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}
protected:
	enum EJS_JavelinState
	{
		EJS_Pickable = 0,		//initially spawned or dropped somewhere
		EJS_Carried = 1,		//picked up and not deployed
		EJS_Deploying = 2,		//deploying at site
		EJS_Deployed = 3,		//deployed at site
		EJS_Defused = 4,		//been defused, can remove
		EJS_Finished = 5,		//fired
	};

	const static int k_renderSlot = 1;

	void SetDroppedPositionAndRotation();
	void SetState(EJS_JavelinState newState, bool serverCommand);
	EntityId GetTargetEntityId();
	void ResetVariables();
	bool IsInsideDeploymentArea(EntityId entityId);
	void FireJavelinMissile();
	void CheckDefuse(float frametime);
	void ClChangedInsideState();
	void ClEnableInputFilter(bool enable);
	void OpenAnimation();

	void UpdateStateFeedback(EJS_JavelinState state);		//in update
	void ChangedStateFeedback(EJS_JavelinState state);	//on state change
	void DefuseFeedback();
	
	SMissileParams m_missileParams;
	STimingParams m_timingParams;
	STargetParams m_targetParams;
	SAudioParams m_audioParams;

	Vec3 m_droppedLocation;

	float m_deployProgress;
	float m_defuseProgress;
	float m_heldTimer;

	EntityId m_defuserId;
	EntityId m_targetId;
	EntityId m_deployerId;

	int m_state;

	bool m_inLaunchArea;
	bool m_inputsLocked;
};


#endif //__JAVELIN_H__
