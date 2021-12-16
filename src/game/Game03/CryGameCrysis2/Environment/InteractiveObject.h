/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Crysis2 interactive object, for playing co-operative animations with player

-------------------------------------------------------------------------
History:
- 10:12:2009: Created by Benito G.R.

*************************************************************************/

#pragma once

#ifndef __CRYSIS2_INTERACTIVE_OBJECT_H__
#define __CRYSIS2_INTERACTIVE_OBJECT_H__

#include <IGameObject.h>
#include "../ItemString.h"

class CInteractiveObjectEx : public CGameObjectExtensionHelper<CInteractiveObjectEx, IGameObjectExtension>
{

private:

	enum EState
	{
		eState_NotUsed = 0,
		eState_InUse,
		eState_Done
	};

public:
	CInteractiveObjectEx();
	virtual ~CInteractiveObjectEx();

	// IGameObjectExtension
	virtual bool Init(IGameObject *pGameObject);
	virtual void InitClient(int channelId);
	virtual void PostInit(IGameObject *pGameObject);
	virtual void PostInitClient(int channelId);
	virtual void Release();
	virtual void FullSerialize( TSerialize ser );
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	virtual void PostSerialize();
	virtual void SerializeSpawnInfo( TSerialize ser );
	virtual ISerializableInfoPtr GetSpawnInfo();
	virtual void Update( SEntityUpdateContext &ctx, int updateSlot);
	virtual void PostUpdate(float frameTime );
	virtual void PostRemoteSpawn();
	virtual void HandleEvent( const SGameObjectEvent &goEvent);
	virtual void ProcessEvent(SEntityEvent &entityEvent);
	virtual void SetChannelId(uint16 id);
	virtual void SetAuthority(bool auth);
	virtual void GetMemoryUsage(ICrySizer *pSizer) const;
	//~IGameObjectExtension

	//Script callbacks
	bool CanUse(EntityId entityId) const;
	void Use(EntityId entityId);	
	void StopUse(EntityId entityId);
	//~Script callbacks
	
private:

	bool Reset();

	void StartUse(EntityId entityId);
	void Done(EntityId entityId);
	void CalculateAlignmentHelperLocation(const char* helperName);
	bool IsUserNearEnough(EntityId userId) const;
	bool IsValidInteractionName(const char* interactionName) const;

	uint32 GetCrcForName(const char* name) const;
	void Physicalize(pe_type physicsType);

	QuatT		m_alignmentHelperLocation;

	EState	m_state;
	int			m_physicalizationAfterAnimation;
	uint32  m_currentLoadedCharacterCrc;
	uint32  m_currentInteractionCrc;

	float		m_interactionRadius;
	float		m_interactionAngle;
};

#endif //__CRYSIS2_INTERACTIVE_OBJECT_H__