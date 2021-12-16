#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include "Nodes/G2FlowBaseNode.h"
#include "GameRules.h"


//////////////////////////////////////////////////////////////////////////

class CFlowNode_CoopBranchPlayers : public CFlowBaseNode
{
public:
	CFlowNode_CoopBranchPlayers( SActivationInfo * pActInfo )
	{
	}

	enum EOutputs
	{
		eOUT_Player1,
		eOUT_Player2
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig<EntityId>("CheckId", _HELP("Checks incoming entity id")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<EntityId>("Player1", _HELP("Outputs id of player 1 when it matches with incoming id from Check input")),
			OutputPortConfig<EntityId>("Player2", _HELP("Outputs id of player 2 when it matches with incoming id from Check input")),
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("branch gamelogic for each player");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
			case eFE_Initialize:
			case eFE_Activate:
			{
				EntityId checkId = GetPortEntityId( pActInfo, 0 );
			
				if (checkId)
				{
					EntityId client = g_pGame->GetGameRules()->GetCoopPlayerClient();
					EntityId host = g_pGame->GetGameRules()->GetCoopPlayerHost();
					
					if (host==checkId)
						ActivateOutput(pActInfo, eOUT_Player1, host);
					if (client==checkId)
						ActivateOutput(pActInfo, eOUT_Player2, client);
				}
				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////

class CFlowNode_CoopPlayerCount : public CFlowBaseNode
{
public:
	CFlowNode_CoopPlayerCount( SActivationInfo * pActInfo )
	{
	}

	enum EOutputs
	{
		eOUT_1Player,
		eOUT_2Players,
		eOUT_Amount
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Check", _HELP("Triggers the check")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void("1Player", _HELP("Activated if only 1 player is in game")),
			OutputPortConfig_Void("2Players", _HELP("Activated if 2 players are in game")),
			OutputPortConfig<int>("Amount", _HELP("Number of players that are currently in game")),
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Check number of players that are currently in game");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
			case eFE_Initialize:
			case eFE_Activate:
			{
				int numPlayers = g_pGame->GetGameRules()->GetPlayerCount();
				
				if (!gEnv->bServer)
				{
					IActorSystem *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
					IActorIteratorPtr actorIter = pActorSystem->CreateActorIterator();
					
					numPlayers = 0;
					while(IActor *pActor = actorIter->Next())
					{
						if (pActor->GetChannelId())
							++numPlayers;
					}
				}
				
				if (numPlayers==1)					
					ActivateOutput( pActInfo, eOUT_1Player, true );
				if (numPlayers==2)
					ActivateOutput( pActInfo, eOUT_2Players, true );
				ActivateOutput( pActInfo, eOUT_Amount, numPlayers );
				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////

class CFlowNode_CoopClientEnter : public CFlowBaseNode, public SGameRulesListener
{
public:
	CFlowNode_CoopClientEnter( SActivationInfo * pActInfo )
	{
	}
	
	~CFlowNode_CoopClientEnter()
	{
		CGameRules* pGameRules = g_pGame->GetGameRules();
		if(pGameRules)
			pGameRules->RemoveGameRulesListener(this);
	}

	enum EOutputs
	{
		eOUT_ClientId
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<EntityId>("ClientId", _HELP("Outputs entity id of client who joins the game")),
			{0}
		};
		config.pInputPorts = 0;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Checks when a client enters the game and outputs his id");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
			case eFE_Initialize:
			{
				m_actInfo = *pActInfo;
				CGameRules* pGameRules = g_pGame->GetGameRules();
				if (pGameRules)
					pGameRules->AddGameRulesListener( this );  // can be called many times. but the code in game rules takes care of that
				break;
			}
		}
	}

	// inherited from SGameRulesListener	
	virtual void ClientEnteredGame( EntityId clientId )
	{
		ActivateOutput( &m_actInfo, eOUT_ClientId, clientId );
	}
	

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
	
	SActivationInfo m_actInfo;
};


//////////////////////////////////////////////////////////////////////////

class CFlowNode_CoopClientDisconnect : public CFlowBaseNode, public SGameRulesListener
{
public:
	CFlowNode_CoopClientDisconnect( SActivationInfo * pActInfo )
	{
	}

	~CFlowNode_CoopClientDisconnect()
	{
		CGameRules* pGameRules = g_pGame->GetGameRules();
		if(pGameRules)
			pGameRules->RemoveGameRulesListener(this);
	}

	enum EOutputs
	{
		eOUT_ClientId
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<EntityId>("ClientId", _HELP("Outputs entity id of client who disconnects")),
			{0}
		};
		config.pInputPorts = 0;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Checks when a client disconnects from the server and outputs his id");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
			case eFE_Initialize:
			{
				m_actInfo = *pActInfo;
				CGameRules* pGameRules = g_pGame->GetGameRules();
				if (pGameRules)
					pGameRules->AddGameRulesListener( this );  // can be called many times. but the code in game rules takes care of that
				break;
			}
		}
	}

	// inherited from SGameRulesListener	
	virtual void ClientDisconnect( EntityId clientId )
	{
		ActivateOutput( &m_actInfo, eOUT_ClientId, clientId );
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
	
	SActivationInfo m_actInfo;
};


//////////////////////////////////////////////////////////////////////////
REGISTER_FLOW_NODE( "Coop:BranchPlayers",	CFlowNode_CoopBranchPlayers );
REGISTER_FLOW_NODE( "Coop:PlayerCount",	CFlowNode_CoopPlayerCount );
REGISTER_FLOW_NODE( "Coop:ClientDisconnect",	CFlowNode_CoopClientDisconnect );
REGISTER_FLOW_NODE( "Coop:ClientEnter",	CFlowNode_CoopClientEnter );
