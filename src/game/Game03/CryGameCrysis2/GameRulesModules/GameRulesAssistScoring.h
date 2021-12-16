/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 
Game rules module to handle scoring points values
-------------------------------------------------------------------------
History:
- 14:09:2009  : Created by James Bamford

*************************************************************************/

#ifndef __GAMERULESASSISTSCORING_H__
#define __GAMERULESASSISTSCORING_H__

#include "IGameRulesAssistScoringModule.h"

class CGameRules;

class CGameRulesAssistScoring : public IGameRulesAssistScoringModule
{
protected:
	CGameRules *m_pGameRules;
	float m_maxTimeBetweenAttackAndKillForAssist;

	typedef struct SAttackerData
	{	
		EntityId m_entityId;
		float m_time;

		SAttackerData(EntityId entityId, float time)
		{
			m_entityId = entityId;
			m_time = time;
		}

	};
	typedef std::vector<SAttackerData>					TAttackers;
	typedef std::map<EntityId, TAttackers>			TPlayerAttackers;

	TPlayerAttackers m_playerAttackers;

public:
	CGameRulesAssistScoring();
	virtual ~CGameRulesAssistScoring();

	virtual void	Init(XmlNodeRef xml);

	virtual void	SvPlayerJoined(EntityId playerId);
	virtual void	SvPlayerLeft(EntityId playerId);
	virtual void  SvOnPlayerHit(const HitInfo &info);
//virtual void	SvOnPlayerKilled(const HitInfo &info);

//virtual void  ClAwardAssistKillPoints(EntityId victimId);
	virtual void  SvDoScoringForDeath(IActor *pActor, EntityId shooterId, const char *weaponClassName, int damage, int material, int hit_type);

	virtual EntityId	SvGetMostRecentAttacker(EntityId targetId);

	virtual void  SvBecomeServer();		// Host migration
};

#endif // __GAMERULESASSISTSCORING_H__