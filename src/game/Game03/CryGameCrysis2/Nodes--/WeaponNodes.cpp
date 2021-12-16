#include "StdAfx.h"
#include "Game.h"

#include "Nodes/G2FlowBaseNode.h"
#include "Weapon.h"

#include <StringUtils.h>

namespace
{
	IWeapon* GetWeapon(IActor* pActor)
	{
		IInventory *pInventory = pActor->GetInventory();
		if (!pInventory)
			return 0;
		EntityId itemId = pInventory->GetCurrentItem();
		if (itemId == 0)
			return 0;
		IItem* pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(itemId);
		if (pItem == 0)
			return 0;
		IWeapon* pWeapon = pItem->GetIWeapon();
		return pWeapon;
	}

	IWeapon* GetWeapon(IActor* pActor, const char* className)
	{
		IInventory *pInventory = pActor->GetInventory();
		if (!pInventory)
			return 0;
		IEntityClass* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(className);
		if (!pEntityClass)
			return 0;
		EntityId itemId = pInventory->GetItemByClass(pEntityClass);
		if (itemId == 0)
			return 0;
		IItem* pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(itemId);
		if (pItem == 0)
			return 0;
		IWeapon* pWeapon = pItem->GetIWeapon();
		return pWeapon;
	}

	IActor* GetActor(CFlowBaseNode::SActivationInfo* pActInfo)
	{
		IEntity* pEntity = pActInfo->pEntity;
		if (pEntity == 0)
			return 0;
		IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
		return pActor;
	}
};

// WeaponTracker not yet finished

/*
class CFlowNode_WeaponTracker : public CFlowBaseNode, public IItemSystemListener, public IWeaponEventListener
{
public:
CFlowNode_WeaponTracker( SActivationInfo * pActInfo ) : m_entityId(0)
{
}

~CFlowNode_WeaponTracker()
{
Unregister();
}

void Register()
{
IItemSystem* pItemSys = g_pGame->GetIGameFramework()->GetIItemSystem();
if (pItemSys)
pItemSys->RegisterListener(this);
}

void Unregister()
{
IItemSystem* pItemSys = g_pGame->GetIGameFramework()->GetIItemSystem();
if (pItemSys)
pItemSys->UnregisterListener(this);
}

IFlowNodePtr Clone(SActivationInfo* pActInfo)
{
return new CFlowNode_WeaponTracker(pActInfo);
}

void GetConfiguration( SFlowNodeConfig& config )
{
static const SInputPortConfig in_ports[] = 
{
InputPortConfig_Void( "Activate",    _HELP("Activate") ),
InputPortConfig_Void( "Deactivate",  _HELP("Deactivate") ),
{0}
};
static const SOutputPortConfig out_ports[] = 
{
OutputPortConfig<string>( "AccessoryAdded",   _HELP("Accessory was added")),
OutputPortConfig<string>( "AccessoryRemoved", _HELP("Accessory was removed")),
{0}
};
config.nFlags |= EFLN_TARGET_ENTITY;
config.pInputPorts = in_ports;
config.pOutputPorts = out_ports;
config.sDescription = _HELP("Listens for Player's accessory changes");
config.SetCategory(EFLN_APPROVED);
}

void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
{
switch (event)
{
case eFE_Initialize:
m_actInfo = *pActInfo;
break;
case eFE_SetEntityId:
m_entityId = pActInfo->pEntity ? pActInfo->pEntity->GetId() : 0;
break;
case eFE_Activate:
{
if (IsPortActive(pActInfo, 1))
{
UnregisterWeapon();
Unregister();
}
if (IsPortActive(pActInfo, 0))
{
Register();
RegisterCurrent(pActInfo);
}
}
break;
}
}

protected:
virtual void OnSetActorItem(IActor *pActor, IItem *pItem )
{
if (pActor->GetEntityId() != m_entityId)
return;

UnregisterWeapon();
if (pItem == 0)
return;

IWeapon* pWeapon = pItem->GetIWeapon();
if (pWeapon == 0)
return;
RegisterWeapon(pWeapon);
}

void RegisterCurrent(SActivationInfo* pActInfo)
{
IActor* pActor = GetActor(pActInfo);
if (pActor == 0)
return;
IWeapon* pWeapon = GetWeapon(pActor);
if (pWeapon == 0)
return;
RegisterWeapon(pWeapon);
}

void RegisterWeapon(IWeapon* pWeapon)
{
m_weaponId = pWeapon->GetEntity()->GetId();
pWeapon->AddEventListener(this);
}

void UnregisterWeapon()
{
if (m_weaponId != 0)
{
IItemSystem* pItemSys = g_pGame->GetIGameFramework()->GetIItemSystem();
if (pItemSys != 0)
{
IItem* pItem = pItemSys->GetItem(m_weaponId);
if (pItem != 0)
{
IWeapon* pWeapon = pItem->GetIWeapon();
if (pWeapon != 0)
pWeapon->RemoveEventListener(this);
}
}
}
m_weaponId = 0;
}

SActivationInfo m_actInfo;
EntityId m_entityId;
EntityId m_weaponId;
};
*/


