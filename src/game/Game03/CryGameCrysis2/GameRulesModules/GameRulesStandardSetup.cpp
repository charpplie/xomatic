/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 03:09:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesStandardSetup.h"
#include "IXml.h"
#include "GameRules.h"
#include "Player.h"
#include "HUD/HUD.h"
#include "Item.h"
#include "GameRulesModules/IGameRulesTeamsModule.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"
#include "GameRulesModules/IGameRulesRoundsModule.h"
#include "EquipmentLoadout.h"

//------------------------------------------------------------------------
CGameRulesStandardSetup::CGameRulesStandardSetup()
{
	m_pGameRules = NULL;
	m_itemAbandonedTimerLength = 45.f;		// Default to removing after 45 seconds
	m_numIgnoreItems = 0;
	memset(m_itemRemoveIgnoreClasses, 0, sizeof(IEntityClass*) * MAX_IGNORE_REMOVE_ITEM_CLASSES);
}

//------------------------------------------------------------------------
CGameRulesStandardSetup::~CGameRulesStandardSetup()
{
	if (gEnv->bServer)
	{
		m_pGameRules->UnRegisterPickupListener(this);
	}

	if (m_usesTeamSpecifics)
	{
		if (gEnv->bClient)
		{
			CallLuaFunc(&m_luaResetPlayerTeamSpecificsFunc);

			m_pGameRules->UnRegisterRoundsListener(this);
			m_pGameRules->UnRegisterTeamChangedListener(this);
			m_pGameRules->UnRegisterRevivedListener(this);
			if (gEnv->bServer)  // ie. not dedicated server
			{
				m_pGameRules->UnRegisterClientConnectionListener(this);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesStandardSetup::Init( XmlNodeRef xml )
{
	m_pGameRules = g_pGame->GetGameRules();
	CRY_ASSERT(m_pGameRules);

	m_usesTeamSpecifics = false;

	int numChildren = xml->getChildCount();
	for (int i = 0; i < numChildren; ++ i)
	{
		XmlNodeRef xmlChild = xml->getChild(i);

		if (!stricmp(xmlChild->getTag(), "RemoveItemsOnDrop"))
		{
			xmlChild->getAttr("time", m_itemAbandonedTimerLength);

			m_numIgnoreItems = xmlChild->getChildCount();
			if (m_numIgnoreItems >= MAX_IGNORE_REMOVE_ITEM_CLASSES)
			{
				CRY_ASSERT_MESSAGE(false, "Too many ignore item classes");
				m_numIgnoreItems = MAX_IGNORE_REMOVE_ITEM_CLASSES - 1;
			}

			for (int j = 0; j < m_numIgnoreItems; ++ j)
			{
				XmlNodeRef xmlIgnore = xmlChild->getChild(j);
				if (!stricmp(xmlIgnore->getTag(), "IgnoreEntity"))
				{
					const char *pClassName = 0;
					if (xmlIgnore->getAttr("class", &pClassName))
					{
						const IEntityClass *pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pClassName);
						CRY_ASSERT(pClass);
						m_itemRemoveIgnoreClasses[j] = pClass;
					}
				}
			}
		}
		else if (!stricmp(xmlChild->getTag(), "TeamSpecifics"))
		{
			m_usesTeamSpecifics = true;

			m_luaSetupPlayerTeamSpecificsFunc.Format("%s", xmlChild->getAttr("luaSetupPlayerTeamSpecificsFunc"));
			m_luaResetPlayerTeamSpecificsFunc.Format("%s", xmlChild->getAttr("luaResetPlayerTeamSpecificsFunc"));
			m_luaEquipTeamSpecificsFunc.Format("%s", xmlChild->getAttr("luaEquipTeamSpecificsFunc"));
		}
	}

	if (gEnv->bServer)
	{
		m_pGameRules->RegisterPickupListener(this);
	}
	if (m_usesTeamSpecifics)
	{
		if (gEnv->bClient)
		{
			m_pGameRules->RegisterRoundsListener(this);
			m_pGameRules->RegisterTeamChangedListener(this);
			m_pGameRules->RegisterRevivedListener(this);
			if (gEnv->bServer)  // ie. not dedicated server
			{
				m_pGameRules->RegisterClientConnectionListener(this);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesStandardSetup::PostInit()
{

}

//------------------------------------------------------------------------
void CGameRulesStandardSetup::OnClientConnect(int channelId, bool isReset, const char* playerName)
{
	Vec3 pos(1.f, 1.f, 1.f);		// Default to 1,1,1 because rendering doesn't start until the camera leaves 0,0,0
	Ang3 angles(0.f, 0.f, 0.f);

	IGameRulesSpectatorModule*  specmod = m_pGameRules->GetSpectatorModule();

	IActor *pActor = m_pGameRules->SpawnPlayer(channelId, playerName, "Player", pos, angles);
	if (pActor)
	{
		if (specmod)
		{
			EntityId  spectatorPointId = specmod->GetInterestingSpectatorLocation();
			if (spectatorPointId)
			{
				specmod->ChangeSpectatorMode(pActor, CActor::eASM_Fixed, spectatorPointId, false);
			}
			else
			{
				specmod->ChangeSpectatorMode(pActor, CActor::eASM_Free, 0, false);
			}
		}

		if (!isReset)
		{
			IGameRulesTeamsModule *teamModule = m_pGameRules->GetTeamsModule();
			if (teamModule)
			{
				EntityId playerEntityId = pActor->GetEntityId();
				m_pGameRules->SetTeam(teamModule->GetAutoAssignTeamId(playerEntityId), playerEntityId);
			}
		}
			
		// Setup ammo capacities
		CActor *pPlayer = static_cast<CActor*>(pActor);
		if (pPlayer)
		{
			IInventory *pInventory = pPlayer->GetInventory();
			if (pInventory)
			{
				CRY_TODO(07, 09, 2009, "Move ammo capacities into xml");
				SetAmmoCapacity(pInventory, "bullet", 30*8);
				SetAmmoCapacity(pInventory, "fybullet", 30*4);
				SetAmmoCapacity(pInventory, "lightbullet", 20*8);
				SetAmmoCapacity(pInventory, "smgbullet", 2);
				SetAmmoCapacity(pInventory, "explosivegrenade", 2);
				SetAmmoCapacity(pInventory, "flashbang", 2);
				SetAmmoCapacity(pInventory, "smokegrenade", 2);
				SetAmmoCapacity(pInventory, "empgrenade", 3);
				SetAmmoCapacity(pInventory, "scargrenade", 3);
				SetAmmoCapacity(pInventory, "sniperbullet", 10*4);
				SetAmmoCapacity(pInventory, "tacbullet", 5*4);
				SetAmmoCapacity(pInventory, "tagbullet", 10);
				SetAmmoCapacity(pInventory, "gaussbullet", 5*4);
				SetAmmoCapacity(pInventory, "hurricanebullet", 500*2);
				SetAmmoCapacity(pInventory, "incendiarybullet", 30*4);
				SetAmmoCapacity(pInventory, "shotgunshell", 8*8);
				SetAmmoCapacity(pInventory, "avexplosive", 2);
				SetAmmoCapacity(pInventory, "c4explosive", 2);
				SetAmmoCapacity(pInventory, "claymoreexplosive", 2);
				SetAmmoCapacity(pInventory, "rubberbullet", 30*4);
				SetAmmoCapacity(pInventory, "tacgunprojectile", 1);
				SetAmmoCapacity(pInventory, "fgl40fraggrenade", 6);
				SetAmmoCapacity(pInventory, "fgl40empgrenade", 6);
				SetAmmoCapacity(pInventory, "aybullet", 160);
			}
		}
	}
	else
	{
		CryLogAlways("CGameRulesStandardSetup::OnClientConnect(), failed to create player for channel %i (isReset=%i)", channelId, (isReset ? 1 : 0));
	}
}

//------------------------------------------------------------------------
void CGameRulesStandardSetup::OnPlayerRevived(EntityId playerId)
{
	CActor *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));
	if (pActor)
	{
		CRY_ASSERT_MESSAGE(pActor->IsPlayer(), "Actor that is being spawned is not a player!");
		CPlayer *pPlayer = static_cast<CPlayer*>(pActor);

		pActor->GetInventory()->Destroy();

		bool  luaEquip = (m_usesTeamSpecifics && !m_luaEquipTeamSpecificsFunc.empty());

		CEquipmentLoadout *equipmentLoadout = m_pGameRules->GetEquipmentLoadout();
		if (equipmentLoadout && equipmentLoadout->SvHasClientEquipmentLoadout(pActor->GetChannelId()))
		{
			equipmentLoadout->SvAssignClientEquipmentLoadout(pActor->GetChannelId(), playerId);
		}
		else if (!luaEquip)	// Give a default as no loadout is set yet, and it's not gonna be set by lua either
		{
			IItemSystem * pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();

			pItemSystem->GiveItem(pActor, "DrillerAttachment", false, true, true);
			pItemSystem->GiveItem(pActor, "FlashBangGrenades", false, true, true);
			pItemSystem->GiveItem(pActor, "FragGrenades", false, true, true);
			pItemSystem->GiveItem(pActor, "Revolver", false, true, true);

			EntityId weaponId = pItemSystem->GiveItem(pActor, "SCAR", false, true, true);

			CItem * pWeaponItem = static_cast<CItem*>(pItemSystem->GetItem(weaponId));
			CRY_ASSERT_MESSAGE(pWeaponItem, "Failed to find CItem for SCAR");

			pPlayer->EquipAttachmentsOnCarriedWeapons();
		}

		if (luaEquip)
		{
			CallLuaFunc1e(&m_luaEquipTeamSpecificsFunc, playerId);
			//pPlayer->EquipAttachmentsOnCarriedWeapons();
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesStandardSetup::SetupPlayer(EntityId playerId)
{
	if (g_pGame->GetIGameFramework()->GetClientActorId() == playerId)
	{
		SAFE_HUD_FUNC(ShowPDA(true, false));
	}
}

//------------------------------------------------------------------------
void CGameRulesStandardSetup::SetAmmoCapacity( IInventory *pInventory, const char *pAmmoClass, int amount )
{
	IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pAmmoClass);
	if (pClass)
	{
		pInventory->SetAmmoCapacity(pClass, amount);
	}
	else
	{
		CryLog("CGameRulesStandardSetup::SetAmmoCapacity(), failed to find ammo class '%s'", pAmmoClass);
	}
}

//------------------------------------------------------------------------
bool CGameRulesStandardSetup::IsInIgnoreItemTypeList(EntityId itemId)
{
	bool found = false;

	IItem *pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(itemId);
	if (pItem)
	{
		const IEntityClass *pItemClass = pItem->GetEntity()->GetClass();
		for (int i = 0; i < m_numIgnoreItems; ++ i)
		{
			if (m_itemRemoveIgnoreClasses[i] == pItemClass)
			{
				found = true;
				break;
			}
		}
	}

	return found;
}

//------------------------------------------------------------------------
void CGameRulesStandardSetup::OnItemPickedUp( EntityId itemId, EntityId actorId )
{
	if (!IsInIgnoreItemTypeList(itemId))
	{
		m_pGameRules->AbortEntityRemoval(itemId);
	}
}

//------------------------------------------------------------------------
void CGameRulesStandardSetup::OnItemDropped( EntityId itemId, EntityId actorId )
{
	if (!IsInIgnoreItemTypeList(itemId))
	{
		m_pGameRules->ScheduleEntityRemoval(itemId, m_itemAbandonedTimerLength, false);
	}
}

//------------------------------------------------------------------------
void CGameRulesStandardSetup::SvOnStartNewRound(bool isReset)
{
	CryLog("[tlh] @ CGameRulesStandardSetup::SvOnStartNewRound(isReset=%d)", isReset);
	CRY_ASSERT(gEnv->bServer);
	if (gEnv->bClient)
	{
		if (m_usesTeamSpecifics)
		{
			if (g_pGame->GetIGameFramework()->GetClientActorId())
			{
				CallLuaFunc(&m_luaSetupPlayerTeamSpecificsFunc);
			}
		}
	}
}

//------------------------------------------------------------------------
// IGameRulesRoundsListener
void CGameRulesStandardSetup::ClRoundsNetSerializeReadState(int newState, int curState)
{
	CryLog("[tlh] @ CGameRulesStandardSetup::ClRoundsNetSerializeReadState(newState=%d, curState=%d)", newState, curState);
	CRY_ASSERT(!gEnv->bServer);
	CRY_ASSERT(gEnv->bClient);
	if (m_usesTeamSpecifics)
	{
		CRY_ASSERT(g_pGame->GetIGameFramework()->GetClientActorId());
		IGameRulesRoundsModule*  pRoundsModule = m_pGameRules->GetRoundsModule();
		CRY_ASSERT(pRoundsModule);
		if (pRoundsModule->IsRestartingRound(curState))
		{
			CallLuaFunc(&m_luaSetupPlayerTeamSpecificsFunc);
		}
	}
}

//------------------------------------------------------------------------
// IGameRulesTeamChangedListener
void CGameRulesStandardSetup::OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId)
{
	CryLog("CGameRulesStandardSetup::OnChangedTeam(entityId=%d, oldTeamId=%d, newTeamId=%d)", entityId, oldTeamId, newTeamId);
	CRY_ASSERT(gEnv->bClient);
	if (m_usesTeamSpecifics)
	{
		if (entityId == g_pGame->GetIGameFramework()->GetClientActorId())
		{
			CallLuaFunc(&m_luaSetupPlayerTeamSpecificsFunc);
		}
	}
}

//------------------------------------------------------------------------
// IGameRulesRevivedListener
// [tlh] NOTE this doesn't seem to get called for server player on game start if server player not already joined game (left spectator mode) when starts
void CGameRulesStandardSetup::EntityRevived(EntityId entityId)
{
	CryLog("[tlh] @ CGameRulesStandardSetup::EntityRevived(entityId=%d)", entityId);
	CRY_ASSERT(gEnv->bClient);
	if (m_usesTeamSpecifics)
	{
		if (entityId == g_pGame->GetIGameFramework()->GetClientActorId())
		{
			bool  call = true;
			if (gEnv->bServer)
			{
				IGameRulesRoundsModule*  pRoundsModule = m_pGameRules->GetRoundsModule();
				call = (!pRoundsModule || !pRoundsModule->IsRestarting());  // ie. don't do this on server if round restarting, because for the server's client the lua func will get called after the restart anyway
			}
			if (call)
			{
				CallLuaFunc(&m_luaSetupPlayerTeamSpecificsFunc);
			}
		}
	}
}

//------------------------------------------------------------------------
// IGameRulesClientConnectionListener
void CGameRulesStandardSetup::OnOwnClientEnteredGame()
{
	CryLog("[tlh] @ CGameRulesStandardSetup::OnOwnClientEnteredGame()");
	CRY_ASSERT(gEnv->bServer);
	CRY_ASSERT(gEnv->bClient);
	CRY_ASSERT(g_pGame->GetIGameFramework()->GetClientActorId());
	if (m_usesTeamSpecifics)
	{
		CallLuaFunc(&m_luaSetupPlayerTeamSpecificsFunc);
	}
}

//------------------------------------------------------------------------
void CGameRulesStandardSetup::CallLuaFunc(TFixedString* funcName)
{
	if (funcName && !funcName->empty())
	{
		IScriptTable*  pTable = m_pGameRules->GetEntity()->GetScriptTable();
		HSCRIPTFUNCTION  func;
		if (pTable && pTable->GetValue(funcName->c_str(), func))
		{
			IScriptSystem*  pScriptSystem = gEnv->pScriptSystem;
			Script::Call(pScriptSystem, func, pTable);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesStandardSetup::CallLuaFunc1e(TFixedString* funcName, EntityId e)
{
	if (funcName && !funcName->empty())
	{
		IScriptTable*  pTable = m_pGameRules->GetEntity()->GetScriptTable();
		HSCRIPTFUNCTION  func;
		if (pTable && pTable->GetValue(funcName->c_str(), func))
		{
			IScriptSystem*  pScriptSystem = gEnv->pScriptSystem;
			Script::Call(pScriptSystem, func, pTable, ScriptHandle(e));
		}
	}
}
