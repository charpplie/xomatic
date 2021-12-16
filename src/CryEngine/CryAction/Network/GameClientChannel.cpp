/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 11:8:2004   11:40 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "GameClientChannel.h"
#include "GameServerChannel.h"
#include "GameContext.h"
#include "CryAction.h"
#include "GameRulesSystem.h"
#include "GameObjects/GameObject.h"
#include "GameClientNub.h"
#include "ILevelSystem.h"
#include "IActorSystem.h"
#include "ActionGame.h"
#include "INetworkService.h"
#include "Network/DownloadTask.h"


#define LOCAL_ACTOR_VARIABLE "g_localActor"
#define LOCAL_CHANNELID_VARIABLE "g_localChannelId"
#define LOCAL_ACTORID_VARIABLE "g_localActorId"

CGameClientChannel::CGameClientChannel(INetChannel *pNetChannel, CGameContext *pContext, CGameClientNub * pNub)
: m_pNub(pNub),
	m_hasLoadedLevel(false)
{
	pNetChannel->SetClient(pContext->GetNetContext(), true);
	Init( pNetChannel, pContext );

	INetChannel::SPerformanceMetrics pm;
	if (!gEnv->bMultiplayer)
		pm.pPacketRateDesired = gEnv->pConsole->GetCVar("g_localPacketRate");
	else
		pm.pPacketRateDesired = gEnv->pConsole->GetCVar("cl_packetRate");
	pm.pBitRateDesired = gEnv->pConsole->GetCVar("cl_bandwidth");
	pNetChannel->SetPerformanceMetrics(&pm);

	CRY_ASSERT(pNetChannel);
	gEnv->pScriptSystem->SetGlobalToNull(LOCAL_ACTORID_VARIABLE);
	gEnv->pScriptSystem->SetGlobalToNull(LOCAL_ACTOR_VARIABLE);
	gEnv->pScriptSystem->SetGlobalToNull(LOCAL_CHANNELID_VARIABLE);

  CCryAction::GetCryAction()->OnActionEvent(SActionEvent(eAE_channelCreated,0));
}

CGameClientChannel::~CGameClientChannel()
{
  CCryAction::GetCryAction()->OnActionEvent(SActionEvent(eAE_channelDestroyed,0));
	m_pNub->ClientChannelClosed();

	gEnv->pScriptSystem->SetGlobalToNull(LOCAL_ACTORID_VARIABLE);
	gEnv->pScriptSystem->SetGlobalToNull(LOCAL_ACTOR_VARIABLE);
	gEnv->pScriptSystem->SetGlobalToNull(LOCAL_CHANNELID_VARIABLE);

	// If we're not migrating the host, restore the cached cvars.  If we *are*
	// migrating the host, we need to preserve the current cvar settings in order
	// to start the new server in the same manner.
	if (!gEnv->bHostMigrating)
	{
		for (std::map<string,string>::iterator it = m_originalCVarValues.begin(); it != m_originalCVarValues.end(); ++it)
		{
			if (ICVar * pVar = gEnv->pConsole->GetCVar( it->first.c_str() ))
			{
				if(it->second != pVar->GetString())
					CryLog("Restore cvar '%s' to '%s'", it->first.c_str(), it->second.c_str());
				int flags = pVar->GetFlags();
				pVar->SetFlags( flags | VF_NOT_NET_SYNCED_INTERNAL );
				pVar->ForceSet( it->second.c_str() );
				pVar->SetFlags( flags & ~VF_NOT_NET_SYNCED_INTERNAL );
			}
		}
	}
}

void CGameClientChannel::Release()
{
	delete this;
	CryLogAlways("CGameClientChannel::Release");
}

void CGameClientChannel::AddUpdateLevelLoaded( IContextEstablisher * pEst )
{
	if (!m_hasLoadedLevel)
		AddSetValue( pEst, eCVS_InGame, &m_hasLoadedLevel, true, "AllowChaining" );
}

bool CGameClientChannel::CheckLevelLoaded() const
{
	return m_hasLoadedLevel;
}

