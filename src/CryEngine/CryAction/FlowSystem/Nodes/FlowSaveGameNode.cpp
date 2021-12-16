////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   FlowSaveGameNode.cpp
//  Version:     v1.00
//  Created:     28-08-2006 by AlexL
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

#include "CryAction.h"
#include "IGame.h"
#include "ICheckPointSystem.h"

const static int NUM_SAVE_LOAD_ENTITIES = 5;

class CFlowSaveGameNode : public CFlowBaseNode
{
public:
	CFlowSaveGameNode(SActivationInfo* pActInfo)
	{
	}

	~CFlowSaveGameNode()
	{
	}

	/*
	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return this; // new CFlowSaveGameNode(pActInfo);
	}
	*/

	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	enum 
	{
		EIP_Save = 0,
		EIP_Load,
		EIP_Name,
		EIP_Desc,
		EIP_InGameOnly,
		EIP_EnableSave,
		EIP_DisableSave,
	};

	enum 
	{
		EOP_Saved = 0,
		EOP_Loaded,
	};

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void ("Save",_HELP("Trigger to save game")),
			InputPortConfig_Void( "Load",_HELP("Trigger to load game")),
			InputPortConfig<string> ("Name", string("quicksave"), _HELP("Name of SaveGame to save/load. Use $LAST to load last savegame")),
			InputPortConfig<string> ("Desc", string(""), _HELP("Description [Currently ignored]"), _HELP("Description")),
			InputPortConfig<bool> ("InGameOnly", true, _HELP("Load/Save in pure game mode only. Default=true.")),
			InputPortConfig_Void ("EnableSave",_HELP("Trigger to globally allow quick-saving")),
			InputPortConfig_Void ("DisableSave",_HELP("Trigger to globally disallow quick-saving")),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_Void ("SaveDone", _HELP("Triggered when saved")),
			OutputPortConfig_Void ("LoadDone", _HELP("Triggered when loaded")),
			{0}
		};
		config.sDescription = _HELP("SaveGame for Autosave");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			break;
		case eFE_Activate:
			if (IsPortActive(pActInfo, EIP_DisableSave))
				CCryAction::GetCryAction()->AllowSave(false);
			if (IsPortActive(pActInfo, EIP_EnableSave))
				CCryAction::GetCryAction()->AllowSave(true);

			// when GameOnly and we're in Editor, return
			if (GetPortBool(pActInfo, EIP_InGameOnly) && gEnv->IsEditor())
				return;

			if (IsPortActive(pActInfo, EIP_Save))
			{
				string name = GetPortString(pActInfo, EIP_Name);
				PathUtil::RemoveExtension(name);
				//name+=".CRYSISJMSF";

				if(IGame *pGame = gEnv->pGame)
					CCryAction::GetCryAction()->SaveGame(pGame->CreateSaveGameName(), true, false, eSGR_FlowGraph, false, name.c_str());
				else
					CCryAction::GetCryAction()->SaveGame(name.c_str(), true, false, eSGR_FlowGraph);
				ActivateOutput(pActInfo, EOP_Saved, true);
			}
			if (IsPortActive(pActInfo, EIP_Load))
			{
				string name = GetPortString(pActInfo, EIP_Name);
				if (name == "$LAST")
				{
					CCryAction::GetCryAction()->ExecuteCommandNextFrame("loadLastSave");
				}
				else
				{
					PathUtil::RemoveExtension(name);
					name+=".CRYSISJMSF"; 
					CCryAction::GetCryAction()->LoadGame(name.c_str(), true); 
				}
				ActivateOutput(pActInfo, EOP_Loaded, true);
			}
			break;
		}
	}
};

//////////////////////////////////////////////////////////////////////////

class CFlowNodeCheckpoint : public CFlowBaseNode, ICheckpointListener
{
public:
	CFlowNodeCheckpoint(SActivationInfo* pActInfo)
	{
		m_iSaveId = m_iLoadId = 0;
		memset(m_saveLoadEntities, 0, sizeof(m_saveLoadEntities));
		CCryAction::GetCryAction()->GetICheckpointSystem()->RegisterListener(this);
	}

	virtual ~CFlowNodeCheckpoint()
	{
		CCryAction::GetCryAction()->GetICheckpointSystem()->RemoveListener(this);
	}

