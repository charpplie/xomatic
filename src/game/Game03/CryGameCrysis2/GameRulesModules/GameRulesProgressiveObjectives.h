/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Game rules module for dealing with modes that have objectives that 
		change (completing objective 1 starts objective 2)
	-------------------------------------------------------------------------
	History:
	- 21:09:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_PROGRESSIVE_OBJECTIVES_H_
#define _GAME_RULES_PROGRESSIVE_OBJECTIVES_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameRulesObjectivesModule.h"
#include "IGameRulesObjective.h"

class CGameRulesProgressiveObjectives : public IGameRulesObjectivesModule,
																				public IGameFrameworkListener
{
public:
	CGameRulesProgressiveObjectives();
	virtual ~CGameRulesProgressiveObjectives();

	// IGameRulesObjectivesModule
	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();
	virtual void OnGameReset();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual bool HasCompleted(int teamId);

	virtual void OnHostMigration(bool becomeServer);		// Host migration
	// ~IGameRulesObjectivesModule

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime) {}
	virtual void OnSaveGame(ISaveGame* pSaveGame) {}
	virtual void OnLoadGame(ILoadGame* pLoadGame) {}
	virtual void OnLevelEnd(const char* pNextLevel) {}
	virtual void OnActionEvent(const SActionEvent& event);
	// ~IGameFrameworkListener

protected:
	enum EStageConditionType
	{
		ESCT_None,
		ESCT_TeamScore,
		ESCT_Completed,
		ESCT_Time,
	};

	union UStageExitCondition
	{
		struct 
		{
			int m_requiredScore;
		} m_score;
	};

	typedef std::vector<IGameRulesObjective*> TObjectivesVector;

	struct SObjectivesStage
	{
		TObjectivesVector m_objectives;
		UStageExitCondition m_stageExitCondition;
		EStageConditionType m_stageExitConditionType;
	};

	typedef std::vector<SObjectivesStage> TStagesVector;
	typedef std::map<int, int> TTeamStageMap;
	typedef std::map<EntityId, int> TPlayerStageMap;


	void DoStartGame(bool isReset);
	IGameRulesObjective *ReadObjective(XmlNodeRef xml);
	bool ReadExitType(XmlNodeRef xml, EStageConditionType &outType, UStageExitCondition &outCondition);
	static void SetObjectivesEnabled(TObjectivesVector &pObjectivesVec, int teamId, bool enabled);
	void ChangeStage(int teamId, int newStageIdx);

	TStagesVector m_stages;
	TTeamStageMap m_teamStages;
	TPlayerStageMap m_playerStages;
	bool m_teamBased;
	bool m_isResetting;

	bool m_finished[2];
};

#endif // _GAME_RULES_PROGRESSIVE_OBJECTIVES_H_
