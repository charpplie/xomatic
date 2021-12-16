#include "StdAfx.h"
#include "Game.h"
#include "Player.h"
#include "NanoSuit_v2.h"
#include "Nodes/G2FlowBaseNode.h"

#define GetPlayer GetPlayerNodes

namespace
{
	CPlayer* GetPlayer(EntityId entityId)
	{ 
		static IActorSystem* pActorSys = 0;
		if (pActorSys == 0)
			pActorSys = g_pGame->GetIGameFramework()->GetIActorSystem();
		
		if (pActorSys != 0)
		{
			IActor* pIActor = pActorSys->GetActor(entityId);
			if (pIActor)
			{
				CActor* pActor = (CActor*) pIActor;
				if (pActor && CPlayer::GetActorClassType() == pActor->GetActorClass())
					return static_cast<CPlayer*> (pActor);
			}
		}
		return 0;
	}
};

class CFlowNanoSuitNode : public CFlowBaseNode, public CNanoSuit::INanoSuitListener
{
public:
	CFlowNanoSuitNode( SActivationInfo * pActInfo ) : m_entityId (0)
	{
	}

	~CFlowNanoSuitNode()
	{
		RemoveAsNanoSuitListener();
	}

	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowNanoSuitNode(pActInfo);
	}

	enum EInputPorts
	{
		EIP_Power = 0,
		EIP_Armor,
		EIP_Stealth,
		EIP_Tactical,
		EIP_NodeEnabled,
		//EIP_Energy,
		//EIP_Energy,
		//EIP_CloakLevel,
	};

	enum EOutputPorts
	{
		EOP_Power = 0,
		EOP_Armor,
		EOP_Stealth,
		EOP_Tactical,
		EOP_Normal,
		EOP_Critical,
		EOP_Disabled,
		EOP_Passive,
		EOP_Active,
		//EOP_Energy,
		//EOP_Energy,
		//EOP_CloakLevel,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void  ("Power", _HELP("Trigger to select Power Mode")),
			InputPortConfig_Void  ("Armor", _HELP("Trigger to select Armor Mode")),
			InputPortConfig_Void  ("Stealth", _HELP("Trigger to select Stealth Mode")),
			InputPortConfig_Void  ("Tactical", _HELP("Trigger to select Tactical Mode")),
			InputPortConfig<bool> ("NodeEnabled", true, _HELP("Enables/disables this node (not the nanosuit)")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<bool> ("Power", _HELP("Triggered on Power Mode")),
			OutputPortConfig<bool> ("Armor", _HELP("Triggered on Armor Mode")),
			OutputPortConfig<bool> ("Stealth", _HELP("Triggered on Stealth Mode")),
			OutputPortConfig<bool> ("Tactical", _HELP("Triggered on Tactical Mode")),
			OutputPortConfig<bool> ("SuitNormal", _HELP("Outputs if nanosuit is in normal state")),
			OutputPortConfig<bool> ("SuitCritical", _HELP("Outputs if nanosuit is in critical state")),
			OutputPortConfig<bool> ("SuitDisabled", _HELP("Outputs if nanosuit is disabled")),
			OutputPortConfig<bool> ("Passive", _HELP("Outputs if nanosuit mode is passive")),
			OutputPortConfig<bool> ("Active", _HELP("Outputs if nanosuit mode is active")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("NanoSuit2 Node");
		config.SetCategory(EFLN_APPROVED);
	}

	void RemoveAsNanoSuitListener()
	{
		if (!GetISystem()->IsQuitting())
		{
			CPlayer* pPlayer = GetPlayer(m_entityId);
			if (pPlayer)
			{
				pPlayer->UnregisterPlayerSuitEventListener(this);
			}
		}
	}

	void AddAsNanoSuitListener()
	{
		CPlayer* pPlayer = GetPlayer(m_entityId);
		if (pPlayer != 0)
		{
			pPlayer->RegisterPlayerSuitEventListener(this);
		}
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				m_actInfo = *pActInfo;
				CPlayer* pPlayer = GetPlayer(m_entityId);
				if (pPlayer && pPlayer->HasNanoSuit())
				{
					ModeChanged(pPlayer->GetActorSuitGameParameters().GetMode());
					EnergyChanged(pPlayer->GetActorSuitGameParameters().GetEnergy().Get());
				}
			}
			break;
		case eFE_SetEntityId:
			{
				m_actInfo = *pActInfo;
				RemoveAsNanoSuitListener();
				m_entityId = pActInfo->pEntity ? pActInfo->pEntity->GetId() : 0;
				if (IsNodeEnabled())
					AddAsNanoSuitListener();
			}
			break;

		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, EIP_NodeEnabled))
				{
					if (IsNodeEnabled())
						AddAsNanoSuitListener();    // redundant removals or adds are not a problem
					else
						RemoveAsNanoSuitListener(); 
				}
			
				if (IsNodeEnabled())
				{
					CPlayer* pPlayer = GetPlayer(m_entityId);
					if (pPlayer == 0)
						return;

					CNanoSuit* pSuit = pPlayer->GetNanoSuit();
					if(!pSuit)
						return;

					if (IsPortActive(pActInfo, EIP_Power))
						pSuit->ActivateMode(eNanoSuitMode_Power);
					else if (IsPortActive(pActInfo, EIP_Armor))
						pSuit->ActivateMode(eNanoSuitMode_Armor);
					else if (IsPortActive(pActInfo, EIP_Stealth))
						pSuit->ActivateMode(eNanoSuitMode_Stealth);
					else if (IsPortActive(pActInfo, EIP_Tactical))
						pSuit->ActivateMode(eNanoSuitMode_Tactical);

					UpdateOutputPowerMode();
					UpdateOutputSuitMode( pSuit->GetGameParams().GetMode() );
					UpdateOutputSuitState( pSuit->GetGameParams().GetState() );
				}
			}
			break;
		}
	}
	
	bool IsNodeEnabled()
	{
		return GetPortBool(&m_actInfo, EIP_NodeEnabled);
	}

	// INanoSuitListener
	void OnModeSelected(ENanoSuitMode mode)
	{}

	void OnSuitPowerActivated(const bool activated)
	{
		if (!IsNodeEnabled())
			return;
			
		UpdateOutputPowerMode();
	}

	void ModeChanged(ENanoSuitMode mode)
	{
		if (!IsNodeEnabled())
			return;
			
		UpdateOutputSuitMode( mode );
	}

	void EnergyChanged(float energy)
	{
		//ActivateOutput(&m_actInfo, EOP_Energy, energy);
	}

	void OnSuitStateChanged(ENanoSuitState state)
	{
		if (!IsNodeEnabled())
			return;
		UpdateOutputSuitState( state );
	}

	void UpdateOutputPowerMode()
	{
		CPlayer* pPlayer = GetPlayer(m_entityId);
		if (pPlayer && pPlayer->HasNanoSuit())
		{
			bool active = pPlayer->GetActorSuitGameParameters().IsSuitPowerActive();
			ActivateOutput(&m_actInfo, EOP_Passive, !active);
			ActivateOutput(&m_actInfo, EOP_Active, active);
		}
	}	
	
	void UpdateOutputSuitMode( ENanoSuitMode mode )
	{
		ActivateOutput( &m_actInfo, EOP_Power, mode==eNanoSuitMode_Power );
		ActivateOutput( &m_actInfo, EOP_Armor, mode==eNanoSuitMode_Armor );
		ActivateOutput( &m_actInfo, EOP_Stealth, mode==eNanoSuitMode_Stealth );
		ActivateOutput( &m_actInfo, EOP_Tactical, mode==eNanoSuitMode_Tactical );

		//Force update of state ports as well
		CPlayer* pPlayer = GetPlayer(m_entityId);
		if (pPlayer && pPlayer->HasNanoSuit())
		{
			ENanoSuitState state = pPlayer->GetActorSuitGameParameters().GetState();
			ActivateOutput(&m_actInfo, EOP_Normal, (state == eNanoSuitState_Normal) ? true : false);
			ActivateOutput(&m_actInfo, EOP_Critical, (state == eNanoSuitState_Critical) ? true : false);
			ActivateOutput(&m_actInfo, EOP_Disabled, (state == eNanoSuitState_Disabled) ? true : false);

			bool active = pPlayer->GetActorSuitGameParameters().IsSuitPowerActive();
			ActivateOutput(&m_actInfo, EOP_Passive, !active);
			ActivateOutput(&m_actInfo, EOP_Active, active);
		}
	}	

	void UpdateOutputSuitState( ENanoSuitState state )
	{
		ActivateOutput(&m_actInfo, EOP_Normal, (state == eNanoSuitState_Normal) ? true : false);
		ActivateOutput(&m_actInfo, EOP_Critical, (state == eNanoSuitState_Critical) ? true : false);
		ActivateOutput(&m_actInfo, EOP_Disabled, (state == eNanoSuitState_Disabled) ? true : false);
	}
	

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	SActivationInfo m_actInfo;
	EntityId        m_entityId;
	//~INanoSuitListener
};


