/********************************************************************
	Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2005.
 -------------------------------------------------------------------------
  File name:   FlowNode_AI.cpp
	$Id$
  Description: place for miscellaneous AI related flow graph nodes
  
 -------------------------------------------------------------------------
  History:
  - 15:6:2005   15:24 : Created by Kirill Bulatsev

*********************************************************************/



#include "StdAfx.h"
#include <IAIAction.h>
#include <IAISystem.h>
#include <IAgent.h>
#include "FlowBaseNode.h"
#include "FlowEntityNode.h"
#include "AIProxy.h"

#include "ICachedAIValues.h"

#include <IInterestSystem.h>
#include "INavigation.h"
#include "IAIObjectManager.h"
#include "IAIActor.h"
#include <ICoordinationManager.h>
/*
//////////////////////////////////////////////////////////////////////////
// broadcast AI signal
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISignal : public CFlowBaseNode
{
public:
	CFlowNode_AISignal( SActivationInfo * pActInfo )
	{
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<bool>("send"),
			InputPortConfig<string>("signal",_HELP("AI signal to be send") ),
			InputPortConfig<Vec3>("pos" ),
			InputPortConfig<float>("radius" ),
			{0}
		};
		config.sDescription = _HELP( "broadcast AI signal" );
		config.pInputPorts = in_config;
		config.pOutputPorts = 0;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(event != eFE_Activate || !IsBoolPortActive(pActInfo, 0))
			return;
		string signalText = GetPortString(pActInfo,1);

		Vec3	pos = *pActInfo->pInputPorts[2].GetPtr<Vec3>();
		//		Vec3	pos = GetPortVec3(pActInfo,2);
		float	radius = GetPortFloat(pActInfo,3);
		IAISystem *pAISystem(gEnv->pAISystem);

		pAISystem->SendAnonymousSignal(1, signalText, pos, radius, NULL);
	}
private:
};
*/


//////////////////////////////////////////////////////////////////////////
// Global AI enemies awareness of player.
// this supposed to represent how much player is exposed to the AI enemies
// currently
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIAwareness : public CFlowBaseNode
{
public:
	CFlowNode_AIAwareness( SActivationInfo * pActInfo )
	{
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<float>( "awareness", _HELP("Global AI enemies awareness of the player\n0 - Minimum (Green)\n100 - Maximum (Red)") ),
			{0}
		};
		config.sDescription = _HELP( "Global AI enemies awareness of the player" );
		config.pInputPorts = 0;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if ( event == eFE_Update )
		{
			IAISystem* pAISystem = gEnv->pAISystem;

			SAIDetectionLevels levels;
			gEnv->pAISystem->GetDetectionLevels(0, levels);
			float currentAwareness = max(levels.vehicleThreat, levels.puppetThreat) * 100.0f;
			ActivateOutput( pActInfo, 0, currentAwareness );
		}
		else if ( event == eFE_Initialize )
		{
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			ActivateOutput( pActInfo, 0, 0 );
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Global AI enemies alertness level.
// It represents the current alertness level of AI enemies, more precisely, 
// the highest level of alertness of any active agent. It should be 
// used to select appropriate music theme or whatever else
//////////////////////////////////////////////////////////////////////////
// this node now does a polling thru all actors only every interval of time (defined in a constant), to avoid performance problems.
// but the right solution would be that the AISystem store a generic alertness value, updated from the active AIs. And probably a similar system for the AIActiveCount
class CFlowNode_AIAlertness : public CFlowBaseNode
{
public:
	CFlowNode_AIAlertness( SActivationInfo * pActInfo )
		: m_localAlertness(-1)
	{
	}
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowNode_AIAlertness(pActInfo);
	}
	void Serialize( SActivationInfo * pActInfo, TSerialize ser )
	{
		ser.Value("currentAlertness", m_currentAlertness);
		ser.Value("m_localAlertness", m_localAlertness);
		ser.Value("timeNextUpdate", m_timeNextUpdate );
	}
	
	enum EOutputPorts
	{
		eOUT_Alertness = 0,
		eOUT_Green,
		eOUT_Orange,
		eOUT_Red
	};
	
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<int>("alertness", "0 - green\n1 - orange\n2 - red"),
			OutputPortConfig_Void("green"),
			OutputPortConfig_Void("orange"),
			OutputPortConfig_Void("red"),
			{0}
		};

		config.sDescription = _HELP( "The highest level of alertness of any agent in the level" );
		config.pInputPorts = 0;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event == eFE_Initialize)
		{
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			m_localAlertness = -1; // to force a local update
			m_currentAlertness = 0;
			m_timeNextUpdate = -1; // to force a global update
		}
		
		if (event != eFE_Update)
			return;
			
		float currTime = gEnv->pTimer->GetCurrTime();

		if (currTime>m_timeNextUpdate)
		{
			CalculateCurrentAlertness();
			m_timeNextUpdate = currTime + UPDATE_INTERVAL;
		}

		if (m_localAlertness!=m_currentAlertness)
		{
			m_localAlertness = m_currentAlertness;
			ActivateOutput( pActInfo, eOUT_Alertness, m_currentAlertness );
			uint32 colorPort = min( max( m_localAlertness + 1, eOUT_Green ), eOUT_Red );  // clamp between Green and Red
			ActivateOutput( pActInfo, colorPort, true );
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
private:
	void CalculateCurrentAlertness()
	{
		IActorSystem *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
		IActorIteratorPtr actorIt = pActorSystem->CreateActorIterator();
		int highestAlertness = -1;
		while (IActor *pActor=actorIt->Next())
		{
			if (pActor)
			{
				const IAIObject* pAIObject = pActor->GetEntity()->GetAI();
				if (pAIObject)
				{
					const IAIActor* pAIActor = pAIObject->CastToIAIActor();
					highestAlertness = max(highestAlertness, pAIActor->GetProxy()->GetAlertnessState());
				}
			}
		}

		m_currentAlertness = highestAlertness;
	}

	// using statics instead a singleton node because we want the outputs to be activated only on change. 
	int m_localAlertness;
	static int m_currentAlertness;
	static float m_timeNextUpdate;
	static const float UPDATE_INTERVAL;
};

int CFlowNode_AIAlertness::m_currentAlertness = 0;
float CFlowNode_AIAlertness::m_timeNextUpdate = 0;
const float CFlowNode_AIAlertness::UPDATE_INTERVAL = 0.8f;    // seconds


//////////////////////////////////////////////////////////////////////////
// Counts how many AIs are active
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIActiveCount : public CFlowBaseNode
{
	enum EOutputPorts
	{
		eOUT_AICount = 0,
		eOUT_EnemyCount
	};
public:
	CFlowNode_AIActiveCount( SActivationInfo * pActInfo )
		: m_localAICount(0)
		, m_localAIEnemyCount(0)
	{
	}
	void Serialize( SActivationInfo * pActInfo, TSerialize ser )
	{
		ser.Value("LocalAICount", m_localAICount);
		ser.Value("LocalAIEnemyCount", m_localAIEnemyCount);
		ser.Value("CurrentAICount", m_currentAICount);
		ser.Value("CurrentAIEnemyCount", m_currentAIEnemyCount);
		ser.Value("timeNextUpdate", m_timeNextUpdate );
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<int>("Total"),
			OutputPortConfig<int>("Enemy"),
			{0}
		};

		config.sDescription = _HELP( "Counts how many AIs are active" );
		config.pInputPorts = 0;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	} 
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event == eFE_Initialize)
		{
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			m_localAICount = m_localAIEnemyCount = -1; // to force a local update
			m_currentAICount = m_currentAIEnemyCount = 0;
			m_timeNextUpdate = -1; // to force a global update
		}

		if (event != eFE_Update)
			return;

		float currTime = gEnv->pTimer->GetCurrTime();

		if (currTime>m_timeNextUpdate)
		{
			RecalcAICounters();
			m_timeNextUpdate = currTime + UPDATE_INTERVAL;
		}

		if (m_localAICount!=m_currentAICount)
		{
			m_localAICount = m_currentAICount;
			ActivateOutput( pActInfo, eOUT_AICount, m_localAICount );
		}
		if (m_localAIEnemyCount!=m_currentAIEnemyCount)
		{
			m_localAIEnemyCount = m_currentAIEnemyCount;
			ActivateOutput( pActInfo, eOUT_EnemyCount, m_localAIEnemyCount );
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
private:

	void RecalcAICounters()
	{
		bool changed = false;
		int AICount   = 0;
		int AIEnemies = 0;

		const IActor*     pLocalPlayer    = CCryAction::GetCryAction()->GetClientActor();
		const IAIObject*  pAILocalPlayer  = pLocalPlayer ? pLocalPlayer->GetEntity()->GetAI() : NULL;

		IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
		while (IActor *pActor = pIter->Next())
		{
			if (!pActor->IsPlayer() && pActor->GetEntity()->IsActive())
			{
				const IAIObject* pAIObject = pActor->GetEntity()->GetAI();
				if (pAIObject)
				{
					const IAIActor* pAIActor = pAIObject->CastToIAIActor();
					if (pAIActor->IsActive())
					{
						AICount++;
						if (pAIObject->IsHostile( pAILocalPlayer ))
							AIEnemies++;
					}
				}
			}
		}
		m_currentAICount = AICount;
		m_currentAIEnemyCount = AIEnemies;
	}

private:
	// using statics instead a singleton node because we want the outputs to be activated only on change. 
	int m_localAICount;
	int m_localAIEnemyCount;
	static int m_currentAICount;
	static int m_currentAIEnemyCount;
	static float m_timeNextUpdate;
	static const float UPDATE_INTERVAL;
};

