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

#ifndef __GAMERULESMPSPECTATOR_H__
#define __GAMERULESMPSPECTATOR_H__

#include "GameRulesModules/IGameRulesSpectatorModule.h"

class CGameRulesMPSpectator : public IGameRulesSpectatorModule
{
	public:

	protected:

		TChannelSpectatorModeMap  m_channelSpectatorModes;

		TSpawnLocations  m_spectatorLocations;
		TSpawnLocations  m_cctvLocations;

		CGameRules*  m_pGameRules;
		IGameFramework*  m_pGameFramework;
		IGameplayRecorder*  m_pGameplayRecorder;
		IActorSystem*  m_pActorSys; 
		IUIDraw*  m_pUIDraw;

		uint8  m_eatsActorActions	:1;
		uint8  m_enableFree				:1;
		uint8  m_enableFollow			:1;
		uint8  m_enableCCTV				:1;

		float  m_timeFromDeathTillAutoSpectate;

		int  m_specCctvCursorTexId;

	public:

		CGameRulesMPSpectator();
		virtual ~CGameRulesMPSpectator();

		virtual void Init(XmlNodeRef xml);
		virtual void PostInit();
		virtual void Update(float frameTime);

		virtual bool ModeIsEnabled(const int mode) const;
		virtual bool ModeIsAvailable(const EntityId playerId, const int mode, EntityId* outOthEnt);
		virtual int GetBestAvailableMode(const EntityId playerId, EntityId* outOthEnt);
		virtual int GetNextMode(EntityId playerId, int inc, EntityId* outOthEid);

		virtual void SvOnChangeSpectatorMode(EntityId playerId, int mode, EntityId targetId, bool resetAll);
		virtual void SvRequestSpectatorTarget( EntityId playerId, int change );

		virtual void ClOnChangeSpectatorMode(EntityId playerId, int mode, EntityId targetId, bool resetAll);

		virtual bool GetModeFromChannelSpectatorMap(int channelId, int* outMode) const;
		virtual void FindAndRemoveFromChannelSpectatorMap(int channelId);
	
		virtual EntityId GetNextSpectatorTarget(EntityId playerId, int change );
		virtual bool CanChangeSpectatorMode(EntityId playerId) const;
		virtual const char* GetActorSpectatorModeName(uint8 mode);

		virtual void ChangeSpectatorMode(IActor* pActor, uint8 mode, EntityId targetId, bool resetAll);
		virtual void ChangeSpectatorModeBestAvailable(IActor *pActor, bool resetAll);

		virtual void RequestNextSpectatorTarget(IActor* pActor, int change);

		virtual void AddSpectatorLocation(EntityId location);
		virtual void RemoveSpectatorLocation(EntityId id);
		virtual int GetSpectatorLocationCount() const;
		virtual EntityId GetSpectatorLocation(int idx) const;
		virtual void GetSpectatorLocations(TSpawnLocations &locations) const;
		virtual EntityId GetRandomSpectatorLocation() const;
		virtual EntityId GetInterestingSpectatorLocation() const;

		virtual void AddCCTVLocation(EntityId location);
		virtual void RemoveCCTVLocation(EntityId id);
		virtual int GetCCTVLocationCount() const;
		virtual EntityId GetCCTVLocationAtIdx(int idx) const;
		virtual EntityId GetCurCCTVLocation(const CPlayer* pPlayer) const;
		virtual EntityId GetNextCCTVLocation(const CActor* pPlayer) const;
		virtual EntityId GetPrevCCTVLocation(const CActor* pPlayer) const;
		virtual void GetCCTVLocations(TSpawnLocations &locations) const;
		virtual EntityId GetRandomCCTVLocation() const;
		virtual EntityId GetInterestingCCTVLocation() const;

		virtual float GetTimeFromDeathTillAutoSpectate() const { return m_timeFromDeathTillAutoSpectate; }

		virtual void PostUpdateSpectatorCCTVTagging();
		virtual void HandleCCTVTaggingAction();

	protected:

		EntityId FindEntityUnderCCTVCursor();
};


#endif // __GAMERULESMPSPECTATOR_H__
