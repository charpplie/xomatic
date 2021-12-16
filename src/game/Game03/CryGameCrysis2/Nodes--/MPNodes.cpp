/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 26:04:2007   17.47: Created by Steve Humphreys

*************************************************************************/

#include "StdAfx.h"

#include "GameRules.h"
#include "GameCVars.h"
#include "Player.h"
#include "Nodes/G2FlowBaseNode.h"

#include <IVehicleSystem.h>

class CFlowNode_MP : public CFlowBaseNode, public SGameRulesListener
{
	enum INPUTS
	{
		EIP_GameEndTime = 0,
	};

	enum OUTPUTS
	{
		//EOP_GameStarted,
		EOP_EnteredGame,
		EOP_EndGameNear,
		EOP_EndGameInvalid,
		EOP_GameWon,
		EOP_GameLost,
		EOP_GameTied,
	};

public:
	CFlowNode_MP( SActivationInfo * pActInfo )
	{
		if(pActInfo && pActInfo->pGraph)
		{
			pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
		}

		m_localPlayerSpectatorMode = 0;
		m_endGameNear = false;
		m_timeRemainingTriggered = true;		// default to true so it won't be triggered again until player enters game.
	}

	~CFlowNode_MP()
	{
		CGameRules* pGameRules = g_pGame->GetGameRules();
		if(pGameRules)
			pGameRules->RemoveGameRulesListener(this);
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_MP(pActInfo);
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig<float>("GameEndTime", _HELP("Number of seconds remaining at which to trigger the EndGameNear output")),
			{0}
		};
		static const SOutputPortConfig out_ports[] = 
		{
			//OutputPortConfig_Void( "GameStarted", _HELP("Triggered when MP game begins")),
			OutputPortConfig_Void( "EnteredGame", _HELP("Triggered when local player enters the game")),
			OutputPortConfig_Void( "EndGameNear", _HELP("Triggered when game-ending condition is near")),
			OutputPortConfig_Void( "EndGameInvalid", _HELP("Triggered when game-ending condition is invalidated")),
			OutputPortConfig_Void( "GameWon", _HELP("Triggered when local player's team wins the game")),
			OutputPortConfig_Void( "GameLost", _HELP("Triggered when local player's team loses the game")),
			OutputPortConfig_Void( "GameTied", _HELP("Triggered when neither team wins the game")),
			{0}
		};
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.sDescription = _HELP("MP MPNode");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				CGameRules* pGameRules = g_pGame->GetGameRules();
				if(pGameRules)
					pGameRules->AddGameRulesListener(this);