void CGameClientChannel::OnDisconnect(EDisconnectionCause cause, const char *description)
{
  //CryLogAlways("CGameClientChannel::OnDisconnect(%d, %s)", cause, description?description:"");
  CCryAction::GetCryAction()->OnActionEvent(SActionEvent(eAE_disconnected,int(cause),description));
  
  IGameRules *pGameRules = CCryAction::GetCryAction()->GetIGameRulesSystem()->GetCurrentGameRules();
	if (pGameRules)
		pGameRules->OnDisconnect(cause, description);

	if (IInput* pInput = gEnv->pInput)
	{
		pInput->EnableDevice(eDI_Keyboard, true);
		pInput->EnableDevice(eDI_Mouse, true);
	}
}

void CGameClientChannel::DefineProtocol(IProtocolBuilder *pBuilder)
{
	pBuilder->AddMessageSink(this, CGameServerChannel::GetProtocolDef(), CGameClientChannel::GetProtocolDef());
	CCryAction::GetCryAction()->GetIGameObjectSystem()->DefineProtocol( false, pBuilder );
	CCryAction::GetCryAction()->GetGameContext()->DefineContextProtocols(pBuilder, false);
	gEnv->pGame->ConfigureGameChannel(false, pBuilder);
}

// message implementation
NET_IMPLEMENT_SIMPLE_ATSYNC_MESSAGE(CGameClientChannel, RegisterEntityClass, eNRT_ReliableUnordered, eMPF_BlocksStateChange)
{
	return GetGameContext()->RegisterClassName( param.name, param.id );
}

NET_IMPLEMENT_SIMPLE_ATSYNC_MESSAGE(CGameClientChannel, SetGameType, eNRT_ReliableOrdered, eMPF_BlocksStateChange)
{
	string rulesClass;
	string levelName = param.levelName;
	if (!GetGameContext()->ClassNameFromId(rulesClass, param.rulesClass))
		return false;
	bool ok = true;
	if (!GetGameContext()->SetImmersive( param.immersive ))
		return false;
	if (!bFromDemoSystem)
	{
		CryLogAlways( "Game rules class: %s", rulesClass.c_str() );
		SGameContextParams params;
		params.levelName = levelName.c_str();
		params.gameRules = rulesClass.c_str();
		ok = GetGameContext()->ChangeContext( false, &params );
	}
	else
	{
		GetGameContext()->SetLevelName(levelName.c_str());
		GetGameContext()->SetGameRules(rulesClass.c_str());
    //
    IActor* pDemoPlayPlayer = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetOriginalDemoSpectator();
    // clear all entities slot - because render data will be released in LoadLevel and these releases are not clears entities slots data (FogVolume for example)
    CScopedRemoveObjectUnlock unlockRemovals(CCryAction::GetCryAction()->GetGameContext());
    IEntityItPtr i = gEnv->pEntitySystem->GetEntityIterator();
    while(!i->IsEnd())
    {
      IEntity* pEnt = i->Next();
      //
      IActor* pActor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(pEnt->GetId());
      if (pActor && pActor==pDemoPlayPlayer )
      {
        continue;
      }
      //
      pEnt->ClearFlags(ENTITY_FLAG_UNREMOVABLE);
      gEnv->pEntitySystem->RemoveEntity(pEnt->GetId(), true);
      //
    }
    //
		CCryAction::GetCryAction()->GetIGameRulesSystem()->CreateGameRules(rulesClass.c_str()); // workaround - since we don't do context establishment tasks when playing back demo
		ok = CCryAction::GetCryAction()->GetILevelSystem()->LoadLevel( levelName.c_str() ) != 0;
    //
    ILevelSystem * pLS = CCryAction::GetCryAction()->GetILevelSystem();
    if (pLS->GetCurrentLevel())
    {
      pLS->OnLoadingComplete(pLS->GetCurrentLevel());
    }
    if( pDemoPlayPlayer  )
    {
      gEnv->pGame->PlayerIdSet(pDemoPlayPlayer->GetEntityId());
      CCryAction::GetCryAction()->OnActionEvent(eAE_inGame);
    }
	}

	return ok;
}

NET_IMPLEMENT_SIMPLE_ATSYNC_MESSAGE(CGameClientChannel, ResetMap, eNRT_ReliableOrdered, eMPF_BlocksStateChange)
{
	GetGameContext()->ResetMap(false);
	return true;
}

