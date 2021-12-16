/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
Description: 
This file holds shared data and functionality across all actor using the 
HitDeathReactions system
-------------------------------------------------------------------------
History:
- 22:2:2010	13:01 : Created by David Ramos
*************************************************************************/
#pragma once
#ifndef __HIT_DEATH_REACTIONS_SYSTEM_H
#define __HIT_DEATH_REACTIONS_SYSTEM_H

#include "HitDeathReactionsDefs.h"
#include <VectorMap.h>

class CActor;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CHitDeathReactionsSystem
{
public:
	static void		Warning(const char* szFormat, ...);


	CHitDeathReactionsSystem();

	bool					GetReactionParamsForActor(const CActor& actor, ReactionsContainerConstPtr& pHitReactions, ReactionsContainerConstPtr& pDeathReactions, ReactionsContainerConstPtr& pCollisionReactions);
	void					Reload();

private:
	// private types
	struct SSharedReactions
	{
		SSharedReactions() : pHitReactions(NULL), pDeathReactions(NULL) {}
		SSharedReactions(ReactionsContainerConstPtr pHitReactions, ReactionsContainerConstPtr pDeathReactions, ReactionsContainerConstPtr pCollisionReactions, ScriptTablePtr pHitAndDeathReactionsTable) : 
		pHitReactions(pHitReactions), pDeathReactions(pDeathReactions), pCollisionReactions(pCollisionReactions), pHitAndDeathReactionsTable(pHitAndDeathReactionsTable) {}

		ScriptTablePtr							pHitAndDeathReactionsTable;
		ReactionsContainerConstPtr	pHitReactions;
		ReactionsContainerConstPtr	pDeathReactions;
		ReactionsContainerConstPtr	pCollisionReactions;
	};

	typedef uint32																					ReactionsParamsKey;
	typedef VectorMap<ReactionsParamsKey, SSharedReactions> ReactionsParamsContainers;
	typedef ReactionsParamsContainers::value_type						ReactionsParamsContainersItem;

	enum
	{
		INVALID_REACTION_PARAMS_ID = 0xFFFFFFFF,
	};


	// Private methods
	void								ExecuteHitDeathReactionsScripts(bool bForceReload);
	ReactionsParamsKey	GetKey(const CActor& actor) const;
	ScriptTablePtr			LoadReactionsScriptTable(const CActor& actor) const;
	bool								LoadHitDeathReactionsParams(const CActor& actor, ScriptTablePtr pHitDeathReactionsTable, ReactionsContainerPtr pHitReactions, ReactionsContainerPtr pDeathReactions, ReactionsContainerPtr pCollisionReactions);
	void								LoadReactionsParams(const CActor& actor, IScriptTable* pHitDeathReactionsTable, const char* szReactionParamsName, bool bDeathReactions, ReactionId baseReactionId, ReactionsContainer& reactions);
	void								GetReactionParamsFromScript(const CActor& actor, const ScriptTablePtr pScriptTable, SReactionParams& reactionParams) const;
	void								GetReactionAnimParamsFromScript(const CActor& actor, ScriptTablePtr pScriptTable, SReactionParams::SReactionAnim& reactionAnim) const;
	void								PreProcessReactionParams(SmartScriptTable pReactionTable);
	void								FillAllowedPartIds(const CActor& actor, const ScriptTablePtr pScriptTable, SReactionParams& reactionParams) const;
	ECardinalDirection	GetCardinalDirectionFromString(const char* szCardinalDirection) const;

	ReactionsParamsContainers m_reactionParamsPool;
};

#endif // __HIT_DEATH_REACTIONS_SYSTEM_H