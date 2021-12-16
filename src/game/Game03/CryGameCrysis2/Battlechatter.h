/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
Description:
Automatically plays sounds for nearby actors
**************************************************************************/

#ifndef __BATTLECHATTER_H__
#define __BATTLECHATTER_H__

#include "PlayerPlugin.h"
#include "AutoEnum.h"
#include "GameRulesModules/IGameRulesKillListener.h"

//Note - enum is used as lookup in for AudioSignalPlayer;
#define BATTLECHATTER_LIST(f) \
	f(BC_Reloading) \
	f(BC_LowHealth) \
	f(BC_EnemyKilled) \
	f(BC_GrenadeThrow) \
	f(BC_Ripoff) \
	f(BC_Visual) \
	f(BC_VisualHigh) \
	f(BC_VisualLow) \
	f(BC_MovingUp) \
	f(BC_LowAmmo) \
	f(BC_SecondaryWeapon) \
	f(BC_SniperCountermeasures) \
	f(BC_MissileCountermeasures) \
	f(BC_Ditto) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EBattlechatter, BATTLECHATTER_LIST, BC_Null, BC_Last);

class CBattlechatter: public IGameRulesKillListener
{
public:
	CBattlechatter();
	virtual ~CBattlechatter();

	void Update(const float dt);
	void Event(EBattlechatter chatter, EntityId actorId);

	void SetLocalPlayer(CPlayer* pPlayer);

	//IGameRulesKillListener
	void OnEntityKilled(const HitInfo &hitInfo);

#if !defined(_RELEASE)
	static void CmdPlay(IConsoleCmdArgs* pCmdArgs);
#endif

protected:

	struct SVoiceInfo
	{
		SVoiceInfo(int voiceIndex)
		{
			m_voiceIndex = voiceIndex;
			m_lastTimePlayed = 0.0f;
		}

		int m_voiceIndex;
		float m_lastTimePlayed;
	};

	void Init();
	void InitVoices();
	void InitData();
	bool PlayedRecently(const SVoiceInfo* pInfo, const float currentTime) const;
	bool CanPlayDitto(const float currentTime) const;
	bool ShouldClientHear() const;
	bool ShouldPlayForActor(EntityId actorId, float range) const;

	float DistanceToActor(EntityId actorId) const;

	void UpdateNearestPlayer(const float dt);
	EntityId GetNearestHearbleActor();
	void SetNearestPlayer(EntityId prevId, EntityId newId);
	
	void UpdateNearestPlayerBattlechatter();
	float GetForwardAmount(CPlayer* pPlayer);

	SVoiceInfo* GetVoiceInfo(EntityId actorId);

	void Play(SVoiceInfo* pInfo, EBattlechatter chatter, EntityId actorId, float currentTime);

	struct SVoice
	{
		struct SSignalList
		{
			SSignalList();
			bool Init(int voiceIndex, int chatterIndex);

			std::vector<TAudioSignalID> m_signal;
		};

		SVoice();
		bool Init(int voiceIndex);
		void clear();

		SSignalList m_chatter[BC_Last];
	};

	CPlayer* m_clientPlayer;
	EntityId m_nearestPlayerId;
	EBattlechatter m_lastPlayed;

	float m_lastTimePlayed;
	float m_nearestPlayerTimer;

	std::vector <SVoice> m_voice;

	typedef std::map<EntityId, SVoiceInfo> ActorVoiceMap;
	ActorVoiceMap m_actorVoice;

	std::vector<int> m_nextActorVoiceList;
	int m_nextActorVoiceIndex;

	float m_range[BC_Last];
	float m_maxRange;
};

#define BATTLECHATTER(chatter, actorId)	\
	CGameRules* pGameRules = g_pGame->GetGameRules(); \
	if(pGameRules) \
	{	\
		CBattlechatter* pBattlechatter = pGameRules->GetBattlechatter();	\
		if(pBattlechatter) \
		{	\
			pBattlechatter->Event(chatter, actorId);	\
		}	\
	}	\

#endif __BATTLECHATTER_H__