NET_IMPLEMENT_IMMEDIATE_MESSAGE(CGameClientChannel, DefaultSpawn, eNRT_UnreliableOrdered, eMPF_BlocksStateChange | eMPF_DecodeInSync)
{
	bool bFromDemoSystem = nCurSeq == DEMO_PLAYBACK_SEQ_NUMBER && nOldSeq == DEMO_PLAYBACK_SEQ_NUMBER;

	SBasicSpawnParams param;
	param.SerializeWith(ser);

	string actorClass;
	if (!GetGameContext()->ClassNameFromId(actorClass, param.classId))
		return false;

	uint16 channelId = bFromDemoSystem ? ( param.nChannelId != 0 ? -1 : 0 ) : param.nChannelId;
	
	IGameObjectSystem::SEntitySpawnParamsForGameObjectWithPreactivatedExtension userData;
	userData.hookFunction = HookCreateActor;
	userData.pUserData = &channelId;

	IEntitySystem * pEntitySystem = gEnv->pEntitySystem;
	SEntitySpawnParams esp;
	esp.id = 0;

	esp.nFlags = param.flags;
	esp.pClass = pEntitySystem->GetClassRegistry()->FindClass( actorClass );
	if (!esp.pClass)
		return false;
	esp.pUserData = &userData;
	esp.qRotation = param.rotation;
	esp.sName = param.name.c_str();
	esp.vPosition	= param.pos;
	esp.vScale = param.scale;
	CCryAction::GetCryAction()->GetIGameObjectSystem()->SetSpawnSerializer(&ser);
	if (IEntity * pEntity = pEntitySystem->SpawnEntity( esp ))
	{
		CGameObject * pGameObject = (CGameObject*)CCryAction::GetCryAction()->GetIGameObjectSystem()->CreateGameObjectForEntity(pEntity->GetId());
		CRY_ASSERT(pGameObject);
		if (param.bClientActor)
		{
			IActor * pActor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor( pEntity->GetId() );
			if (!pActor)
			{
				pEntitySystem->RemoveEntity( pEntity->GetId() );
				CCryAction::GetCryAction()->GetIGameObjectSystem()->SetSpawnSerializer(NULL);
				pNetChannel->Disconnect(eDC_ContextCorruption, "Client actor spawned entity was not an actor");
				return false;
			}
			SetPlayerId( pGameObject->GetEntityId() );
		}
		GetGameContext()->GetNetContext()->SpawnedObject( pEntity->GetId() );
		CCryAction::GetCryAction()->GetIGameObjectSystem()->SetSpawnSerializer(NULL);
		pGameObject->PostRemoteSpawn();
		return true;
	}
	pNetChannel->Disconnect(eDC_ContextCorruption, "Failed to spawn entity");
	CCryAction::GetCryAction()->GetIGameObjectSystem()->SetSpawnSerializer(NULL);
	return false;
}

NET_IMPLEMENT_SIMPLE_ATSYNC_MESSAGE( CGameClientChannel, SetPlayerId_LocalOnly, eNRT_ReliableUnordered, eMPF_BlocksStateChange )
{
	if (!GetNetChannel()->IsLocal())
		return false;
	SetPlayerId( param.id );
	return true;
}

NET_IMPLEMENT_SIMPLE_ATSYNC_MESSAGE( CGameClientChannel, StartFileDownload, eNRT_ReliableUnordered, eMPF_BlocksStateChange )
{
	if(GetNetChannel()->IsLocal())
		return true;

	SFileDownloadParameters dl = param;

	if(CCryAction::GetCryAction()->GetDownloadTask())
		CCryAction::GetCryAction()->GetDownloadTask()->Cl_StartMapDownload(dl);
	
	return true;
}

NET_IMPLEMENT_SIMPLE_ATSYNC_MESSAGE( CGameClientChannel, SetConsoleVariable, eNRT_ReliableUnordered, eMPF_BlocksStateChange )
{
	if (GetNetChannel()->IsLocal() && !bFromDemoSystem)
		return false;

	return SetConsoleVar(param.key, param.value);
}

NET_IMPLEMENT_SIMPLE_ATSYNC_MESSAGE( CGameClientChannel, SetBatchConsoleVariables, eNRT_ReliableUnordered, eMPF_BlocksStateChange )
{
	if (GetNetChannel()->IsLocal() && !bFromDemoSystem)
		return false;

	bool res = true;

	for (int i = 0; i < param.actual && res; ++i)
		res = SetConsoleVar(param.vars[i].key,  param.vars[i].value);

	return res;
}

