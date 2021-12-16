#include "StdAfx.h"
#include "FlowBaseNode.h"
#include "CryAction.h"
#include "IActorSystem.h"
#include "GameObjects/GameObject.h"
#include "IGameRulesSystem.h"

inline IActor* GetAIActor( IFlowNode::SActivationInfo *pActInfo )
{
	if (!pActInfo->pEntity)
		return 0;
	return CCryAction::GetCryAction()->GetIActorSystem()->GetActor(pActInfo->pEntity->GetId());
}

class CFlowPlayer : public CFlowBaseNode
{
public:
	CFlowPlayer( SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void   ("update", _HELP("Retriggers the entity id. Required for multiplayer")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<EntityId>("entityId", _HELP("Player entity id")),
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Outputs the local players entity id - NOT USABLE FOR MULTIPLAYER WITHOUT UPDATING BY HAND AFTER GAMESTART");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				bool done = UpdateEntityIdOutput( pActInfo );
				if (!done)
					pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
				break;
			}

		case eFE_Activate:
			UpdateEntityIdOutput( pActInfo );
			break;

		case eFE_Update:
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
			UpdateEntityIdOutput( pActInfo );
			break;
		}
	}

	bool UpdateEntityIdOutput( SActivationInfo *pActInfo )	
	{
		IActor * pActor = CCryAction::GetCryAction()->GetClientActor();
		if (pActor)
		{
			ActivateOutput(pActInfo, 0, pActor->GetEntityId());
			return true;
		}
		else
			return false;
	}
	

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