int CFlowNode_AIActiveCount::m_currentAICount = 0;
int CFlowNode_AIActiveCount::m_currentAIEnemyCount = 0;
float CFlowNode_AIActiveCount::m_timeNextUpdate = 0;
const float CFlowNode_AIActiveCount::UPDATE_INTERVAL = 0.30f;    // seconds


//////////////////////////////////////////////////////////////////////////
// Filters the inputs depending on owner's AI alertness level.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIAlertnessFilter : public CFlowBaseNode
{
public:
	CFlowNode_AIAlertnessFilter( SActivationInfo * pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<int>( "Threshold", 1, _HELP("The alertness level needed to output the values on the Input port to the High port.\nIf current alertness is less than this then the values are sent to the Low output port.") ),
			InputPortConfig_AnyType( "Input", _HELP("The group ID of the group to get the alertness from") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_AnyType( "Low", _HELP("Outputs here all values passed on Input if current alertness is less than threshold value") ),
			OutputPortConfig_AnyType( "High", _HELP("Outputs here all values passed on Input if current alertness is equal or greater than threshold value") ),
			{0}
		};
		config.sDescription = _HELP( "Filters the inputs depending on owner's AI alertness level." );
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory( EFLN_ADVANCED );
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if ( !pActInfo->pEntity )
			return;

		if ( event != eFE_Initialize && event != eFE_Activate )
			return;

		if ( !IsPortActive(pActInfo,1) )
			return;

		EntityId entityID = pActInfo->pEntity->GetId();
		IActorSystem *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
		IActor *pActor = pActorSystem->GetActor(entityID);
		if(!pActor)
			return;

		// read the input value...
		// ...and depending on threshold...
		/*int currentAlertness = pOldActor->GetCachedAIValues()->GetAlertnessState();
		// ... output it to one of two possible output ports

		int outValue = ( currentAlertness < GetPortInt(pActInfo,0) ) ? 0 : 1;
		ActivateOutput( pActInfo, outValue, GetPortAny(pActInfo, 1) );*/
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Group AI enemies alertness level.
// It represents the current alertness level of AI enemies, more precisely, 
// the highest level of alertness of any active agent in a group.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIGroupAlertness : public CFlowBaseNode
{
public:
	CFlowNode_AIGroupAlertness( SActivationInfo * pActInfo )
		: m_CurrentAlertness(0)
	{
	}
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowNode_AIGroupAlertness(pActInfo);
	}
	void Serialize( SActivationInfo * pActInfo, TSerialize ser )
	{
		ser.Value("CurrentAlertness", m_CurrentAlertness);
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<int>("alertness", "0 - green\n1 - orange\n2 - red"),
			OutputPortConfig_Void("green"),
			OutputPortConfig_Void("orange"),
			OutputPortConfig_Void("red"),
			{0}
		};

		static const SInputPortConfig in_config[] = {
			InputPortConfig<int>( "groupID", _HELP("The group ID of the group to get the alertness from") ),
			{0}
		};

		config.sDescription = _HELP( "The highest level of alertness of any agent in the group." );
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event == eFE_Initialize)
		{
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			m_CurrentAlertness = 0;
			ActivateOutput( pActInfo, 0, m_CurrentAlertness );
		}
		else if (event == eFE_Update)
		{
			// Get the max alertness of the group.
			int	maxAlertness = 0;
			IAISystem* pAISystem = gEnv->pAISystem;
			int	groupID = GetPortInt(pActInfo, 0);		// group ID
			int n = pAISystem->GetGroupCount(groupID, IAISystem::GROUP_ENABLED);
			for (int i = 0; i < n; i++)
			{
				IAIObject*	pMember = pAISystem->GetGroupMember(groupID, i, IAISystem::GROUP_ENABLED);
				if (pMember && pMember->GetProxy())
					maxAlertness = max(maxAlertness, pMember->GetProxy()->GetAlertnessState());
			}

			if (maxAlertness != m_CurrentAlertness)
			{
				m_CurrentAlertness = maxAlertness;
				ActivateOutput(pActInfo, 0, m_CurrentAlertness);
				ActivateOutput(pActInfo, m_CurrentAlertness + 1, true);
			}
		}

	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
private:
	int	m_CurrentAlertness;
};