				CPlayer* pPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetClientActor());
				if(pPlayer)
					m_localPlayerSpectatorMode = pPlayer->GetSpectatorMode();

				m_actInfo = *pActInfo;
			}
			break;
		case eFE_Activate:
			break;
		case eFE_Update:
			{
				CPlayer* pPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetClientActor());
				if(!pPlayer)
					return;

				// first check: tac weapons trigger endgamenear / endgameinvalid
				if(m_MDList.empty() && m_endGameNear)
				{
					// if less than 3 min remaining don't return to normal
					bool cancel = true;
					if(g_pGame && g_pGame->GetGameRules() && g_pGame->GetGameRules()->IsTimeLimited() && m_timeRemainingTriggered)
					{
						float timeRemaining = g_pGame->GetGameRules()->GetRemainingGameTime();
						if(timeRemaining < GetPortFloat(pActInfo, EIP_GameEndTime) )
							cancel = false;
					}

					if(cancel)
					{
						if(g_pGame->GetCVars()->i_debug_mp_flowgraph != 0)
						{
							CryLog("MP flowgraph: EndGameInvalid");
						}
						ActivateOutput(&m_actInfo, EOP_EndGameInvalid, true);
						m_endGameNear = false;
					}
				}
				else if(!m_MDList.empty())
				{
					// go through the list of tac/sing weapons and check if they are still present
					std::list<EntityId>::iterator next;
					std::list<EntityId>::iterator it = m_MDList.begin();
					for(; it != m_MDList.end(); it = next)
					{
						next = it; ++next;
						if(gEnv->pEntitySystem->GetEntity(*it))
						{
							// entity exists so trigger loud music if not already done
							if(!m_endGameNear)
							{
								if(g_pGame->GetCVars()->i_debug_mp_flowgraph != 0)
								{
									CryLog("--MP flowgraph: EndGameNear");
								}
								ActivateOutput(&m_actInfo, EOP_EndGameNear, true);
								m_endGameNear = true;
							}

							// in the case of tanks, entity isn't removed for quite some time after destruction.
							//	Check vehicle health directly here...
							IVehicle* pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(*it);
							if(pVehicle && pVehicle->IsDestroyed())
							{
								m_MDList.erase(it);
							}
						}
						else
						{
							// entity no longer exists - remove from list.
 							m_MDList.erase(it);
						}
					}
				}

				// get the remaining time from game rules
				if(!m_timeRemainingTriggered && g_pGame->GetGameRules() && g_pGame->GetGameRules()->IsTimeLimited() && pPlayer->GetSpectatorMode() == 0 && !m_endGameNear)
				{
					float timeRemaining = g_pGame->GetGameRules()->GetRemainingGameTime();
					if(timeRemaining < GetPortFloat(pActInfo, EIP_GameEndTime) )
					{
						if(g_pGame->GetCVars()->i_debug_mp_flowgraph != 0)
						{
							CryLog("--MP flowgraph: EndGameNear");
						}
						ActivateOutput(&m_actInfo, EOP_EndGameNear, timeRemaining);
						m_timeRemainingTriggered = true;
						m_endGameNear = true;
					}
				}

				// also check whether the local player is in game yet
  				if(pPlayer->GetSpectatorMode() == 0 && m_localPlayerSpectatorMode != 0)
  				{
						if(g_pGame->GetCVars()->i_debug_mp_flowgraph != 0)
						{
							CryLog("--MP flowgraph: EnteredGame");
						}
  					ActivateOutput(&m_actInfo, EOP_EnteredGame, true);
  					m_localPlayerSpectatorMode = pPlayer->GetSpectatorMode();
  				}
			}
			break;
		}
	}

protected:
		virtual void GameOver(EGameOverType localWinner)
		{
			switch(localWinner)
			{
				case EGOT_Win:
					if(g_pGame->GetCVars()->i_debug_mp_flowgraph != 0)
					{
						CryLog("--MP flowgraph: GameWon");
					}
					ActivateOutput(&m_actInfo, EOP_GameWon,true);
					break;
			
				case EGOT_Lose:
					if(g_pGame->GetCVars()->i_debug_mp_flowgraph != 0)
					{
						CryLog("--MP flowgraph: GameLost");
					}
					ActivateOutput(&m_actInfo, EOP_GameLost,true);
					break;

				default:
					if(g_pGame->GetCVars()->i_debug_mp_flowgraph != 0)
					{
						CryLog("--MP flowgraph: GameTied");
					}
					ActivateOutput(&m_actInfo, EOP_GameTied, true);
					break;
			}
		}
		virtual void EnteredGame()
		{
// 			CPlayer* pPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetClientActor());
// 			if(pPlayer && pPlayer->GetSpectatorMode() == 0)
// 			{
// 				ActivateOutput(&m_actInfo, EOP_EnteredGame,true);
// 				m_timeRemainingTriggered = false;
// 			}
		}
		virtual void EndGameNear(EntityId id)
		{
			if(id != 0)
			{
				m_MDList.push_back(id);
			}
		}
		
	SActivationInfo m_actInfo;
	int m_localPlayerSpectatorMode;
	bool m_endGameNear;
	bool m_timeRemainingTriggered;

	std::list<EntityId> m_MDList;
};

//----------------------------------------------------------------------------------------

class CFlowNode_MPGameType : public CFlowBaseNode
{
	enum INPUTS
	{
		EIP_GetGameMode = 0,
	};