class CFlowAllPlayers : public CFlowBaseNode
{
public:
	CFlowAllPlayers( SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void   ("update", _HELP("Retriggers the entity id. Required for multiplayer")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<EntityId>("entityId1", _HELP("Player 1")),
			OutputPortConfig<EntityId>("entityId2", _HELP("Player 2")),
			OutputPortConfig<EntityId>("entityId3", _HELP("Player 3")),
			OutputPortConfig<EntityId>("entityId4", _HELP("Player 4")),
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
				IActorSystem *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
				IActorIteratorPtr actorIt = pActorSystem->CreateActorIterator();
				int iNumPlayers = 0;
				IActor *pActor = actorIt->Next();
				while(iNumPlayers < 4 && pActor)
				{
					if(pActor->GetChannelId())
					{
						ActivateOutput(pActInfo, iNumPlayers, pActor->GetEntityId());
						++iNumPlayers;
					}

					pActor = actorIt->Next();
				}
			}
			break;
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

class CFlowIsPlayer : public CFlowBaseNode
{
public:
	CFlowIsPlayer( SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void   ("update", _HELP("Retriggers the output.")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<bool>("isPlayer", _HELP("Entity is a player (local or multiplayer client).")),
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.sDescription = _HELP("Outputs whether an entity is a player.");
		config.SetCategory(EFLN_OBSOLETE);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
		case eFE_Activate:
			{
				if(!pActInfo->pEntity)
					ActivateOutput(pActInfo, 0, false);
				else
				{
					CRY_ASSERT(gEnv->pGame);
					IActor * pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pActInfo->pEntity->GetId());
					if (pActor && pActor->GetChannelId() != 0) //is this a client ?
						ActivateOutput(pActInfo, 0, true);
					else
						ActivateOutput(pActInfo, 0, false);
				}

			}
			break;
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

class CFlowDamageActor : public CFlowBaseNode
{
public:
	CFlowDamageActor( SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void   ("Trigger", _HELP("Trigger this port to actually damage the actor")),
			InputPortConfig<int>   ("damage", 0, _HELP("Amount of damage to exert when [Trigger] is activated"), _HELP("Damage")),
			InputPortConfig<Vec3>  ("Position", Vec3(ZERO), _HELP("Position of damage")),
			{0}
		};
		config.pInputPorts = inputs;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.sDescription = _HELP("Damages attached entity by [Damage] when [Trigger] is activated");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, 0))
			{
				IActor * pActor = GetAIActor(pActInfo);
				if (pActor)
				{ 
					if (IEntity* pGameRules = CCryAction::GetCryAction()->GetIGameRulesSystem()->GetCurrentGameRulesEntity())
					{
						if (IScriptTable* pGameRulesScript = pGameRules->GetScriptTable())
						{
							IScriptSystem* pSS = pGameRulesScript->GetScriptSystem();
							if (pGameRulesScript->GetValueType("CreateHit") == svtFunction &&
								  pSS->BeginCall(pGameRulesScript, "CreateHit"))
							{
								pSS->PushFuncParam(pGameRulesScript);
								pSS->PushFuncParam(ScriptHandle(pActor->GetEntityId())); // target
								pSS->PushFuncParam(ScriptHandle(pActor->GetEntityId())); // shooter
								pSS->PushFuncParam(ScriptHandle(0)); // weapon
								pSS->PushFuncParam(GetPortInt(pActInfo, 1)); // damage
								pSS->PushFuncParam(0);  // radius
								pSS->PushFuncParam(""); // material
								pSS->PushFuncParam(0); // partID
								pSS->PushFuncParam("FG");  // type
								pSS->PushFuncParam(GetPortVec3(pActInfo, 2)); // pos
								pSS->PushFuncParam(FORWARD_DIRECTION);  // dir
								pSS->PushFuncParam(FORWARD_DIRECTION); // normal
								pSS->EndCall();
							}
						}
					}
				}
			}
			break;
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


class CFlowActorGrabObject : public CFlowBaseNode
{
public:
  CFlowActorGrabObject( SActivationInfo * pActInfo )
  {
  }

  virtual void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = {      
      InputPortConfig<EntityId>("objectId", _HELP("Entity to grab")),
      InputPortConfig_Void("grab", _HELP("Pulse this to grab object.")),
      InputPortConfig_Void("drop", _HELP("Pulse this to drop currently grabbed object.")),
      InputPortConfig<bool>("throw", _HELP("If true, object is thrown forward.")),
      {0}
    };
    static const SOutputPortConfig outputs[] = {
      OutputPortConfig<bool>("success", _HELP("true if object was grabbed/dropped successfully")),
      OutputPortConfig<EntityId>("grabbedObjId", _HELP("Currently grabbed object, or 0")),
      {0}
    };    
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.nFlags |= EFLN_TARGET_ENTITY;
    config.sDescription = _HELP("Target entity will try to grab the input object, respectively drop/throw its currently grabbed object.");
		config.SetCategory(EFLN_APPROVED);
  }

  virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
  {
    switch (event)
    {    
    case eFE_Activate:
      {
        IActor* pActor = GetAIActor(pActInfo);
        if (!pActor)
          break;
        
        IEntity* pEnt = pActor->GetEntity();
        if (!pEnt)
          break;
        
        HSCRIPTFUNCTION func = 0;
        int ret = 0;
        IScriptTable* pTable = pEnt->GetScriptTable();
        if (!pTable)
          break;

        if (IsPortActive(pActInfo, 1) && pTable->GetValue("GrabObject", func))
        {                   
          IEntity* pObj = gEnv->pEntitySystem->GetEntity( GetPortEntityId(pActInfo, 0) );
          if (pObj)
          {
            IScriptTable* pObjTable = pObj->GetScriptTable();
            Script::CallReturn(gEnv->pScriptSystem, func, pTable, pObjTable, ret);
          }
          ActivateOutput(pActInfo, 0, ret);
        }  
        else if (IsPortActive(pActInfo, 2) && pTable->GetValue("DropObject", func))
        {
          bool bThrow = GetPortBool(pActInfo, 3);
          Script::CallReturn(gEnv->pScriptSystem, func, pTable, bThrow, ret);
          ActivateOutput(pActInfo, 0, ret);
        }
        
        if (pTable->GetValue("GetGrabbedObject", func))
        {          
          ScriptHandle sH(0);
          Script::CallReturn(gEnv->pScriptSystem, func, pTable, sH);
          ActivateOutput(pActInfo, 1, EntityId(sH.n));
        }
        
				if(func)
					gEnv->pScriptSystem->ReleaseFunc(func);

        break;
      }
    }
  }

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


class CFlowActorGetHealth : public CFlowBaseNode
{
public:
	CFlowActorGetHealth( SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Trigger", _HELP("Trigger this port to get the current health")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<int>("Health", _HELP("Current health of entity")),
			{0}
		};    
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.sDescription = _HELP("Get health of an entity (actor)");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, 0))
			{
				IActor* pActor = GetAIActor(pActInfo);
				if(pActor)
				{
					ActivateOutput(pActInfo, 0, pActor->GetHealth());
				}
				else
				{
					GameWarning("CFlowActorGetHealth - No Entity or Entity not an Actor!");
				}
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


class CFlowActorSetHealth : public CFlowBaseNode
{
public:
	CFlowActorSetHealth( SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Trigger", _HELP("Trigger this port to set health")),
			InputPortConfig<int>("Value", _HELP("Health value to be set on entity when Trigger is activated")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<int>("Health", _HELP("Current health of entity (activated when Trigger is activated)")),
			{0}
		};    
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.sDescription = _HELP("Set health of an entity (actor)");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, 0)) 
			{
				IActor* pActor = GetAIActor(pActInfo);
				if(pActor)
				{
					pActor->SetHealth( GetPortInt(pActInfo, 1) );
					ActivateOutput(pActInfo, 0, pActor->GetHealth()); // use pActor->GetHealth (might have been clamped to maxhealth]
				}
				else
				{
					GameWarning("CFlowActorSetHealth - No Entity or Entity not an actor!");
				}
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


class CFlowActorCheckHealth : public CFlowBaseNode
{
public:
	CFlowActorCheckHealth( SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Trigger", _HELP("Trigger this port to check health")),
			InputPortConfig<int>("MinHealth", 0, _HELP("Lower limit of range")),
			InputPortConfig<int>("MaxHealth", 100, _HELP("Upper limit of range")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<bool>("InRange", _HELP("True if Health is in range, False otherwise")),
			{0}
		};    
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.sDescription = _HELP("Check if health of entity (actor) is in range [MinHealth, MaxHealth]");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, 0)) 
			{
				IActor* pActor = GetAIActor(pActInfo);
				if(pActor)
				{
					int health = pActor->GetHealth();
					int minH = GetPortInt(pActInfo, 1);
					int maxH = GetPortInt(pActInfo, 2);
					ActivateOutput(pActInfo, 0, minH <= health && health <= maxH);
				}
				else
				{
					CryLogAlways("CFlowActorCheckHealth - No Entity or Entity not an Actor!");
				}
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

class CFlowGameObjectEvent : public CFlowBaseNode
{
public:
	CFlowGameObjectEvent( SActivationInfo * pActInfo )
	{
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "Trigger", _HELP("Trigger this port to send the event" )),
			InputPortConfig<string>( "EventName", _HELP("Name of event to send" )),
			InputPortConfig<string>( "EventParam", _HELP("Parameter of the event [event-specific]" )),			
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = 0;
		config.sDescription = _HELP("Broadcast a game object event or send to a specific entity. EventParam is an event specific string");
		config.SetCategory(EFLN_ADVANCED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(eFE_Activate == event && IsPortActive(pActInfo,0))
		{
			const string& eventName = GetPortString(pActInfo, 1);
			uint32 eventId = CCryAction::GetCryAction()->GetIGameObjectSystem()->GetEventID(eventName.c_str());
			SGameObjectEvent evt(eventId, eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, (void*) (GetPortString(pActInfo, 2).c_str()));
			if (pActInfo->pEntity == 0)
			{
				// broadcast to all gameobject events
				CCryAction::GetCryAction()->GetIGameObjectSystem()->BroadcastEvent(evt);
			}
			else
			{
				// send to a specific entity only
				if (CGameObject * pGameObject = (CGameObject*)pActInfo->pEntity->GetProxy(ENTITY_PROXY_USER))
					pGameObject->SendEvent(evt);
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Display a message tag above an entity 
class CFlowNode_DisplayTag : public CFlowBaseNode
{
public:
	enum EInputs
	{
		IN_DISPLAY,
		IN_MSG,
		IN_DURATION,
		IN_FONTSIZE,
		IN_COLOR
	};
	enum EOutputs
	{
		OUT_DONE
	};

	CFlowNode_DisplayTag( SActivationInfo * pActInfo ) : m_waitTime(0.f)
	{
	}

	//void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	//{
	//}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SEntityTagParams defaultTag;

		// declare input ports
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "display", _HELP("Display tag")),
			InputPortConfig<string>( "message", _HELP("The message to display" )),
			InputPortConfig<float>( "duration",	defaultTag.fadeTime, _HELP("Seconds to display for" )),
			InputPortConfig<float>( "fontSize",	defaultTag.size, _HELP("Input font size" )),
			InputPortConfig<Vec3>( "color", Vec3(defaultTag.color.r,defaultTag.color.g,defaultTag.color.b), _HELP("Text color - RGB [0.0 to 1.0]" )),
			{0}
		}; 
		static const SOutputPortConfig out_ports[] = {
			OutputPortConfig_AnyType("Finished", _HELP("Tag has finished being displayed")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.sDescription = _HELP("Displays a text message above entity");
		config.SetCategory(EFLN_DEBUG);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(eFE_Initialize == event && IsPortActive(pActInfo,IN_DISPLAY))
		{
			m_waitTime = 0.f;
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
		}
		else if(eFE_Activate == event && IsPortActive(pActInfo,IN_DISPLAY))
		{
			IEntity *ent = pActInfo->pEntity;
			if (ent)
			{
				IPersistantDebug* pDebug = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();

				SEntityTagParams params;
				params.entity = ent->GetId();
				params.text = GetPortString(pActInfo,IN_MSG);
				params.size = GetPortFloat(pActInfo,IN_FONTSIZE);
				params.color = ColorF(GetPortVec3(pActInfo,IN_COLOR), 1.0f);
				params.fadeTime = GetPortFloat(pActInfo,IN_DURATION);
				params.visibleTime = 0.0f;
				params.tagContext = "flowgraph";
				pDebug->AddEntityTag(params);

				m_waitTime = gEnv->pTimer->GetFrameStartTime() + (params.fadeTime + params.visibleTime);
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
			}
		}
		else if (eFE_Update == event)
		{
			if (m_waitTime < gEnv->pTimer->GetFrameStartTime())
			{
				m_waitTime.SetSeconds(0.f);
				pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
				ActivateOutput(pActInfo, OUT_DONE, GetPortAny(pActInfo, IN_DISPLAY));
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s) 
	{
		s->Add(*this);
	}

private:
	CTimeValue m_waitTime;
};


//////////////////////////////////////////////////////////////////////////
// Display a message tag above an entity 
class CFlowNode_DisplayTagAdv : public CFlowBaseNode
{
public:
	enum EInputs
	{
		IN_DISPLAY,
		IN_MSG,
		IN_VISIBLETIME,
		IN_FADETIME,
		IN_FONTSIZE,
		IN_COLOR,
		IN_VIEWDISTANCE,
		IN_STATICID,
		IN_COLUMN
	};
	enum EOutputs
	{
		OUT_DONE
	};

	CFlowNode_DisplayTagAdv( SActivationInfo * pActInfo ) : m_waitTime(0.f)
	{
	}

	//void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	//{
	//}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SEntityTagParams defaultTag;

		// declare input ports
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "display", _HELP("Display tag")),
			InputPortConfig<string>( "message", _HELP("The message to display" )),
			InputPortConfig<float>( "visibleTime",	defaultTag.visibleTime, _HELP("Seconds to display at full alpha for" )),
			InputPortConfig<float>( "fadeTime",	defaultTag.fadeTime, _HELP("Seconds for fade out" )),
			InputPortConfig<float>( "fontSize",	defaultTag.size, _HELP("Input font size" )),
			InputPortConfig<Vec3>( "color", Vec3(defaultTag.color.r,defaultTag.color.g,defaultTag.color.b), _HELP("Text color - RGB [0.0 to 1.0]" )),
			InputPortConfig<float>( "viewDistance",	defaultTag.viewDistance, _HELP("Distance from camera entity must be within" )),
			InputPortConfig<string>( "staticID", _HELP("Identifier for displaying static tags" )),
			InputPortConfig<int>( "columnNum",	defaultTag.column, _HELP("Which column to display on (usually 1)" )),
			{0}
		}; 
		static const SOutputPortConfig out_ports[] = {
			OutputPortConfig_AnyType("Finished", _HELP("Tag has finished being displayed")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.sDescription = _HELP("Displays a text message above entity");
		config.SetCategory(EFLN_DEBUG);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(eFE_Initialize == event && IsPortActive(pActInfo,IN_DISPLAY))
		{
			m_waitTime = 0.f;
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
		}
		else if(eFE_Activate == event && IsPortActive(pActInfo,IN_DISPLAY))
		{
			IEntity *ent = pActInfo->pEntity;
			if (ent)
			{
				IPersistantDebug* pDebug = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();

				SEntityTagParams params;
				params.entity = ent->GetId();
				params.text = GetPortString(pActInfo,IN_MSG);
				params.size = GetPortFloat(pActInfo,IN_FONTSIZE);
				params.color = ColorF(GetPortVec3(pActInfo,IN_COLOR), 1.0f);
				params.visibleTime = GetPortFloat(pActInfo,IN_VISIBLETIME);
				params.fadeTime = GetPortFloat(pActInfo,IN_FADETIME);
				params.viewDistance = GetPortFloat(pActInfo,IN_VIEWDISTANCE);
				params.staticId = GetPortString(pActInfo,IN_STATICID);
				params.column = GetPortInt(pActInfo,IN_COLUMN);
				params.tagContext = "flowgraph";
				pDebug->AddEntityTag(params);

				m_waitTime = gEnv->pTimer->GetFrameStartTime() + (params.fadeTime + params.visibleTime);
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
			}
		}
		else if (eFE_Update == event)
		{
			if (m_waitTime < gEnv->pTimer->GetFrameStartTime())
			{
				m_waitTime.SetSeconds(0.f);
				pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
				ActivateOutput(pActInfo, OUT_DONE, GetPortAny(pActInfo, IN_DISPLAY));
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s) 
	{
		s->Add(*this);
	}

private:
	CTimeValue m_waitTime;
};


REGISTER_FLOW_NODE_SINGLETON("Game:LocalPlayer", CFlowPlayer);
REGISTER_FLOW_NODE_SINGLETON("Game:AllPlayers", CFlowAllPlayers);
REGISTER_FLOW_NODE_SINGLETON("Game:IsPlayer", CFlowIsPlayer);
REGISTER_FLOW_NODE_SINGLETON("Game:DamageActor", CFlowDamageActor);
REGISTER_FLOW_NODE_SINGLETON("Game:ActorGrabObject", CFlowActorGrabObject);
REGISTER_FLOW_NODE_SINGLETON("Game:ActorGetHealth", CFlowActorGetHealth);
REGISTER_FLOW_NODE_SINGLETON("Game:ActorSetHealth", CFlowActorSetHealth);
REGISTER_FLOW_NODE_SINGLETON("Game:ActorCheckHealth", CFlowActorCheckHealth);
REGISTER_FLOW_NODE_SINGLETON("Game:GameObjectEvent", CFlowGameObjectEvent);
REGISTER_FLOW_NODE( "Game:DisplayTag",CFlowNode_DisplayTag);
REGISTER_FLOW_NODE( "Game:DisplayTagAdv",CFlowNode_DisplayTagAdv);