//////////////////////////////////////////////////////////////////////////
// AI:AIMergeGroups
// Merges two AI groups.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIMergeGroups : public CFlowBaseNode
{
public:
	CFlowNode_AIMergeGroups(SActivationInfo * pActInfo) {}
	virtual void GetConfiguration(SFlowNodeConfig & config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_AnyType("sink"),
			InputPortConfig<int>("fromID", 0, _HELP("The group which is to be merged.")),
			InputPortConfig<int>("toID", 0, _HELP("The group to merge to.")),
			InputPortConfig<bool>("enableFromGroup", true, _HELP("Set if the members of the 'from' group should be enabled before merging.")),
			{0}
		};

		config.sDescription = _HELP("Merges AI group 'from' to group 'to'.");
		config.pInputPorts = in_config;
		config.pOutputPorts = 0;
		config.SetCategory(EFLN_ADVANCED);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		if (event == eFE_Activate && IsPortActive(pActInfo, 0))
		{
			int	fromID = GetPortInt(pActInfo, 1);
			int	toID = GetPortInt(pActInfo, 2);
			bool enable = GetPortBool(pActInfo, 3);

			// Collect members
			std::vector<IAIObject*> members;
			AutoAIObjectIter it(gEnv->pAISystem->GetAIObjectManager()->GetFirstAIObject(IAISystem::OBJFILTER_GROUP, fromID));
			for (; it->GetObject(); it->Next())
				members.push_back(it->GetObject());

			// Enable entities if they are not currently enabled.
			if (enable)
			{
				for (unsigned i = 0, ni = members.size(); i < ni; ++i)
				{
					IEntity* pEntity = members[i]->GetEntity();
					if (pEntity)
					{
						IEntityScriptProxy* pScriptProxy = (IEntityScriptProxy*)pEntity->GetProxy(ENTITY_PROXY_SCRIPT);
						if (pScriptProxy)
							pScriptProxy->CallEvent("Enable");
					}
				}
			}

			// Change group IDs
			for (unsigned i = 0, ni = members.size(); i < ni; ++i)
				members[i]->SetGroupId(toID);

			// Send signals to notify about the change.
			for (unsigned i = 0, ni = members.size(); i < ni; ++i)
			{
				IAIActor* pAIActor = members[i]->CastToIAIActor();
				if (pAIActor)
					pAIActor->SetSignal(10, "OnGroupChanged", members[i]->GetEntity(), 0);
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////
// AI:GroupBeacon node.
// This node returns the position of the group beacon.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIGroupBeacon : public CFlowBaseNode
{
public:
	CFlowNode_AIGroupBeacon( SActivationInfo * pActInfo ) : m_lastVal(ZERO)
	{
	}

	IFlowNodePtr Clone( SActivationInfo* pActInfo )
	{
		return new CFlowNode_AIGroupBeacon( pActInfo );
	}

	void Serialize(SActivationInfo *, TSerialize ser)
	{
		// forces re-triggering on load
		if (ser.IsReading())
			m_lastVal.Set(0,0,0);
	}

	virtual void GetConfiguration( SFlowNodeConfig &config )
	{

		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<Vec3>( "pos", _HELP("Beacon position") ),
			{0}
		};
		static const SInputPortConfig in_config[] = {
			InputPortConfig<int>( "groupID", _HELP("The group ID of the group to get the beacon from") ),
			{0}
		};
		config.sDescription = _HELP( "Outputs AI Group's beacon position" );
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event == eFE_Initialize)
		{
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			m_lastVal.Set(0,0,0);
			ActivateOutput(pActInfo,0,m_lastVal);
		}
		else if (event == eFE_Update)
		{
			IAISystem*	pAISystem = gEnv->pAISystem;
			int	groupID = GetPortInt(pActInfo, 0);

			IAIObject* pBeacon = pAISystem->GetBeacon(groupID);
			Vec3	beacon(ZERO);
			if (pBeacon)
				beacon = pBeacon->GetPos();
			if (beacon != m_lastVal)
			{
				ActivateOutput(pActInfo,0,beacon);
				m_lastVal = beacon;
			}
		}
	}
	Vec3	m_lastVal;

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////
// AI:GroupID node.
// Returns the groupID of the entity.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIGroupID : public CFlowBaseNode
{
public:
	CFlowNode_AIGroupID( SActivationInfo * pActInfo ) : m_lastID(-1)
	{
	}

	IFlowNodePtr Clone( SActivationInfo* pActInfo )
	{
		return new CFlowNode_AIGroupID( pActInfo );
	}

	void Serialize(SActivationInfo *, TSerialize ser)
	{
		// forces re-triggering on load
		if (ser.IsReading())
			m_lastID = -1;
	}

	virtual void GetConfiguration( SFlowNodeConfig &config )
	{

		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<int>( "groupID", _HELP("AI Group ID of the entity") ),
			{0}
		};
		static const SInputPortConfig in_config[] = {
			InputPortConfig_AnyType( "sink" ),
			InputPortConfig<int>("groupID", _HELP("New group ID to set to the entity.")),
			{0}
		};

		config.sDescription = _HELP( "Sets and outputs AI's group ID" );
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(event == eFE_Initialize)
		{
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
		}
		else if(event == eFE_SetEntityId || ( event == eFE_Activate && IsPortActive(pActInfo, 0)) )
		{
			IEntity* pEntity = pActInfo->pEntity;
			if(pEntity  )
			{
				IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
				if( pAIActor )
				{
					if (event == eFE_Activate)
					{
						// set group of ai object
						AgentParameters ap = pAIActor->GetParameters();
						ap.m_nGroup = GetPortInt(pActInfo,1);
						pAIActor->SetParameters(ap);
					}
					m_lastID = pAIActor->GetParameters().m_nGroup;
					ActivateOutput(pActInfo,0,pAIActor->GetParameters().m_nGroup);
				}
			}
		}
		else if(event == eFE_Update)
		{
			// Update the output when it changes.
			IEntity* pEntity = pActInfo->pEntity;
			if(pEntity  )
			{
				IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
				if( pAIActor )
				{
					int	id = pAIActor->GetParameters().m_nGroup;
					if (m_lastID != id)
					{
						m_lastID = id;
						ActivateOutput(pActInfo,0,id);
					}
				}
			}
		}
	}
	int	m_lastID;

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////
// AI Group count.
// Returns the number of entities in a group.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIGroupCount : public CFlowBaseNode
{
public:
	CFlowNode_AIGroupCount( SActivationInfo * pActInfo )
		: m_CurrentCount(-1), m_decreasing(false)
	{
	}
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowNode_AIGroupCount(pActInfo);
	}
	void Serialize( SActivationInfo * pActInfo, TSerialize ser )
	{
		ser.Value("CurrentCount", m_CurrentCount);
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<int>("count", _HELP("The number of agents in a group.")),
			OutputPortConfig_Void("empty", _HELP("Triggered when the whole group is wasted.")),
			{0}
		};

		static const SInputPortConfig in_config[] = {
			InputPortConfig<int>("groupID", _HELP("The group ID of the group to get the alertness from")),
			{0}
		};

		config.sDescription = _HELP( "The highest level of alertness of any agent in the group." );
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event == eFE_Initialize)
		{
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			IAISystem* pAISystem = gEnv->pAISystem;
			int	groupID = GetPortInt( pActInfo, 0 );		// group ID
			int	n = pAISystem->GetGroupCount( groupID, true );

			m_CurrentCount = n;
			ActivateOutput( pActInfo, 0, m_CurrentCount );
			m_decreasing = false;
		}
		else if (event == eFE_Update)
		{
			IAISystem* pAISystem = gEnv->pAISystem;
			int	groupID = GetPortInt( pActInfo, 0 );		// group ID
			int	n = pAISystem->GetGroupCount( groupID, true );

			// Get the max alertness of the group.
			if ( n != m_CurrentCount )
			{
				m_decreasing = n < m_CurrentCount;

				// If the group count has changed, update it.
				m_CurrentCount = n;
				ActivateOutput( pActInfo, 0, m_CurrentCount );
				// If the current count is zero activate the empty output.
				if ( m_CurrentCount == 0 && m_decreasing )
				{
					ActivateOutput( pActInfo, 1, true );
				}
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

private:
	int	m_CurrentCount;
	bool	m_decreasing;
};

//////////////////////////////////////////////////////////////////////////
// AI:ActionStart node.
// This should be used as start node in AI Action graphs.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIActionStart : public CFlowBaseNode
{
public:
	CFlowNode_AIActionStart( SActivationInfo * pActInfo )
	{
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] =
		{
			InputPortConfig_Void( "restart", "Restarts execution of the whole AI Action from the beginning" ),
			InputPortConfig_Void( "cancel", "Cancels execution" ),
			InputPortConfig_Void( "suspend", "Suspends execution" ),
			InputPortConfig_Void( "resume", "Resumes execution if it was suspended" ),
			{0}
		};
		static const SOutputPortConfig out_config[] =
		{
			OutputPortConfig< EntityId >( "UserId", "Entity ID of the agent executing AI Action" ),
			OutputPortConfig< EntityId >( "ObjectId", "Entity ID of the object on which the agent is executing AI Action" ),
			OutputPortConfig< Vec3 >( "Position", "World position of smart object action." ),
			{0}
		};
		config.pInputPorts = 0; // in_config;
		config.pOutputPorts = out_config;
		config.sDescription = _HELP( "Represents the User and the used Object of an AI Action.\nUse this node to start the AI Action graph." );
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		IAIAction* pAIAction = pActInfo->pGraph->GetAIAction();
		switch ( event )
		{
		case eFE_Update:
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
			if ( pAIAction )
			{
				IEntity* pUser = pAIAction->GetUserEntity();

				if(pUser && pUser->GetAI())
				{
					IAIActor *pAIActor = pUser->GetAI()->CastToIAIActor();
					Vec3 vSOPosition = pAIActor->GetState()->actorTargetReq.animFinalLocation;
					ActivateOutput( pActInfo, 2, vSOPosition);
				}

				ActivateOutput( pActInfo, 0, pUser ? pUser->GetId() : 0 );
				IEntity* pTarget = pAIAction->GetObjectEntity();
				ActivateOutput( pActInfo, 1, pTarget ? pTarget->GetId() : 0 );
			}
			break;
		case eFE_Initialize:
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			break;
		case eFE_Activate:
			break;
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////
// AI:ActionEnd node.
// This should be used as end node in AI Action graphs.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIActionEnd : public CFlowBaseNode
{
public:
	CFlowNode_AIActionEnd( SActivationInfo * pActInfo )
	{
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] =
		{
			InputPortConfig_Void( "end", _HELP("ends execution of AI Action reporting it as succeeded") ),
			InputPortConfig_Void( "cancel", _HELP("ends execution of AI Action reporting it as failed") ),
			{0}
		};
		config.pInputPorts = in_config;
		config.pOutputPorts = NULL;
		config.sDescription = _HELP( "Use this node to end the AI Action graph." );
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		IAIAction* pAIAction = pActInfo->pGraph->GetAIAction();
		if ( !pAIAction )
			return;
		if ( event != eFE_Activate )
			return;

		if ( IsPortActive(pActInfo, 0) )
			pAIAction->EndAction();
		else if ( IsPortActive(pActInfo, 1) )
			pAIAction->CancelAction();
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////
// AI:ActionAbort node.
// This node serves as a destructor in AI Action graphs.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIActionAbort : public CFlowBaseNode
{
protected:
	bool bAborted;

public:
	CFlowNode_AIActionAbort( SActivationInfo * pActInfo )
	{
		bAborted = false;
	}
	IFlowNodePtr Clone( SActivationInfo* pActInfo )
	{
		return new CFlowNode_AIActionAbort( pActInfo );
	}
	void Serialize( SActivationInfo * pActInfo, TSerialize ser )
	{
		ser.Value("Aborted", bAborted);
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] =
		{
			InputPortConfig_Void( "Abort", "Aborts execution of AI Action" ),
			{0}
		};
		static const SOutputPortConfig out_config[] =
		{
			OutputPortConfig< EntityId >( "UserId", "Entity ID of the agent executing AI Action (activated on abort)" ),
			OutputPortConfig< EntityId >( "ObjectId", "Entity ID of the object on which the agent is executing AI Action (activated on abort)" ),
			{0}
		};
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = _HELP( "Use this node to define clean-up procedure executed when AI Action is aborted." );
		config.SetCategory( EFLN_APPROVED );
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		IAIAction* pAIAction = pActInfo->pGraph->GetAIAction();
		switch ( event )
		{
		case eFE_Update:
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
			if ( pAIAction && !bAborted )
			{
				// abort only once
				bAborted = true;

				IEntity* pUser = pAIAction->GetUserEntity();
				ActivateOutput( pActInfo, 0, pUser ? pUser->GetId() : 0 );
				IEntity* pTarget = pAIAction->GetObjectEntity();
				ActivateOutput( pActInfo, 1, pTarget ? pTarget->GetId() : 0 );
			}
			break;
		case eFE_Initialize:
			bAborted = false;
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
			break;
		case eFE_Activate:
			if ( !bAborted && pAIAction && IsPortActive(pActInfo, 0) )
				pAIAction->AbortAction();
			break;
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////
// AI:AttTarget node.
// Returns the position of the attention target.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIAttTarget : public CFlowBaseNode
{
public:
	CFlowNode_AIAttTarget( SActivationInfo * pActInfo )
	{
	}

	enum INPUTS
	{
		eIP_Active = 0,
	};
	
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<bool>( "Active", true, _HELP("Activate/deactivate the node") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<Vec3>( "pos", _HELP("Attention target position") ),
			OutputPortConfig<EntityId>( "entityId", _HELP("Entity id of attention target") ),
			OutputPortConfig_Void( "none", _HELP("Activated when there's no attention target") ),
			{0}
		};
		config.sDescription = _HELP( "Outputs AI's attention target" );
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch(event)
		{
			case eFE_Initialize:
			{
				bool needUpdate = (IsPortActive(pActInfo, -1) && GetPortBool(pActInfo, eIP_Active));
				pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, needUpdate );
				break;
			}
			
			case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIP_Active))
					pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, GetPortBool(pActInfo, eIP_Active) );
				break;
			}
			
			case eFE_Update:
			{
				IEntity* pEntity = pActInfo->pEntity;
				if ( pEntity )
				{
					IPipeUser* pPiper = CastToIPipeUserSafe(pEntity->GetAI());
					if (pPiper)
					{
						IAIObject* pTarget = pPiper->GetAttentionTarget();
						if ( pTarget )
						{
							ActivateOutput( pActInfo, 0, pTarget->GetPos() );

							if (pTarget->CastToIPipeUser() || pTarget->CastToCAIPlayer())
							{
								pEntity = pTarget->GetEntity();
								if ( pEntity )
									ActivateOutput( pActInfo, 1, pEntity->GetId() );
								else
									ActivateOutput( pActInfo, 1, 0 );
							}
							else
								ActivateOutput( pActInfo, 1, 0 );
						}
						else
						{
							ActivateOutput( pActInfo, 1, 0 );
							ActivateOutput( pActInfo, 2, true );
						}
					}
				}
				break;
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////
// AI:SmartObjectHelper node.
// This node gives position and orientation of any Smart Object Helper
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISOHelper : public CFlowEntityNodeBase
{
public:
	CFlowNode_AISOHelper( SActivationInfo * pActInfo )
		: CFlowEntityNodeBase()
	{
		m_event = ENTITY_EVENT_XFORM;
		m_nodeID = pActInfo->myID;
		m_pGraph = pActInfo->pGraph;
		m_entityId = (EntityId)(UINT_PTR)pActInfo->m_pUserData;
	}

	virtual IFlowNodePtr Clone( SActivationInfo *pActInfo ) { pActInfo->m_pUserData = (void*)(UINT_PTR)m_entityId; return new CFlowNode_AISOHelper(pActInfo); };

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<string>( "soclass_class", _HELP("Smart Object Class for which the helper should be defined") ),
			InputPortConfig<string>( "sonavhelper_helper", _HELP("Helper name") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<Vec3>( "pos", _HELP("Position of the Smart Object Helper in world coordinates") ),
			OutputPortConfig<Vec3>( "fwd", _HELP("Forward direction of the Smart Object Helper in world coordinates") ),
			OutputPortConfig<Vec3>( "up", _HELP("Up direction of the Smart Object Helper in world coordinates") ),
			{0}
		};
		config.sDescription = _HELP( "Outputs AI's attention target parameters" );
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		CFlowEntityNodeBase::ProcessEvent( event, pActInfo );

		/*if ( event == eFE_Edited )
		{
			// sent from the editor when an input port is edited
			string className = GetPortString( pActInfo, 0 );
			string helperName = GetPortString( pActInfo, 1 );
			int pos = helperName.find(':');
			if ( pos <= 0 || className != helperName.substr(0,pos) )
			{
			}
			return;
		}*/

		if ( event != eFE_SetEntityId)
			return;

		Compute( pActInfo->pEntity );
	}

	void Compute( IEntity* pEntity )
	{
		if ( !pEntity )
			return;

		const string* pClassName = m_pGraph->GetInputValue( m_nodeID, 1 )->GetPtr<string>(); // GetPortString( pActInfo, 0 );
		const string* pHelperName = m_pGraph->GetInputValue( m_nodeID, 2 )->GetPtr<string>(); // GetPortString( pActInfo, 1 );
		if ( !pClassName || !pHelperName )
			return;

		string helperName = *pHelperName;
		string className = *pClassName;

		helperName.erase( 0, helperName.find(':')+1 );
		if ( className.empty() || helperName.empty() )
			return;

		SmartObjectHelper* pHelper = gEnv->pAISystem->GetSmartObjectManager()->GetSmartObjectHelper( className, helperName );
		if ( !pHelper )
			return;

		// calculate position and output it
		Vec3 pos = pEntity->GetWorldTM().TransformPoint( pHelper->qt.t );
		//ActivateOutput( pActInfo, 0, pos );
		m_pGraph->ActivatePort( SFlowAddress( m_nodeID, 0, true ), pos );

		// calculate forward direction and output it
		pos = pHelper->qt.q * FORWARD_DIRECTION;
		pos = pEntity->GetWorldTM().TransformVector( pos );
		pos.NormalizeSafe( FORWARD_DIRECTION );
		//ActivateOutput( pActInfo, 1, pos );
		m_pGraph->ActivatePort( SFlowAddress( m_nodeID, 1, true ), pos );

		// calculate up direction and output it
		pos = pHelper->qt.q * Vec3(0,0,1);
		pos = pEntity->GetWorldTM().TransformVector( pos );
		pos.NormalizeSafe( Vec3(0,0,1) );
		//ActivateOutput( pActInfo, 2, pos );
		m_pGraph->ActivatePort( SFlowAddress( m_nodeID, 2, true ), pos );
	}


	//////////////////////////////////////////////////////////////////////////
	// IEntityEventListener
	virtual void OnEntityEvent( IEntity *pEntity,SEntityEvent &event )
	{
		if ( !m_pGraph->IsEnabled() || m_pGraph->IsSuspended() || !m_pGraph->IsActive() )
			return;

		if ( event.event == ENTITY_EVENT_XFORM )
			Compute( pEntity );
	};
	//////////////////////////////////////////////////////////////////////////

protected:
	IFlowGraph *m_pGraph;
	TFlowNodeId m_nodeID;
};


//////////////////////////////////////////////////////////////////////////
// AI:SmartObjectEvent node.
// Triggers a smart object event.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISmartObjectEvent : public CFlowBaseNode
{
public:
	CFlowNode_AISmartObjectEvent( SActivationInfo * pActInfo ) {}

	//virtual IFlowNodePtr Clone( SActivationInfo *pActInfo ) { return new CFlowNode_AISmartObjectEvent(pActInfo); };

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<string>( "soevent_event", _HELP("smart object event to be triggered") ),
			InputPortConfig_Void( "trigger", _HELP("triggers the smart object event") ),
			InputPortConfig<EntityId>( "userId", _HELP("if used will limit the event on only one user") ),
			InputPortConfig<EntityId>( "objectId", _HELP("if used will limit the event on only one object") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<EntityId>( "userId", _HELP("the used selected by smart object rules") ),
			OutputPortConfig<EntityId>( "objectId", _HELP("the object selected by smart object rules") ),
  			OutputPortConfig_Void( "ruleName", _HELP("activated if a matching rule was found"), _HELP("start") ),
			OutputPortConfig_Void( "noRule", _HELP("activated if no matching rule was found") ),
		//	OutputPortConfig_Void( "Done", _HELP("activated after the executed action is finished") ),
		//	OutputPortConfig_Void( "Succeed", _HELP("activated after the executed action is successfully finished") ),
		//	OutputPortConfig_Void( "Fail", _HELP("activated if the executed action was failed") ),
			{0}
		};
		config.sDescription = _HELP( "Triggers a smart object event" );
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch ( event )
		{
		case eFE_Initialize:
			break;

		case eFE_Activate:
			if ( IsPortActive(pActInfo, 1) )
				pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			break;

		case eFE_Update:
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );

			EntityId userId = GetPortEntityId( pActInfo, 2 );
			EntityId objectId = GetPortEntityId( pActInfo, 3 );

			IEntity* pUser = userId ? gEnv->pEntitySystem->GetEntity( userId ) : NULL;
			IEntity* pObject = objectId ? gEnv->pEntitySystem->GetEntity( objectId ) : NULL;

			const string& actionName = GetPortString( pActInfo, 0 );
			if ( !actionName.empty() )
			{
				bool highPriority = false;
				if ( IAIAction* pAIAction = pActInfo->pGraph->GetAIAction() )
					highPriority = pAIAction->IsHighPriority();
				int id = gEnv->pAISystem->GetSmartObjectManager()->SmartObjectEvent( actionName, pUser, pObject, NULL, highPriority );
				if ( id )
				{
					if ( pUser )
						ActivateOutput( pActInfo, 0, pUser->GetId() );
					if ( pObject )
						ActivateOutput( pActInfo, 1, pObject->GetId() );
					ActivateOutput( pActInfo, 2, true );
				}
				else
					ActivateOutput( pActInfo, 3, true );
			}
			break;
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// Enable/Disable AI for an entity/
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIEnable : public CFlowBaseNode
{
public:
	enum EInputs {
		IN_ENABLE,
		IN_DISABLE,
	};
	CFlowNode_AIEnable( SActivationInfo * pActInfo ) {};

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
	/*
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return this; // no internal state! // new CFlowNode_AIEnable(pActInfo);
	}
	*/
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_AnyType( "Enable",_HELP("Enable AI for target entity") ),
			InputPortConfig_AnyType( "Disable",_HELP("Disable AI for target entity") ),
			{0}
		};
		config.sDescription = _HELP( "Plays an Animation" );
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = 0;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event,SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			{
				if (!pActInfo->pEntity)
					return;

				if(IsPortActive(pActInfo, IN_ENABLE))
				{
					if (pActInfo->pEntity->GetAI())
						pActInfo->pEntity->GetAI()->Event(AIEVENT_ENABLE,0);
				}
				if(IsPortActive(pActInfo, IN_DISABLE))
				{
					if (pActInfo->pEntity->GetAI())
						pActInfo->pEntity->GetAI()->Event(AIEVENT_DISABLE,0);
				}
				break;
			}

		case eFE_Initialize:
			break;
		};
	};
};

//////////////////////////////////////////////////////////////////////////
// control auto disabling
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIAutoDisable : public CFlowBaseNode
{
public:
	enum EInputs {
		IN_ON,
		IN_OFF,
	};
	CFlowNode_AIAutoDisable( SActivationInfo * pActInfo ) {};

	/*
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return this; // no internal state! // new CFlowNode_AIAutoDisable(pActInfo);
	}
	*/

	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_AnyType( "ON",_HELP("allow AutoDisable for target entity") ),
			InputPortConfig_AnyType( "OFF",_HELP("no AutoDisable for target entity") ),
			{0}
		};
		config.sDescription = _HELP( "controls AutoDisable " );
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = 0;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event,SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			{
				if (!pActInfo->pEntity || !pActInfo->pEntity->GetAI() ||
						!pActInfo->pEntity->GetAI()->GetProxy())
					return;
				CAIProxy*	proxy = (CAIProxy*)pActInfo->pEntity->GetAI()->GetProxy();
				if(IsPortActive(pActInfo, IN_ON))
						proxy->UpdateMeAlways(false);
				if(IsPortActive(pActInfo, IN_OFF))
						proxy->UpdateMeAlways(true);
				break;
			}
		case eFE_Initialize:
			break;
		};
	};

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};