	enum OUTPUTS
	{
		EOP_InstantAction = 0,
		EOP_TeamInstantAction,
		EOP_PowerStruggle,
		EOP_Coop,
		EOP_AllOrNothing,
		EOP_Survival,
		EOP_Assault,
		EOP_CaptureTheFlag,
		EOP_BombTheBase,
		EOP_CrashSite,
		EOP_Race,
		EOP_Extraction,
		EOP_PowerStruggleLite,
		EOP_Countdown,
		EOP_Payload,
		EOP_GameRulesName,
	};

public:
	CFlowNode_MPGameType( SActivationInfo * pActInfo )
	{
	}

	~CFlowNode_MPGameType()
	{
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_MPGameType(pActInfo);
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "GetGameType", _HELP("Activate this to retrigger relevent outputs")),
			{0}
		};
		static const SOutputPortConfig out_ports[] = 
		{
			OutputPortConfig_Void( "InstantAction", _HELP("Triggered on level load if InstantAction game")),
			OutputPortConfig_Void( "TeamInstantAction", _HELP("Triggered on level load if TeamInstantAction game")),
			OutputPortConfig_Void( "PowerStruggle", _HELP("Triggered on level load if PowerStruggle game")),
			OutputPortConfig_Void( "Coop", _HELP("Triggered on level load if coop game")),
			OutputPortConfig_Void( "AllOrNothing", _HELP("Triggered on level load if AllOrNothing game")),
			OutputPortConfig_Void( "Survival", _HELP("Triggered on level load if Survival game")),
			OutputPortConfig_Void( "Assault", _HELP("Triggered on level load if Assault game")),
			OutputPortConfig_Void( "CaptureTheFlag", _HELP("Triggered on level load if CaptureTheFlag game")),
			OutputPortConfig_Void( "BombTheBase", _HELP("Triggered on level load if BombTheBase game")),
			OutputPortConfig_Void( "CrashSite", _HELP("Triggered on level load if CrashSite game")),
			OutputPortConfig_Void( "Race", _HELP("Triggered on level load if Race game")),
			OutputPortConfig_Void( "Extraction", _HELP("Triggered on level load if Extraction game")),
			OutputPortConfig_Void( "PowerStruggleLite", _HELP("Triggered on level load if PowerStruggleLite game")),
			OutputPortConfig_Void( "Countdown", _HELP("Triggered on level load if Countdown game")),
			OutputPortConfig_Void( "Payload", _HELP("Triggered on level load if Payload game")),
			OutputPortConfig<string>("GameRulesName", _HELP("Outputs the current game rules name")),
			{0}
		};
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.sDescription = _HELP("MP GameTypeNode");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				m_actInfo = *pActInfo;
			}
			break;
		case eFE_Activate:
			{
				if(IsPortActive(pActInfo, EIP_GetGameMode))
				{
					TriggerGameTypeOutput();
				}
			}
			break;
		}
	}

	void TriggerGameTypeOutput()
	{
		if(g_pGame->GetGameRules())
		{
			const char* gameRulesName = g_pGame->GetGameRules()->GetEntity()->GetClass()->GetName();
			if(!strcmp(gameRulesName, "PowerStruggle"))
				ActivateOutput(&m_actInfo, EOP_PowerStruggle, true);
			else if(!strcmp(gameRulesName, "InstantAction"))
				ActivateOutput(&m_actInfo, EOP_InstantAction, true);
			else if(!strcmp(gameRulesName, "Coop"))
				ActivateOutput(&m_actInfo, EOP_Coop, true);
			else if(!strcmp(gameRulesName, "TeamInstantAction"))
				ActivateOutput(&m_actInfo, EOP_TeamInstantAction, true);
			else if(!strcmp(gameRulesName, "AllOrNothing"))
				ActivateOutput(&m_actInfo, EOP_AllOrNothing, true);
			else if(!strcmp(gameRulesName, "Survival"))
				ActivateOutput(&m_actInfo, EOP_Survival, true);
			else if(!strcmp(gameRulesName, "Assault"))
				ActivateOutput(&m_actInfo, EOP_Assault, true);
			else if(!strcmp(gameRulesName, "CaptureTheFlag"))
				ActivateOutput(&m_actInfo, EOP_CaptureTheFlag, true);
			else if(!strcmp(gameRulesName, "BombTheBase"))
				ActivateOutput(&m_actInfo, EOP_BombTheBase, true);
			else if(!strcmp(gameRulesName, "CrashSite"))
				ActivateOutput(&m_actInfo, EOP_CrashSite, true);
			else if(!strcmp(gameRulesName, "Race"))
				ActivateOutput(&m_actInfo, EOP_Race, true);
			else if(!strcmp(gameRulesName, "Extraction"))
				ActivateOutput(&m_actInfo, EOP_Extraction, true);
			else if(!strcmp(gameRulesName, "PowerStruggleLite"))
				ActivateOutput(&m_actInfo, EOP_PowerStruggleLite, true);
			else if(!strcmp(gameRulesName, "Countdown"))
				ActivateOutput(&m_actInfo, EOP_Countdown, true);
			else if(!strcmp(gameRulesName, "Payload"))
				ActivateOutput(&m_actInfo, EOP_Payload, true);

			// output the name as well (for supporting any additional modes that might be added)
			ActivateOutput(&m_actInfo, EOP_GameRulesName, string(gameRulesName));
		}
	}

