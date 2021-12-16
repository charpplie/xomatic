#pragma once

#ifndef AINodes_H
#define AINodes_H

#include "G2FlowBaseNode.h"

class CFlowNode_BattleFrontControl : public CFlowBaseNode
{
public:
	enum EInputPorts
	{
		EIP_Group,
		EIP_Position,
		EIP_Activate,
		EIP_Deactivate,
	};

	static const SInputPortConfig inputPorts[];

	CFlowNode_BattleFrontControl(SActivationInfo * activationInformation)
		: m_enabled(false)
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config);
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *activationInformation);
	virtual void GetMemoryUsage(ICrySizer * sizer) const;

private:
	bool m_enabled;
};

#endif // AINodes_H