//////////////////////////////////////////////////////////////////////////
// switching species hostility on/off (makes AI ignore enemies/be ignored)
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIIgnore : public CFlowBaseNode
{
public:
	enum EInputs {
		IN_HOSTILE,
		IN_NOHOSTILE,
		IN_RESETPERCEPTION,
	};
	CFlowNode_AIIgnore( SActivationInfo * pActInfo ) {};
	/*
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return this; // no internal state! // new CFlowNode_AIIgnore(pActInfo);
	}
	*/
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_AnyType( "Hostile",_HELP("Default perception") ),
			InputPortConfig_AnyType( "Ignore",_HELP("Make it ignore enemies/be ignored") ),
			InputPortConfig_AnyType( "ResetPerception", _HELP("Reset current perception state") ),
			{0}
		};
		config.sDescription = _HELP( "switching species hostility on/off (makes AI ignore enemies/be ignored)" );
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = 0;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event,SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			{
				if (!pActInfo->pEntity)
					return;

				if(IsPortActive(pActInfo, IN_HOSTILE))
				{
					IAIActor* pAIActor = CastToIAIActorSafe(pActInfo->pEntity->GetAI());
					if( pAIActor )
					{
					AgentParameters ap = pAIActor->GetParameters();
						ap.m_bAiIgnoreFgNode = false;
						pAIActor->SetParameters(ap);
					}
				}
				if(IsPortActive(pActInfo, IN_NOHOSTILE))
				{
					IAIActor* pAIActor = CastToIAIActorSafe(pActInfo->pEntity->GetAI());
					if( pAIActor )
					{
						AgentParameters ap = pAIActor->GetParameters();
						ap.m_bAiIgnoreFgNode = true;
						pAIActor->SetParameters(ap);
					}
				}
				if(IsPortActive(pActInfo, IN_RESETPERCEPTION))
				{
					IAIActor* pAIActor = CastToIAIActorSafe(pActInfo->pEntity->GetAI());
					if( pAIActor )
						pAIActor->ResetPerception();
				}
				break;
			}
		case eFE_Initialize:
			break;
		};
	};

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////
// scales down the perception of assigned AI
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIPerceptionScale : public CFlowBaseNode
{
public:
	CFlowNode_AIPerceptionScale( SActivationInfo * pActInfo ) {};
	/*
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return this; // no internal state! // new CFlowNode_AIPerceptionScale(pActInfo);
	}
	*/
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void( "Trigger", _HELP("Desired perception scale is set when this node is activated") ),
			InputPortConfig<float>( "Scale", 1.0f, _HELP("Desired visual perception scale factor"), _HELP("Visual") ),
			InputPortConfig<float>( "Audio", 1.0f, _HELP("Desired audio perception scale factor") ),
			{0}
		};
		config.sDescription = _HELP( "Scales the agent's sensitivity" );
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = 0;
		config.SetCategory( EFLN_APPROVED );
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo )
	{
		if ( event == eFE_Activate && IsPortActive( pActInfo, 0 ) && pActInfo->pEntity )
		{
			IAIActor* pAIActor = CastToIAIActorSafe(pActInfo->pEntity->GetAI());
			if( pAIActor )
			{
				AgentParameters ap = pAIActor->GetParameters();
				ap.m_PerceptionParams.perceptionScale.visual = GetPortFloat( pActInfo, 1 );
				ap.m_PerceptionParams.perceptionScale.audio = GetPortFloat( pActInfo, 2 );
				pAIActor->SetParameters(ap);
			}
		};
	};

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////
// changes parameter of assigned AI
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIChangeParameter : public CFlowBaseNode
{
	enum EAIParamNames
	{
		AIPARAM_SPECIES = 0, // make sure they match the UICONFIG below
		AIPARAM_GROUPID,
		AIPARAM_RANK,
		AIPARAM_FORGETFULNESS
	};

public:
	CFlowNode_AIChangeParameter( SActivationInfo * pActInfo ) {};
	/*
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return this; // new CFlowNode_AIChangeParam(pActInfo);
	}
	*/
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void  ( "Trigger", _HELP("Change the parameter of the AI") ),
			InputPortConfig<int>  ( "Param", AIPARAM_SPECIES, _HELP("Parameter to change"), _HELP("Parameter"),
				_UICONFIG("enum_int:Species=0,GroupId=1,Rank=2,Forgetfulness=3") ),
			InputPortConfig<float>( "Value", 0.0f, _HELP("Value") ),
			{0}
		};
		config.sDescription = _HELP( "Changes an AI's parameters dynamically" );
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = 0;
		config.SetCategory( EFLN_ADVANCED );
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo )
	{
		if ( event == eFE_Activate && IsPortActive( pActInfo, 0 ) && pActInfo->pEntity )
		{
			IAIActor* pAIActor = CastToIAIActorSafe(pActInfo->pEntity->GetAI());
			if( pAIActor )
			{
				int param = GetPortInt(pActInfo, 1);
				float val = GetPortFloat(pActInfo, 2);
				AgentParameters ap = pAIActor->GetParameters();
				switch (param)
				{
				case AIPARAM_SPECIES:
					ap.m_nSpecies = (int) val;
					break;
				case AIPARAM_GROUPID:
					ap.m_nGroup = (int) val;
					break;
				case AIPARAM_RANK:
					ap.m_nRank = (int) val;
					break;
				case AIPARAM_FORGETFULNESS:
					ap.m_PerceptionParams.forgetfulness = val;
					break;
				default:
					break;
				}
				pAIActor->SetParameters(ap);
			}
		};
	};

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// VS2 workaround: make hunter's freezing beam follow the player but with
// some limited speed to allow the player avoid the beam if he's 
// running fast enough
//////////////////////////////////////////////////////////////////////////
class CFlowNode_HunterShootTarget : public CFlowBaseNode
{
	IAIObject* m_pTarget;
	Vec3 m_targetPos;
	Vec3 m_lastPos;
	float m_startTime;
	float m_lastTime;
	float m_duration;

public:
	CFlowNode_HunterShootTarget( SActivationInfo * pActInfo ) {};
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowNode_HunterShootTarget( pActInfo );
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void( "Trigger", _HELP("Starts shooting") ),
			InputPortConfig<float>( "Speed", _HELP("Speed of oscillation") ),
			InputPortConfig<float>( "Width", _HELP("Width factor of oscillation") ),
			InputPortConfig<float>( "Duration", _HELP("Duration") ),
			InputPortConfig<float>( "TargetAlignSpeed", _HELP("Speed of aligning to target") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<Vec3>( "targetPos", _HELP("Target shooting position") ),
			{0}
		};
		config.sDescription = _HELP( "switching species hostility on/off (makes AI ignore enemies/be ignored)" );
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory( EFLN_DEBUG );
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo )
	{
		if ( event == eFE_Update )
		{
			IAIObject* pAI = pActInfo->pEntity->GetAI();
			if ( !pAI )
			{
				pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
				return;
			}

			float time = gEnv->pTimer->GetCurrTime();

			float relTime = time - m_startTime;
			float timeStep = time - m_lastTime;
			m_lastTime = time;

			if(relTime >= m_duration)
			{
				pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
				return;
			}

			Vec3 pos = pAI->GetPos();

			IPipeUser* pPiper = pAI->CastToIPipeUser();
			if(!pPiper) 
			{
				pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
				return;
			}

			IAIObject* pTarget = pPiper->GetAttentionTarget();

			Vec3 targetPos(m_lastPos);
			if(pTarget && (pTarget==m_pTarget || Distance::Point_PointSq(pTarget->GetPos(),m_lastPos) < 4*4))
				targetPos = pTarget->GetPos();

			Vec3 fireDir = pos - targetPos;
			Vec3 fireDir2d( fireDir.x, fireDir.y, 0 );
			Vec3 sideDir = fireDir.Cross( fireDir2d );
			Vec3 upDir(0,0,1);// = fireDir.Cross( sideDir ).normalized();
			sideDir.NormalizeSafe();

			float dist = fireDir2d.GetLength2D();
			
			Vec3 diffTarget = targetPos - m_targetPos;
			float distTarget = diffTarget.GetLength();

			if ( timeStep &&  distTarget > 0.05f)
			{
				float alignSpeed = GetPortFloat( pActInfo, 4 );

				m_targetPos += diffTarget.GetNormalizedSafe()*timeStep*alignSpeed;
/*
				// make m_lastPos follow m_targetPos with limited speed
				Vec3 aim = targetPos + (targetPos - m_targetPos) * 2.0f;
				m_targetPos = targetPos;
				Vec3 dir = aim - m_lastPos;
				float maxDelta = GetPortFloat( pActInfo, 1 ) * timeStep;
				if ( maxDelta > 0 )
				{
					if ( dir.GetLengthSquared() > maxDelta*maxDelta )
						dir.SetLength( maxDelta );
					m_lastPos += dir;
				}
			*/

			}


	
#ifndef PI
#define PI 3.14159265358979323f  /* You know this number, don't you? */
#endif
			const float maxAmp = 8;
			float a = max(0.f,(m_duration*0.6f - relTime)/m_duration ); //0.6f = 60% chasing target and 40% exact aiming
			float b = sinf( relTime/m_duration *GetPortFloat( pActInfo, 1 ) * 180.0f / PI ) ;
			a *= a;

			
			fireDir = pos - m_targetPos;
			fireDir2d.x = fireDir.x;
			fireDir2d.y = fireDir.y;
			sideDir = fireDir.Cross( fireDir2d );
			sideDir.NormalizeSafe();

			Vec3 result = m_targetPos - upDir * a * dist *maxAmp;

			result.z -=2.5f;
			
			Vec3 actualCenteredFireDir(result - pos);
			actualCenteredFireDir.NormalizeSafe();
			b *= GetPortFloat( pActInfo, 2 ) * dist * a * (1 - actualCenteredFireDir.Dot(-fireDir.GetNormalizedSafe()));

			result += sideDir * b;

			ActivateOutput( pActInfo, 0, result );

			return;
		}


		if ( event != eFE_Activate )
			return;

		if ( !pActInfo->pEntity )
			return;

		IAIObject* pAI = pActInfo->pEntity->GetAI();
		if ( !pAI )
			return;

		if ( IsPortActive(pActInfo, 0) )
		{
			m_pTarget = pAI->GetSpecialAIObject( "atttarget" );
			if ( m_pTarget )
			{
				m_lastTime = m_startTime = gEnv->pTimer->GetCurrTime();
				m_lastPos = m_targetPos = m_pTarget->GetPos();
				m_duration = GetPortFloat( pActInfo, 3 );
				pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			}
		}
	};
};





