#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowNode_Platform : public CFlowBaseNode
{
public:
	CFlowNode_Platform(SActivationInfo * pActInfo) {}

	enum EInPorts
	{
		Get = 0
	};
	enum EOutPorts
	{
		Pc = 0,
		Xbox360,
		Ps3
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig in_config[] =
		{
			InputPortConfig_AnyType("Check", _HELP("Triggers a check of the current platform")),
			{0}
		};
		static const SOutputPortConfig out_config[] =
		{
			OutputPortConfig_AnyType("PC", _HELP("Outputs the signal from Check input if the game runs on PC")),
			OutputPortConfig_AnyType("Xbox360", _HELP("Outputs the signal from Check input if the game runs on PS3")),
			OutputPortConfig_AnyType("PS3", _HELP("Outputs the signal from Check input if the game runs on X360")),
			{0}
		};
		config.sDescription = _HELP("Provides branching logic for different platforms");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_DEBUG);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		if (event == eFE_Activate)
		{
			if (!IsPortActive(pActInfo, Get))
				return;
#if defined(WIN32) || defined(WIN64)
			ActivateOutput(pActInfo, Pc, GetPortAny(pActInfo, Get));




#endif
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

REGISTER_FLOW_NODE_SINGLETON("Game:CheckPlatform", CFlowNode_Platform);

