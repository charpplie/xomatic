/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 11:09:2009  : Created by Thomas Houghton

*************************************************************************/

#ifndef __IGAMERULESSPECTATORMODULE_H__
#define __IGAMERULESSPECTATORMODULE_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameRulesSystem.h"

class CGameRules;
class CCryAction;
class CGameplayRecorder;
struct IActor;
class CActor;
class CPlayer;

class IGameRulesSpectatorModule
{
	public:

	protected:

	typedef std::map<int, int>			TChannelSpectatorModeMap;
	typedef std::vector<EntityId>		TSpawnLocations;  // FIXME this is a duplicate of what's in GameRules, look at renaming to TSpecLocations or something?

	public:

	virtual ~IGameRulesSpectatorModule() {}

	virtual void Init(XmlNodeRef xml) = 0;
	virtual void PostInit() = 0;
	virtual void Update(float frameTime) = 0;

	virtual bool ModeIsEnabled(const int mode) const = 0;
	virtual bool ModeIsAvailable(const EntityId playerId, const int mode, EntityId* outOthEnt) = 0;
	virtual int GetBestAvailableMode(const EntityId playerId, EntityId* outOthEnt) = 0;
	virtual int GetNextMode(EntityId playerId, int inc, EntityId* outOthEid) = 0;

	virtual void SvOnChangeSpectatorMode(EntityId playerId, int mode, EntityId targetId, bool resetAll) = 0;
	virtual void SvRequestSpectatorTarget(EntityId playerId, int change) = 0;

	virtual void ClOnChangeSpectatorMode(EntityId playerId, int mode, EntityId targetId, bool resetAll) = 0;

	virtual bool GetModeFromChannelSpectatorMap(int channelId, int* outMode) const = 0;
	virtual void FindAndRemoveFromChannelSpectatorMap(int channelId) = 0;

	virtual EntityId GetNextSpectatorTarget(EntityId playerId, int change ) = 0;
	virtual bool CanChangeSpectatorMode(EntityId playerId) const = 0;
	virtual const char* GetActorSpectatorModeName(uint8 mode) = 0;
	virtual void ChangeSpectatorMode(IActor* pActor, uint8 mode, EntityId targetId, bool resetAll) = 0;
	virtual void ChangeSpectatorModeBestAvailable(IActor *pActor, bool resetAll) = 0;

	virtual void RequestNextSpectatorTarget(IActor* pActor, int change) = 0;

	virtual void AddSpectatorLocation(EntityId location) = 0;
	virtual void RemoveSpectatorLocation(EntityId id) = 0;
	virtual int GetSpectatorLocationCount() const = 0;
	virtual EntityId GetSpectatorLocation(int idx) const = 0;
	virtual void GetSpectatorLocations(TSpawnLocations &locations) const = 0;
	virtual EntityId GetRandomSpectatorLocation() const = 0;
	virtual EntityId GetInterestingSpectatorLocation() const = 0;

	virtual void AddCCTVLocation(EntityId location) = 0;
	virtual void RemoveCCTVLocation(EntityId id) = 0;
	virtual int GetCCTVLocationCount() const = 0;
	virtual EntityId GetCCTVLocationAtIdx(int idx) const = 0;
	virtual EntityId GetCurCCTVLocation(const CPlayer* pPlayer) const = 0;
	virtual EntityId GetNextCCTVLocation(const CActor* pPlayer) const = 0;
	virtual EntityId GetPrevCCTVLocation(const CActor* pPlayer) const = 0;
	virtual void GetCCTVLocations(TSpawnLocations &locations) const = 0;
	virtual EntityId GetRandomCCTVLocation() const = 0;
	virtual EntityId GetInterestingCCTVLocation() const = 0;

	virtual float GetTimeFromDeathTillAutoSpectate() const = 0;

	virtual void PostUpdateSpectatorCCTVTagging() = 0;
	virtual void HandleCCTVTaggingAction() = 0;
};


#endif  // __IGAMERULESSPECTATORMODULE_H__