/**
 * Makes the Hunter's freeze beam sweep around without any specific target.
 */
// Spinned off of CFlowNode_HunterShootTarget.
class CFlowNode_HunterBeamSweep : public CFlowBaseNode
{
	Vec3 m_targetPos;
	float m_startTime;
	float m_duration;

	void ProcessActivate( SActivationInfo* pActInfo )
	{
		if ( ! IsPortActive(pActInfo, 0) )
			return;

		if ( !pActInfo->pEntity )
			return;

		IAIObject* pAI = pActInfo->pEntity->GetAI();
		if ( !pAI )
			return;

		m_startTime = gEnv->pTimer->GetCurrTime();
		m_targetPos = pAI->GetPos() + GetPortFloat( pActInfo, 1 ) * pAI->GetBodyDir();
		m_duration = GetPortFloat( pActInfo, 6 );
		pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
	}

	void ProcessUpdate( SActivationInfo* pActInfo )
	{
		if ( !pActInfo->pEntity )
			return;

		IAIObject* pAI = pActInfo->pEntity->GetAI();
		if ( !pAI )
		{
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
			return;
		}

		float time = gEnv->pTimer->GetCurrTime();
		float relTime = time - m_startTime;
		if(relTime >= m_duration)
		{
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
			return;
		}

		Matrix34 worldTransform = pActInfo->pEntity->GetWorldTM();
		Vec3 fireDir = worldTransform.GetColumn1();
		Vec3 sideDir = worldTransform.GetColumn0();
		Vec3 upDir = worldTransform.GetColumn2();

		float baseDist = GetPortFloat( pActInfo, 1 );
		float leftRightSpeed = GetPortFloat( pActInfo, 2);
		float width = GetPortFloat( pActInfo, 3);
		float nearFarSpeed = GetPortFloat( pActInfo, 4);
		float dist = GetPortFloat( pActInfo, 5);

		// runs from 0 to 2*PI and indicates where in the 2*PI long cycle we are now
		float t = 2*PI * relTime/m_duration;

		// NOTE Mrz 31, 2007: <pvl> essetially a Lissajous curve, with some
		// irregularity added by a higher frequency harmonic and offset vertically
		// because we don't want the hunter to shoot at the sky.
		Vec3 result = pAI->GetPos() +
			baseDist * fireDir +
			dist  * sinf (nearFarSpeed * t) * upDir  +
			0.2f * dist * sinf (2.3f*nearFarSpeed * t + 1.0f/*phase shift*/) * upDir +
			-1.8f * dist * upDir +
			width * sinf (leftRightSpeed * t) * sideDir +
			0.2f * width * sinf (2.3f*leftRightSpeed * t + 1.0f/*phase shift*/) * sideDir;

		ActivateOutput( pActInfo, 0, result );
	}

public:
	CFlowNode_HunterBeamSweep( SActivationInfo * pActInfo ) {};
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowNode_HunterBeamSweep( pActInfo );
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void( "Trigger", _HELP("Starts shooting") ),
			InputPortConfig<float>( "BaseDistance", _HELP("Average distance around which sweeping will oscillate") ),
			InputPortConfig<float>( "LeftRightSpeed", _HELP("Speed of width oscillation") ),
			InputPortConfig<float>( "Width", _HELP("Amplitude of width oscillation") ),
			InputPortConfig<float>( "NearFarSpeed", _HELP("Speed of distance oscillation") ),
			InputPortConfig<float>( "Distance", _HELP("Amplitude of distance oscillation") ),
			InputPortConfig<float>( "Duration", _HELP("Duration") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<Vec3>( "targetPos", _HELP("Target shooting position") ),
			{0}
		};
		config.sDescription = _HELP( "switching species hostility on/off (makes AI ignore enemies/be ignored)" );
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory( EFLN_DEBUG );
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo )
	{
		if ( event == eFE_Update )
		{
			ProcessUpdate( pActInfo );
		}
		else if ( event == eFE_Activate )
		{
			ProcessActivate( pActInfo );
		}
	}
};