class CFlowNanoSuitGetNode : public CFlowBaseNode
{
public:
	CFlowNanoSuitGetNode( SActivationInfo * pActInfo )
	{
	}

	~CFlowNanoSuitGetNode()
	{
	}

	/*
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return this; // new CFlowNanoSuitNode(pActInfo);
	}
	*/

	enum EInputPorts
	{
		EIP_Trigger = 0,
	};

	enum EOutputPorts
	{
		EOP_Power = 0,
		EOP_Armor,
		EOP_Stealth,
		EOP_Tactical,
		EOP_Normal,
		EOP_Critical,
		EOP_Disabled,
		EOP_Passive,
		EOP_Active,
		EOP_Energy,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void  ("Trigger", _HELP("Trigger to get current NanoSuit values")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<bool> ("Power", _HELP("Triggered on Power Mode")),
			OutputPortConfig<bool> ("Armor", _HELP("Triggered on Armor Mode")),
			OutputPortConfig<bool> ("Stealth", _HELP("Triggered on Stealth Mode")),
			OutputPortConfig<bool> ("Tactical", _HELP("Triggered on Tactical Mode")),
			OutputPortConfig<bool> ("SuitNormal", _HELP("Outputs if nanosuit is in normal state")),
			OutputPortConfig<bool> ("SuitCritical", _HELP("Outputs if nanosuit is in critical state")),
			OutputPortConfig<bool> ("SuitDisabled", _HELP("Outputs if nanosuit is disabled")),
			OutputPortConfig<bool> ("Passive", _HELP("Outputs if nanosuit mode is passive")),
			OutputPortConfig<bool> ("Active", _HELP("Outputs if nanosuit mode is active")),
			OutputPortConfig<float>("Energy", _HELP("Current Energy")),
			{0}
		};
		
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("NanoSuit2 Get Node");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
			case eFE_Activate:
			{
				if (!IsPortActive(pActInfo, EIP_Trigger))
					return;

				if (pActInfo->pEntity == 0)
					return;
				CPlayer* pPlayer = GetPlayer(pActInfo->pEntity->GetId());
				if (pPlayer == 0)
					return;
				
				const SNanoSuitGameParameters& suitInfo = pPlayer->GetActorSuitGameParameters();
				ActivateOutput( pActInfo, EOP_Power, suitInfo.GetMode()==eNanoSuitMode_Power );
				ActivateOutput( pActInfo, EOP_Armor, suitInfo.GetMode()==eNanoSuitMode_Armor );
				ActivateOutput( pActInfo, EOP_Stealth, suitInfo.GetMode()==eNanoSuitMode_Stealth );
				ActivateOutput( pActInfo, EOP_Tactical, suitInfo.GetMode()==eNanoSuitMode_Tactical );
				
				const ENanoSuitState& state = suitInfo.GetState();
				ActivateOutput( pActInfo, EOP_Normal, state==eNanoSuitState_Normal );
				ActivateOutput( pActInfo, EOP_Critical, state==eNanoSuitState_Critical );
				ActivateOutput( pActInfo, EOP_Disabled, state==eNanoSuitState_Disabled );
				
				ActivateOutput( pActInfo, EOP_Active, suitInfo.IsSuitPowerActive() );
				ActivateOutput( pActInfo, EOP_Passive, !suitInfo.IsSuitPowerActive() );
					
				ActivateOutput( pActInfo, EOP_Energy, suitInfo.GetEnergy().Get() );
				
				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};

class CFlowNanoSuitDisable : public CFlowBaseNode
{
public:
	CFlowNanoSuitDisable( SActivationInfo * pActInfo )
	{
	}

