#include "StdAfx.h"
#include "AINodes.h"

#include "AI/GameAISystem.h"
#include "AI/AIBattleFront.h"

void CFlowNode_BattleFrontControl::GetConfiguration( SFlowNodeConfig& config )
{
	static const SInputPortConfig in_ports[] = 
	{
		InputPortConfig<int>("Group", 0, _HELP("Target battle front group id.")),
		InputPortConfig<Vec3>("Position", _HELP("Desired battle front position")),
		InputPortConfig_Void("Activate", _HELP("Activate battle front position control." )),
		InputPortConfig_Void("Deactivate", _HELP("Deactivate battle front position control." )),
		{0}
	};
	config.pInputPorts = in_ports;
	config.pOutputPorts = 0;
	config.sDescription = _HELP("BattleFront control. Used for overriding the default position of a AI group's BattleFront.");
	config.SetCategory(EFLN_ADVANCED);
}

void CFlowNode_BattleFrontControl::ProcessEvent( EFlowEvent event, SActivationInfo *activationInformation )
{
	if (event == eFE_Initialize)
	{
		m_enabled = false;
	}

	if(event == eFE_Activate)
	{
		CAIBattleFrontModule* battleFrontModule = static_cast<CAIBattleFrontModule*>(CGameAISystem::GetInstance()->FindModule("BattleFront"));
		assert(battleFrontModule);
		if (!battleFrontModule)
			return;

		int	groupID = GetPortInt(activationInformation, EIP_Group);	
		CAIBattleFrontGroup* battleFrontGroup = battleFrontModule->GetGroupByID(groupID);
		if (!battleFrontGroup)
		{
			GameWarning("[flow] CFlowNode_BattleFrontControl: Couldn't get group with id '%d' from battle front module.", groupID );
			return;
		}

		if(m_enabled)
		{
			if (IsPortActive(activationInformation, EIP_Deactivate))
			{
				m_enabled = false;
				battleFrontGroup->DisableDesignerControlledBattleFront();
			}
			else if (IsPortActive(activationInformation, EIP_Position))
			{
				Vec3 position = GetPortVec3(activationInformation, EIP_Position);	
				battleFrontGroup->SetDesignerControlledBattleFrontAt(position);
			}
		}
		else
		{
			if (IsPortActive(activationInformation, EIP_Activate))
			{
				m_enabled = true;
				Vec3 position = GetPortVec3(activationInformation, EIP_Position);	
				battleFrontGroup->SetDesignerControlledBattleFrontAt(position);
			}
		}

	}
}

void CFlowNode_BattleFrontControl::GetMemoryUsage( ICrySizer * sizer ) const
{
	sizer->Add(*this);
}

REGISTER_FLOW_NODE( "AI:BattleFrontControl", CFlowNode_BattleFrontControl)