protected:
	SActivationInfo m_actInfo;
};

//----------------------------------------------------------------------------------------

class CFlowCoopPlayers : public CFlowBaseNode
{
public:
	CFlowCoopPlayers( SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void   ("update", _HELP("Retriggers the entity id. Required for multiplayer")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<EntityId>("serverId", _HELP("Player 1")),
			OutputPortConfig<EntityId>("clientId", _HELP("Player 2")),
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Outputs the players entity id..");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
		case eFE_Activate:
			{
				IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
				EntityId localActorId = pActor?pActor->GetEntityId():0;
				ActivateOutput(pActInfo, gEnv->bServer?0:1, localActorId);

				IActorSystem *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
				IActorIteratorPtr actorIt = pActorSystem->CreateActorIterator();
				pActor = actorIt->Next();
				while(pActor)
				{
					if(pActor->GetChannelId() && pActor->GetEntityId() != localActorId)
					{
						ActivateOutput(pActInfo, gEnv->bServer?1:0, pActor->GetEntityId());
						break;
					}
					pActor = actorIt->Next();
				}

			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};

//----------------------------------------------------------------------------------------

class CFlowNode_IsCoop : public CFlowBaseNode
{
	enum INPUTS
	{
		EIP_GetGameMode = 0,
	};

	enum OUTPUTS
	{
		EOP_SinglePlayer = 0,
		EOP_Coop,
	};

public:
	CFlowNode_IsCoop( SActivationInfo * pActInfo )
	{
	}

	~CFlowNode_IsCoop()
	{
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "Check", _HELP("Activate this to retrigger relevent outputs")),
			{0}
		};
		static const SOutputPortConfig out_ports[] = 
		{
			OutputPortConfig_Void( "Singleplayer", _HELP("Triggered on level load if singpleplayer game")),
			OutputPortConfig_Void( "Coop", _HELP("Triggered on level load if coop game")),
			{0}
		};
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.sDescription = _HELP("Coop GameTypeNode");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				m_actInfo = *pActInfo;
			}
			break;
		case eFE_Activate:
			{
				if(IsPortActive(pActInfo, EIP_GetGameMode))
				{
					const char* gameRulesName = g_pGame->GetGameRules()->GetEntity()->GetClass()->GetName();
					if(!stricmp(gameRulesName, "SinglePlayer"))
						ActivateOutput(&m_actInfo, EOP_SinglePlayer, true);
					else if(!stricmp(gameRulesName, "Coop"))
						ActivateOutput(&m_actInfo, EOP_Coop, true);
				}
			}
			break;
		}
	}

protected:
	SActivationInfo m_actInfo;
};


REGISTER_FLOW_NODE("Multiplayer:MP",	CFlowNode_MP);
REGISTER_FLOW_NODE("Multiplayer:GameType", CFlowNode_MPGameType);
REGISTER_FLOW_NODE_SINGLETON("Coop:CoopPlayers", CFlowCoopPlayers); // TODO: deprecated, to be deleted (Actor:Players substitute it )
REGISTER_FLOW_NODE("Coop:IsCoop", CFlowNode_IsCoop);