	~CFlowNanoSuitDisable()
	{
	}

	enum EInputPorts
	{
		EIP_EnableTrigger = 0,
		EIP_DisableTrigger,
		EIP_Time,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void ("EnableSuit", _HELP("Trigger to enable the suit")),
			InputPortConfig_Void  ("DisableSuit", _HELP("Trigger to disable the suit")),
			InputPortConfig<float> ("DisabledTime", 0.0f, _HELP("Time that the suit will be disabled. (Set to 0 to use default value / Set to negative value to disable permanently the suit")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = 0;
		config.sDescription = _HELP("NanoSuit2 Enable/Disable");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		//case eFE_Initialize:
			//{
				//ActivateOutput(pActInfo, EIP_Time, 0.0f);
			//}
			//break;

		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, EIP_DisableTrigger))
				{
					if (pActInfo->pEntity == 0)
						return;
					CPlayer* pPlayer = GetPlayer(pActInfo->pEntity->GetId());
					if (pPlayer == 0)
						return;

					SNanoSuitEvent event;
					event.event = eNanoSuitEvent_DISABLE_SUIT;
					event.fParam = GetPortFloat(pActInfo, EIP_Time);
					event.sParam = "Suit_Disabled";
					pPlayer->SendActorSuitEvent(event);
				}
				else if (IsPortActive(pActInfo, EIP_EnableTrigger))
				{
					if (pActInfo->pEntity == 0)
						return;
					CPlayer* pPlayer = GetPlayer(pActInfo->pEntity->GetId());
					if (pPlayer == 0)
						return;

					SNanoSuitEvent event;
					event.event = eNanoSuitEvent_REBOOT_SUIT;
					pPlayer->SendActorSuitEvent(event);
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


class CFlowNanoSuitFakeMaterial : public CFlowBaseNode
{
public:
	CFlowNanoSuitFakeMaterial( SActivationInfo * pActInfo )
	{
	}

	~CFlowNanoSuitFakeMaterial()
	{
	}

	enum EInputPorts
	{
		EIP_Asian = 0,
		EIP_Cloak,
		EIP_Strength,
		EIP_Defense,
		EIP_Speed,
	};

	enum EOutputPorts
	{
		EOP_Done = 0,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig<bool> ("Asian", false, _HELP("If true, use Asian material, otherwise US")),
			InputPortConfig_Void  ("Cloak", _HELP("Trigger to select Cloak Mode")),
			InputPortConfig_Void  ("Strength", _HELP("Trigger to select Strength Mode")),
			InputPortConfig_Void  ("Defense", _HELP("Trigger to select Defense Mode")),
			InputPortConfig_Void  ("Speed", _HELP("Trigger to select Speed Mode")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void("Done", _HELP("Triggered when Done.")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Fake Materials on Characters (non Player/AI) NanoSuit for Cinematics");
		config.SetCategory(EFLN_APPROVED);
	}


	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			{
				IEntity* pEntity = pActInfo->pEntity;
				if (pEntity != 0)
				{
					ENanoSuitMode nanoMode = eNanoSuitMode_Last;
					if (IsPortActive(pActInfo, EIP_Cloak))
						nanoMode = eNanoSuitMode_Power;
					else if (IsPortActive(pActInfo, EIP_Speed))
						nanoMode = eNanoSuitMode_Power;
					else if (IsPortActive(pActInfo, EIP_Strength))
						nanoMode = eNanoSuitMode_Power;
					else if (IsPortActive(pActInfo, EIP_Defense))
						nanoMode = eNanoSuitMode_Power;
					if (nanoMode != eNanoSuitMode_Last)
					{
						const bool bAsian = GetPortBool(pActInfo, EIP_Asian);
						//CNanoSuit::SNanoMaterial* pNanoMat = CNanoSuit::GetNanoMaterial(nanoMode, bAsian);
						//if (pNanoMat)
						{
							//CNanoSuit::AssignNanoMaterialToEntity(pEntity, pNanoMat);
							//ActivateOutput(pActInfo, EOP_Done, true);

						}
					}
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


//////////////////////////////////////////////////////////////////////////
class CFlowNanoSuitFilterMode : public CFlowBaseNode
{
public:
	CFlowNanoSuitFilterMode( SActivationInfo * pActInfo )
	{
	}

	~CFlowNanoSuitFilterMode()
	{
	}
	
	enum EInputPorts
	{
		eIP_NanosuitMode = 0,
		eIP_Allow,
		eIP_Forbid
	};
	

	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowNanoSuitFilterMode(pActInfo);
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		// the ports need to be in the same order than ENanoSuitMode  (NanosuitDefs.h)
		static const SInputPortConfig inputs[] = {
			InputPortConfig<int>  ("NanosuitMode", 0, _HELP("Nanosuit mode"), 0, _UICONFIG("enum_int:Power=0,Armor=1,Stealth=2,Tactical=3")),
			InputPortConfig_Void  ("Allow", _HELP("Allow the especified nanosuit mode")),
			InputPortConfig_Void  ("Forbid", _HELP("Forbid the especified nanosuit mode")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Allow/forbid the use of an especific nanosuit mode.");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
			case eFE_Activate:
			{
				CPlayer* pPlayer = GetPlayer(pActInfo->pEntity->GetId());
				if (!pPlayer)
					return;
				CNanoSuit* pSuit = pPlayer->GetNanoSuit();
				if(!pSuit)
					return;

				if (IsPortActive(pActInfo, eIP_Allow))
					pSuit->AllowMode( ENanoSuitMode(GetPortInt( pActInfo, eIP_NanosuitMode )), true );

				if (IsPortActive(pActInfo, eIP_Forbid))
					pSuit->AllowMode( ENanoSuitMode(GetPortInt( pActInfo, eIP_NanosuitMode )), false );
			}
			break;
		}
	}


	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CFlowNanoSuitPerk : public CFlowBaseNode
{
	enum EInputPorts
	{
		eIP_Enable = 0,
		eIP_Disable,
		eIP_Perk,
		eIP_PerkType
	};

public:
	CFlowNanoSuitPerk( SActivationInfo * pActInfo )
	{
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "Enable", _HELP("Connect event here to enable perk" )),
			InputPortConfig_Void( "Disable", _HELP("Connect event here to disable perk" )),
			InputPortConfig<string>( "Perk", _HELP("Perk to enable/disable" ), _HELP("Perk"), _UICONFIG("enum_global_ref:perk%s:PerkType")),
			InputPortConfig<string>( "PerkType", "", _HELP("Select from which perks to choose"), 0, _UICONFIG("enum_string:All=,AnyMode=_anyMode,Tactical=_tacticalMode,Power=_powerMode,Armor=_armorMode,Stealth=_stealthMode")),
			{0}
		};

		static const SOutputPortConfig out_ports[] = 
		{
			{0}
		};

		config.sDescription = _HELP("Enable/Disable perks for player");
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event == eFE_Activate)
		{
			bool isEnablePort = IsPortActive(pActInfo, eIP_Enable);
			bool isDisablePort = IsPortActive(pActInfo, eIP_Disable);

			if (isDisablePort || isEnablePort)
			{
				EntityId playerId = pActInfo->pEntity ? pActInfo->pEntity->GetId() : 0;

				CPlayer* pPlayer = GetPlayer(playerId);
						
				CPerk* pPerkInstance = CPerk::GetInstance();
				CRY_ASSERT(pPerkInstance);
				if (pPerkInstance && pPlayer)
				{
					EPerks perkId = pPerkInstance->FindPerkNumberByName(GetPortString(pActInfo, eIP_Perk).c_str());
					if (perkId != ePerk_Null)
					{
						pPlayer->SetPerkActive(perkId, isEnablePort ? true : false);
					}
				}
			}	
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE("NanoSuit:NanoSuit", CFlowNanoSuitNode);
REGISTER_FLOW_NODE("NanoSuit:Disable", CFlowNanoSuitDisable);
REGISTER_FLOW_NODE_SINGLETON("NanoSuit:NanoSuitGet", CFlowNanoSuitGetNode);
REGISTER_FLOW_NODE("NanoSuit:FilterMode", CFlowNanoSuitFilterMode);
REGISTER_FLOW_NODE_SINGLETON("NanoSuit:FakeMaterial", CFlowNanoSuitFakeMaterial);
REGISTER_FLOW_NODE("NanoSuit:Perks", CFlowNanoSuitPerk);

#undef GetPlayer