class CFlowNode_WeaponAccessoryChanged : public CFlowBaseNode
{
public:
	CFlowNode_WeaponAccessoryChanged( SActivationInfo * pActInfo )
	{
	}

	~CFlowNode_WeaponAccessoryChanged()
	{
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_WeaponAccessoryChanged(pActInfo);
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig<string>( "Weapon",  _HELP("Restrict listening on this weapon. Empty=Listen for all"), 0, _UICONFIG("enum_global:weapon") ),
			InputPortConfig<string>( "Item",  _HELP("Restrict listening for this accessory. Empty=Listen for all"), 0, _UICONFIG("enum_global:item_givable") ),
			{0}
		};
		static const SOutputPortConfig out_ports[] = 
		{
			OutputPortConfig<string>( "Weapon", _HELP("Weapon name on Accessory change")),
			OutputPortConfig<string>( "AccessoryAdded",   _HELP("Accessory was added. Accessory Name will be outputted.")),
			OutputPortConfig<string>( "AccessoryRemoved", _HELP("Accessory was removed. Accessory Name will be outputted.")),
			{0}
		};
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.sDescription = _HELP("Listens for the Player's accessory changes");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			m_actInfo = *pActInfo;
			break;
		case eFE_Activate:
			{
			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

protected:
	virtual void HUDDestroyed() {}
	virtual void WeaponAccessoryChanged(CWeapon* pWeapon, const char* accessory, bool bAdd)
	{
		const char* actWeaponName = pWeapon->GetEntity()->GetClass()->GetName();

		const string& weaponName = GetPortString(&m_actInfo, 0);
		if (weaponName.empty() == false && stricmp(actWeaponName, weaponName.c_str()) != 0)
			return;

		const string& itemName = GetPortString(&m_actInfo, 1);
		if (itemName.empty() == false && stricmp(accessory, itemName.c_str()) != 0)
			return;

		string name (actWeaponName);
		string accName (accessory);
		ActivateOutput(&m_actInfo, 0, name);
		ActivateOutput(&m_actInfo, bAdd ? 1 : 2, accName);
	}

	SActivationInfo m_actInfo;
};

class CFlowNode_WeaponCheckAccessory : public CFlowBaseNode 
{
public:
	CFlowNode_WeaponCheckAccessory( SActivationInfo * pActInfo ) { }

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "Check", _HELP("Trigger this port to check for accessory on the Actor's current weapon" )),
			InputPortConfig<string>( "Accessory", _HELP("Name of accessory to check for."), 0, _UICONFIG("enum_global:item")),
			{0}
		};
		static const SOutputPortConfig out_ports[] = 
		{
			OutputPortConfig_Void( "False", _HELP("Triggered if accessory is not attached.")),
			OutputPortConfig_Void( "True",  _HELP("Triggered if accessory is attached.")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.sDescription = _HELP("Checks if target actor's current weapon has [Accessory] attached.");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(eFE_Activate == event && IsPortActive(pActInfo,0))
		{
			IActor* pActor = GetActor(pActInfo);
			if (pActor == 0)
				return;

			CWeapon* pWeapon = static_cast<CWeapon*> (GetWeapon(pActor));
			if (pWeapon != 0)
			{
				CItem* pAcc = pWeapon->GetAccessory(GetPortString(pActInfo, 1).c_str());
				if (pAcc != 0)
					ActivateOutput(pActInfo, 1, true); // [True]
				else
					ActivateOutput(pActInfo, 0, true); // [False]
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};

class CFlowNode_WeaponCheckZoom : public CFlowBaseNode 
{
public:
	CFlowNode_WeaponCheckZoom( SActivationInfo * pActInfo ) { }

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "Check", _HELP("Trigger this port to check if Actor's current weapon is zoomed" )),
			InputPortConfig<string>( "Weapon", _HELP("Name of Weapon to check. Empty=All."), 0, _UICONFIG("enum_global:weapon")),
			{0}
		};
		static const SOutputPortConfig out_ports[] = 
		{
			OutputPortConfig_Void( "False", _HELP("Triggered if weapon is not zoomed.")),
			OutputPortConfig_Void( "True",  _HELP("Triggered if weapon is zoomed.")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.sDescription = _HELP("Checks if target actor's current weapon is zoomed.");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(eFE_Activate == event && IsPortActive(pActInfo,0))
		{
			IActor* pActor = GetActor(pActInfo);
			if (pActor == 0)
				return;

			CWeapon* pWeapon = static_cast<CWeapon*> (GetWeapon(pActor));
			bool bZoomed = false;
			if (pWeapon != 0)
			{
				const string& weaponClass = GetPortString(pActInfo, 1);
				if (weaponClass.empty() == true || weaponClass == pWeapon->GetEntity()->GetClass()->GetName())
				{
					bZoomed = pWeapon->IsZoomed();
				}
			}
			ActivateOutput(pActInfo, bZoomed ? 1 : 0, true);			
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};

class CFlowNode_WeaponAccessory : public CFlowBaseNode 
{
public:
	CFlowNode_WeaponAccessory( SActivationInfo * pActInfo ) { }

	enum INPUTS
	{
		EIP_ATTACH = 0,
		EIP_DETACH,
		EIP_WEAPON,
		EIP_ACCESSORY,
	};

	enum OUTPUTS
	{
		EOP_ATTACHED = 0,
		EOP_DETACHED,
	};

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "Attach", _HELP("Trigger to attach accessory on the Actor's weapon" )),
			InputPortConfig_Void( "Detach", _HELP("Trigger to detach accessory from the Actor's weapon" )),
			InputPortConfig<string>( "Weapon", _HELP("Name of weapon the accessory should be attached/detached"), 0, _UICONFIG("enum_global:weapon")),
			InputPortConfig<string>( "Accessory", _HELP("Name of accessory"), 0, _UICONFIG("enum_global:item")),
			{0}
		};
		static const SOutputPortConfig out_ports[] = 
		{
			OutputPortConfig_Void( "Attached", _HELP("Triggered if accessory was attached.")),
			OutputPortConfig_Void( "Detached",  _HELP("Triggered if accessory was detached.")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.sDescription = _HELP("Attach/Detach [Accessory] from Actor's weapon [Weapon]. Both must be in the Inventory.");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(eFE_Activate == event)
		{
			const bool bAttach = IsPortActive(pActInfo, EIP_ATTACH);
			const bool bDetach = IsPortActive(pActInfo, EIP_DETACH);
			if (!bAttach && !bDetach)
				return;

			IActor* pActor = GetActor(pActInfo);
			if (pActor == 0)
				return;

			const string& className = GetPortString(pActInfo, EIP_WEAPON);

			CWeapon* pWeapon = static_cast<CWeapon*> ( className.empty() ? GetWeapon(pActor) : GetWeapon(pActor, className.c_str()) );
			if (pWeapon != 0)
			{
				ItemString acc = ItemString(GetPortString(pActInfo, EIP_ACCESSORY));
				if (bAttach && pWeapon->GetAccessory(acc) == 0)
				{
					pWeapon->SwitchAccessory(acc);
					ActivateOutput(pActInfo, EOP_ATTACHED, true);
				}
				else if (bDetach && pWeapon->GetAccessory(acc) != 0)
				{
					pWeapon->SwitchAccessory(acc);
					ActivateOutput(pActInfo, EOP_DETACHED, true);
				}
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};

//------------------------------------------------------------------------------
class CFlowNode_PingerPingHelper : public CFlowBaseNode , IEntityEventListener
{
	enum OutputPorts
	{
		eOP_Ping_Now = 0,
		eOP_Last,
	};

public:
			CFlowNode_PingerPingHelper( SActivationInfo * pActInfo ):
			m_entityId(0) 
			{ 
				m_nodeID = pActInfo->myID;
				m_pGraph = pActInfo->pGraph;
			}

			virtual ~CFlowNode_PingerPingHelper()
			{
				UnregisterEvents();
			}

			void GetConfiguration( SFlowNodeConfig& config )
			{
				static const SOutputPortConfig out_ports[] = 
				{
					OutputPortConfig<bool>( "Ping_Now", _HELP("True when assigned pinger pings!")),
					{0}
				};
				config.nFlags |= EFLN_TARGET_ENTITY;
				config.pInputPorts = 0;
				config.pOutputPorts = out_ports;
				config.sDescription = _HELP("");
				config.SetCategory(EFLN_DEBUG);
			}

			void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
			{
				if (event == eFE_Initialize)
				{
					ActivateOutput(pActInfo, eOP_Ping_Now, false);
				}

				if (event == eFE_SetEntityId)
				{
					UnregisterEvents();

					if (pActInfo->pEntity)
						m_entityId = pActInfo->pEntity->GetId();
					else
						m_entityId = 0;

					RegisterEvents();
				}

			}

			void OnEntityEvent( IEntity *pEntity,SEntityEvent &event )
			{
				if ( !m_pGraph->IsEnabled() || m_pGraph->IsSuspended() || !m_pGraph->IsActive() )
					return;

				if(event.event == ENTITY_EVENT_DONE)
				{
					m_pGraph->SetEntityId(m_nodeID, 0);
				}
				else if(event.event == ENTITY_EVENT_ACTIVE_FLOW_NODE_OUTPUT)
				{
					SFlowNodeConfig config;
					GetConfiguration(config);

					for(int i = 0; i < eOP_Last; i++)
					{
						if(!strcmp(config.pOutputPorts[i].name, (const char*)event.nParam[0]))
						{
							SFlowAddress addr( m_nodeID, 0, true );
							if(event.nParam[1] == IEntityClass::EVT_BOOL)
								m_pGraph->ActivatePort( addr,*(bool*)event.nParam[2] );

							break;
						}
					}

				}
			}

			// Return entity of this node.
			IEntity* GetEntity()
			{
				return gEnv->pEntitySystem->GetEntity(m_entityId);
			}

			void RegisterEvents( )
			{
				if(m_entityId)
				{
					gEnv->pEntitySystem->AddEntityEventListener( m_entityId,ENTITY_EVENT_DONE,this );
					gEnv->pEntitySystem->AddEntityEventListener( m_entityId,ENTITY_EVENT_ACTIVE_FLOW_NODE_OUTPUT,this );
				}

			}
			void UnregisterEvents()
			{
				if ( m_entityId )
				{
					gEnv->pEntitySystem->RemoveEntityEventListener( m_entityId,ENTITY_EVENT_DONE,this );
					gEnv->pEntitySystem->RemoveEntityEventListener( m_entityId,ENTITY_EVENT_ACTIVE_FLOW_NODE_OUTPUT,this );
				}
			}

			virtual void GetMemoryUsage(ICrySizer * s) const
			{
				s->Add(*this);
			}

protected:
	EntityId m_entityId;

	IFlowGraph *m_pGraph;
	TFlowNodeId m_nodeID;
};


class CFlowNode_EntityHacked : public CFlowBaseNode , IEntityEventListener
{
private:

	enum InputPorts
	{
		eIP_Enable,
		eIP_Disable,
		eIP_Last
	};

	enum OutputPorts
	{
		eOP_Enabled,
		eOP_Hacked,
		eOP_Last
	};

public:

	CFlowNode_EntityHacked(SActivationInfo* pActInfo)
		:	m_entityId(0)
		,	m_nodeID(pActInfo->myID)
		,	m_pGraph(pActInfo->pGraph)
		,	m_enabled(true)
	{
	}


	virtual ~CFlowNode_EntityHacked()
	{
		UnregisterEvents();
	}


	void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inPorts[] = 
		{
			InputPortConfig_Void("Enable", _HELP("Makes object hackable.")),
			InputPortConfig_Void("Disable", _HELP("Makes object not hackable.")),
			{0}
		};

		static const SOutputPortConfig outPorts[] = 
		{
			OutputPortConfig<bool>("Enabled", _HELP("Tells if node is active.")),
			OutputPortConfig_Void("Hacked", _HELP("Activated when this entity is hacked.")),
			{0}
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inPorts;
		config.pOutputPorts = outPorts;
		config.sDescription = _HELP("Checks when this entity is hacked.");
		// config.SetCategory(EFLN_OBSOLETE);
	}


	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		if (event == eFE_Initialize)
		{
			ActivateOutput(pActInfo, eOP_Hacked, false);
		}

		else if (event == eFE_Activate)
		{
			if (IsPortActive(pActInfo, eIP_Enable))
			{
				m_enabled = true;
			}
			else if (IsPortActive(pActInfo, eIP_Disable))
			{
				m_enabled = false;
			}
			SFlowAddress addr(m_nodeID, eOP_Enabled, true);
			m_pGraph->ActivatePort(addr, m_enabled);
		}

		else if (event == eFE_SetEntityId)
		{
			UnregisterEvents();

			if (pActInfo->pEntity)
				m_entityId = pActInfo->pEntity->GetId();
			else
				m_entityId = 0;

			RegisterEvents();
		}
	}


	void OnEntityEvent(IEntity *pEntity, SEntityEvent &event)
	{
		if (!m_pGraph->IsEnabled() || m_pGraph->IsSuspended() || !m_pGraph->IsActive())
			return;

		if(event.event == ENTITY_EVENT_DONE)
		{
			m_pGraph->SetEntityId(m_nodeID, 0);
		}
		else if(event.event == ENTITY_EVENT_ACTIVE_FLOW_NODE_OUTPUT)
		{
			SFlowNodeConfig config;
			GetConfiguration(config);

			for(int i = 0; i < eOP_Last; i++)
			{
				if(m_enabled && !strcmp(config.pOutputPorts[i].name, (const char*)event.nParam[0]))
				{
					SFlowAddress addr(m_nodeID, eOP_Hacked, true);
					m_pGraph->ActivatePort(addr, 1);

					break;
				}
			}

		}
	}


	IEntity* GetEntity()
	{
		return gEnv->pEntitySystem->GetEntity(m_entityId);
	}


	void RegisterEvents()
	{
		if(m_entityId)
		{
			gEnv->pEntitySystem->AddEntityEventListener(m_entityId, ENTITY_EVENT_DONE, this);
			gEnv->pEntitySystem->AddEntityEventListener(m_entityId, ENTITY_EVENT_ACTIVE_FLOW_NODE_OUTPUT, this);
		}
	}


	void UnregisterEvents()
	{
		if ( m_entityId )
		{
			gEnv->pEntitySystem->RemoveEntityEventListener(m_entityId, ENTITY_EVENT_DONE, this);
			gEnv->pEntitySystem->RemoveEntityEventListener(m_entityId, ENTITY_EVENT_ACTIVE_FLOW_NODE_OUTPUT, this);
		}
	}


	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}


protected:
	EntityId m_entityId;

	IFlowGraph *m_pGraph;
	TFlowNodeId m_nodeID;

	bool m_enabled;
};



REGISTER_FLOW_NODE("Weapon:AccessoryChanged",	CFlowNode_WeaponAccessoryChanged);
REGISTER_FLOW_NODE("Pinger:PingHelperNode", CFlowNode_PingerPingHelper);
REGISTER_FLOW_NODE_SINGLETON("Weapon:CheckAccessory",		CFlowNode_WeaponCheckAccessory);
REGISTER_FLOW_NODE_SINGLETON("Weapon:CheckZoom",		CFlowNode_WeaponCheckZoom);
REGISTER_FLOW_NODE_SINGLETON("Weapon:Accessory",		CFlowNode_WeaponAccessory);
REGISTER_FLOW_NODE("Entity:EntityHacked", CFlowNode_EntityHacked);
