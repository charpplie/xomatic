/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
History:
- 12:05:2009   Created by Federico Rebora
*************************************************************************/

#include "StdAfx.h"

#include "ColorGradientNode.h"
#include "Graphics/ColorGradientManager.h"
#include <IColorGradingController.h>

enum InputPorts
{
	eIP_Trigger,
};

const SInputPortConfig CFlowNode_ColorGradient::inputPorts[] =
{
	InputPortConfig_Void("Trigger", _HELP("")),
	InputPortConfig<string>("TexturePath", _HELP("Path to the Color Chart texture.")),
	InputPortConfig<float>("TransitionTime", _HELP("Fade in time (Seconds).")),
	{0},    
};

CFlowNode_ColorGradient::CFlowNode_ColorGradient(SActivationInfo* activationInformation)
{

}

void CFlowNode_ColorGradient::GetConfiguration(SFlowNodeConfig& config)
{
	config.pInputPorts = inputPorts;
	config.SetCategory(EFLN_ADVANCED);
}

void CFlowNode_ColorGradient::ProcessEvent(EFlowEvent event, SActivationInfo* activationInformation)
{
	if (event==IFlowNode::eFE_Activate && IsPortActive(activationInformation, eIP_Trigger))
	{
		const string texturePath = GetPortString(activationInformation, eInputPorts_TexturePath);
		const float timeToFade = GetPortFloat(activationInformation, eInputPorts_TransitionTime);

		g_pGame->GetColorGradientManager().TriggerFadingColorGradient(texturePath, timeToFade);
	}
}

void CFlowNode_ColorGradient::GetMemoryUsage(ICrySizer* sizer) const
{
	sizer->Add(*this);
}

REGISTER_FLOW_NODE("CrysisFX:ColorGradient", CFlowNode_ColorGradient);
