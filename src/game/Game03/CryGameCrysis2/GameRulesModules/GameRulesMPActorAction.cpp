/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
	
	-------------------------------------------------------------------------
	History:
	- 07:09:2009  : Created by Ben Johnson

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesMPActorAction.h"
#include "IXml.h"
#include "GameRules.h"
#include "Actor.h"
#include "Player.h"
#include "GameActions.h"
#include "IGameRulesSpawningModule.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "HUD/HUD.h"

#include "RecordingSystem.h"

CGameRulesMPActorAction::CGameRulesMPActorAction()
{
}

CGameRulesMPActorAction::~CGameRulesMPActorAction()
{
}

void CGameRulesMPActorAction::Init( XmlNodeRef xml )
{
}

void CGameRulesMPActorAction::PostInit()
{
}

void CGameRulesMPActorAction::OnActorAction(IActor *pActor, const ActionId& actionId, int activationMode, float value)
{
	CActor *pActorImpl = static_cast<CActor *>(pActor);
	if (pActorImpl)
	{
		EntityId  pid = pActor->GetEntityId();  // player id

		CGameRules *pGameRules = g_pGame->GetGameRules();

		if (actionId == g_pGame->Actions().hud_toggle_mp_select_menu && activationMode == eAAM_OnPress)
		{
			SHUDEvent hudEvent(eHUDEvent_ToggleSelectMenu);
			CHUD::CallEvent(hudEvent);
		}

		IGameRulesSpectatorModule *specmod = pGameRules->GetSpectatorModule();

		if (!specmod || (pActorImpl->GetSpectatorMode() <= 0))
		{
			// Not in spectator mode

			if (pActorImpl->GetHealth() <= 0)
			{
				CRecordingSystem *crs = g_pGame->GetRecordingSystem();

				if (crs && crs->IsPlayingBack())
				{
					if (actionId == g_pGame->Actions().spectate_gen_spawn)
						crs->StopPlayback();
				}
				else if ((actionId == g_pGame->Actions().spectate_gen_spawn || actionId == g_pGame->Actions().hud_mouseclick) && activationMode == eAAM_OnPress)
				{
					if (IGameRulesSpawningModule* pSpawningModule=pGameRules->GetSpawningModule())
					{
						IGameRulesStateModule*  stateModule = pGameRules->GetStateModule();
						if (!stateModule || (stateModule->GetGameState() != IGameRulesStateModule::EGRS_PostGame))
						{
							CryLog("CGameRulesMPActorAction::OnActorAction() Requesting revive");
							pSpawningModule->ClRequestRevive(pActor->GetEntityId());
						}
					}
				}
				else if ((specmod && specmod->CanChangeSpectatorMode(pid)) && (((actionId == g_pGame->Actions().spectate_gen_nextmode) || (actionId == g_pGame->Actions().spectate_gen_prevmode)) && (activationMode == eAAM_OnPress)))  // get into spectate mode
				{
					if (!crs || crs->IsStopped())
					{
						specmod->ChangeSpectatorModeBestAvailable(pActor, false);
					}
				}
			}
		}
		else
		{
			// is spectating

			int  curspecmode = pActorImpl->GetSpectatorMode();

			// actions general across all spectator modes

			if ((actionId == g_pGame->Actions().spectate_gen_spawn) && (activationMode == eAAM_OnPress))
			{
				IGameRulesSpawningModule *pSpawningModule = pGameRules->GetSpawningModule();
				if (pSpawningModule)
				{
					IGameRulesStateModule*  stateModule = pGameRules->GetStateModule();
					if (!stateModule || (stateModule->GetGameState() != IGameRulesStateModule::EGRS_PostGame))
					{
						CryLog("CGameRulesMPActorAction::OnActorAction() Spectating, received spectate_gen_spawn action, requesting revive");
						pSpawningModule->ClRequestRevive(pActor->GetEntityId());
					}			
				}			
			}
			else if (((actionId == g_pGame->Actions().spectate_gen_nextmode) || (actionId == g_pGame->Actions().spectate_gen_prevmode)) && (activationMode == eAAM_OnPress))
			{
				CryLog("[tlh] > changemode button pressed");
				if (specmod->CanChangeSpectatorMode(pid))
				{
					CryLog("[tlh] > can change");
					int  mode;
					EntityId  othEntId;
					mode = specmod->GetNextMode(pid, ((actionId == g_pGame->Actions().spectate_gen_nextmode) ? 1 : -1), &othEntId);
					if (mode != curspecmode)
					{
						CryLog("[tlh] > changing to mode %d with othEnt %d",mode,othEntId);
						specmod->ChangeSpectatorMode(pActor, mode, othEntId, false);
					}
				}
			}
			else
			{
				// actions specific to individual spectator modes

				if (curspecmode == CActor::eASM_Fixed)
					;  // none
				else if (curspecmode == CActor::eASM_Free)
					;  // none
				else if (curspecmode == CActor::eASM_Follow)
				{
					if (((actionId == g_pGame->Actions().spectate_3rdperson_nextteammate) || (actionId == g_pGame->Actions().spectate_3rdperson_prevteammate)) && (activationMode == eAAM_OnPress))
					{
						int  change = 1;
						if (actionId == g_pGame->Actions().spectate_3rdperson_prevteammate)
							change = -1;
						EntityId  newTargetId = specmod->GetNextSpectatorTarget(pid, change);
						if (newTargetId)
						{
							pActorImpl->SetSpectatorTarget(newTargetId);
						}
					}
				}
				else if (curspecmode == CActor::eASM_CCTV)
				{
					if (((actionId == g_pGame->Actions().spectate_cctv_nextcam) || (actionId == g_pGame->Actions().spectate_cctv_prevcam)) && (activationMode == eAAM_OnPress))
					{
						EntityId  locationId;
						if (actionId == g_pGame->Actions().spectate_cctv_nextcam)
						{
							CryLog(" > spectate_cctv_nextcam");
							locationId = specmod->GetNextCCTVLocation(pActorImpl);
						}
						else
						{
							CryLog(" > spectate_cctv_prevcam");
							locationId = specmod->GetPrevCCTVLocation(pActorImpl);
						}
						pActorImpl->SetSpectatorCCTVCam(locationId);
					}
					else if ((actionId == g_pGame->Actions().spectate_cctv_tagenemy) && (activationMode == eAAM_OnPress))
					{
						specmod->HandleCCTVTaggingAction();
					}
				}
			}
		}

		if (specmod)
		{
			// spectator mode is available, but could be either in it or not in it

			if (g_pGameCVars->g_spectate_DisableSpawnOptOut == 0)
			{
				if (pActorImpl->GetHealth() <= 0)
				{
					if ((actionId == g_pGame->Actions().spectate_gen_optoutofspawn) && (activationMode == eAAM_OnPress))
					{
						if (pGameRules)
						{
							IGameRulesSpawningModule *pSpawningModule = pGameRules->GetSpawningModule();
							if (pSpawningModule)
							{
								pSpawningModule->HandleOptOutOfSpawn(pid);
							}
						}
					}
				}
			}
		}
	}
}