	// ICheckpointListener
	void OnSave(SCheckpointData *pCheckpoint, ICheckpointSystem *pSystem)
	{
		CRY_ASSERT(pSystem);

		//output onSave
		m_iSaveId = (int)(pCheckpoint->m_checkPointId);

		//this saves designer-controlled entities (breakables, destructables ...)
		for(int i = 0; i < NUM_SAVE_LOAD_ENTITIES; ++i)
		{
			//this comes with the ICheckpointListener
			pSystem->SaveExternalEntity(m_saveLoadEntities[i]);
		}
	}
	void OnLoad(SCheckpointData *pCheckpoint, ICheckpointSystem *pSystem)
	{
		//output onLoad
		m_iLoadId = (int)(pCheckpoint->m_checkPointId);

		//loading external entities happens inside the CheckpointSystem
	}
	//~ICheckpointListener

	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	enum 
	{
		EIP_LoadLastCheckpoint = 0,
		EIP_SaveLoadEntityStart
	};

	enum 
	{
		EOP_OnSave = 0,
		EOP_OnLoad,
	};

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] =
		{
			InputPortConfig_Void("LoadLastCheckpoint", _HELP("Load the last checkpoint which was saved during the current session.")),
			InputPortConfig<EntityId>("SaveLoadEntity1", _HELP("Save and load this entity when a checkpoint is triggered.")),
			InputPortConfig<EntityId>("SaveLoadEntity2", _HELP("Save and load this entity when a checkpoint is triggered.")),
			InputPortConfig<EntityId>("SaveLoadEntity3", _HELP("Save and load this entity when a checkpoint is triggered.")),
			InputPortConfig<EntityId>("SaveLoadEntity4", _HELP("Save and load this entity when a checkpoint is triggered.")),
			InputPortConfig<EntityId>("SaveLoadEntity5", _HELP("Save and load this entity when a checkpoint is triggered.")),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<int> ("SaveDone", _HELP("Triggered when saved")),
			OutputPortConfig<int> ("LoadDone", _HELP("Triggered when loaded")),
			{0}
		};
		config.sDescription = _HELP("Checkpoint System Output");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		//initialization event
		switch (event)
		{
		case eFE_Initialize:
			//since we cannot send data to the flowgraph at any time, we need to wait for updates ..
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			break;
		case eFE_Activate:
			if (IsPortActive(pActInfo, EIP_LoadLastCheckpoint))
			{
				CCryAction::GetCryAction()->GetICheckpointSystem()->LoadLastCheckpoint();
			}
			break;
		case eFE_Update:
			if(m_iSaveId > 0)
			{
				ActivateOutput(pActInfo, EOP_OnSave, m_iSaveId);
				m_iSaveId = 0;
			}
			if(m_iLoadId > 0)
			{
				ActivateOutput(pActInfo, EOP_OnLoad, m_iLoadId);
				m_iLoadId = 0;
			}

			for(int entityIndex=0, portIndex = EIP_SaveLoadEntityStart; portIndex < (EIP_SaveLoadEntityStart+NUM_SAVE_LOAD_ENTITIES); ++portIndex, ++entityIndex)
			{
				m_saveLoadEntities[entityIndex] = GetPortEntityId(pActInfo, portIndex);


			}
/*
			m_saveLoadEntities[0] = GetPortEntityId(pActInfo, EIP_SaveLoadEntity1);
			m_saveLoadEntities[1] = GetPortEntityId(pActInfo, EIP_SaveLoadEntity2);
			m_saveLoadEntities[2] = GetPortEntityId(pActInfo, EIP_SaveLoadEntity3);
			m_saveLoadEntities[3] = GetPortEntityId(pActInfo, EIP_SaveLoadEntity4);
			m_saveLoadEntities[4] = GetPortEntityId(pActInfo, EIP_SaveLoadEntity5);
*/
			break;
		}
	}

private:

	int				m_iSaveId, m_iLoadId;
	EntityId	m_saveLoadEntities[NUM_SAVE_LOAD_ENTITIES];
};
//////////////////////////////////////////////////////////////////////////
// Register nodes

REGISTER_FLOW_NODE("System:CheckpointSystem", CFlowNodeCheckpoint);
REGISTER_FLOW_NODE_SINGLETON( "System:SaveGame", CFlowSaveGameNode );