//////////////////////////////////////////////////////////////////////////
// Nav cost factor
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISetNavCostFactor : public CFlowBaseNode
{
public:
	CFlowNode_AISetNavCostFactor( IFlowNode::SActivationInfo * pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

void CFlowNode_AISetNavCostFactor::GetConfiguration( SFlowNodeConfig &config )
{
	static const SInputPortConfig in_config[] = {
		InputPortConfig_Void( "sink", _HELP("for synchronization only"), _HELP("sync") ),
		InputPortConfig<float>( "Factor", _HELP("Nav cost factor. <0 disables navigation. 0 has no effect. >0 discourages navigation") ),
		InputPortConfig<string>( "NavModifierName", _HELP("Name of the cost-factor navigation modifier") ),
		{0}
	};
	static const SOutputPortConfig out_config[] = {
		OutputPortConfig<int>( "Done", _HELP("Done") ),
		{0}
	};
	config.sDescription = _HELP( "Set navigation cost factor for travelling through a region" );
	config.pInputPorts = in_config;
	config.pOutputPorts = out_config;
	config.SetCategory(EFLN_APPROVED);
}

//
//-------------------------------------------------------------------------------------------------------------
void CFlowNode_AISetNavCostFactor::ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo )
{
	if ( event == eFE_Activate )
	{
    float factor = GetPortFloat(pActInfo, 1);
    const string& pathName = GetPortString(pActInfo, 2);

    gEnv->pAISystem->GetINavigation()->ModifyNavCostFactor( pathName.c_str(), factor ); 

    ActivateOutput( pActInfo, 0, 1 );
  }
}


//////////////////////////////////////////////////////////////////////////
// Enable or Disable AIShape
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIEnableShape : public CFlowBaseNode
{
public:
	CFlowNode_AIEnableShape( IFlowNode::SActivationInfo * pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

void CFlowNode_AIEnableShape::GetConfiguration( SFlowNodeConfig &config )
{
	static const SInputPortConfig in_config[] = {
		InputPortConfig_Void( "enable", _HELP("Enables the AI shape"), _HELP("Enable") ),
		InputPortConfig_Void( "disable", _HELP("Disables the AI shape"), _HELP("Disable") ),
		InputPortConfig<string>( "ShapeName", _HELP("Name of the shape to set as the territory.") ),
		{0}
	};
	config.sDescription = _HELP( "Enable or Disable AIShape." );
	config.nFlags = 0;
	config.pInputPorts = in_config;
	config.pOutputPorts = 0;
	config.SetCategory(EFLN_ADVANCED);
}

//
//-------------------------------------------------------------------------------------------------------------
void CFlowNode_AIEnableShape::ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo )
{
	if(event == eFE_Activate)
	{
		if(IsPortActive(pActInfo, 0))
		{
			const string& shapeName = GetPortString(pActInfo, 2);
			gEnv->pAISystem->EnableGenericShape(shapeName.c_str(), true);
		}
		else if(IsPortActive(pActInfo, 1))
		{
			const string& shapeName = GetPortString(pActInfo, 2);
			gEnv->pAISystem->EnableGenericShape(shapeName.c_str(), false);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// AI Event Listener
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIEventListener : public CFlowBaseNode, IAIEventListener
{
public:
	CFlowNode_AIEventListener( SActivationInfo * pActInfo )
		: m_pos(0,0,0), m_radius(0.0f),
		m_thresholdSound(0.5f),
		m_thresholdCollision(0.5f),
		m_thresholdBullet(0.5f),
		m_thresholdExplosion(0.5f)
	{
		m_nodeID = pActInfo->myID;
		m_pGraph = pActInfo->pGraph;
	}
	~CFlowNode_AIEventListener()
	{
		if(gEnv->pAISystem)
			gEnv->pAISystem->UnregisterAIEventListener(this);
	}
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowNode_AIEventListener(pActInfo);
	}
	void Serialize( SActivationInfo * pActInfo, TSerialize ser )
	{
		ser.Value("pos", m_pos);
		ser.Value("radius", m_radius);
		ser.Value("m_thresholdSound", m_thresholdSound);
		ser.Value("m_thresholdCollision", m_thresholdCollision);
		ser.Value("m_thresholdBullet", m_thresholdBullet);
		ser.Value("m_thresholdExplosion", m_thresholdExplosion);
	}
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_Void("Sound"),
			OutputPortConfig_Void("Collision"),
			OutputPortConfig_Void("Bullet"),
			OutputPortConfig_Void("Explosion"),
			{0}
		};
		static const SInputPortConfig in_config[] = {
			InputPortConfig<Vec3>( "Pos", _HELP("Position of the listener")),
			InputPortConfig<float>( "Radius", 0.0f, _HELP("Radius of the listener")),
			InputPortConfig<float>( "ThresholdSound", 0.5f, _HELP("Sensitivity of the sound output")),
			InputPortConfig<float>( "ThresholdCollision", 0.5f, _HELP("Sensitivity of the collision output")),
			InputPortConfig<float>( "ThresholdBullet", 0.5f, _HELP("Sensitivity of the bullet output")),
			InputPortConfig<float>( "ThresholdExplosion", 0.5f, _HELP("Sensitivity of the explosion output")),
			{0}
		};
		config.sDescription = _HELP( "The highest level of alertness of any agent in the level" );
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_ADVANCED);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		const int stimFlags = (1<<AISTIM_SOUND) | (1<<AISTIM_EXPLOSION) | (1<<AISTIM_BULLET_HIT) | (1<<AISTIM_COLLISION);

		if (event == eFE_Initialize)
		{
			m_pos = GetPortVec3(pActInfo, 0);
			m_radius = GetPortFloat(pActInfo, 1);
			gEnv->pAISystem->RegisterAIEventListener(this, m_pos, m_radius, stimFlags);

			m_thresholdSound = GetPortFloat(pActInfo, 2);
			m_thresholdCollision = GetPortFloat(pActInfo, 3);
			m_thresholdBullet = GetPortFloat(pActInfo, 4);
			m_thresholdExplosion = GetPortFloat(pActInfo, 5);
		}

		if (event == eFE_Activate)
		{
			if (IsPortActive(pActInfo, 0))
			{
				// Change pos
				Vec3 pos = GetPortVec3(pActInfo, 0);
				if (Distance::Point_PointSq(m_pos, pos) > sqr(0.01f))
				{
					m_pos = pos;
					gEnv->pAISystem->RegisterAIEventListener(this, m_pos, m_radius, stimFlags);
				}
			}
			else if (IsPortActive(pActInfo, 1))
			{
				// Change radius
				float rad = GetPortFloat(pActInfo, 1);
				if (fabsf(rad - m_radius) > 0.0001f)
				{
					m_radius = rad;
					gEnv->pAISystem->RegisterAIEventListener(this, m_pos, m_radius, stimFlags);
				}
			}
			else if (IsPortActive(pActInfo, 2))
			{
				// Change threshold
				m_thresholdSound = GetPortFloat(pActInfo, 2);
			}
			else if (IsPortActive(pActInfo, 3))
			{
				// Change threshold
				m_thresholdCollision = GetPortFloat(pActInfo, 3);
			}
			else if (IsPortActive(pActInfo, 4))
			{
				// Change threshold
				m_thresholdBullet = GetPortFloat(pActInfo, 4);
			}
			else if (IsPortActive(pActInfo, 5))
			{
				// Change threshold
				m_thresholdExplosion = GetPortFloat(pActInfo, 5);
			}
		}
	}
	virtual void OnAIEvent(EAIStimulusType type, const Vec3& pos, float radius, float threat, EntityId sender)
	{
		CRY_ASSERT(m_pGraph->IsEnabled() && !m_pGraph->IsSuspended() && m_pGraph->IsActive());

		float dist = Distance::Point_Point(m_pos, pos);
		float u = 0.0f;

		if (radius > 0.001f)
			u = (1.0f - clamp((dist - m_radius)/radius, 0.0f, 1.0f)) * threat;

		float thr = 0.0f;

		switch(type)
		{
		case AISTIM_SOUND: thr = m_thresholdSound; break;
		case AISTIM_COLLISION: thr = m_thresholdCollision; break;
		case AISTIM_BULLET_HIT: thr = m_thresholdBullet; break;
		case AISTIM_EXPLOSION: thr = m_thresholdExplosion; break;
		default: return;
		}

		if (thr > 0.0f && u > thr)
		{
			int i = 0;
			switch(type)
			{
			case AISTIM_SOUND: i = 0; break;
			case AISTIM_COLLISION: i = 1; break;
			case AISTIM_BULLET_HIT: i = 2; break;
			case AISTIM_EXPLOSION: i = 3; break;
			default: return;
			}
			SFlowAddress addr(m_nodeID, i, true );
			m_pGraph->ActivatePort( addr, true );
		}
	}
	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
private:
	Vec3 m_pos;
	float m_radius;
	float m_thresholdSound;
	float m_thresholdCollision;
	float m_thresholdBullet;
	float m_thresholdExplosion;
	IFlowGraph *m_pGraph;
	TFlowNodeId m_nodeID;
};

//////////////////////////////////////////////////////////////////////////
// Register an entity with the interest system
class CFlowNode_InterestingEntity : public CFlowBaseNode, public IInterestListener
{
public:
	enum EInputs
	{
		IN_REGISTER_INTERESTING,
		IN_UNREGISTER_INTERESTING,
		IN_RADIUS,
		IN_INTEREST_LEVEL,
		IN_ACTION,
	};

	enum EOutputs
	{
		OUT_ACTOR_INTERESTED,
		OUT_ACTOR_NOT_INTERESTED,
	};

	CFlowNode_InterestingEntity( SActivationInfo * pActInfo )
	{
		m_nodeID = 0;
		m_pGraph = NULL;
	}

	//void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	//{
	//}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<EntityId>("OnActorInterested", _HELP("Outputs when an AI Entity gets interested in this. It carries the EntityId")),
			OutputPortConfig<EntityId>("OnActorLoseInterest", _HELP("Outputs when an AI Entity is no longer interested in this. It carries the EntityId")),
			{0}
		};

		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "Register", _HELP("Register entity as interesting")),
			InputPortConfig_Void( "Unregister", _HELP("Unregister entity as interesting")),
			InputPortConfig<float>( "Radius",	-1.0f, _HELP("This object will be only interesting from this radius. Negative value will use previous setup" )),		
			InputPortConfig<float>( "InterestLevel",	-1.0f, _HELP("Base interest level of object or minimum interest level of actor. Negative value will use previous setup" )),		
			InputPortConfig<string>( "Action",	_HELP("AI Action. If empty it will be used the instance value if any" ), 0),		
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_config;
		config.sDescription = _HELP("Registers an entity as interesting");
		config.SetCategory(EFLN_ADVANCED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		IEntity *pEntity = pActInfo->pEntity;
		if (!pEntity)
			return;

		if(event == eFE_Activate)
		{
			ICentralInterestManager * pCIM = gEnv->pAISystem->GetCentralInterestManager();

			if (IsPortActive(pActInfo, IN_REGISTER_INTERESTING))
			{
				float radius = GetPortFloat(pActInfo, IN_RADIUS);
				float interestLevel = GetPortFloat(pActInfo, IN_INTEREST_LEVEL);
				const string& action = GetPortString(pActInfo, IN_ACTION);

				pCIM->RegisterInterestingEntity(pEntity, radius, interestLevel, action.c_str());
				pCIM->RegisterListener(this, pEntity->GetId());

				m_nodeID = pActInfo->myID;
				m_pGraph = pActInfo->pGraph;
			}
			else if (IsPortActive(pActInfo, IN_UNREGISTER_INTERESTING))
			{
				pCIM->DeregisterInterestingEntity(pEntity);
				pCIM->UnRegisterListener(this, pEntity->GetId());

				m_nodeID = 0;
				m_pGraph = NULL;
			}
		}
	}

	void OnInterested(EntityId idInterestedActor, EntityId idInterestingEntity)
	{
		if(m_pGraph != NULL)
		{
			m_pGraph->ActivatePort( SFlowAddress( m_nodeID, OUT_ACTOR_INTERESTED, true ), idInterestedActor );
		}
	}

	void OnLoseInterest(EntityId idInterestedActor, EntityId idInterestingEntity)
	{
		if(m_pGraph != NULL)
		{
			m_pGraph->ActivatePort( SFlowAddress( m_nodeID, OUT_ACTOR_NOT_INTERESTED, true ), idInterestedActor );
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s) 
	{
		s->Add(*this);
	}

private:

	IFlowGraph *m_pGraph;
	TFlowNodeId m_nodeID;
};


