/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:
Groups all methods related to accessing 'mind control' through the console.

-------------------------------------------------------------------------
History:
- 3:11:2009: Created by Sven Van Soom

*************************************************************************/
#include "StdAfx.h"
#include "MindControlConsole.h"

#if ENABLE_MINDCONTROL

#include "Player.h"

const char* szMc_entity = "mc_entity";

// ---------------------------------------------------------------------------
// TODO: Found this in (of all places) EditorUtils.h
// Should be moved into CryCommon/StringUtils.h if deemed
// worthy.
static const char* strstri( const char *s1, const char *s2) 
{
	int i,j,k; 
	for(i=0;s1[i];i++) 
		for(j=i,k=0;tolower(s1[j])==tolower(s2[k]);j++,k++) 
			if(!s2[k+1]) 
				return (s1+i); 

	return NULL; 
}



// ---------------------------------------------------------------------------
// Controls specified entity
// When entityId is zero or the Dude's entity it will stop mind-controlling
/*static*/ void CMindControlConsole::ControlEntity(uint32 entityId)
{
	CPlayer *pClient = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	CRY_ASSERT(pClient);
	if (!pClient)
		return;

	CMindControlMaster* pMaster = pClient->GetMindControlMaster();
	CRY_ASSERT(pMaster);
	if (!pMaster)
		return;

	if (entityId == 0)
	{
		// Detach
		if (pMaster->IsBusy())
		{
			CryLog("MindControl: Detaching");
			pMaster->Leave();
		}
	}
	else
	{
		// Attach (or just detach when the entity specific IS the client
		IActor* pSlaveActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entityId);
		if (!pSlaveActor)
			return;

		//CRY_ASSERT(!strcmp(pSlaveActor->GetActorClass(),"CPlayer"));

		// Don't re-attach
		if (pMaster->IsBusy() && (pMaster->GetSlaveMind()->GetPlayer() == pSlaveActor))
			return;

		if (pMaster->IsBusy())
		{
			CryLog("MindControl: Detaching");
			pMaster->Leave();
		}

		if (pSlaveActor != pClient)
		{
			CPlayer* pSlavePlayer = static_cast<CPlayer*>(pSlaveActor);
			CMindControlSlave* pSlave = pSlavePlayer->GetMindControlSlave();
			if (!pSlave)
				return;

			CRY_ASSERT(pMaster->CanExecuteOn(pSlavePlayer));
			if (!pMaster->CanExecuteOn(pSlavePlayer))
				return;

			CryLog("MindControl: Attaching to '%s'", gEnv->pEntitySystem->GetEntity(entityId)->GetName());
			pMaster->Enter(pSlave);
		}
	}
}


// ---------------------------------------------------------------------------
// Finds a player with a name that contains szPartialName.
// The search prefers matches at the front of the name, and in case of a tie the 'tighter' fit or first fit is chosen.
/*static*/ IEntity* CMindControlConsole::FindPlayerEntityByPartialName(const char* szPartialName)
{
	CRY_ASSERT(szPartialName);
	if (!szPartialName)
		return NULL;

	// First try a complete name match
	IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName(szPartialName);
	if (pEntity)
		return pEntity;

	IEntityIt* itEntity = gEnv->pEntitySystem->GetEntityIterator();
	if (!itEntity)
		return NULL;

	// Iterate through all entities and look for the CPlayer that has the best name match
	size_t firstMatchPosition = std::numeric_limits<size_t>::max();
	size_t bestNameLen = std::numeric_limits<size_t>::max();
	IEntity* pBestEntity = NULL;

	itEntity->MoveFirst();
	while (!itEntity ->IsEnd())
	{
		pEntity = itEntity->Next();
		if (!pEntity)
			continue;

		IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());	
		if (!pActor)
			continue;

		if (pActor->GetActorClass() != CPlayer::GetActorClassType()) // we only want CPlayer classes
			continue;

		// We could do more filtering here, for example filtering out only the client
		// and controllable AI actors

		const char* szEntityName = pEntity->GetName();
		size_t nameLen = strlen(szEntityName);

		if (nameLen == 0)
			continue;

		const char* pMatch = ::strstri(szEntityName, szPartialName);
		if (pMatch != NULL)
		{
			size_t matchPosition = pMatch - szEntityName;
			if ((matchPosition < firstMatchPosition)
				||
				((matchPosition == firstMatchPosition) && (nameLen < bestNameLen)))
			{
				pBestEntity = pEntity;

				firstMatchPosition = matchPosition;
				bestNameLen = nameLen;
			}
		}
	}

	return pBestEntity;
}


// ---------------------------------------------------------------------------
/*static*/ void CMindControlConsole::OnChangeMindControlEntityCVar(ICVar* pICVar)
{
	if (gEnv->bEditor && !gEnv->bEditorGameMode)
	{
		GameWarning("Note: You need to go to Game Mode to control minds.");
		return;
	}

	ChangeMindControlEntity( pICVar->GetString() );
}

// ---------------------------------------------------------------------------
/*static*/ void CMindControlConsole::ChangeMindControlEntity(const char* szEntityName)
{
	CRY_ASSERT(szEntityName);
	if (!szEntityName)
		return;

	if (strlen(szEntityName) > 0)
	{
		uint32 entityId = 0;
		if (0 == strcmp(szEntityName, "0")) // for people that are used to the "0" console variable to disable ag_debug
		{
			ControlEntity(0);
		}
		else if (IEntity * pEntity = FindPlayerEntityByPartialName(szEntityName))
		{
			ControlEntity(pEntity->GetId());
		}
		else
			GameWarning("Entity '%s' not found.", szEntityName);
	}
	else
	{
		// Detach
		ControlEntity(0);
	}
}

// ---------------------------------------------------------------------------
/*static*/ void CMindControlConsole::InitCVars(IConsole *pConsole)
{
	REGISTER_STRING(szMc_entity, "", VF_CHEAT, "Entity to control mind of (partial name is also allowed)")->SetOnChangeCallback(OnChangeMindControlEntityCVar);
}


// ---------------------------------------------------------------------------
/*static*/ void CMindControlConsole::ReleaseCVars(IConsole *pConsole)
{
	pConsole->UnregisterVariable(szMc_entity, true);
}


// ---------------------------------------------------------------------------
/*static*/ void CMindControlConsole::OnStartGame()
{
	if (!gEnv->bEditor)
		return;

	IConsole* pConsole = gEnv->pConsole;

	ICVar* pVar = pConsole->GetCVar(szMc_entity);
	CRY_ASSERT(pVar);
	if (!pVar)
		return;

	ChangeMindControlEntity( pVar->GetString() );
}

#endif
