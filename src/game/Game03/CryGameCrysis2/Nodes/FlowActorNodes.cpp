#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include "Nodes/G2FlowBaseNode.h"
#include "GameRules.h"
#include "Actor.h"

//////////////////////////////////////////////////////////////////////////

class CFlowNode_ActorPlayers : public CFlowBaseNode
{
public:
	CFlowNode_ActorPlayers( SActivationInfo * pActInfo )
	{
	}

	enum EOutputs
	{
		eOUT_AllPlayers = 0,
		eOUT_Player1,
		eOUT_Player2
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void ("update", _HELP("Updates id's and triggers all respective outputs")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<EntityId>("AllPlayers", _HELP("Outputs the id which is used to trigger events for both players  (is equivalent to local player id )")),
			OutputPortConfig<EntityId>("Player1", _HELP("Outputs id of player 1 (host) ")),
			OutputPortConfig<EntityId>("Player2", _HELP("Outputs id of player 2 (client) ")),
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Outputs the entity id of the players");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
			case eFE_Initialize:
			case eFE_Activate:
			{
				IActor* pLocalActor = g_pGame->GetIGameFramework()->GetClientActor();
				if (pLocalActor)
				{
					EntityId localPlayerID = pLocalActor->GetEntityId();
					ActivateOutput( pActInfo, eOUT_AllPlayers, localPlayerID );

					EntityId client = g_pGame->GetGameRules()->GetCoopPlayerClient();
					EntityId host = g_pGame->GetGameRules()->GetCoopPlayerHost();

					if (host)
						ActivateOutput(pActInfo, eOUT_Player1, host);
					if (client)
						ActivateOutput(pActInfo, eOUT_Player2, client);
				}
			}
			break;
		}
	}

//Diesel cut
	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}
//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s) 
	{
		s->Add(*this);
	}



};



//////////////////////////////////////////////////////////////////////////
// Counts how many AIs died
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIBodyCount : public CFlowBaseNode, SGameRulesListener
{
	enum EInputPorts
	{
		eINP_Enable = 0,
		eINP_Disable,
		eINP_Reset
	};
	
	enum EOutputPorts
	{
		eOUT_TotalDeaths = 0,
		eOUT_EnemyDeaths
	};
public:
	CFlowNode_AIBodyCount( SActivationInfo * pActInfo )
	: m_totalDeaths( 0 )
	, m_enemyDeaths( 0 )
	{
	}
	~CFlowNode_AIBodyCount()
	{
		CGameRules* pGameRules = g_pGame->GetGameRules();
		if(pGameRules)
			pGameRules->RemoveGameRulesListener(this);
	}
	void Serialize( SActivationInfo * pActInfo, TSerialize ser )
	{
		ser.Value("TotalDeaths", m_totalDeaths);
		ser.Value("EnemyDeaths", m_enemyDeaths);
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig inp_config[] = {
			InputPortConfig_Void ("Enable", _HELP("Enables the body counter (disabled by default from start)")),
			InputPortConfig_Void ("Disable", _HELP("Disables the body counter")),
			InputPortConfig_Void ("Reset", _HELP("Resets the body counter to 0 for both outputs")),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<int>("Total"),
			OutputPortConfig<int>("Enemy"),
			{0}
		};

		config.sDescription = _HELP( "Counts how many AIs have been killed" );
		config.pInputPorts = inp_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
			case eFE_Initialize:
			{
				m_actInfo = *pActInfo;
				m_totalDeaths = 0;
				m_enemyDeaths = 0;
				break;
			}
			case eFE_Activate:
			{
				if (IsPortActive( pActInfo, eINP_Enable ))
				{
					CGameRules* pGameRules = g_pGame->GetGameRules();
					if (pGameRules)
						pGameRules->AddGameRulesListener( this );
				}
				
				if (IsPortActive( pActInfo, eINP_Disable ))
				{
					CGameRules* pGameRules = g_pGame->GetGameRules();
					if (pGameRules)
						pGameRules->RemoveGameRulesListener( this );
				}
				
				if (IsPortActive( pActInfo, eINP_Reset ))
				{
					m_totalDeaths = 0;
					m_enemyDeaths = 0;
					ActivateOutput( &m_actInfo, eOUT_TotalDeaths, m_totalDeaths );
					ActivateOutput( &m_actInfo, eOUT_EnemyDeaths, m_enemyDeaths );
				}
				break;
			}
		}
	}
	
	// inherited from SGameRulesListener	
	virtual void OnActorDeath( CActor* pActor )
	{
		IAIObject* pAI = pActor->GetEntity()->GetAI();

		if (!pAI)
			return;
			
		++m_totalDeaths;
		ActivateOutput( &m_actInfo, eOUT_TotalDeaths, m_totalDeaths );
	
		IAIObject* pClientAI = gEnv->pGame->GetIGameFramework()->GetClientActor()->GetEntity()->GetAI();
		
		if (pAI->IsHostile( pClientAI, false ))
		{
			++m_enemyDeaths;
			ActivateOutput( &m_actInfo, eOUT_EnemyDeaths, m_enemyDeaths );
		}
	}
	
//Diesel cut
	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}
//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s) 
	{
		s->Add(*this);
	}

	
	SActivationInfo m_actInfo;
	int m_totalDeaths;
	int m_enemyDeaths;
};


class CFlowActorAliveCheck : public CFlowBaseNode
{
public:
	CFlowActorAliveCheck( SActivationInfo * pActInfo )
	{
	}
	
	enum EInputs
	{
		eIP_Trigger = 0
	};
	
	enum EOutputs
	{
		eOP_Status = 0,
		eOP_Alive,
		eOP_Dead
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Trigger", _HELP("Trigger this port to get the current actor status")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<bool>("Status", _HELP("true if is alive, false if dead")),
			OutputPortConfig_Void("Alive", _HELP("triggered if is alive")),
			OutputPortConfig_Void("Dead", _HELP("triggered if is dead")),
			{0}
		};    
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.sDescription = _HELP("Check the death/alive status of an entity (actor)");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, eIP_Trigger))
			{
				IActor* pActor = pActInfo->pEntity ? gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pActInfo->pEntity->GetId()) : NULL;
				if(pActor)
				{
					bool isAlive = pActor->GetHealth()>0;
					ActivateOutput(pActInfo, eOP_Status, isAlive );
					if (isAlive)
						ActivateOutput(pActInfo, eOP_Alive, true );
					else
						ActivateOutput(pActInfo, eOP_Dead, true );
				}
				else
				{
					GameWarning("CFlowActorGetHealth - No Entity or Entity not an Actor!");
				}
			}
		}
	}

//Diesel cut
	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}
//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s) 
	{
		s->Add(*this);
	}
};




//////////////////////////////////////////////////////////////////////////
REGISTER_FLOW_NODE( "Actor:Players",	CFlowNode_ActorPlayers );
REGISTER_FLOW_NODE( "AI:BodyCount",		CFlowNode_AIBodyCount )
REGISTER_FLOW_NODE_SINGLETON("Actor:AliveCheck", CFlowActorAliveCheck);
