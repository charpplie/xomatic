#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowNode_ExecuteString : public CFlowBaseNode
{
public:
	CFlowNode_ExecuteString( SActivationInfo * pActInfo ) {}

	enum EInPorts
	{
		SET = 0,
		STRING,
	};

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<SFlowSystemVoid>( "Set", _HELP("Trigger to Set CVar's value") ),
			InputPortConfig<string>( "String", _HELP("String you want to execute") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			{0}
		};
		config.sDescription = _HELP("Executes a string like when using the console");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_DEBUG);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event == eFE_Activate)
		{
			const bool isSet = (IsPortActive(pActInfo, SET));
			if (isSet)
			{
				const string& stringToExecute = GetPortString(pActInfo, STRING);
				gEnv->pConsole->ExecuteString(stringToExecute, true);
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

REGISTER_FLOW_NODE_SINGLETON( "System:ExecuteString", CFlowNode_ExecuteString );