//////////////////////////////////////////////////////////////////////////
// Register an entity with the interest system
class CFlowNode_Interested : public CFlowBaseNode
{
public:
	enum EInputs
	{
		IN_REGISTER_INTERESTED,
		IN_UNREGISTER_INTERESTED,
		IN_INTEREST_FILTER,
		IN_ANGLE
	};

	CFlowNode_Interested( SActivationInfo * pActInfo )
	{
	}

	//void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	//{
	//}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		// declare input ports
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "Register", _HELP("Register entity as interested")),
			InputPortConfig_Void( "Unregister", _HELP("Unregister entity as interested")),
			InputPortConfig<float>( "InterestFilter",	-1.0f, _HELP("Minimum interest level of actor. Negative value will use previous setup" )),
			InputPortConfig<float>( "Angle", -1.0f, _HELP("Actor will ignore object out of this angle. Negative value will use previous setup" )),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = 0;
		config.sDescription = _HELP("Registers an entity with the interest system");
		config.SetCategory(EFLN_ADVANCED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		IEntity *pEntity = pActInfo->pEntity;
		if (!pEntity)
			return;

		if(event == eFE_Activate)
		{
			ICentralInterestManager * pCIM = gEnv->pAISystem->GetCentralInterestManager();

			if (IsPortActive(pActInfo, IN_REGISTER_INTERESTED))
			{
				float interestLevel = GetPortFloat(pActInfo, IN_INTEREST_FILTER);
				float angle = GetPortFloat(pActInfo, IN_ANGLE);

				pCIM->RegisterInterestedAIActor(pEntity, true, interestLevel, angle);
			}
			else if (IsPortActive(pActInfo, IN_UNREGISTER_INTERESTED))
			{
				pCIM->DeregisterInterestedAIActor(pEntity);
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s) 
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Load or Unload an individual Layered Navigation Mesh
// The mesh is identified by navigation modifier and agent type.
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AILoadNavMesh : public CFlowBaseNode
{
public:
	CFlowNode_AILoadNavMesh( IFlowNode::SActivationInfo * pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

void CFlowNode_AILoadNavMesh::GetConfiguration( SFlowNodeConfig &config )
{
	static const SInputPortConfig in_config[] = {
		InputPortConfig_Void( "load", _HELP("Loads the mesh"), _HELP("Load") ),
		InputPortConfig_Void( "unload", _HELP("Unloads the mesh"), _HELP("Unload") ),
		InputPortConfig<string>( "LnmModifier", _HELP("Name of the modifier that generated the mesh.") ),
		InputPortConfig<string>( "AgentType", _HELP("Name of the agent type that the mesh was generated for.") ),
		{0}
	};

	static const SOutputPortConfig out_config[] = {
		OutputPortConfig_Void( "done", _HELP("loading/unloading done") ),
		OutputPortConfig_Void( "succeed", _HELP("loading/unloading succeeded") ),
		OutputPortConfig_Void( "fail", _HELP("loading/unloading failed") ),
		{0}
	};

	config.sDescription = _HELP( "Load or Unload Navigation Mesh." );
	config.nFlags = 0;
	config.pInputPorts = in_config;
	config.pOutputPorts = out_config;
	config.SetCategory(EFLN_ADVANCED);
}

void CFlowNode_AILoadNavMesh::ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo )
{
	if(event == eFE_Activate)
	{
		bool success = true;
		if(IsPortActive(pActInfo, 0))
		{
			const string& navModifName = GetPortString(pActInfo, 2);
			const string& agentTypeName = GetPortString(pActInfo, 3);
			success = gEnv->pAISystem->LoadNavMesh (navModifName, agentTypeName);
		}
		else if(IsPortActive(pActInfo, 1))
		{
			const string& navModifName = GetPortString(pActInfo, 2);
			const string& agentTypeName = GetPortString(pActInfo, 3);
			success = gEnv->pAISystem->UnloadNavMesh (navModifName, agentTypeName);
		}
		ActivateOutput( pActInfo, 0, true );
		ActivateOutput( pActInfo, success ? 1 : 2, true );
	}
}

class CFlowNode_AICoordination : public CFlowBaseNode
{
public:
	enum eInputs {
		eInput_GroupID,
		eInput_CoordinationName,
		eInput_StartCoordination,
	};

	CFlowNode_AICoordination ( IFlowNode::SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics( ICrySizer * crySizer )
	{
		crySizer->Add(*this);
	}
};

void CFlowNode_AICoordination::GetConfiguration( SFlowNodeConfig &config )
{
	static const SInputPortConfig in_config[] = {
		InputPortConfig<int>("GroupId"),
		InputPortConfig<string>("Coordination", "", _HELP("Name of the coordination to be executed."), 0, _UICONFIG("enum_string:=,Assault=assault,DefensiveLine=DefensiveLine")),
		InputPortConfig_Void("StartCoordination"),
		{0}
	};

	config.sDescription = _HELP("Triggers an coordination plan for the specified group");
	config.nFlags = 0;
	config.pInputPorts = in_config;
	config.pOutputPorts = 0;
	config.SetCategory(EFLN_ADVANCED);
}

void CFlowNode_AICoordination::ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo )
{
	if(event == eFE_Activate && IsPortActive(pActInfo, eInput_StartCoordination))
	{
		ICoordinationManager* coordinationManager = gEnv->pAISystem->GetCoordinationManager();
		assert(coordinationManager);

		if(!coordinationManager)
			return;

		int groupId = GetPortInt(pActInfo, eInput_GroupID);
		const string& coordName = GetPortString(pActInfo, eInput_CoordinationName);

		std::vector<EntityId> actors;
		int groupCount = gEnv->pAISystem->GetGroupCount(groupId, 0, 0); 
		for (int i = 0; i < groupCount; ++i)
		{
			if (IAIObject* pAIObject = gEnv->pAISystem->GetGroupMember(groupId, i, 0, 0))
				actors.push_back(pAIObject->GetEntityID());
		}

		CoordinationStartParams params;
		params.actorCount = actors.size();
		params.actors = &actors.front();

		if(!coordinationManager->StartCoordination(coordName, params))
			GameWarning("Coordination Flow Node : Failed to trigger coordination '%s' for the group '%d'", coordName.c_str(), groupId);

	}
}

REGISTER_FLOW_NODE_SINGLETON( "AI:AIAwareness",CFlowNode_AIAwareness )
REGISTER_FLOW_NODE( "AI:AIAlertness",CFlowNode_AIAlertness )
REGISTER_FLOW_NODE( "AI:AIActiveCount",CFlowNode_AIActiveCount )
REGISTER_FLOW_NODE_SINGLETON( "AI:AlertnessFilter",CFlowNode_AIAlertnessFilter )
//REGISTER_FLOW_NODE( "AI:AISignal",CFlowNode_AISignal )
REGISTER_FLOW_NODE_SINGLETON( "AI:AIEnable",CFlowNode_AIEnable )
REGISTER_FLOW_NODE_SINGLETON( "AI:AIIgnore",CFlowNode_AIIgnore )

REGISTER_FLOW_NODE_SINGLETON( "AI:Interested", CFlowNode_Interested)
REGISTER_FLOW_NODE_SINGLETON( "AI:InterestingEntity", CFlowNode_InterestingEntity)

REGISTER_FLOW_NODE_SINGLETON( "AI:ActionStart", CFlowNode_AIActionStart )
REGISTER_FLOW_NODE_SINGLETON( "AI:ActionEnd", CFlowNode_AIActionEnd )
REGISTER_FLOW_NODE( "AI:ActionAbort", CFlowNode_AIActionAbort )
REGISTER_FLOW_NODE_SINGLETON( "AI:AIAttTarget", CFlowNode_AIAttTarget )
REGISTER_FLOW_NODE( "AI:SmartObjectHelper", CFlowNode_AISOHelper )
REGISTER_FLOW_NODE( "AI:AIGroupAlertness", CFlowNode_AIGroupAlertness)
REGISTER_FLOW_NODE_SINGLETON( "AI:AIMergeGroups", CFlowNode_AIMergeGroups )
REGISTER_FLOW_NODE( "AI:AIGroupBeacon", CFlowNode_AIGroupBeacon )
REGISTER_FLOW_NODE( "AI:AIGroupID", CFlowNode_AIGroupID )
REGISTER_FLOW_NODE( "AI:AIGroupCount", CFlowNode_AIGroupCount)
REGISTER_FLOW_NODE_SINGLETON( "AI:SmartObjectEvent", CFlowNode_AISmartObjectEvent )
REGISTER_FLOW_NODE_SINGLETON( "AI:PerceptionScale", CFlowNode_AIPerceptionScale )
REGISTER_FLOW_NODE_SINGLETON( "AI:ChangeParameter", CFlowNode_AIChangeParameter )
REGISTER_FLOW_NODE_SINGLETON( "AI:AISetNavCostFactor", CFlowNode_AISetNavCostFactor )
REGISTER_FLOW_NODE_SINGLETON( "AI:AutoDisable", CFlowNode_AIAutoDisable )
REGISTER_FLOW_NODE( "Crysis:HunterShootTarget", CFlowNode_HunterShootTarget )
REGISTER_FLOW_NODE( "Crysis:HunterBeamSweep", CFlowNode_HunterBeamSweep )
REGISTER_FLOW_NODE_SINGLETON( "AI:AIEnableShape",CFlowNode_AIEnableShape)
REGISTER_FLOW_NODE( "AI:EventListener", CFlowNode_AIEventListener)
REGISTER_FLOW_NODE( "AI:LoadNavMesh", CFlowNode_AILoadNavMesh)
REGISTER_FLOW_NODE( "AI:Coordination", CFlowNode_AICoordination)
