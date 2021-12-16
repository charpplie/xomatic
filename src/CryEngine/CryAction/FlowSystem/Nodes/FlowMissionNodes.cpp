#include "StdAfx.h"
#include "FlowBaseNode.h"
#include "CryAction.h"
#include "IGameRulesSystem.h"

class CFlowNode_EndLevel : public CFlowBaseNode
{
public:
	CFlowNode_EndLevel( SActivationInfo * pActInfo )
	{
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void   ( "Trigger",   _HELP("Finish the current mission (go to next level)") ),
			InputPortConfig<string>( "NextLevel", _HELP("Which level is the next level?") ),
			{0}
		};
		config.sDescription = _HELP("Advance to a new level");
		config.pInputPorts  = in_ports;
		config.pOutputPorts = 0;
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event == eFE_Activate && IsPortActive(pActInfo,0))
		{
			CCryAction::GetCryAction()->ScheduleEndLevel(GetPortString(pActInfo, 1));
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

REGISTER_FLOW_NODE_SINGLETON("Mission:EndLevelNew", CFlowNode_EndLevel);