void CGameClientChannel::SetPlayerId( EntityId id )
{
	CGameChannel::SetPlayerId( id );

	CCryAction::GetCryAction()->GetGameContext()->PlayerIdSet(id);

	IScriptSystem * pSS = gEnv->pScriptSystem;

	if (id)
	{
		ScriptHandle hdl;
		hdl.n = GetPlayerId();
		pSS->SetGlobalValue( LOCAL_ACTORID_VARIABLE, hdl );
		IEntity * pEntity = gEnv->pEntitySystem->GetEntity(id);
		if (pEntity)
		{
			IGameObject *pGameObject=CCryAction::GetCryAction()->GetGameObject(id);
			pSS->SetGlobalValue( LOCAL_ACTOR_VARIABLE, pEntity->GetScriptTable() );
			pSS->SetGlobalValue( LOCAL_CHANNELID_VARIABLE, pGameObject ? pGameObject->GetChannelId() : 0 );
		}
		else
		{
			pSS->SetGlobalToNull( LOCAL_ACTOR_VARIABLE );
			pSS->SetGlobalToNull( LOCAL_CHANNELID_VARIABLE );
		}

		CallOnSetPlayerId();
	}
	else
	{
		gEnv->pScriptSystem->SetGlobalToNull( LOCAL_ACTORID_VARIABLE );
		gEnv->pScriptSystem->SetGlobalToNull( LOCAL_ACTOR_VARIABLE );
		gEnv->pScriptSystem->SetGlobalToNull( LOCAL_CHANNELID_VARIABLE );
	}

	if (gEnv->pGame)
		gEnv->pGame->PlayerIdSet(id);
}

void CGameClientChannel::CallOnSetPlayerId()
{
	IEntity	*pPlayer = gEnv->pEntitySystem->GetEntity(GetPlayerId());
	if (!pPlayer)
		return;

	IScriptTable *pScriptTable = pPlayer->GetScriptTable();
	if (!pScriptTable)
		return;
	
	SmartScriptTable client;
	if (pScriptTable->GetValue("Client", client))
	{
		if (pScriptTable->GetScriptSystem()->BeginCall(client, "OnSetPlayerId"))
		{
			pScriptTable->GetScriptSystem()->PushFuncParam(pScriptTable);
			pScriptTable->GetScriptSystem()->EndCall();
		}
	}

	IActor* pActor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(GetPlayerId());
	CRY_ASSERT(pActor);

	pActor->InitLocalPlayer();
}

bool CGameClientChannel::HookCreateActor( IEntity *pEntity, IGameObject *pGameObject, void *pUserData )
{
	//[AlexMcC|23.11.09]: Set the ChannelId for remote obejcts at the same time as local objects
	// (which is very early). Setting the ChannelId early like this means that we can trust
	// IActor::IsPlayer() very early, such as in Player::Init().
	// Copied from CActorSystem.cpp
	if (pGameObject)
	{
		pGameObject->SetChannelId( *static_cast<uint16*>(pUserData) );
	}
	return true;
}
bool CGameClientChannel::SetConsoleVar(const string& key, const string& val)
{
	IConsole * pConsole = gEnv->pConsole;
	ICVar * pVar = pConsole->GetCVar( key.c_str() );
	if (!pVar)
	{
		CryLog("Server sets console variable '%s' to '%s'", key.c_str(), val.c_str());
		CryLog("   cvar not found, ignoring");
		return true;
	}
	int flags = pVar->GetFlags();
	if (flags & VF_NOT_NET_SYNCED_INTERNAL)
	{
		CryLog("Server sets console variable '%s' to '%s'", key.c_str(), val.c_str());
		CryLog("   cvar not synchronized, disconnecting");
		return false;
	}

	if(val != pVar->GetString())
		CryLog("Server sets console variable '%s' to '%s'", key.c_str(), val.c_str());

	std::map<string, string>::iterator orit = m_originalCVarValues.lower_bound(key);
	if (orit == m_originalCVarValues.end() || orit->first != key)
		m_originalCVarValues.insert( std::make_pair(key, pVar->GetString()) );

	pVar->SetFlags( flags | VF_NOT_NET_SYNCED_INTERNAL );
	pVar->ForceSet( val.c_str() );
	pVar->SetFlags( flags & ~VF_NOT_NET_SYNCED_INTERNAL );

	return true;
}
